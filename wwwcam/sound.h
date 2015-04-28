#ifndef SOUND_H_INC
#define SOUND_H_INC

#include "c11threads.h"

#ifdef __cplusplus
extern "C" {
#endif /* } */

#define SND_WAVE_HEADER_SIZE 44

struct snd_buf {
	unsigned id;
	unsigned char *buf;
};

struct snd_ctx {
	struct snd_buf *bufs;
	unsigned bufs_cnt;
	size_t buf_size;

	/* Current buffer index: */
	unsigned cur;
	/* Last buffer Id: */
	unsigned last_id;

	unsigned char header[SND_WAVE_HEADER_SIZE];

	unsigned secs;
	unsigned freq;
	unsigned bits;
	int stereo;

	int error;

	FILE *src; /* Sound source */

	int stop;
	mtx_t lock;

	thrd_t thread;
};

struct snd_ctx* snd_open(const char *cmdline, unsigned buf_secs, unsigned bufs_cnt, unsigned freq, unsigned bits, int stereo);
void snd_stop(struct snd_ctx *snd);
/* Get sound buffer by Id.
 * If id == 0 returns current buffer.
 * In all cases id could be changed.
 * RETURNS 0 on success -1 on failure
 */
int snd_buf(struct snd_ctx *snd, unsigned *id, unsigned char **buf, size_t *len);

unsigned snd_current_buf(struct snd_ctx *snd);

/* extern "C" { */
#ifdef __cplusplus
}
#endif

#endif

