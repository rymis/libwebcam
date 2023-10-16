#include "web/http.h"
#include "optcfg.h"
#include "sound.h"
#include <libwebcam.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

/* Simple webcam-based http camera interface */

static int exit_now = 0;
static void sigint(int sig)
{
	++exit_now;
}

unsigned char *FRAME = NULL;
size_t FRAME_SZ = 0;
char CAM_NAME[256] = "";
struct snd_ctx *sound = NULL;

char ROOT[256];
static mtx_t G_MUTEX;

static void LOCK(void)
{
    mtx_lock(&G_MUTEX);
}

static void UNLOCK(void)
{
    mtx_unlock(&G_MUTEX);
}

static void new_frame(void *ctx, webcam_t *cam, unsigned char *pixels, size_t bpl, size_t size);
static int save_jpeg(unsigned char **buf, size_t *len, int quality, unsigned char *data, int width, int height, int bpl);
static int current_image_get(http_context_t *cnx, void *param);
static int snd_enabled_get(http_context_t *cnx, void *param);
static int snd_wav_get(http_context_t *cnx, void *param);


static long delta_time(struct timeval *t1, struct timeval *t2)
{
	return (t2->tv_usec - t1->tv_usec) + (t2->tv_sec - t1->tv_sec) * 1000000;
}

int main(int argc, char *argv[])
{
	struct optcfg_option options[] = {
		{ 'p', "port",
			"Specify port number to serve", 0, "8888" },
		{ 'H', "host",
			"Specify host name to listen", 0, NULL },
		{ 'F', "fps",
			"Specify how many frames per second will be captured", 0, "5" },
		{ 'r', "root",
			"Specify root HTML directory", 0, NULL },
		{ 'c', "config",
			"Load configuration from file", OPTCFG_CFGFILE, NULL },
		{ 'd', "delay",
			"Sound delay and fragment size in seconds", 0, "1" },
		{ 'f', "frequency",
			"Sound frequency in Hz.", 0, "8000" },
		{ 'b', "bits",
			"Bits per fragment of sound", 0, "8" },
		{ 'S', "stereo",
			"Enable stereo mode", OPTCFG_FLAG, "no" },
		{ 'e', "exec",
			"Execute program as sound source", 0, NULL }
	};
	unsigned options_cnt = sizeof(options) / sizeof(options[0]);
	struct optcfg *opts;
	int cams[64];
	unsigned cam_cnt = 64;
	webcam_t *cam;
    http_server_t* srv;
	struct timeval last, cur;
	long dtime;
	int port;
	const char *host;
	int bsecs, freq, bits, stereo;
	const char *snd_cmd = NULL;
	const char *root = NULL;

    mtx_init(&G_MUTEX, mtx_plain);
	
	opts = optcfg_new();
	if (!opts) {
		return EXIT_FAILURE;
	}

	if (optcfg_default_config(opts, "wwwcam")) {
		return EXIT_FAILURE;
	}

	if (optcfg_parse_options(opts, "wwwcam", argc - 1, argv + 1, options, options_cnt)) {
		return EXIT_FAILURE;
	}

	optcfg_save(opts, stdout);

	dtime = optcfg_get_int(opts, "fps", 5);
	if (dtime > 30 || dtime < 0)
		dtime = 5;
	dtime = 1000000 / dtime;

	port = optcfg_get_int(opts, "port", -1);
	if (port < 0) {
		fprintf(stderr, "Error: TCP port information is not found.\n");
		return EXIT_FAILURE;
	}

	host = optcfg_get(opts, "host", NULL);

	root = optcfg_get(opts, "root", ".");
	snprintf(ROOT, sizeof(ROOT), "%s/", root);

	if (webcam_list(cams, &cam_cnt)) {
		fprintf(stderr, "Error: Can't get list of cameras!\n");
		return 1;
	}

	if (cam_cnt == 0) {
		fprintf(stderr, "Error: no cameras connected!\n");
		return 1;
	}

	signal(SIGINT, sigint);

	cam = webcam_open(cams[0], 640, 480);
	if (!cam) {
		http_server_free(srv);
		fprintf(stderr, "Error: can't open camera!\n");
		return 1;
	}
	snprintf(CAM_NAME, sizeof(CAM_NAME), "WEBCAM: %s", cam->name);

    srv = http_server_new(host, port);
    if (!srv) {
        fprintf(stderr, "Error: can't create web server!\n");
        return 1;
    }

    http_server_static_file(srv, "/index.html", "index.html");
    http_server_static_file(srv, "/jquery-2.1.3.min.js", "jquery-2.1.3.min.js");
    http_server_get(srv, "/image.jpg", current_image_get, NULL);
	http_server_get(srv, "/jpeg/", current_image_get, NULL);
	http_server_get(srv, "/sound_enabled.txt", snd_enabled_get, NULL);
	http_server_get(srv, "/audio.wav*", snd_wav_get, NULL);
    http_server_static_file(srv, "/", "index.html");

	webcam_start(cam);

	printf("Waiting for the first frame...");
	fflush(stdout);

	while (!FRAME) {
		if (webcam_wait_frame_cb(cam, new_frame, NULL, 10) < 0) {
			webcam_stop(cam);
			webcam_close(cam);
            http_server_free(srv);
			fprintf(stderr, "Error: can't get frames from camera!\n");
			return 1;
		}

		gettimeofday(&last, NULL);
	}
	printf("           Ok\n");
	fflush(stdout);

	/* Init sound: */
	snd_cmd = optcfg_get(opts, "exec", NULL);
	if (snd_cmd) {
		bsecs = optcfg_get_int(opts, "delay", 1);
		freq = optcfg_get_int(opts, "frequency", 8000);
		bits = optcfg_get_int(opts, "bits", 8);
		stereo = optcfg_get_flag(opts, "stereo");

		printf("Starting sound...\n");
		sound = snd_open(snd_cmd, bsecs, 8, freq, bits, stereo);

		if (!sound) {
			fprintf(stderr, "WARNING: can't open sound!\n");
		}
	}

    http_server_start(srv);
	for (;;) {
		int cam_status;

		gettimeofday(&cur, NULL);
		if (delta_time(&last, &cur) > dtime) {
			cam_status = webcam_wait_frame_cb(cam, new_frame, NULL, 10);
			if (cam_status < 0)
				break;
			if (cam_status > 0)
				memcpy(&last, &cur, sizeof(struct timeval));
		}

		if (exit_now)
			break;

        http_server_update(srv);
	}
    http_server_stop(srv);
	webcam_stop(cam);
	webcam_close(cam);

	if (sound) {
		snd_stop(sound);
		sound = NULL;
	}

	printf("EXIT!\n");
	http_server_free(srv);

	return 0;
}

static int current_image_get(http_context_t *cnx, void *param)
{
	char date[80];
	time_t curtime = time(NULL);
	struct tm *gmt = gmtime(&curtime);

	strftime(date, sizeof(date) - 1, "%c", gmt);

	if (!FRAME) {
		return -1; /* TODO! */
	}

    http_set_header(cnx, "accept-ranges", "bytes");
    http_set_header(cnx, "date", date);
    http_set_header(cnx, "content-type", "image/jpeg");
    http_set_header(cnx, "cache-control", "no-cache");

    LOCK();
    http_write(cnx, FRAME, FRAME_SZ);
    UNLOCK();

	return 0;
}

static int snd_enabled_get(http_context_t *cnx, void *param)
{
	http_set_header(cnx, "content-type", "text/plain");

	if (sound) {
		http_write(cnx, "TRUE", -1);
	} else {
		http_write(cnx, "FALSE", -1);
	}

	return 0;
}

static int snd_wav_get(http_context_t *cnx, void *param)
{
	char date[80];
	time_t curtime = time(NULL);
	struct tm *gmt = gmtime(&curtime);
	unsigned char *buf = NULL;
	size_t buf_len = 0;
	unsigned id = 0;

	strftime(date, sizeof(date) - 1, "%c", gmt);

	id = snd_current_buf(sound);
	printf("SND: %u --- ", id);
	if (snd_buf(sound, &id, &buf, &buf_len)) {
		return -1; /* TODO! */
	}
	printf("%u\n", id);

    http_set_header(cnx, "accept-ranges", "bytes");
    http_set_header(cnx, "content-type", "audio/wave");
    http_set_header(cnx, "date", date);
    http_set_header(cnx, "cache-control", "no-cache");
    http_write(cnx, buf, buf_len);

	return 0;
}

#if defined(WITH_LIBJPEG) && WITH_LIBJPEG

#include <jpeglib.h>

static int save_jpeg(unsigned char **buf, size_t *len, int quality, unsigned char *data, int width, int height, int bpl)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *b = NULL;
	unsigned long l = 0;
	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_mem_dest(&cinfo, &b, &l);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_EXT_RGB;

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = (void*)&data[cinfo.next_scanline * bpl];
		(void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);

	*buf = b;
	*len = l;

	jpeg_destroy_compress(&cinfo);

	return 0;
}
#else
#include <jpge.h>
static int save_jpeg(unsigned char **buf, size_t *len, int quality, unsigned char *data, int width, int height, int bpl)
{
	struct jpeg_params params;
	void *pbuf = NULL;
	int plen = 0;

	jpeg_params_init(&params);
	params.m_quality = quality;

	if (!compress_image_to_jpeg_file_alloc_memory(&pbuf, &plen, width, height, 3, data, &params)) {
		return -1;
	}
	*buf = pbuf;
	*len = plen;

	return 0;
}
#endif

static void new_frame(void *ctx, webcam_t *cam, unsigned char *pixels, size_t bpl, size_t size)
{
	unsigned char *buf = NULL;
	size_t len = 0;

	if (save_jpeg(&buf, &len, 75, pixels, cam->width, cam->height, bpl)) {
		fprintf(stderr, "Error: can't save frame!\n");
		return;
	}

	LOCK();
		free(FRAME);
		FRAME = buf;
		FRAME_SZ = len;
	UNLOCK();
}

