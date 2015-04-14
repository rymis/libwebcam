#ifndef LIBWEBCAM_H_INC
#define LIBWEBCAM_H_INC

#include <stdlib.h>

#ifdef WEBCAM_COLOR_T
/* User-defined type for color. It MUST be of size 4 */
typedef WEBCAM_COLOR_T webcam_color_t;
#else
# ifdef WIN32
#  include <windows.h>
typedef DWORD webcam_color_t;
# else
#  include <stdint.h>
typedef uint32_t webcam_color_t;
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif /* } */

#define webcam_color_r(c) (((c) >> 16) & 0xff)
#define webcam_color_g(c) (((c) >> 8) & 0xff)
#define webcam_color_b(c) ((c) & 0xff)
#define webcam_color_rgb(r, g, b) (((r) << 16) | ((g) << 8) | (b))

/* Here you can see only must frequently used controls */
typedef enum webcam_controls {
	WEBCAM_BRIGHTNESS,
	WEBCAM_CONTRAST,
	WEBCAM_SATURATION,
	WEBCAM_GAMMA
} webcam_controls_t;

typedef enum webcam_format {
	WEBCAM_RGB32,            /* 00000000 rrrrrrrr gggggggg bbbbbbbb as uint32_t */
	WEBCAM_RGB24,            /*          rrrrrrrr gggggggg bbbbbbbb             */
	WEBCAM_BGR24,            /*          bbbbbbbb gggggggg rrrrrrrr             */
	WEBCAM_RGB555,           /*                   0rrrrrgg gggbbbbb             */
	WEBCAM_RGB565,           /*                   rrrrrggg gggbbbbb             */
	WEBCAM_RGB332,           /*                            rrrgggbb             */
	WEBCAM_BGR233,           /*                            bbgggrrr             */
	WEBCAM_YUV,              /*          yyyyyyyy uuuuuuuu vvvvvvvv             */
	WEBCAM_YUV422,           /*          Y Cb Y Cr... as bytes                  */
	WEBCAM_GRAY,             /*                            yyyyyyyy             */
	WEBCAM_JPEG              /* JPEG encoded data                               */
} webcam_format_t;

typedef struct webcam {
	void *priv;

	/* Name of the camera. */
	char *name;

	/* Width and height of the image: */
	unsigned width, height;

	/* Image data */
	webcam_color_t *image;

	/* img contains uint32_t's in form 0x00rrggbb */
} webcam_t;

typedef void (*webcam_frame_cb)(void *ctx, webcam_t *cam, webcam_format_t fmt, void *pixels, size_t bpl, size_t size);

/* List all cameras connected */
int webcam_list(int *ids, unsigned *cnt);

/* Get the name of camera with number =num. You must free name with free. */
char* webcam_name(int id);

/* Try to open camera with number =num. Width and height is recomended values so you must look inside webcam_t for actual sizes. */
webcam_t* webcam_open(int id, unsigned width, unsigned height);

/* Close device and free resources */
void webcam_close(webcam_t *cam);

/* Enable capturing: */
int webcam_start(webcam_t *cam);

/* Disable capturing: */
int webcam_stop(webcam_t *cam);

/* Wait for next frame for maximum =delay ms (0 = forever) */
int webcam_wait_frame(webcam_t *cam, unsigned delay);

/* Wait for next frame but call function with arguments with data not copy it */
int webcam_wait_frame_cb(webcam_t *cam, webcam_frame_cb cb, void *arg, unsigned delay);

/* Set control to camera. Value must be in interval [0-100] */
int webcam_set_control(webcam_t *cam, webcam_controls_t id, int value);
/* Get control from camera. Returns value in [0-100] */
int webcam_get_control(webcam_t *cam, webcam_controls_t id);

/* Convert image from one format to another: */
int webcam_convert_image(unsigned width, unsigned height,
		webcam_format_t from_cs, size_t from_bpl,
		void *from_pixels, size_t from,
		webcam_format_t to_cs, size_t to_bpl,
		void *to_pixels, size_t *to_size,
		webcam_color_t *convert_buffer, size_t convert_buffer_cnt);

/* extern "C" { */
#ifdef __cplusplus
}
#endif

#endif

