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
	cfg->allocated = 32;
	cfg->count = 0;

	if (!cfg->names || !cfg->values) {
		free(cfg->names);
		free(cfg->values);
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
	}
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
	}

	cfg->values[i] = val;

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

	if (i != cfg->count - 1) {
		cfg->names[i] = cfg->names[cfg->count - 1];
		cfg->values[i] = cfg->values[cfg->count - 1];
	}

	--cfg->count;

	return 0;
}

int cmdcfg_array_parse(const char *str, char **array, unsigned *len)
{
	return -1;
}

