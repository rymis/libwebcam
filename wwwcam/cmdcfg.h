#ifndef CMD_CFG_H_INC
#define CMD_CFG_H_INC

/* Command line and configuration files parser in one file */

#include <stdlib.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif /* } */

#define CMDCFG_ERR_GENERAL (-1)
#define CMDCFG_ERR_MEMORY  (-2)
#define CMDCFG_ERR_SYNTAX  (-3)

struct cmdcfg {
	/* Variables: */
	char **names;
	char **values;
	unsigned count;
	unsigned allocated;
};

struct cmdcfg_option {
	char opt;
	const char *name;
	const char *help;

	int type;
#define CMDCFG_TYPE_FLAG    1
#define CMDCFG_TYPE_NUMBER  2
#define CMDCFG_TYPE_STRING  3
#define CMDCFG_TYPE_LIST    4
};

struct cmdcfg* cmdcfg_new(void);
void cmdcfg_free(struct cmdcfg *cfg);

int cmdcfg_set(struct cmdcfg *cfg, const char *name, const char *value);
int cmdcfg_del(struct cmdcfg *cfg, const char *name);

const char* cmdcfg_get(struct cmdcfg *cfg, const char *name, const char *def);
int cmdcfg_get_int(struct cmdcfg *cfg, const char *name, int def);
int cmdcfg_get_list(struct cmdcfg *cfg, char **list, unsigned *len);

int cmdcfg_array_parse(const char *str, char **array, unsigned *len);

struct cmdcfg* cmdcfg_parse_options(int argc, const char *argv[], const char *config_file, struct cmdcfg_option *opts, unsigned opts_cnt);

/* extern "C" { */
#ifdef __cplusplus
}
#endif

#endif

