#include "cmdcfg.h"
#include <string.h>

struct cmdcfg* cmdcfg_new(void)
{
	struct cmdcfg *cfg;

	cfg = calloc(1, sizeof(struct cmdcfg));
	if (!cfg) {
		return NULL;
	}

	cfg->names = calloc(32, sizeof(char*));
	cfg->values = calloc(32, sizeof(char*));
	cfg->funcs = calloc(32, sizeof(struct cmdfunc));
	cfg->allocated = 32;
	cfg->count = 0;

	if (!cfg->names || !cfg->values || !cfg->funcs) {
		free(cfg->names);
		free(cfg->values);
		free(cfg->funcs);
		free(cfg);
		return NULL;
	}

	return cfg;
}

void cmdcfg_free(struct cmdcfg *cfg)
{
	unsigned i;

	if (!cfg)
		return;

	for (i = 0; i < cfg->count; i++) {
		free(cfg->names[i]);
		free(cfg->values[i]);
		if (cfg->funcs[i].freectx)
			cfg->funcs[i].freectx(cfg->funcs[i].ctx);
	}
	free(cfg->funcs);
	free(cfg->names);
	free(cfg->values);
	free(cfg);

	return;
}

static int cfg_alloc(struct cmdcfg *cfg)
{
	unsigned cnt;
	void *tmp;

	if (cfg->allocated > 512) {
		cnt = cfg->allocated + 512;
	} else {
		cnt = cfg->allocated * 2;
	}

	if (cnt < cfg->allocated) {
		return CMDCFG_ERR_MEMORY; /* Overflow */
	}

	tmp = realloc(cfg->names, cnt * sizeof(char*));
	if (!tmp) {
		return CMDCFG_ERR_MEMORY;
	} else {
		cfg->names = tmp;
	}

	tmp = realloc(cfg->values, cnt * sizeof(char*));
	if (!tmp) {
		return CMDCFG_ERR_MEMORY;
	} else {
		cfg->values = tmp;
	}

	tmp = realloc(cfg->funcs, cnt * sizeof(struct cmdfunc));
	if (!tmp) {
		return CMDCFG_ERR_MEMORY;
	} else {
		cfg->funcs = tmp;
	}

	cfg->allocated = cnt;

	return 0;
}

int cmdcfg_set(struct cmdcfg *cfg, const char *name, const char *value)
{
	unsigned i;
	char *val;

	if (!cfg || !name || !value) {
		return CMDCFG_ERR_GENERAL;
	}

	val = strdup(value);
	if (!val) {
		return CMDCFG_ERR_MEMORY;
	}

	for (i = 0; i < cfg->count; i++)
		if (!strcmp(cfg->names[i], name))
			break;

	if (i == cfg->count) {
		if (cfg->count >= cfg->allocated) {
			if (cfg_alloc(cfg)) {
				free(val);
				return CMDCFG_ERR_MEMORY;
			}
		}

		cfg->names[i] = strdup(name);
		if (!cfg->names[i]) {
			free(val);
			return CMDCFG_ERR_MEMORY;
		}

		++cfg->count;
	} else {
		if (cfg->funcs[i].freectx) {
			cfg->funcs[i].freectx(cfg->funcs[i].ctx);
		}
	}

	cfg->values[i] = val;
	cfg->funcs[i].freectx = NULL;
	cfg->funcs[i].call = NULL;
	cfg->funcs[i].ctx = NULL;

	return 0;
}

int cmdcfg_set_func(struct cmdcfg *cfg,
		const char *name,
		int (*call)(void *ctx, int argc, char **argv, char **res),
		void *ctx,
		void (*freectx)(void *ctx))
{
	unsigned i;

	if (!cfg || !name || !call) {
		return CMDCFG_ERR_GENERAL;
	}

	for (i = 0; i < cfg->count; i++)
		if (!strcmp(cfg->names[i], name))
			break;

	if (i == cfg->count) {
		if (cfg->count >= cfg->allocated) {
			if (cfg_alloc(cfg)) {
				return CMDCFG_ERR_MEMORY;
			}
		}

		cfg->names[i] = strdup(name);
		if (!cfg->names[i]) {
			return CMDCFG_ERR_MEMORY;
		}

		++cfg->count;
	} else {
		free(cfg->values[i]);
	}

	cfg->values[i] = NULL;
	cfg->funcs[i].freectx = freectx;
	cfg->funcs[i].call = call;
	cfg->funcs[i].ctx = ctx;

	return 0;
}

int cmdcfg_del(struct cmdcfg *cfg, const char *name)
{
	unsigned i;

	if (!cfg) {
		return CMDCFG_ERR_GENERAL;
	}

	for (i = 0; i < cfg->count; i++)
		if (!strcmp(cfg->names[i], name))
			break;

	if (i == cfg->count)
		return CMDCFG_ERR_GENERAL;

	free(cfg->names[i]);
	free(cfg->values[i]);
	if (cfg->funcs[i].freectx)
		cfg->funcs[i].freectx(cfg->funcs[i].ctx);

	if (i != cfg->count - 1) {
		cfg->names[i] = cfg->names[cfg->count - 1];
		cfg->values[i] = cfg->values[cfg->count - 1];
		memcpy(cfg->funcs + i, cfg->funcs + (cfg->count - 1), sizeof(struct cmdfunc));
	}

	--cfg->count;

	return 0;
}

/* Read one string from stream. This function will read:
 *   identifier
 *   $var
 *   ${var}
 *   [subcommand....]
 *   {string}
 *   Function returns end position of new word. If there are no new words function will return 0.
 *   On errors function returns -1
 *   Function sets *pos = start position on success
 */
static ssize_t read_one(const char *str, ssize_t len, ssize_t *pos)
{
	ssize_t i;

	i = *pos;
	/* Searching for the word */



}

int cmdcfg_array_parse(const char *str, char **array, unsigned *len)
{
	return -1;
}

static int e_var();

int cmdcfg_exec(struct cmdcfg *cfg, const char *str, ssize_t len)
{
	ssize_t l = len < 0? strlen(str): len;
	ssize_t i = 0;

	if (!cfg) {
		return CMDCFG_ERR_GENERAL;
	}

	while (i < l && isspace(str[i])) {
		++i;
	}

	if (str[i] == '#') { /* Skip comment */
		while (i < l && str[i] != '\n')
			++i;
	}
}

