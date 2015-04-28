#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>

#include "sound.h"

/* Sound support for wwwcam */

static int sound(void *ptr);

static void wle32(unsigned char *to, unsigned val)
{
	to[0] = val & 0xff;
	to[1] = (val >> 8) & 0xff;
	to[2] = (val >> 16) & 0xff;
	to[3] = (val >> 24) & 0xff;
}

static void wle16(unsigned char *to, unsigned val)
{
	to[0] = val & 0xff;
	to[1] = (val >> 8) & 0xff;
}

struct snd_ctx* snd_open(const char *cmdline, unsigned buf_secs, unsigned bufs_cnt, unsigned freq, unsigned bits, int stereo)
{
	struct snd_ctx* res;
	size_t buf_sz;
	unsigned i;

	if (bits & 0x03) {
		return NULL;
	}

	if (buf_secs == 0)
		buf_secs = 1;

	res = calloc(1, sizeof(struct snd_ctx));
	if (!res) {
		return NULL;
	}

	if (bufs_cnt < 3)
		bufs_cnt = 8;

	buf_sz = buf_secs * freq * (bits / 8);
	if (stereo)
		buf_sz *= 2;

	res->bufs = calloc(bufs_cnt, sizeof(struct snd_buf));
	if (!res->bufs) {
		free(res);
		return NULL;
	}
	res->bufs_cnt = bufs_cnt;
	res->buf_size = buf_sz;
	res->secs = buf_secs;
	res->freq = freq;
	res->bits = bits;
	res->stereo = stereo;
	res->cur = 0;
	res->last_id = 0;
	res->stop = 0;
	res->error = 0;

	/* Header: */
	res->header[0] = 'R';
	res->header[1] = 'I';
	res->header[2] = 'F';
	res->header[3] = 'F';
	wle32(res->header + 4, res->buf_size + SND_WAVE_HEADER_SIZE - 4);
	res->header[8] = 'W';
	res->header[9] = 'A';
	res->header[10] = 'V';
	res->header[11] = 'E';
	res->header[12] = 'f';
	res->header[13] = 'm';
	res->header[14] = 't';
	res->header[15] = ' ';
	wle32(res->header + 16, 16); /* Chunk size */
	wle16(res->header + 20, 0x01); /* PCM */
	wle16(res->header + 22, stereo? 2: 1); /* channels */
	wle32(res->header + 24, freq); /* sampling rate */
	wle32(res->header + 28, stereo? 2 * freq * (bits / 8): freq * (bits / 8)); /* Data rate */
	wle16(res->header + 32, bits / 8); /* block align */
	wle16(res->header + 34, bits); /* bits per sample */
	res->header[36] = 'd';
	res->header[37] = 'a';
	res->header[38] = 't';
	res->header[39] = 'a';
	wle32(res->header + 40, res->buf_size);

	for (i = 0; i < bufs_cnt; i++) {
		res->bufs[i].buf = calloc(1, buf_sz + SND_WAVE_HEADER_SIZE);
		if (!res->bufs[i].buf) {
			while (i > 0) {
				free(res->bufs[i - 1].buf);
				--i;
			}
			free(res->bufs);
			free(res);
			return NULL;
		}
		memcpy(res->bufs[i].buf, res->header, SND_WAVE_HEADER_SIZE);
		res->bufs[i].id = 0;
	}
	/* all buffers allocated */

	if (mtx_init(&res->lock, mtx_plain) != thrd_success) {
		for (i = 0; i < res->bufs_cnt; i++)
			free(res->bufs[i].buf);
		free(res->bufs);
		free(res);
		return NULL;
	}

	res->src = popen(cmdline, "r");
	if (!res->src) {
		fprintf(stderr, "Error: can't exec `%s': %s\n", cmdline, strerror(errno));
		mtx_destroy(&res->lock);
		for (i = 0; i < res->bufs_cnt; i++)
			free(res->bufs[i].buf);
		free(res->bufs);
		free(res);
		return NULL;
	}

	if (thrd_create(&res->thread, sound, res) != thrd_success) {
		pclose(res->src);
		mtx_destroy(&res->lock);
		for (i = 0; i < res->bufs_cnt; i++)
			free(res->bufs[i].buf);
		free(res->bufs);
		free(res);
		return NULL;
	}

	return res;
}

static unsigned deltatime(struct timeval *from, struct timeval *to)
{
	return (to->tv_sec - from->tv_sec) * 1000000 + (to->tv_usec - from->tv_usec);
}

static int sound(void *ptr)
{
	struct snd_ctx *snd = ptr;
	unsigned next;
	struct timeval last;
	struct timeval cur;

	gettimeofday(&last, NULL);

	snd->last_id = 1;
	/*  Main function :) */
	for (;;) {
		gettimeofday(&cur, NULL);
		if (deltatime(&last, &cur) < snd->secs * 1000000) {
			usleep(10);
			continue;
		}

		mtx_lock(&snd->lock);
		if (snd->stop) {
			mtx_unlock(&snd->lock);
			break;
		}

		next = (snd->cur + 1) % snd->bufs_cnt;

		mtx_unlock(&snd->lock);

		if (fread(snd->bufs[next].buf + SND_WAVE_HEADER_SIZE, snd->buf_size, 1, snd->src) != 1) {
			mtx_lock(&snd->lock);
			snd->error = 1;
			mtx_unlock(&snd->lock);
			break;
		}

		gettimeofday(&last, NULL);

		mtx_lock(&snd->lock);
		snd->cur = next;
		snd->bufs[next].id = snd->last_id++;
		mtx_unlock(&snd->lock);
	}

	thrd_exit(0);

	return 0; /* not reached */
}

void snd_stop(struct snd_ctx *snd)
{
	int res;
	unsigned i;

	if (!snd)
		return;

	mtx_lock(&snd->lock);
	snd->stop = 1;
	mtx_unlock(&snd->lock);

	if (thrd_join(snd->thread, &res) != thrd_success) {
		/* WTF? */
		return;
	}

	pclose(snd->src);
	mtx_destroy(&snd->lock);

	for (i = 0; i < snd->bufs_cnt; i++)
		free(snd->bufs[i].buf);
	free(snd->bufs);
	free(snd);

	return;
}

int snd_buf(struct snd_ctx *snd, unsigned *id, unsigned char **buf, size_t *len)
{
	unsigned i;

	if (!snd || !id || !buf || !len)
		return -1;

	mtx_lock(&snd->lock);
	if (snd->error) {
		mtx_unlock(&snd->lock);
		return -1;
	}

	for (i = 0; i < snd->bufs_cnt; i++) {
		if (snd->bufs[i].id == *id) {
			*buf = snd->bufs[i].buf;
			*len = snd->buf_size + SND_WAVE_HEADER_SIZE;
			mtx_unlock(&snd->lock);

			return 0;
		}
	}

	/* not found */
	*id = snd->bufs[snd->cur].id;
	*buf = snd->bufs[snd->cur].buf;
	*len = snd->buf_size + SND_WAVE_HEADER_SIZE;

	mtx_unlock(&snd->lock);

	return 0;
}

unsigned snd_current_buf(struct snd_ctx *snd)
{
	unsigned res;
	if (!snd)
		return 0;

	mtx_lock(&snd->lock);
	res = snd->bufs[snd->cur].id;
	mtx_unlock(&snd->lock);

	return res;
}

