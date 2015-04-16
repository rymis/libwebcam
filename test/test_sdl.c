/* test_sdl.c */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include <libwebcam.h>

#include <SDL.h>

static int lineY = 100;

/* Create SDL_Surface from image: */
SDL_Surface* img2surface(unsigned width, unsigned height, unsigned bpl, unsigned char* img)
{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	const Uint32 Rmask = 0x000000FF;
	const Uint32 Gmask = 0x0000FF00;
	const Uint32 Bmask = 0x00FF0000;
#else
	const Uint32 Rmask = 0x00FF0000;
	const Uint32 Gmask = 0x0000FF00;
	const Uint32 Bmask = 0x000000FF;
#endif

	return SDL_CreateRGBSurfaceFrom(img, width, height, 24, bpl,
			Rmask, Gmask, Bmask, 0);
}

static void draw_image(void *screen_in, webcam_t *cam, unsigned char *image, size_t bpl, size_t size)
{
	SDL_Surface *bitmap;
	SDL_Rect dest;
	SDL_Surface *screen = screen_in;

	if (size < bpl * cam->height) {
		fprintf(stderr, "Error: length invalid!\n");
		return;
	}

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return;
	}

	bitmap = img2surface(cam->width, cam->height, bpl, image);
	/* Draw: */
	dest.x = 0;
	dest.y = 0;
	dest.w = bitmap->w;
	dest.h = bitmap->h;
	SDL_BlitSurface(bitmap, NULL, screen, &dest);
	SDL_FreeSurface(bitmap);

	if (SDL_MUSTLOCK(screen)) {
		SDL_UnlockSurface(screen);
	}

	dest.x = 0;
	dest.y = 0;
	dest.w = screen->w;
	dest.h = screen->h;
	SDL_UpdateRects(screen, 1, &dest);
}

int main(int argc, char *argv[])
{
	const unsigned width = 640;
	const unsigned height = 480;
	webcam_t *cam;
	int rv;

	SDL_Surface *screen;
	SDL_Event event;
	int quit = 0;
	struct timeval p_start, p_end;
	int fcnt = 0;
	int cams[64];
	unsigned cam_cnt = 64;

	if (webcam_list(cams, &cam_cnt)) {
		fprintf(stderr, "Error: Can't get list of cameras!\n");
		return 1;
	}

	if (cam_cnt == 0) {
		fprintf(stderr, "Error: no cameras connected!\n");
		return 1;
	}

	gettimeofday(&p_start, NULL);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Error: can not init SDL.\n");
		return 1;
	}
	atexit(SDL_Quit);

	/* Openning webcam: */
	cam = webcam_open(cams[0], width, height);
	if (!cam) {
		fprintf(stderr, "Error: can't open camera!\n");
		return 1;
	}

	/* Try to open screen: */
	screen = SDL_SetVideoMode(cam->width, cam->height + 256, 24, SDL_SWSURFACE);
	if (!screen) {
		webcam_close(cam);
		fprintf(stderr, "Error: can not set video mode: %s\n", SDL_GetError());
		return 1;
	}

	webcam_start(cam);
	printf("WxH: %dx%d\n", cam->width, cam->height);

	/* Wait for events: */
	for (;!quit;) {
		rv = webcam_wait_frame_cb(cam, draw_image, screen, 10);
		if (rv < 0) {
			fprintf(stderr, "Error: frame processing failed!\n");
			quit = 1;
		}
		if (rv > 0)
			++fcnt;

		while (SDL_PollEvent(&event)) { /* Check for SDL events */
			switch (event.type) {
				case SDL_QUIT: quit = 1; break;
				case SDL_MOUSEBUTTONDOWN: lineY = event.button.y; break;
			}
		}
	}
	webcam_stop(cam);

	gettimeofday(&p_end, NULL);
	if (fcnt) {
		int delta = 1000 * (p_end.tv_sec - p_start.tv_sec) + (p_end.tv_usec - p_start.tv_usec) / 1000;
		float fps = fcnt * 1000.0 / delta;

		printf("%d frames processed in %2.2fsecs (%f FPS)\n", fcnt, delta / 1000.0, fps);
	}

	SDL_FreeSurface(screen);

	return 0;
}


