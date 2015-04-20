#ifndef CMD_CFG_H_INC
#define CMD_CFG_H_INC

#include <stdlib.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif /* } */

#define CMDCFG_ERR_GENERAL (-1)
#define CMDCFG_ERR_MEMORY  (-2)
#define CMDCFG_ERR_SYNTAX  (-3)

struct cmdfunc {
	int (*call)(void *ctx, int argc, char **argv, char **res);
	void *ctx;
	void (*freectx)(void *ctx);
};

struct cmdcfg {
	/* Variables: */
	char **names;
	char **values;
	struct cmdfunc *funcs;
	unsigned count;
	unsigned allocated;
};

struct cmdcfg* cmdcfg_new(void);
void cmdcfg_free(struct cmdcfg *cfg);

int cmdcfg_set(struct cmdcfg *cfg, const char *name, const char *value);
int cmdcfg_set_func(struct cmdcfg *cfg,
		const char *name,
		int (*call)(void *ctx, int argc, char **argv, char **res),
		void *ctx,
		void (*freectx)(void *ctx));
int cmdcfg_del(struct cmdcfg *cfg, const char *name);
int cmdcfg_array_parse(const char *str, char **array, unsigned *len);
int cmdcfg_exec(struct cmdcfg *cfg, const char *str, ssize_t len);

/* extern "C" { */
#ifdef __cplusplus
}
#endif

#endif

