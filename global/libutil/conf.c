/*
 * Copyright (c) 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001, 2002 Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <ctype.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "gparam.h"
#include "conf.h"
#include "die.h"
#include "env.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "strmake.h"
#include "test.h"
#include "usable.h"

static FILE *fp;
static STRBUF *ib;
static char *confline;
/*
 * 8 level nested tc= or include= is allowed.
 */
static int allowed_nest_level = 8;
static int opened;

static void trim(char *);
static char *readrecord(const char *);
static void includelabel(STRBUF *, const char *, int);

#ifndef isblank
#define isblank(c)	((c) == ' ' || (c) == '\t')
#endif

/*
 * trim: trim string.
 *
 * : var1=a b :
 *	|
 *	v
 * :var1=a b :
 */
static void
trim(l)
	char *l;
{
	char *f, *b;
	int colon = 0;

	/*
	 * delete blanks.
	 */
	for (f = b = l; *f; f++) {
		if (colon && isblank(*f))
			continue;
		colon = 0;
		if ((*b++ = *f) == ':')
			colon = 1;
	}
	*b = 0;
	/*
	 * delete duplicate semi colons.
	 */
	for (f = b = l; *f;) {
		if ((*b++ = *f++) == ':') {
			while (*f == ':')
				f++;
		}
	}
	*b = 0;
}
/*
 * readrecord: read recoed indexed by label.
 *
 *	i)	label	label in config file
 *	r)		record
 *
 * Jobs:
 * o skip comment.
 * o append following line.
 * o format check.
 */
static char *
readrecord(label)
	const char *label;
{
	char *p;
	int flag = STRBUF_NOCRLF;
	int count = 0;

	rewind(fp);
	while ((p = strbuf_fgets(ib, fp, flag)) != NULL) {
		count++;
		/*
		 * ignore \<new line>.
		 */
		flag &= ~STRBUF_APPEND;
		if (*p == '#' || *p == '\0')
			continue;
		if (strbuf_unputc(ib, '\\')) {
			flag |= STRBUF_APPEND;
			continue;
		}
		trim(p);
		for (;;) {
			char *candidate;
			/*
			 * pick up candidate.
			 */
			if ((candidate = strmake(p, "|:")) == NULL)
				die("invalid config file format (line %d).", count);
			if (!strcmp(label, candidate)) {
				if (!(p = locatestring(p, ":", MATCH_FIRST)))
					die("invalid config file format (line %d).", count);
				p = strdup(p);
				if (!p)
					die("short of memory.");
				return p;
			}
			/*
			 * locate next candidate.
			 */
			p += strlen(candidate);
			if (*p == ':')
				break;
			else if (*p == '|')
				p++;
			else
				die("invalid config file format (line %d).", count);
		}
	}
	/*
	 * config line not found.
	 */
	return NULL;
}
/*
 * includelabel: procedure for tc= (or include=)
 *
 *	o)	sb	string buffer
 *	i)	label	record label
 *	i)	level	nest level for check
 */
static	void
includelabel(sb, label, level)
	STRBUF	*sb;
	const char *label;
	int	level;
{
	char *savep, *p, *q;

	if (++level > allowed_nest_level)
		die("nested include= (or tc=) over flow.");
	if (!(savep = p = readrecord(label)))
		die("label '%s' not found.", label);
	while ((q = locatestring(p, ":include=", MATCH_FIRST)) || (q = locatestring(p, ":tc=", MATCH_FIRST))) {
		STRBUF *inc = strbuf_open(0);

		strbuf_nputs(sb, p, q - p);
		q = locatestring(q, "=", MATCH_FIRST) + 1;
		for (; *q && *q != ':'; q++)
			strbuf_putc(inc, *q);
		includelabel(sb, strbuf_value(inc), level);
		p = q;
		strbuf_close(inc);
	}
	strbuf_puts(sb, p);
	free(savep);
}
/*
 * configpath: get path of configuration file.
 */
static char *
configpath() {
	static char config[MAXPATHLEN+1];
	char *p;

	/*
	 * at first, check environment variable GTAGSCONF.
	 */
	if (getenv("GTAGSCONF") != NULL)
		strlimcpy(config, getenv("GTAGSCONF"), sizeof(config));
	/*
	 * if GTAGSCONF not set then check standard config files.
	 */
	else if ((p = get_home_directory()) && test("r", makepath(p, GTAGSRC, NULL)))
		strlimcpy(config, makepath(p, GTAGSRC, NULL), sizeof(config));
#ifdef __DJGPP__
	else if ((p = get_home_directory()) && test("r", makepath(p, DOS_GTAGSRC, NULL)))
		strlimcpy(config, makepath(p, DOS_GTAGSRC, NULL), sizeof(config));
#endif
	else if (test("r", GTAGSCONF))
		strlimcpy(config, GTAGSCONF, sizeof(config));
	else if (test("r", OLD_GTAGSCONF))
		strlimcpy(config, OLD_GTAGSCONF, sizeof(config));
	else if (test("r", DEBIANCONF))
		strlimcpy(config, DEBIANCONF, sizeof(config));
	else if (test("r", OLD_DEBIANCONF))
		strlimcpy(config, OLD_DEBIANCONF, sizeof(config));
	else
		return NULL;
	return config;
}
/*
 * openconf: load configuration file.
 *
 *	go)	line	specified entry
 */
void
openconf()
{
	STRBUF *sb;
	char *config;
	extern int vflag;

	assert(opened == 0);
	opened = 1;

	/*
	 * if config file not found then return default value.
	 */
	if (!(config = configpath())) {
		if (vflag)
			fprintf(stderr, " Using default configuration.\n");
		confline = strdup("");
		if (!confline)
			die("short of memory.");
	}
	/*
	 * if it is not an absolute path then assumed config value itself.
	 */
	else if (!isabspath(config)) {
		confline = strdup(config);
		if (!confline)
			die("short of memory.");
		if (!locatestring(confline, ":", MATCH_FIRST))
			die("GTAGSCONF must be absolute path name.");
	}
	/*
	 * else load value from config file.
	 */
	else {
		const char *label;

		if (test("d", config))
			die("config file '%s' is a directory.", config);
		if (!test("f", config))
			die("config file '%s' not found.", config);
		if (!test("r", config))
			die("config file '%s' is not readable.", config);
		if ((label = getenv("GTAGSLABEL")) == NULL)
			label = "default";
	
		if (!(fp = fopen(config, "r")))
			die("cannot open '%s'.", config);
		if (vflag)
			fprintf(stderr, " Using config file '%s'.\n", config);
		ib = strbuf_open(MAXBUFLEN);
		sb = strbuf_open(0);
		includelabel(sb, label, 0);
		confline = strdup(strbuf_value(sb));
		if (!confline)
			die("short of memory.");
		strbuf_close(ib);
		strbuf_close(sb);
		fclose(fp);
	}
	/*
	 * make up lacked variables.
	 */
	sb = strbuf_open(0);
	strbuf_puts(sb, confline);
	strbuf_unputc(sb, ':');
	if (!getconfs("suffixes", NULL)) {
		strbuf_puts(sb, ":suffixes=");
		strbuf_puts(sb, DEFAULTSUFFIXES);
	}
	if (!getconfs("skip", NULL)) {
		strbuf_puts(sb, ":skip=");
		strbuf_puts(sb, DEFAULTSKIP);
	}
	/*
	 * GTAGS, GRTAGS and GSYMS have no default values but non of them
	 * specified then use default values.
	 * (Otherwise, nothing to do for gtags.)
	 */
	if (!getconfs("GTAGS", NULL) && !getconfs("GRTAGS", NULL) && !getconfs("GSYMS", NULL)) {
		char *path;

		/*
		 * Some GNU/Linux has gctags as '/usr/bin/gctags', that is
		 * different from GLOBAL's one.
		 * usable search in BINDIR at first.
		 */
#if defined(_WIN32) || defined(__DJGPP__)
		path = "gctags.exe";
#else
		path = usable("gctags");
		if (!path)
			path = "gctags";
#endif /* _WIN32 */
		strbuf_puts(sb, ":GTAGS=");
		strbuf_puts(sb, path);
		strbuf_puts(sb, " -dt %s");
		strbuf_puts(sb, ":GRTAGS=");
		strbuf_puts(sb, path);
		strbuf_puts(sb, " -dtr %s");
		strbuf_puts(sb, ":GSYMS=");
		strbuf_puts(sb, path);
		strbuf_puts(sb, " -dts %s");
	}
	if (!getconfs("sort_command", NULL))
		strbuf_puts(sb, ":sort_command=sort");
	if (!getconfs("sed_command", NULL))
		strbuf_puts(sb, ":sed_command=sed");
	strbuf_unputc(sb, ':');
	strbuf_putc(sb, ':');
	confline = strdup(strbuf_value(sb));
	if (!confline)
		die("short of memory.");
	strbuf_close(sb);
	trim(confline);
	return;
}
/*
 * getconfn: get property number
 *
 *	i)	name	property name
 *	o)	num	value (if not NULL)
 *	r)		1: found, 0: not found
 */
int
getconfn(name, num)
	const char *name;
	int *num;
{
	char *p;
	char buf[MAXPROPLEN+1];

	if (!opened)
		openconf();
	snprintf(buf, sizeof(buf), ":%s#", name);
	if ((p = locatestring(confline, buf, MATCH_FIRST)) != NULL) {
		p += strlen(buf);
		if (num != NULL)
			*num = atoi(p);
		return 1;
	}
	return 0;
}
/*
 * getconfs: get property string
 *
 *	i)	name	property name
 *	o)	sb	string buffer (if not NULL)
 *	r)		1: found, 0: not found
 */
int
getconfs(name, sb)
	const char *name;
	STRBUF *sb;
{
	char *p;
	char buf[MAXPROPLEN+1];
	int all = 0;
	int exist = 0;

	if (!opened)
		openconf();
	if (!strcmp(name, "suffixes") || !strcmp(name, "skip"))
		all = 1;
	snprintf(buf, sizeof(buf), ":%s=", name);
	p = confline;
	while ((p = locatestring(p, buf, MATCH_FIRST)) != NULL) {
		if (exist && sb)
			strbuf_putc(sb, ',');		
		exist = 1;
		for (p += strlen(buf); *p && *p != ':'; p++) {
			if (*p == '\\')	/* quoted character */
				p++;
			if (sb)
				strbuf_putc(sb, *p);
		}
		if (!all)
			break;
	}
	/*
	 * If 'bindir' and 'datadir' are not defined then
	 * return system configuration value.
	 */
	if (!exist) {
		if (!strcmp(name, "bindir")) {
			strbuf_puts(sb, BINDIR);
			exist = 1;
		} else if (!strcmp(name, "datadir")) {
			strbuf_puts(sb, DATADIR);
			exist = 1;
		}
	}
	return exist;
}
/*
 * getconfb: get property bool value
 *
 *	i)	name	property name
 *	r)		1: TRUE, 0: FALSE
 */
int
getconfb(name)
	const char *name;
{
	char buf[MAXPROPLEN+1];

	if (!opened)
		openconf();
	snprintf(buf, sizeof(buf), ":%s:", name);
	if (locatestring(confline, buf, MATCH_FIRST) != NULL)
		return 1;
	return 0;
}
/*
 * getconfline: print loaded config entry.
 */
char *
getconfline()
{
	if (!opened)
		openconf();
	return confline;
}
void
closeconf()
{
	if (!opened)
		return;
	free(confline);
	confline = NULL;
	opened = 0;
}
