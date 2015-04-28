#include <libwebcam.h>
#include <mihl.h>
#include <tcp_utils.h>
#include <glovars.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "optcfg.h"
#include "sound.h"

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

static void LOCK(void)
{
}

static void UNLOCK(void)
{
}

static const char *get_content_type(const char *fnm);
static int send_f(mihl_cnx_t * cnx, char const *tag, char const *host, const char *filename);
static int file_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
static void new_frame(void *ctx, webcam_t *cam, unsigned char *pixels, size_t bpl, size_t size);
static int save_jpeg(unsigned char **buf, size_t *len, int quality, unsigned char *data, int width, int height, int bpl);
static int main_page_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
static int main_page_post(mihl_cnx_t *cnx, const char *tag, const char *host, int vars_cnt, char **names, char **values, void *param);
static int current_image_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
static int snd_enabled_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
static int snd_wav_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
static struct handler_st {
	const char *path;
	int (*get)(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
	int (*post)(mihl_cnx_t *cnx, const char *tag, const char *host, int vars_cnt, char **names, char **values, void *param);
	void *param;
} handlers[] = {
	{ "/", main_page_get, main_page_post, NULL },
	{ "/jquery-2.1.3.min.js", file_get, NULL, "jquery-2.1.3.min.js" },
	{ "/image.jpg", current_image_get, NULL, NULL },
	{ "/jpeg/*", current_image_get, NULL, NULL },
	{ "/sound_enabled.txt", snd_enabled_get, NULL, NULL },
	{ "/audio.wav*", snd_wav_get, NULL, NULL },
	{ NULL, NULL, NULL, NULL }
};

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
	int i;
	int cams[64];
	unsigned cam_cnt = 64;
	webcam_t *cam;
	mihl_ctx_t *ctx;
	struct timeval last, cur;
	long dtime;
	int port;
	const char *host;
	int bsecs, freq, bits, stereo;
	const char *snd_cmd = NULL;
	const char *root = NULL;
	
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

	ctx = mihl_init(host, port, 16, MIHL_LOG_ERROR | MIHL_LOG_WARNING |
			                                    MIHL_LOG_INFO | MIHL_LOG_INFO_VERBOSE);

	if (webcam_list(cams, &cam_cnt)) {
		fprintf(stderr, "Error: Can't get list of cameras!\n");
		return 1;
	}

	if (cam_cnt == 0) {
		fprintf(stderr, "Error: no cameras connected!\n");
		return 1;
	}

	if (!ctx) {
		fprintf(stderr, "Error: can't init mihl!\n");
		return 1;
	}

	signal(SIGINT, sigint);

	cam = webcam_open(cams[0], 640, 480);
	if (!cam) {
		mihl_end(ctx);
		fprintf(stderr, "Error: can't open camera!\n");
		return 1;
	}
	snprintf(CAM_NAME, sizeof(CAM_NAME), "WEBCAM: %s", cam->name);

	for (i = 0; handlers[i].path; i++) {
		if (handlers[i].get) {
			mihl_handle_get(ctx, handlers[i].path, handlers[i].get, handlers[i].param);
		}

		if (handlers[i].post) {
			mihl_handle_post(ctx, handlers[i].path, handlers[i].post, handlers[i].param);
		}
	}

	webcam_start(cam);

	printf("Waiting for the first frame...");
	fflush(stdout);

	while (!FRAME) {
		if (webcam_wait_frame_cb(cam, new_frame, NULL, 10) < 0) {
			webcam_stop(cam);
			webcam_close(cam);
			mihl_end(ctx);
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

	for (;;) {
		int cam_status;
		int status = mihl_server(ctx);

		if (status == -2)
			break;

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
	}

	webcam_stop(cam);
	webcam_close(cam);

	if (sound) {
		snd_stop(sound);
		sound = NULL;
	}

	printf("EXIT!\n");
	mihl_end(ctx);

	return 0;
}

static int main_page_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param)
{
	send_f(cnx, tag, host, "index.html");
	return 0;
}

static int main_page_post(mihl_cnx_t *cnx, const char *tag, const char *host, int vars_cnt, char **names, char **values, void *param)
{
	return main_page_get(cnx, tag, host, param);
}

static int write_all(int sock, const void *buf, size_t len)
{
	const unsigned char *p = buf;
	ssize_t l, pos = 0;

	while (pos < (ssize_t)len) {
		l = write(sock, p + pos, len - pos);
		if (l < 0) {
			return -1;
		}
		pos += l;
	}

	return pos;
}

static int current_image_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param)
{
	char header[512];
	ssize_t len;
	char date[80];
	time_t curtime = time(NULL);
	struct tm *gmt = gmtime(&curtime);

	strftime(date, sizeof(date) - 1, "%c", gmt);

	if (!FRAME) {
		return -1; /* TODO! */
	}

	len = snprintf(header, sizeof(header),
		"HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %u\r\n"
		"Date: %s\r\n"
		"Content-Type: image/jpeg\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n", (unsigned)FRAME_SZ, date);

	LOCK();
	if (write_all(cnx->sockfd, header, len) != len ||
			write_all(cnx->sockfd, FRAME, FRAME_SZ) != (ssize_t)FRAME_SZ) {
		UNLOCK();
		return -1;
	}
	UNLOCK();

	return 0;
}

static int snd_enabled_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param)
{
	if (sound) {
		mihl_add(cnx, "TRUE");
	} else {
		mihl_add(cnx, "FALSE");
	}

	mihl_send(cnx, NULL, "Content-Type: text/plain\r\n");
	return 0;
}

static int snd_wav_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param)
{
	char header[512];
	ssize_t len;
	char date[80];
	time_t curtime = time(NULL);
	struct tm *gmt = gmtime(&curtime);
	unsigned char *buf = NULL;
	size_t buf_len = 0;
	unsigned id = 0;

	strftime(date, sizeof(date) - 1, "%c", gmt);

	if (snd_buf(sound, &id, &buf, &buf_len)) {
		return -1; /* TODO! */
	}
	printf("SND: %u\n", id);

	len = snprintf(header, sizeof(header),
		"HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Content-Length: %u\r\n"
		"Date: %s\r\n"
		"Content-Type: audio/wave\r\n"
		"Connection: keep-alive\r\n"
		"Cache-Control: no-cache\r\n"
		"\r\n", (unsigned)buf_len, date);

	if (write_all(cnx->sockfd, header, len) != len ||
			write_all(cnx->sockfd, buf, buf_len) != (ssize_t)buf_len) {
		return -1;
	}

	return 0;
}

static int file_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param)
{
	return send_f(cnx, tag, host, (const char*)param);
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

static int send_f(mihl_cnx_t * cnx, char const *tag, char const *host, const char *filename)
{
	char path[512];
	const char *content_type = get_content_type(filename);

	if (strstr(filename, "..")) {
		return -1;
	}

	snprintf(path, sizeof(path), "%s/%s", ROOT, filename);

	return send_file(cnx, tag, path, content_type, 0);
}

static struct content_type {
	const char *ext;
	const char *type;
} CONTENT_TYPES[] = {
	{ "html", "text/html" },
	{ "htm", "text/html" },
	{ "HTML", "text/html" },
	{ "HTM", "text/html" },
	{ "js", "text/javascript" },
	{ "css", "text/css" },
	{ "txt", "text/plain" },
	{ "gif", "image/gif" },
	{ "jpeg", "image/jpeg" },
	{ "jpg", "image/jpeg" },
	{ "png", "image/png" },
	{ "svg", "image/svg+xml" },
	{ NULL, NULL }
};

static const char *get_content_type(const char *fnm)
{
	int ext;
	int i;

	ext = strlen(fnm);

	while (ext > 0 && fnm[ext - 1] != '.')
		--ext;

	if (!ext) {
		return "application/octet-stream";
	}

	for (i = 0; CONTENT_TYPES[i].ext; i++)
		if (!strcmp(fnm + ext, CONTENT_TYPES[i].ext))
			return CONTENT_TYPES[i].type;

	return "application/octet-stream";
}

