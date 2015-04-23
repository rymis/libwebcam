#include <libwebcam.h>
#include <mihl.h>
#include <tcp_utils.h>
#include <glovars.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include "optcfg.h"

static char main_page_html[] =
#include "index_html.inc"
;

/* Simple webcam-based http camera interface */

static int exit_now = 0;
static void sigint(int sig)
{
	++exit_now;
}

unsigned char *FRAME = NULL;
size_t FRAME_SZ = 0;
char CAM_NAME[256] = "";
static void LOCK(void)
{
}

static void UNLOCK(void)
{
}

static void new_frame(void *ctx, webcam_t *cam, unsigned char *pixels, size_t bpl, size_t size);
static int save_jpeg(unsigned char **buf, size_t *len, int quality, unsigned char *data, int width, int height, int bpl);
static int main_page_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
static int main_page_post(mihl_cnx_t *cnx, const char *tag, const char *host, int vars_cnt, char **names, char **values, void *param);
static int current_image_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
static struct handler_st {
	const char *path;
	int (*get)(mihl_cnx_t *cnx, const char *tag, const char *host, void *param);
	int (*post)(mihl_cnx_t *cnx, const char *tag, const char *host, int vars_cnt, char **names, char **values, void *param);
	void *param;
} handlers[] = {
	{ "/", main_page_get, main_page_post, NULL },
	{ "/image.jpg", current_image_get, NULL, NULL },
	{ "/jpeg/*", current_image_get, NULL, NULL },
	{ NULL, NULL, NULL, NULL }
};

int main(int argc, char *argv[])
{
	struct optcfg_option options[] = {
		{ 'p', "port",
			"Specify port number to serve", 0, "8888" },
		{ 'h', "host",
			"Specify host name to listen", 0, NULL },
		{ 'F', "fps",
			"Specify how many frames per second will be captured", 0, "5" },
		{ 'c', "config",
			"Load configuration from file", OPTCFG_CFGFILE, NULL },
		{ 's', "enable-sound",
			"Enable sound support", OPTCFG_FLAG, "no" },
		{ 'd', "delay",
			"Sound delay and fragment size in seconds", 0, "1" },
		{ 'f', "frequency",
			"Sound frequency in Hz.", 0, "8000" },
		{ 'b', "bits",
			"Bits per fragment of sound", 0, "8" },
		{ 'S', "stereo",
			"Enable stereo mode", OPTCFG_FLAG, "no" }
	};
	unsigned options_cnt = sizeof(options) / sizeof(options[0]);
	struct optcfg *opts;
	int i;
	int cams[64];
	unsigned cam_cnt = 64;
	webcam_t *cam;
	mihl_ctx_t *ctx;
	
	opts = optcfg_new();
	if (!opts) {
		return EXIT_FAILURE;
	}

	if (optcfg_default_config(opts, "wwwcam")) {
		return EXIT_FAILURE;
	}

	if (optcfg_parse_options(opts, "wwwcam", argc, argv, options, options_cnt)) {
		return EXIT_FAILURE;
	}

	ctx = mihl_init(NULL, 8888, 16, MIHL_LOG_ERROR | MIHL_LOG_WARNING |
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
	}
	printf("           Ok\n");
	fflush(stdout);

	for (;;) {
		int status = mihl_server(ctx);

		if (status == -2)
			break;

		if (webcam_wait_frame_cb(cam, new_frame, NULL, 10) < 0)
			break;

		if (exit_now)
			break;
	}

	webcam_stop(cam);
	webcam_close(cam);

	printf("EXIT!\n");
	mihl_end(ctx);

	return 0;
}

static int main_page_get(mihl_cnx_t *cnx, const char *tag, const char *host, void *param)
{
	mihl_add(cnx, "%s", main_page_html);
	mihl_send(cnx, NULL, "Content-Type: text/html\r\n");
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


