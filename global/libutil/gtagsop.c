/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
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
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "conf.h"
#include "gparam.h"
#include "dbop.h"
#include "die.h"
#include "gtagsop.h"
#include "locatestring.h"
#include "makepath.h"
#include "path.h"
#include "gpathop.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "strmake.h"
#include "tab.h"

static char	*genrecord(GTOP *);
static int	belongto(GTOP *, char *, char *);
static regex_t reg;

/*
 * format version:
 * GLOBAL-1.0 - 1.8	no idea about format version.
 * GLOBAL-1.9 - 2.24 	understand format version.
 *			support format version 1 (default).
 *			if (format version > 1) then print error message.
 * GLOBAL-3.0 - 4.0	understand format version.
 *			support format version 1 and 2.
 *			if (format version > 2) then print error message.
 * format version 1:
 *	original format.
 * format version 2:
 *	compact format, path index.
 */
static int	support_version = 2;	/* acceptable format version   */
static const char *tagslist[] = {"GPATH", "GTAGS", "GRTAGS", "GSYMS"};
static int init;
static char regexchar[256];
/*
 * dbname: return db name
 *
 *	i)	db	0: GPATH, 1: GTAGS, 2: GRTAGS, 3: GSYMS
 *	r)		dbname
 */
const char *
dbname(db)
int	db;
{
	assert(db >= 0 && db < GTAGLIM);
	return tagslist[db];
}
/*
 * makecommand: make command line to make global tag file
 *
 *	i)	comline	skelton command line
 *	i)	path	path name
 *	o)	sb	command line
 *
 * command skelton is like this:
 *	'gctags -r %s'
 * following skelton is allowed too.
 *	'gctags -r'
 */
void
makecommand(comline, path, sb)
char	*comline;
char	*path;
STRBUF	*sb;
{
	char	*p = locatestring(comline, "%s", MATCH_FIRST);

	if (p) {
		strbuf_nputs(sb, comline, p - comline);
		strbuf_puts(sb, path);
		strbuf_puts(sb, p + 2);
	} else {
		strbuf_puts(sb, comline);
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, path);
	}
}
/*
 * isregex: test whether or not regular expression
 *
 *	i)	s	string
 *	r)		1: is regex, 0: not regex
 */
int
isregex(s)
char	*s;
{
	int	c;

	while ((c = *s++) != '\0')
		if (isregexchar(c))
			return 1;
	return 0;
}
/*
 * formatcheck: check format of tag command's output
 *
 *	i)	line	input
 *	i)	flags	flag
 *	r)	0:	normal
 *		-1:	tag name
 *		-2:	line number
 *		-3:	path
 *
 * [example of right format]
 *
 * $1                $2 $3             $4
 * ----------------------------------------------------
 * main              83 ./ctags.c        main(argc, argv)
 */
int
formatcheck(line, flags)
char	*line;
int	flags;
{
	char	*p, *q;
	/*
	 * $1 = tagname: allowed any char except sepalator.
	 */
	p = q = line;
	while (*p && !isspace(*p))
		p++;
	while (*p && isspace(*p))
		p++;
	if (p == q)
		return -1;
	/*
	 * $2 = line number: must be digit.
	 */
	q = p;
	while (*p && !isspace(*p))
		if (!isdigit(*p))
			return -2;
		else
			p++;
	if (p == q)
		return -2;
	while (*p && isspace(*p))
		p++;
	/*
	 * $3 = path:
	 *	standard format: must start with './'.
	 *	compact format: must be digit.
	 */
	if (flags & GTAGS_PATHINDEX) {
		while (*p && !isspace(*p))
			if (!isdigit(*p))
				return -3;
			else
				p++;
	} else {
		if (!(*p == '.' && *(p + 1) == '/' && *(p + 2)))
			return -3;
	}
	return 0;
}
/*
 * gtags_setinfo: set info string.
 *
 *      i)      info    info string
 *
 * Currently this method is used for postgres.
 */
void
gtags_setinfo(info)
char *info;
{
	dbop_setinfo(info);
}
/*
 * gtags_open: open global tag.
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory (needed when compact format)
 *	i)	db	GTAGS, GRTAGS, GSYMS
 *	i)	mode	GTAGS_READ: read only
 *			GTAGS_CREATE: create tag
 *			GTAGS_MODIFY: modify tag
 *	i)	flags	GTAGS_COMPACT
 *			GTAGS_PATHINDEX
 *			GTAGS_POSTGRES
 *	r)		GTOP structure
 *
 * when error occurred, gtagopen doesn't return.
 * GTAGS_PATHINDEX needs GTAGS_COMPACT.
 */
GTOP	*
gtags_open(dbpath, root, db, mode, flags)
char	*dbpath;
char	*root;
int	db;
int	mode;
int	flags;
{
	GTOP	*gtop;
	char	*path;
	int	dbmode = 0;
	int	dbopflags = 0;

	/* initialize for isregex() */
	if (!init) {
		regexchar['^'] = regexchar['$'] = regexchar['{'] =
		regexchar['}'] = regexchar['('] = regexchar[')'] =
		regexchar['.'] = regexchar['*'] = regexchar['+'] =
		regexchar['['] = regexchar[']'] = regexchar['?'] =
		regexchar['\\'] = init = 1;
	}
	if ((gtop = (GTOP *)calloc(sizeof(GTOP), 1)) == NULL)
		die("short of memory.");
	gtop->db = db;
	gtop->mode = mode;
	gtop->openflags = flags;
	switch (gtop->mode) {
	case GTAGS_READ:
		dbmode = 0;
		break;
	case GTAGS_CREATE:
		dbmode = 1;
		break;
	case GTAGS_MODIFY:
		dbmode = 2;
		break;
	default:
		assert(0);
	}

	/*
	 * allow duplicate records.
	 */
	dbopflags = DBOP_DUP;
	if (flags & GTAGS_POSTGRES)
		dbopflags |= DBOP_POSTGRES;
	path = strdup(makepath(dbpath, dbname(db), NULL));
	if (path == NULL)
		die("short of memory.");
	gtop->dbop = dbop_open(path, dbmode, 0644, dbopflags);
	free(path);
	if (gtop->dbop == NULL) {
		if (dbmode == 1)
			die("cannot make %s.", dbname(db));
		die("%s not found.", dbname(db));
	}
	if (gtop->dbop->openflags & DBOP_POSTGRES)
		gtop->openflags |= GTAGS_POSTGRES;
	/*
	 * decide format version.
	 */
	gtop->format_version = 1;
	gtop->format = GTAGS_STANDARD;
	/*
	 * This is a special case. GSYMS had compact format even if
	 * format version 1.
	 */
	if (db == GSYMS)
		gtop->format |= GTAGS_COMPACT;
	if (gtop->mode == GTAGS_CREATE) {
		if (flags & GTAGS_COMPACT) {
			char	buf[80];

			gtop->format_version = 2;
			snprintf(buf, sizeof(buf),
				"%s %d", VERSIONKEY, gtop->format_version);
			dbop_put(gtop->dbop, VERSIONKEY, buf, "0");
			gtop->format |= GTAGS_COMPACT;
			dbop_put(gtop->dbop, COMPACTKEY, COMPACTKEY, "0");
			if (flags & GTAGS_PATHINDEX) {
				gtop->format |= GTAGS_PATHINDEX;
				dbop_put(gtop->dbop, PATHINDEXKEY, PATHINDEXKEY, "0");
			}
		}
	} else {
		/*
		 * recognize format version of GTAGS. 'format version record'
		 * is saved as a META record in GTAGS and GRTAGS.
		 * if 'format version record' is not found, it's assumed
		 * version 1.
		 */
		char	*p;

		if ((p = dbop_get(gtop->dbop, VERSIONKEY)) != NULL) {
			for (p += strlen(VERSIONKEY); *p && isspace(*p); p++)
				;
			gtop->format_version = atoi(p);
		}
		if (gtop->format_version > support_version)
			die("GTAGS seems new format. Please install the latest GLOBAL.");
		if (gtop->format_version > 1) {
			if (dbop_get(gtop->dbop, COMPACTKEY) != NULL)
				gtop->format |= GTAGS_COMPACT;
			if (dbop_get(gtop->dbop, PATHINDEXKEY) != NULL)
				gtop->format |= GTAGS_PATHINDEX;
		}
	}
	if (gtop->format & GTAGS_PATHINDEX || gtop->mode != GTAGS_READ) {
		if (gpath_open(dbpath, dbmode, dbopflags) < 0) {
			if (dbmode == 1)
				die("cannot create GPATH.");
			else
				die("GPATH not found.");
		}
	}
	/*
	 * Stuff for compact format.
	 */
	if (gtop->format & GTAGS_COMPACT) {
		assert(root != NULL);
		strlimcpy(gtop->root, root, sizeof(gtop->root));
		if (gtop->mode == GTAGS_READ)
			gtop->ib = strbuf_open(MAXBUFLEN);
		else
			gtop->sb = strbuf_open(0);
	}
	return gtop;
}
/*
 * gtags_put: put tag record with packing.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	tag	tag name
 *	i)	record	ctags -x image
 *	i)	fid	file id.
 *
 * NOTE: If format is GTAGS_COMPACT then this function is destructive.
 */
void
gtags_put(gtop, tag, record, fid)
GTOP	*gtop;
char	*tag;
char	*record;
char	*fid;
{
#define PARTS 4
	char *line, *path;
	char *parts[PARTS];

	if (gtop->format == GTAGS_STANDARD) {
		/* entab(record); */
		dbop_put(gtop->dbop, tag, record, fid);
		return;
	}
	/*
	 * gtop->format & GTAGS_COMPACT
	 */
	if (split(record, '\t', PARTS, parts) != PARTS)
		die("illegal format.");
	line = parts[1];
	path = parts[2];
	/*
	 * First time, it occurs, because 'prev_tag' and 'prev_path' are NULL.
	 */
	if (strcmp(gtop->prev_tag, tag) || strcmp(gtop->prev_path, path)) {
		if (gtop->prev_tag[0]) {
			dbop_put(gtop->dbop, gtop->prev_tag, strbuf_value(gtop->sb), gtop->prev_fid);
		}
		strlimcpy(gtop->prev_tag, tag, sizeof(gtop->prev_tag));
		strlimcpy(gtop->prev_path, path, sizeof(gtop->prev_path));
		strlimcpy(gtop->prev_fid, fid, sizeof(gtop->prev_fid));
		/*
		 * Start creating new record.
		 */
		strbuf_reset(gtop->sb);
		strbuf_puts(gtop->sb, strmake(record, " \t"));
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, path);
		strbuf_putc(gtop->sb, ' ');
		strbuf_puts(gtop->sb, line);
	} else {
		strbuf_putc(gtop->sb, ',');
		strbuf_puts(gtop->sb, line);
	}
}
/*
 * gtags_add: add tags belonging to the path into tag file.
 *
 *	i)	gtop	descripter of GTOP
 *	i)	comline	tag command line
 *	i)	path	source file
 *	i)	flags	GTAGS_UNIQUE, GTAGS_EXTRACTMETHOD, GTAGS_DEBUG
 */
void
gtags_add(gtop, comline, path, flags)
GTOP	*gtop;
char	*comline;
char	*path;
int	flags;
{
	char	*ctags_x;
	FILE	*ip;
	STRBUF	*sb = strbuf_open(0);
	STRBUF	*ib = strbuf_open(MAXBUFLEN);
	STRBUF	*sort_command = strbuf_open(0);
	STRBUF	*sed_command = strbuf_open(0);
	char	*fid;

	/*
	 * get command name of sort and sed.
	 */
	if (!getconfs("sort_command", sort_command))
		die("cannot get sort command name.");
#if defined(_WIN32) || defined(__DJGPP__)
	if (!locatestring(strbuf_value(sort_command), ".exe", MATCH_LAST))
		strbuf_puts(sort_command, ".exe");
#endif
	if (!getconfs("sed_command", sed_command))
		die("cannot get sed command name.");
#if defined(_WIN32) || defined(__DJGPP__)
	if (!locatestring(strbuf_value(sed_command), ".exe", MATCH_LAST))
		strbuf_puts(sed_command, ".exe");
#endif
	/*
	 * add path index if not yet.
	 */
	gpath_put(path);
	/*
	 * make command line.
	 */
	makecommand(comline, path, sb);
	/*
	 * get file id.
	 */
	if (gtop->format & GTAGS_PATHINDEX || gtop->openflags & GTAGS_POSTGRES) {
		if (!(fid = gpath_path2fid(path)))
			die("GPATH is corrupted.('%s' not found)", path);
	} else
		fid = "0";
	/*
	 * Compact format.
	 */
	if (gtop->format & GTAGS_PATHINDEX) {
		strbuf_puts(sb, "| ");
		strbuf_puts(sb, strbuf_value(sed_command));
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "\"s@");
		strbuf_puts(sb, path);
		strbuf_puts(sb, "@");
		strbuf_puts(sb, fid);
		strbuf_puts(sb, "@\"");
	}
	if (gtop->format & GTAGS_COMPACT) {
		strbuf_puts(sb, "| ");
		strbuf_puts(sb, strbuf_value(sort_command));
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, "+0 -1 +1n -2");
	}
	if (flags & GTAGS_UNIQUE)
		strbuf_puts(sb, " -u");
	if (flags & GTAGS_DEBUG)
		fprintf(stderr, "gtags_add() executing '%s'\n", strbuf_value(sb));
	if (!(ip = popen(strbuf_value(sb), "r")))
		die("cannot execute '%s'.", strbuf_value(sb));
	while ((ctags_x = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		char	*tag, *p;

		strbuf_trim(ib);
		if (formatcheck(ctags_x, gtop->format) < 0)
			die("invalid parser output.\n'%s'", ctags_x);
		tag = strmake(ctags_x, " \t");		 /* tag = $1 */
		/*
		 * extract method when class method definition.
		 *
		 * Ex: Class::method(...)
		 *
		 * key	= 'method'
		 * data = 'Class::method  103 ./class.cpp ...'
		 */
		if (flags & GTAGS_EXTRACTMETHOD) {
			if ((p = locatestring(tag, ".", MATCH_LAST)) != NULL)
				tag = p + 1;
			else if ((p = locatestring(tag, "::", MATCH_LAST)) != NULL)
				tag = p + 2;
		}
		gtags_put(gtop, tag, ctags_x, fid);
	}
	pclose(ip);
	strbuf_close(sort_command);
	strbuf_close(sed_command);
	strbuf_close(sb);
	strbuf_close(ib);
}
/*
 * belongto: wheather or not record belongs to the path.
 *
 *	i)	gtop	GTOP structure
 *	i)	path	path name (in standard format)
 *			path number (in compact format)
 *	i)	p	record
 *	r)		1: belong, 0: not belong
 */
static int
belongto(gtop, path, p)
GTOP	*gtop;
char	*path;
char	*p;
{
	char	*q;
	int	length = strlen(path);

	/*
	 * seek to path part.
	 */
	if (gtop->format & GTAGS_PATHINDEX) {
		for (q = p; *q && !isspace(*q); q++)
			;
		if (*q == 0)
			die("invalid tag format. '%s'", p);
		for (; *q && isspace(*q); q++)
			;
	} else
		q = locatestring(p, "./", MATCH_FIRST);
	if (*q == 0)
		die("invalid tag format. '%s'", p);
	if (!strncmp(q, path, length) && isspace(*(q + length)))
		return 1;
	return 0;
}
/*
 * gtags_delete: delete records belong to path.
 *
 *	i)	gtop	GTOP structure
 *	i)	path	path name
 */
void
gtags_delete(gtop, path)
GTOP	*gtop;
char	*path;
{
	char *p, *fid;
	/*
	 * In compact format, a path is saved as a file number.
	 */
	if (gtop->format & GTAGS_PATHINDEX)
		if ((path = gpath_fid2path(path)) == NULL)
			die("GPATH is corrupted.('%s' not found)", path);
#ifdef USE_POSTGRES
	if (gtop->openflags & GTAGS_POSTGRES) {
		char *fid;

		if ((fid = gpath_path2fid(path)) == NULL)
			die("GPATH is corrupted.('%s' not found)", path);
		dbop_delete_by_fid(gtop->dbop, fid);
		return;
	}
#endif
	/*
	 * read sequentially, because db(1) has just one index.
	 */
	for (p = dbop_first(gtop->dbop, NULL, NULL, 0); p; p = dbop_next(gtop->dbop))
		if (belongto(gtop, path, p))
			dbop_delete(gtop->dbop, NULL);
	/*
	 * don't delete from path index.
	 */
}
/*
 * gtags_first: return first record
 *
 *	i)	gtop	GTOP structure
 *	i)	pattern	tag name
 *		o may be regular expression
 *		o may be NULL
 *	i)	flags	GTOP_PREFIX	prefix read
 *			GTOP_KEY	read key only
 *			GTOP_NOSOURCE	don't read source file(compact format)
 *			GTOP_NOREGEX	don't use regular expression.
 *			GTOP_IGNORECASE	ignore case distinction.
 *	r)		record
 */
char *
gtags_first(gtop, pattern, flags)
GTOP	*gtop;
char	*pattern;
int	flags;
{
	int	dbflags = 0;
	char	*line;
	char    prefix[IDENTLEN+1], *p;
	regex_t *preg = &reg;
	char	*key;
	int	regflags = REG_EXTENDED;

	gtop->flags = flags;
	if (flags & GTOP_PREFIX && pattern != NULL)
		dbflags |= DBOP_PREFIX;
	if (flags & GTOP_KEY)
		dbflags |= DBOP_KEY;
	if (flags & GTOP_IGNORECASE)
		regflags |= REG_ICASE;

	if (flags & GTOP_NOREGEX) {
		key = pattern;
		preg = NULL;
	} else if (pattern == NULL || !strcmp(pattern, ".*")) {
		key = NULL;
		preg = NULL;
	} else if (isregex(pattern) && regcomp(preg, pattern, regflags) == 0) {
		if (!(flags & GTOP_IGNORECASE) && *pattern == '^' && *(p = pattern + 1) && !isregexchar(*p)) {
			int i = 0;

			while (*p && !isregexchar(*p) && i < IDENTLEN)
				prefix[i++] = *p++;
			prefix[i] = '\0';
			key = prefix;
			dbflags |= DBOP_PREFIX;
		} else {
			key = NULL;
		}
	} else {
		key = pattern;
		preg = NULL;
	}
	if ((line = dbop_first(gtop->dbop, key, preg, dbflags)) == NULL)
		return NULL;
	if (gtop->format == GTAGS_STANDARD || gtop->flags & GTOP_KEY)
		return line;
	/*
	 * Compact format.
	 */
	gtop->line = line;			/* gtop->line = $0 */
	gtop->opened = 0;
	return genrecord(gtop);
}
/*
 * gtags_next: return followed record
 *
 *	i)	gtop	GTOP structure
 *	r)		record
 *			NULL end of tag
 */
char *
gtags_next(gtop)
GTOP	*gtop;
{
	char	*line;

	/*
	 * If it is standard format or only key.
	 * Just return it.
	 */
	if (gtop->format == GTAGS_STANDARD || gtop->flags & GTOP_KEY)
		return dbop_next(gtop->dbop);
	/*
	 * gtop->format & GTAGS_COMPACT
	 */
	if ((line = genrecord(gtop)) != NULL)
		return line;
	/*
	 * read next record.
	 */
	if ((line = dbop_next(gtop->dbop)) == NULL)
		return line;
	gtop->line = line;			/* gtop->line = $0 */
	gtop->opened = 0;
	return genrecord(gtop);
}
/*
 * gtags_close: close tag file
 *
 *	i)	gtop	GTOP structure
 */
void
gtags_close(gtop)
GTOP	*gtop;
{
	if (gtop->format & GTAGS_PATHINDEX || gtop->mode != GTAGS_READ)
		gpath_close();
	if (gtop->sb && gtop->prev_tag[0])
		dbop_put(gtop->dbop, gtop->prev_tag, strbuf_value(gtop->sb), "0");
	if (gtop->sb)
		strbuf_close(gtop->sb);
	if (gtop->ib)
		strbuf_close(gtop->ib);
	dbop_close(gtop->dbop);
	free(gtop);
}
static char *
genrecord(gtop)
GTOP	*gtop;
{
	static char	output[MAXBUFLEN+1];
	char	path[MAXPATHLEN+1];
	static char	buf[1];
	char	*buffer = buf;
	char	*lnop;
	int	tagline;

	if (!gtop->opened) {
		char	*p, *q;

		gtop->opened = 1;
		p = gtop->line;
		q = gtop->tag;				/* gtop->tag = $1 */
		while (!isspace(*p))
			*q++ = *p++;
		*q = 0;
		for (; isspace(*p) ; p++)
			;
		if (gtop->format & GTAGS_PATHINDEX) {	/* gtop->path = $2 */
			char	*name;

			q = path;
			while (!isspace(*p))
				*q++ = *p++;
			*q = 0;
			if ((name = gpath_fid2path(path)) == NULL)
				die("GPATH is corrupted.('%s' not found)", path);
			strlimcpy(gtop->path, name, sizeof(gtop->path));
		} else {
			q = gtop->path;
			while (!isspace(*p))
				*q++ = *p++;
			*q = 0;
		}
		for (; isspace(*p) ; p++)
			;
		gtop->lnop = p;			/* gtop->lnop = $3 */

		if (gtop->root)
			snprintf(path, sizeof(path),
				"%s/%s", gtop->root, &gtop->path[2]);
		else
			snprintf(path, sizeof(path), "%s", &gtop->path[2]);
		if (!(gtop->flags & GTOP_NOSOURCE)) {
			if ((gtop->fp = fopen(path, "r")) != NULL)
				gtop->lno = 0;
		}
	}

	lnop = gtop->lnop;
	if (*lnop >= '0' && *lnop <= '9') {
		/* get line number */
		for (tagline = 0; *lnop >= '0' && *lnop <= '9'; lnop++)
			tagline = tagline * 10 + *lnop - '0';
		if (*lnop == ',')
			lnop++;
		gtop->lnop = lnop;
		if (gtop->fp) {
			if (gtop->lno == tagline)
				return output;
			while (gtop->lno < tagline) {
				if (!(buffer = strbuf_fgets(gtop->ib, gtop->fp, STRBUF_NOCRLF)))
					die("unexpected end of file. '%s'", path);
				strbuf_trim(gtop->ib);
				gtop->lno++;
			}
		}
		snprintf(output, sizeof(output), "%-16s %4d %-16s %s",
			gtop->tag, tagline, gtop->path, buffer);
		return output;
	}
	if (gtop->opened && gtop->fp != NULL) {
		gtop->opened = 0;
		fclose(gtop->fp);
		gtop->fp = NULL;
	}
	return NULL;
}
