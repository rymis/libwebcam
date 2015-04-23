#ifndef OPT_CFG_H_INC
#define OPT_CFG_H_INC

/* Command line and configuration files parser in one file */

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* } */

struct optcfg {
	/* Variables: */
	char **names;
	char **values;
	unsigned count;
	unsigned allocated;
};

struct optcfg_option {
	char opt;
	const char *name;

	const char *help;

	int flags;
#define OPTCFG_MANDATORY     1
#define OPTCFG_FLAG          2
#define OPTCFG_CFGFILE       4
	const char *defval;
};

struct optcfg* optcfg_new(void);
void optcfg_free(struct optcfg *cfg);

int optcfg_set(struct optcfg *cfg, const char *name, const char *value);
int optcfg_del(struct optcfg *cfg, const char *name);

const char* optcfg_get(struct optcfg *cfg, const char *name, const char *def);
int optcfg_get_int(struct optcfg *cfg, const char *name, int def);
int optcfg_get_list(struct optcfg *cfg, const char *name, char ***list, unsigned *len);

int optcfg_array_parse(const char *str, char ***array, unsigned *len);
void optcfg_array_free(char **array, unsigned len);

int optcfg_parse_file(struct optcfg *cfg, FILE *from);
/* Load configuration file from default configuration path.
 * Default configuration for UNIX-like systems is:
 *   ~/.prog/config
 *   /etc/prog.conf
 * For Windows it is ${USERDRIVE}/${USERPATH}/prog.conf
 */
int optcfg_default_config(struct optcfg *cfg, const char *prog);

int optcfg_parse_options(struct optcfg *config,                 /* configuration template */
		const char *prog,                               /* Name of the program    */
		int argc, char **argv,                   /* argc, argv             */
		struct optcfg_option *opts, unsigned opts_cnt); /* options                */

void optcfg_print_help(const char *prog, struct optcfg_option *opts, unsigned opts_cnt, FILE *out);

/* extern "C" { */
#ifdef __cplusplus
}
#endif

#endif

