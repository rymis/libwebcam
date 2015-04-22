#include "optcfg.h"
#include <string.h>
#include <stdarg.h>

void (*optcfg_log)(const char *s) = NULL;

static void err(const char *fmt, ...)
{
	char buf[512];
	va_list args;
	unsigned i;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	buf[511] = 0; /* paranoid */
	for (i = 0; buf[i]; i++) {
		if (buf[i] < ' ')
			buf[i] = ' ';
	}

	if (optcfg_log)
		optcfg_log(buf);
	else
		fprintf(stderr, "Error: %s\n", buf);
}

struct optcfg* optcfg_new(void)
{
	struct optcfg *cfg;

	cfg = calloc(1, sizeof(struct optcfg));
	if (!cfg) {
		err("Not enought memory");
		return NULL;
	}

	cfg->names = calloc(32, sizeof(char*));
	cfg->values = calloc(32, sizeof(char*));
	cfg->allocated = 32;
	cfg->count = 0;

	if (!cfg->names || !cfg->values) {
		err("Not enought memory");
		free(cfg->names);
		free(cfg->values);
		free(cfg);
		return NULL;
	}

	return cfg;
}

void optcfg_free(struct optcfg *cfg)
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

static int cfg_alloc(struct optcfg *cfg)
{
	unsigned cnt;
	void *tmp;

	if (cfg->allocated > 512) {
		cnt = cfg->allocated + 512;
	} else {
		cnt = cfg->allocated * 2;
	}

	if (cnt < cfg->allocated) {
		err("Very strange (counter overflow)");
		return -1; /* Overflow */
	}

	tmp = realloc(cfg->names, cnt * sizeof(char*));
	if (!tmp) {
		err("Not enought memory");
		return -1;
	} else {
		cfg->names = tmp;
	}

	tmp = realloc(cfg->values, cnt * sizeof(char*));
	if (!tmp) {
		err("Not enought memory");
		return -1;
	} else {
		cfg->values = tmp;
	}

	cfg->allocated = cnt;

	return 0;
}

int optcfg_set(struct optcfg *cfg, const char *name, const char *value)
{
	unsigned i;
	char *val;

	if (!cfg || !name || !value) {
		err("Invalid arguments");
		return -1;
	}

	val = strdup(value);
	if (!val) {
		err("Not enought memory");
		return -1;
	}

	for (i = 0; i < cfg->count; i++)
		if (!strcmp(cfg->names[i], name))
			break;

	if (i == cfg->count) {
		if (cfg->count >= cfg->allocated) {
			if (cfg_alloc(cfg)) {
				free(val);
				return -1;
			}
		}

		cfg->names[i] = strdup(name);
		if (!cfg->names[i]) {
			err("Not enought memory");
			free(val);
			return -1;
		}

		++cfg->count;
	}

	cfg->values[i] = val;

	return 0;
}

int optcfg_del(struct optcfg *cfg, const char *name)
{
	unsigned i;

	if (!cfg) {
		err("Invalid arguments");
		return -1;
	}

	for (i = 0; i < cfg->count; i++)
		if (!strcmp(cfg->names[i], name))
			break;

	if (i == cfg->count)
		return -1;

	free(cfg->names[i]);
	free(cfg->values[i]);

	if (i != cfg->count - 1) {
		cfg->names[i] = cfg->names[cfg->count - 1];
		cfg->values[i] = cfg->values[cfg->count - 1];
	}

	--cfg->count;

	return 0;
}

static char norm_char(char c)
{
	if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '@')) {
		c = '_';
	}

	return c;
}

/* Check if strings equals. Returns 0 if no and 1 if yes. */
static int sEQ(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return 0;

	while (*s1 && *s2)
		if (norm_char(*s1++) != norm_char(*s2++))
			return 0;

	return *s1 == *s2; /* It is could be IFF *s1 == 0 and *s2 == 0 */
}

const char* optcfg_get(struct optcfg *cfg, const char *name, const char *def)
{
	unsigned i;

	if (!cfg || !name) {
		err("Invalid arguments");
		return def;
	}

	for (i = 0; i < cfg->count; i++)
		if (sEQ(name, cfg->names[i]))
			return cfg->values[i];

	return def;
}

/* Parse integer and return 0 on success and -1 on errors */
static int parse_int(const char *s, int *r)
{
	int neg = 0;
	int v = 0;
	int v1;

	if (!s || !r)
		return -1;

	while (isspace(*s))
		++s;

	if (*s == '-') {
		++neg;
		++s;
	}

	if (*s == '0' && s[1] == 'x') {
		s += 2;

		while (*s) {
			if (*s >= '0' && *s <= '9') {
				v1 = v * 16 + (*s - '0');
			} else if (*s >= 'a' && *s <= 'f') {
				v1 = v * 16 + (*s - 'a') + 10;
			} else if (*s >= 'A' && *s <= 'F') {
				v1 = v * 16 + (*s - 'A') + 10;
			} else {
				/* Invalid characters */
				return -1;
			}
			
			if (v1 < v) {
				/* Overflow */
				return -1;
			}
			v = v1;
		}
	} else {
		while (*s >= '0' && *s <= '9') {
			v1 = v * 10 + (*s - '0');
			if (v1 < v) /* Overflow */
				return -1;
			v = v1;
		}

		while (isspace(*s))
			++s;
		if (*s)
			return -1;
	}

	*r = neg? -v: v;

	return 0;
}

int optcfg_get_int(struct optcfg *cfg, const char *name, int def)
{
	const char *v = optcfg_get(cfg, name, NULL);
	int r;

	if (!v) {
		return def;
	}

	if (parse_int(v, &r)) {
		err("Invalid integer: %s", v);
		return def;
	}

	return r;
}

int optcfg_get_list(struct optcfg *cfg, const char *name, char ***list, unsigned *len)
{
	const char *v = optcfg_get(cfg, name, NULL);

	if (!v) {
		return -1;
	}

	if (optcfg_array_parse(v, list, len)) {
		return -1;
	}

	return 0;
}


int optcfg_array_parse(const char *str, char ***array, unsigned *len)
{
	const char *p;
	const char *s;
	unsigned cnt = 1;
	unsigned i = 0;
	char **l = NULL;

	if (!str || !array || !len) {
		err("Invalid arguments");
		return -1;
	}

	for (p = str; *p; p++)
		if (*p == ',')
			++cnt;

	l = calloc(cnt, sizeof(char*));
	if (!l) {
		err("Not enought memory");
		return -1;
	}

	for (s = str; *s; s = p) {
		for (p = s; *p && *p != ','; p++) {
			l[i] = malloc(p - s + 1);
			if (!l[i]) {
				err("Not enought memory");
				while (i > 0) {
					free(l[i - 1]);
					--i;
				}

				free(l);

				return -1;
			}
			l[i][p - s] = 0;
			++i;
		}
	}

	*array = l;
	*len = cnt;

	return 0;
}

void optcfg_array_free(char **array, unsigned len)
{
	unsigned i;

	if (!array)
		return;

	for (i = 0; i < len; i++)
		free(array[i]);

	free(array);
}

struct sbuf {
	char *s;
	size_t len;
	size_t allocated;
};

static int sbuf_putc(struct sbuf *buf, int c)
{
	void *tmp;

	if (buf->len < buf->allocated) {
		buf->s[buf->len++] = c;
		return 0;
	}

	if (!buf->s) {
		buf->s = malloc(256);
		buf->allocated = 256;
		buf->len = 0;
		if (!buf->s) {
			err("Not enought memory");
			return -1;
		}
	} else {
		if (buf->allocated < 8192) {
			tmp = realloc(buf->s, buf->allocated * 2);
		} else {
			tmp = realloc(buf->s, buf->allocated + 8192);
		}
		if (!tmp) {
			err("Not enought memory");
			return -1;
		}
		buf->s = tmp;
	}

	buf->s[buf->len++] = c;
	return 0;
}

int optcfg_parse_file(struct optcfg *cfg, FILE *f)
{
	struct sbuf name = { NULL, 0, 0 };
	struct sbuf value = { NULL, 0, 0 };
	int lineno = 1;
	int c;
	int state;
	int res = 0;

#define S_WAIT_NAME   1
#define S_COMMENT     2
#define S_NAME        3
#define S_WAIT_EQ     4
#define S_WAIT_VAL    6
#define S_QSTRING     7
#define S_VALUE      8

	state = S_WAIT_NAME;

	while ((c = getc(f)) != EOF) {
		if (c == '\n')
			++lineno;

		switch (state) {
			case S_WAIT_NAME:
			{
				if (c >= 0 && c <= ' ') {
					/* do nothing */
				} else if (c == '#') {
					state = S_COMMENT;
				} else {
					name.len = 0;
					if (sbuf_putc(&name, c)) {
						res = -1;
						break;
					}
					state = S_NAME;
				}
			}
			break;

			case S_COMMENT:
			{
				if (c == '\n')
					state = S_WAIT_NAME;
			}
			break;

			case S_NAME:
			{
				if (c >= 0 && c <= ' ') {
					if (sbuf_putc(&name, 0)) {
						res = -1;
						break;
					}

					state = S_WAIT_EQ;
				} else if (c == '=') {
					if (sbuf_putc(&name, 0)) {
						res = -1;
						break;
					}

					value.len = 0;
					state = S_WAIT_VAL;
				} else {
					if (sbuf_putc(&name, c)) {
						res = -1;
						break;
					}
				}

			}
			break;

			case S_WAIT_EQ:
			{
				if (c >= 0 && c <= ' ') {
					/* do nothing */
				} else if (c == '=') {
					value.len = 0;
					state = S_WAIT_VAL;
				} else {
					err("Syntax error at line %d", lineno);
					res = -1;
					break;
				}
			}
			break;

			case S_WAIT_VAL:
			{
				if (c >= 0 && c <= ' ') {
					if (c == '\n') {
						if (sbuf_putc(&value, 0)) {
							res = -1;
							break;
						}

						if (optcfg_set(cfg, name.s, value.s)) {
							res = -1;
							break;
						}

						state = S_WAIT_NAME;
					}
				} else if (c == '#') {
					if (sbuf_putc(&value, 0)) {
						res = -1;
						break;
					}

					if (optcfg_set(cfg, name.s, value.s)) {
						res = -1;
						break;
					}

					state = S_COMMENT;
				} else if (c == '"') {
					state = S_QSTRING;
				} else {
					if (sbuf_putc(&value, c)) {
						res = -1;
						break;
					}
					state = S_VALUE;
				}
			}
			break;

			case S_VALUE:
			{
				if (c == '\n') {
					if (sbuf_putc(&value, 0)) {
						res = -1;
						break;
					}

					if (optcfg_set(cfg, name.s, value.s)) {
						res = -1;
						break;
					}

					state = S_WAIT_NAME;
				} else {
					if (sbuf_putc(&value, c)) {
						res = -1;
						break;
					}
				}
			}
			break;

			case S_QSTRING:
			{
				if (c == '"') {
					if (sbuf_putc(&value, 0)) {
						res = -1;
						break;
					}

					if (optcfg_set(cfg, name.s, value.s)) {
						res = -1;
						break;
					}

					state = S_WAIT_NAME;
					while ((c = getc(f)) != EOF) {
						if (c == '\n')
							break;
					}
				}

				if (c == '\\') {
					c = getc(f);
					if (c == EOF) {
						err("Unexpected end of string at line %d", lineno);
						res = -1;
						break;
					} else if (c == '\n') {
						c = '\n';
					} else if (c == '\r') {
						c = '\r';
					} else if (c == '\t') {
						c = '\t';
					}
				}

				if (sbuf_putc(&value, c)) {
					res = -1;
					break;
				}
			}
			break;

			default:
				err("Internal error");
				res = -1;
				break;
		}
	}

	if (state == S_VALUE) {
		if (sbuf_putc(&value, 0)) {
			res = -1;
		} else {
			if (optcfg_set(cfg, name.s, value.s)) {
				res = -1;
			}
		}
		state = S_WAIT_NAME;
	} else if (state == S_COMMENT) {
		state = S_WAIT_NAME;
	}

	if (state != S_WAIT_NAME) {
		err("Unexpected end of file");
	}

	free(name.s);
	free(value.s);

	return res;
}

/* Load configuration file from default configuration path.
 * Default configuration for UNIX-like systems is:
 *   ~/.prog/config
 *   /etc/prog.conf
 * For Windows it is ${USERDRIVE}/${USERPATH}/prog.conf
 */
int optcfg_default_config(struct optcfg *cfg, const char *prog);

struct optcfg* optcfg_parse_options(const char *prog,           /* Name of the program */
		int argc, const char *argv[],                   /* argc, argv          */
		struct optcfg_option *opts, unsigned opts_cnt); /* options             */

