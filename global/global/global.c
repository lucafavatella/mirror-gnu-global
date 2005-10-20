/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2005
 *	Tama Communications Corporation
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

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
#include "getopt.h"

#include "global.h"
#include "regex.h"
#include "const.h"

static void usage(void);
static void help(void);
static void setcom(int);
int main(int, char **);
void makefilter(STRBUF *);
FILE *openfilter(void);
void closefilter(FILE *);
void completion(const char *, const char *, const char *);
void idutils(const char *, const char *);
void grep(const char *);
void pathlist(const char *, const char *);
void parsefile(int, char **, const char *, const char *, const char *, int);
static int exec_parser(const char *, STRBUF *, const char *, const char *, FILE *);
void printtag(FILE *, const char *);
int search(const char *, const char *, const char *, int);
void ffformat(char *, int, const char *);

STRBUF *sortfilter;			/* sort filter		*/
STRBUF *pathfilter;			/* path convert filter	*/
const char *localprefix;		/* local prefix		*/
int aflag;				/* [option]		*/
int cflag;				/* command		*/
int fflag;				/* command		*/
int gflag;				/* command		*/
int Gflag;				/* [option]		*/
int iflag;				/* [option]		*/
int Iflag;				/* command		*/
int lflag;				/* [option]		*/
int nflag;				/* [option]		*/
int oflag;				/* [option]		*/
int pflag;				/* command		*/
int Pflag;				/* command		*/
int qflag;				/* [option]		*/
int rflag;				/* [option]		*/
int sflag;				/* [option]		*/
int tflag;				/* [option]		*/
int Tflag;				/* [option]		*/
int uflag;				/* command		*/
int vflag;				/* [option]		*/
int xflag;				/* [option]		*/
int show_version;
int show_help;
int show_filter;			/* undocumented command */
int debug;
const char *extra_options;

static void
usage(void)
{
	if (!qflag)
		fputs(usage_const, stderr);
	exit(2);
}
static void
help(void)
{
	fputs(usage_const, stdout);
	fputs(help_const, stdout);
	exit(0);
}

static struct option const long_options[] = {
	{"absolute", no_argument, NULL, 'a'},
	{"completion", no_argument, NULL, 'c'},
	{"regexp", required_argument, NULL, 'e'},
	{"file", no_argument, NULL, 'f'},
	{"local", no_argument, NULL, 'l'},
	{"nofilter", no_argument, NULL, 'n'},
	{"grep", no_argument, NULL, 'g'},
	{"basic-regexp", no_argument, NULL, 'G'},
	{"ignore-case", no_argument, NULL, 'i'},
	{"idutils", optional_argument, NULL, 'I'},
	{"other", no_argument, NULL, 'o'},
	{"print-dbpath", no_argument, NULL, 'p'},
	{"path", no_argument, NULL, 'P'},
	{"quiet", no_argument, NULL, 'q'},
	{"reference", no_argument, NULL, 'r'},
	{"rootdir", no_argument, NULL, 'r'},
	{"symbol", no_argument, NULL, 's'},
	{"tags", no_argument, NULL, 't'},
	{"through", no_argument, NULL, 'T'},
	{"update", no_argument, NULL, 'u'},
	{"verbose", no_argument, NULL, 'v'},
	{"cxref", no_argument, NULL, 'x'},

	/* long name only */
	{"debug", no_argument, &debug, 1},
	{"version", no_argument, &show_version, 1},
	{"help", no_argument, &show_help, 1},
	{"filter", no_argument, &show_filter, 1},
	{ 0 }
};

static int command;
static void
setcom(int c)
{
	if (command == 0)
		command = c;
	else if (command != c)
		usage();
}
int
main(int argc, char **argv)
{
	const char *av = NULL;
	int count;
	int db;
	int optchar;
	int option_index = 0;
	char cwd[MAXPATHLEN+1];			/* current directory	*/
	char root[MAXPATHLEN+1];		/* root of source tree	*/
	char dbpath[MAXPATHLEN+1];		/* dbpath directory	*/
	const char *gtags;

	while ((optchar = getopt_long(argc, argv, "ace:ifgGIlnopPqrstTuvx", long_options, &option_index)) != EOF) {
		switch (optchar) {
		case 0:
			if (!strcmp("idutils", long_options[option_index].name))
				extra_options = optarg;
			break;
		case 'a':
			aflag++;
			break;
		case 'c':
			cflag++;
			setcom(optchar);
			break;
		case 'e':
			av = optarg;
			break;
		case 'f':
			fflag++;
			xflag++;
			setcom(optchar);
			break;
		case 'l':
			lflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'g':
			gflag++;
			setcom(optchar);
			break;
		case 'G':
			Gflag++;
			break;
		case 'i':
			iflag++;
			break;
		case 'I':
			Iflag++;
			setcom(optchar);
			break;
		case 'o':
			oflag++;
			break;
		case 'p':
			pflag++;
			setcom(optchar);
			break;
		case 'P':
			Pflag++;
			setcom(optchar);
			break;
		case 'q':
			qflag++;
			setquiet();
			break;
		case 'r':
			rflag++;
			break;
		case 's':
			sflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'T':
			Tflag++;
			break;
		case 'u':
			uflag++;
			setcom(optchar);
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		default:
			usage();
			break;
		}
	}
	if (qflag)
		vflag = 0;
	if (show_help)
		help();

	argc -= optind;
	argv += optind;
	/*
	 * At first, we pickup pattern from -e option. If it is not found
	 * then use argument which is not option.
	 */
	if (!av)
		av = (argc > 0) ? *argv : NULL;

	if (show_version)
		version(av, vflag);
	/*
	 * invalid options.
	 */
	if (sflag && rflag)
		die_with_code(2, "both of -s and -r are not allowed.");
	/*
	 * only -c, -u, -P and -p allows no argument.
	 */
	if (!av && !show_filter) {
		switch (command) {
		case 'c':
		case 'u':
		case 'p':
		case 'P':
			break;
		default:
			usage();
			break;
		}
	}
	/*
	 * -u and -p cannot have any arguments.
	 */
	if (av) {
		switch (command) {
		case 'u':
		case 'p':
			usage();
		default:
			break;
		}
	}
	if (fflag)
		lflag = 0;
	if (tflag)
		xflag = 0;
	/*
	 * remove leading blanks.
	 */
	if (!Iflag && !gflag && av)
		for (; *av == ' ' || *av == '\t'; av++)
			;
	if (cflag && av && isregex(av))
		die_with_code(2, "only name char is allowed with -c option.");
	/*
	 * get path of following directories.
	 *	o current directory
	 *	o root of source tree
	 *	o dbpath directory
	 *
	 * if GTAGS not found, getdbpath doesn't return.
	 */
	getdbpath(cwd, root, dbpath, (pflag && vflag));
	if (Iflag && !test("f", makepath(root, "ID", NULL)))
		die("You must have id-utils's index at the root of source tree.");
	/*
	 * print dbpath or rootdir.
	 */
	if (pflag) {
		fprintf(stdout, "%s\n", (rflag) ? root : dbpath);
		exit(0);
	}
	/*
	 * incremental update of tag files.
	 */
	gtags = usable("gtags");
	if (!gtags)
		die("gtags command not found.");
	gtags = strdup(gtags);
	if (!gtags)
		die("short of memory.");
	if (uflag) {
		STRBUF	*sb = strbuf_open(0);

		if (chdir(root) < 0)
			die("cannot change directory to '%s'.", root);
		strbuf_puts(sb, gtags);
		strbuf_puts(sb, " -i");
		if (vflag)
			strbuf_putc(sb, 'v');
		strbuf_putc(sb, ' ');
		strbuf_puts(sb, dbpath);
		if (system(strbuf_value(sb)))
			exit(1);
		strbuf_close(sb);
		exit(0);
	}

	/*
	 * complete function name
	 */
	if (cflag) {
		completion(dbpath, root, av);
		exit(0);
	}
	/*
	 * make local prefix.
	 */
	if (lflag) {
		char	*p = cwd + strlen(root);
		STRBUF	*sb = strbuf_open(0);
		/*
		 * getdbpath() assure follows.
		 * cwd != "/" and cwd includes root.
		 */
		strbuf_putc(sb, '.');
		if (*p)
			strbuf_puts(sb, p);
		strbuf_putc(sb, '/');
		localprefix = strdup(strbuf_value(sb));
		if (!localprefix)
			die("short of memory.");
		strbuf_close(sb);
	} else {
		localprefix = NULL;
	}
	/*
	 * Decide tag type.
	 */
	db = (rflag) ? GRTAGS : ((sflag) ? GSYMS : GTAGS);
	/*
	 * make sort filter.
	 */
	{
		int unique = 0;
		const char *sort;

		/*
		 * We cannot depend on the command PATH, because global(1)
		 * might be called from WWW server. Since usable() looks for
		 * the command in BINDIR directory first, gnusort need not
		 * be in command PATH.
		 */
		sort = usable("gnusort");
		if (!sort)
			die("gnusort not found.");
		sortfilter = strbuf_open(0);
		strbuf_puts(sortfilter, sort);
		if (sflag) {
			strbuf_puts(sortfilter, " -u");
			unique = 1;
		}
		if (tflag) 			/* ctags format */
			strbuf_puts(sortfilter, " -k 1,1 -k 2,2 -k 3,3n");
		else if (fflag) {
			STRBUF *sb = strbuf_open(0);
			/*
			 * By default, the -f option need sort filter,
			 * because there is a possibility that an external
			 * parser is used instead of gtags-parser(1).
			 * Clear the sort filter when understanding that
			 * the parser is gtags-parser.
			 */
			if (!getconfs(dbname(db), sb))
				die("cannot get parser for %s.", dbname(db));
			if (locatestring(strbuf_value(sb), "gtags-parser", MATCH_FIRST))
				strbuf_setlen(sortfilter, 0);
			else
				strbuf_puts(sortfilter, " -k 3,3 -k 2,2n");
			strbuf_close(sb);
		} else if (xflag)			/* print details */
			strbuf_puts(sortfilter, " -k 1,1 -k 3,3 -k 2,2n");
		else if (!unique)		/* print just a file name */
			strbuf_puts(sortfilter, " -u");
	}
	/*
	 * make path filter.
	 */
	pathfilter = strbuf_open(0);
	strbuf_puts(pathfilter, gtags);
	if (aflag)	/* absolute path name */
		strbuf_puts(pathfilter, " --absolute");
	else		/* relative path name */
		strbuf_puts(pathfilter, " --relative");
	if (xflag || tflag)
		strbuf_puts(pathfilter, " --cxref");
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, root);
	strbuf_putc(pathfilter, ' ');
	strbuf_puts(pathfilter, cwd);
	/*
	 * print filter.
	 */
	if (show_filter) {
		STRBUF  *sb = strbuf_open(0);

		makefilter(sb);
		fprintf(stdout, "%s\n", strbuf_value(sb));
		strbuf_close(sb);
		exit(0);
	}
	/*
	 * exec lid(id-utils).
	 */
	if (Iflag) {
		chdir(root);
		idutils(av, dbpath);
		exit(0);
	}
	/*
	 * grep the pattern in a source tree.
	 */
	if (gflag) {
		chdir(root);
		grep(av);
		exit(0);
	}
	/*
	 * locate the path including the pattern in a source tree.
	 */
	if (Pflag) {
		chdir(root);
		pathlist(dbpath, av);
		exit(0);
	}
	/*
	 * print function definitions.
	 */
	if (fflag) {
		parsefile(argc, argv, cwd, root, dbpath, db);
		exit(0);
	}
	/*
	 * search in current source tree.
	 */
	count = search(av, root, dbpath, db);
	/*
	 * search in library path.
	 */
	if (getenv("GTAGSLIBPATH") && (count == 0 || Tflag) && !lflag && !rflag) {
		STRBUF *sb = strbuf_open(0);
		char libdbpath[MAXPATHLEN+1];
		char *p, *lib;

		strbuf_puts(sb, getenv("GTAGSLIBPATH"));
		p = strbuf_value(sb);
		while (p) {
			lib = p;
			if ((p = locatestring(p, PATHSEP, MATCH_FIRST)) != NULL)
				*p++ = 0;
			if (!gtagsexist(lib, libdbpath, sizeof(libdbpath), 0))
				continue;
			if (!strcmp(dbpath, libdbpath))
				continue;
			if (!test("f", makepath(libdbpath, dbname(db), NULL)))
				continue;
			strbuf_reset(pathfilter);
			strbuf_puts(pathfilter, gtags);
			if (aflag)	/* absolute path name */
				strbuf_puts(pathfilter, " --absolute");
			else		/* relative path name */
				strbuf_puts(pathfilter, " --relative");
			if (xflag || tflag)
				strbuf_puts(pathfilter, " --cxref");
			strbuf_putc(pathfilter, ' ');
			strbuf_puts(pathfilter, lib);
			strbuf_putc(pathfilter, ' ');
			strbuf_puts(pathfilter, cwd);
			count = search(av, lib, libdbpath, db);
			if (count > 0 && !Tflag) {
				strlimcpy(dbpath, libdbpath, sizeof(dbpath));
				break;
			}
		}
		strbuf_close(sb);
	}
	if (vflag) {
		if (count) {
			if (count == 1)
				fprintf(stderr, "%d object located", count);
			if (count > 1)
				fprintf(stderr, "%d objects located", count);
		} else {
			fprintf(stderr, "'%s' not found", av);
		}
		if (!Tflag)
			fprintf(stderr, " (using '%s').\n", makepath(dbpath, dbname(db), NULL));
	}
	strbuf_close(sortfilter);
	strbuf_close(pathfilter);

	return 0;
}
/*
 * makefilter: make filter string.
 *
 *	o)	filter buffer
 */
void
makefilter(STRBUF *sb)
{
	if (!nflag) {
		strbuf_puts(sb, strbuf_value(sortfilter));
		if (strbuf_getlen(sortfilter) && strbuf_getlen(pathfilter))
			strbuf_puts(sb, " | ");
		strbuf_puts(sb, strbuf_value(pathfilter));
	}
}
/*
 * openfilter: open output filter.
 *
 *	gi)	pathfilter
 *	gi)	sortfilter
 *	r)		file pointer for output filter
 */
FILE *
openfilter(void)
{
	FILE *op;
	STRBUF *sb = strbuf_open(0);

	makefilter(sb);
	if (strbuf_getlen(sb) == 0)
		op = stdout;
	else
		op = popen(strbuf_value(sb), "w");
	strbuf_close(sb);
	return op;
}
void
closefilter(FILE *op)
{
	if (op != stdout)
		if (pclose(op) != 0)
			die("terminated abnormally.");
}
/*
 * completion: print completion list of specified prefix
 *
 *	i)	dbpath	dbpath directory
 *	i)	root	root directory
 *	i)	prefix	prefix of primary key
 */
void
completion(const char *dbpath, const char *root, const char *prefix)
{
	const char *p;
	int flags = GTOP_KEY;
	GTOP *gtop;
	int db;

	flags |= GTOP_NOREGEX;
	if (prefix && *prefix == 0)	/* In the case global -c '' */
		prefix = NULL;
	if (prefix)
		flags |= GTOP_PREFIX;
	db = (sflag) ? GSYMS : GTAGS;
	gtop = gtags_open(dbpath, root, db, GTAGS_READ, 0);
	for (p = gtags_first(gtop, prefix, flags); p; p = gtags_next(gtop)) {
		fputs(p, stdout);
		fputc('\n', stdout);
	}
	gtags_close(gtop);
}
/*
 * printtag: print a tag's line
 *
 *	i)	op	output stream
 *	i)	ctags_x	ctags -x format record
 */
void
printtag(FILE *op, const char *ctags_x)		/* virtually const */
{
	if (xflag) {
		fputs(ctags_x, op);
	} else {
		SPLIT ptable;

		/*
		 * Split tag line.
		 */
		split((char *)ctags_x, 4, &ptable);

		if (tflag) {
			fputs(ptable.part[PART_TAG].start, op);	/* tag */
			(void)putc('\t', op);
			fputs(ptable.part[PART_PATH].start, op);/* path */
			(void)putc('\t', op);
			fputs(ptable.part[PART_LNO].start, op);	/* line number */
		} else {
			fputs(ptable.part[PART_PATH].start, op);/* path */
		}
		recover(&ptable);
	}
	fputc('\n', op);
}
/*
 * idutils:  lid(id-utils) pattern
 *
 *	i)	pattern	POSIX regular expression
 *	i)	dbpath	GTAGS directory
 */
void
idutils(const char *pattern, const char *dbpath)
{
	FILE *ip, *op;
	STRBUF *ib = strbuf_open(0);
	char edit[IDENTLEN+1];
	const char *path, *lno, *lid;
	int linenum, count;
	char *p, *line;

	lid = usable("lid");
	if (!lid)
		die("lid(id-utils) not found.");
	/*
	 * convert spaces into %FF format.
	 */
	ffformat(edit, sizeof(edit), pattern);
	/*
	 * make lid command line.
	 */
	strbuf_puts(ib, lid);
	strbuf_puts(ib, " --separator=newline");
	if (!tflag && !xflag)
		strbuf_puts(ib, " --result=filenames --key=none");
	else
		strbuf_puts(ib, " --result=grep");
	if (iflag)
		strbuf_puts(ib, " --ignore-case");
	if (extra_options) {
		strbuf_putc(ib, ' ');
		strbuf_puts(ib, extra_options);
	}
	strbuf_putc(ib, ' ');
	strbuf_puts(ib, quote_string(pattern));
	if (debug)
		fprintf(stderr, "id-utils: %s\n", strbuf_value(ib));
	if (!(ip = popen(strbuf_value(ib), "r")))
		die("cannot execute '%s'.", strbuf_value(ib));
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	while ((line = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		p = line;
		/* extract filename */
		path = p;
		while (*p && *p != ':')
			p++;
		if ((xflag || tflag) && !*p)
			die("invalid lid(id-utils) output format. '%s'", line);
		*p++ = 0;
		if (lflag) {
			if (!locatestring(path, localprefix + 2, MATCH_AT_FIRST))
				continue;
		}
		count++;
		if (!xflag && !tflag) {
			fprintf(op, "./%s\n", path);
			continue;
		}
		/* extract line number */
		while (*p && isspace(*p))
			p++;
		lno = p;
		while (*p && isdigit(*p))
			p++;
		if (*p != ':')
			die("invalid lid(id-utils) output format. '%s'", line);
		*p++ = 0;
		linenum = atoi(lno);
		if (linenum <= 0)
			die("invalid lid(id-utils) output format. '%s'", line);
		/*
		 * print out.
		 */
		if (tflag)
			fprintf(op, "%s\t./%s\t%d\n", edit, path, linenum);
		else {
			char	buf[MAXPATHLEN+1];

			snprintf(buf, sizeof(buf), "./%s", path);
			fprintf(op, "%-16s %4d %-16s %s\n",
					edit, linenum, buf, p);
		}
	}
	if (pclose(ip) < 0)
		die("terminated abnormally.");
	closefilter(op);
	strbuf_close(ib);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "object not found");
		if (count == 1)
			fprintf(stderr, "%d object located", count);
		if (count > 1)
			fprintf(stderr, "%d objects located", count);
		fprintf(stderr, " (using id-utils index in '%s').\n", dbpath);
	}
}
/*
 * grep: grep pattern
 *
 *	i)	pattern	POSIX regular expression
 */
void
grep(const char *pattern)
{
	FILE *op, *fp;
	STRBUF *ib = strbuf_open(MAXBUFLEN);
	const char *path, *p;
	char edit[IDENTLEN+1];
	const char *buffer;
	int linenum, count;
	int flags = 0;
	regex_t	preg;

	/*
	 * convert spaces into %FF format.
	 */
	ffformat(edit, sizeof(edit), pattern);

	if (!Gflag)
		flags |= REG_EXTENDED;
	if (iflag)
		flags |= REG_ICASE;
	if (regcomp(&preg, pattern, flags) != 0)
		die("invalid regular expression.");
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	for (vfind_open(localprefix, oflag); (p = vfind_read()) != NULL; ) {
		path = (*p == ' ') ? ++p : p;
		if (!(fp = fopen(path, "r")))
			die("cannot open file '%s'.", path);
		linenum = 0;
		while ((buffer = strbuf_fgets(ib, fp, STRBUF_NOCRLF)) != NULL) {
			linenum++;
			if (regexec(&preg, buffer, 0, 0, 0) == 0) {
				count++;
				if (tflag)
					fprintf(op, "%s\t%s\t%d\n",
						edit, path, linenum);
				else if (!xflag) {
					fputs(path, op);
					fputc('\n', op);
					break;
				} else {
					fprintf(op, "%-16s %4d %-16s %s\n",
						edit, linenum, path, buffer);
				}
			}
		}
		fclose(fp);
	}
	vfind_close();
	closefilter(op);
	strbuf_close(ib);
	regfree(&preg);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "object not found");
		if (count == 1)
			fprintf(stderr, "%d object located", count);
		if (count > 1)
			fprintf(stderr, "%d objects located", count);
		fprintf(stderr, " (no index used).\n");
	}
}
/*
 * pathlist: print candidate path list.
 *
 *	i)	dbpath
 */
void
pathlist(const char *dbpath, const char *av)
{
	FILE *op;
	const char *path, *p;
	regex_t preg;
	int count;

	if (av) {
		int flags = 0;

		if (!Gflag)
			flags |= REG_EXTENDED;
		if (iflag || getconfb("icase_path"))
			flags |= REG_ICASE;
#ifdef _WIN32
		flags |= REG_ICASE;
#endif /* _WIN32 */
		if (regcomp(&preg, av, flags) != 0)
			die("invalid regular expression.");
	}
	if (!localprefix)
		localprefix = ".";
	if (!(op = openfilter()))
		die("cannot open output filter.");
	count = 0;
	for (vfind_open(localprefix, oflag); (p = vfind_read()) != NULL; ) {
		path = (*p == ' ') ? ++p : p;
		/*
		 * skip localprefix because end-user doesn't see it.
		 */
		p = path + strlen(localprefix) - 1;
		if (av && regexec(&preg, p, 0, 0, 0) != 0)
			continue;
		if (xflag)
			fprintf(op, "path\t1 %s \n", path);
		else if (tflag)
			fprintf(op, "path\t%s\t1\n", path);
		else {
			fputs(path, op);
			fputc('\n', op);
		}
		count++;
	}
	vfind_close();
	closefilter(op);
	if (av)
		regfree(&preg);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "path not found");
		if (count == 1)
			fprintf(stderr, "%d path located", count);
		if (count > 1)
			fprintf(stderr, "%d paths located", count);
		fprintf(stderr, " (using '%s').\n", makepath(dbpath, dbname(GPATH), NULL));
	}
}
/*
 * parsefile: parse file to pick up tags.
 *
 *	i)	argc
 *	i)	argv
 *	i)	cwd	current directory
 *	i)	root	root directory of source tree
 *	i)	dbpath	dbpath
 *	i)	db	type of parse
 */
void
parsefile(int argc, char **argv, const char *cwd, const char *root, const char *dbpath, int db)
{
	char rootdir[MAXPATHLEN+1];
	char buf[MAXPATHLEN+1], *path;
	FILE *op;
	int count;
	STRBUF *comline = strbuf_open(0);
	STRBUF *path_list = strbuf_open(MAXPATHLEN);
	int path_list_max;

	snprintf(rootdir, sizeof(rootdir), "%s/", root);
	/*
	 * teach parser where is dbpath.
	 */
	set_env("GTAGSDBPATH", dbpath);
	/*
	 * get parser.
	 */
	if (!getconfs(dbname(db), comline))
		die("cannot get parser for %s.", dbname(db));
	/*
	 * determine the maximum length of the list of paths.
	 */
	path_list_max = exec_line_limit(strbuf_getlen(comline));

	if (!(op = openfilter()))
		die("cannot open output filter.");
	if (gpath_open(dbpath, 0) < 0)
		die("GPATH not found.");
	count = 0;
	for (; argc > 0; argv++, argc--) {
		const char *av = argv[0];

		if (!test("f", av)) {
			if (test("d", av)) {
				if (!qflag)
					fprintf(stderr, "'%s' is a directory.\n", av);
			} else {
				if (!qflag)
					fprintf(stderr, "'%s' not found.\n", av);
			}
			continue;
		}
		/*
		 * convert path into relative from root directory of source tree.
		 */
		path = realpath(av, buf);
		if (path == NULL)
			die("realpath(%s, buf) failed. (errno=%d).", av, errno);
		if (!isabspath(path))
			die("realpath(3) is not compatible with BSD version.");
		/*
		 * Remove the root part of path and insert './'.
		 *      rootdir  /a/b/
		 *      path     /a/b/c/d.c -> c/d.c -> ./c/d.c
		 */
		path = locatestring(path, rootdir, MATCH_AT_FIRST);
		if (path == NULL) {
			if (!qflag)
				fprintf(stderr, "'%s' is out of source tree.\n", path);
			continue;
		}
		path -= 2;
		*path = '.';
		if (!gpath_path2fid(path)) {
			if (!qflag)
				fprintf(stderr, "'%s' not found in GPATH.\n", path);
			continue;
		}
		/*
		 * Execute parser when path name collects enough.
		 * Though the path_list is \0 separated list of string,
		 * we can think its length equals to the length of
		 * argument string because each \0 can be replaced
		 * with a blank.
		 */
		if (strbuf_getlen(path_list)) {
			if (strbuf_getlen(path_list) + strlen(path) > path_list_max) {
				count += exec_parser(strbuf_value(comline), path_list, cwd, root, op);
				strbuf_reset(path_list);
			}
		}
		/*
		 * Add a path to the path list.
		 */
		strbuf_puts0(path_list, path);
	}
	if (strbuf_getlen(path_list))
		count += exec_parser(strbuf_value(comline), path_list, cwd, root, op);
	gpath_close();
	closefilter(op);
	strbuf_close(comline);
	strbuf_close(path_list);
	if (vflag) {
		if (count == 0)
			fprintf(stderr, "object not found");
		if (count == 1)
			fprintf(stderr, "%d object located", count);
		if (count > 1)
			fprintf(stderr, "%d objects located", count);
		fprintf(stderr, " (no index used).\n");
	}
}
/*
 * exec_parser: execute parser
 *
 *	i)	parser		template of command line
 *	i)	path_list	\0 separated list of paths
 *	i)	cwd		current directory
 *	i)	root		root directory of source tree
 *	i)	op		filter to output
 *	r)			number of objects found
 */
static int
exec_parser(const char *parser, STRBUF *path_list, const char *cwd, const char *root, FILE *op)
{
	const char *p;
	FILE *ip;
	int count;
	STRBUF *com = strbuf_open(0);
	STRBUF *ib = strbuf_open(MAXBUFLEN);

	if (chdir(root) < 0)
		die("cannot move to '%s' directory.", root);
	/*
	 * make command line.
	 */
	makecommand(parser, path_list, com);
	if (debug)
		fprintf(stderr, "executing %s\n", strbuf_value(com));
	if (!(ip = popen(strbuf_value(com), "r")))
		die("cannot execute '%s'.", strbuf_value(com));

	count = 0;
	while ((p = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		count++;
		printtag(op, p);
	}

	if (pclose(ip) < 0)
		die("terminated abnormally.");
	if (chdir(cwd) < 0)
		die("cannot move to '%s' directory.", cwd);

	strbuf_close(com);
	strbuf_close(ib);

	return count;
}
/*
 * search: search specified function 
 *
 *	i)	pattern		search pattern
 *	i)	root		root of source tree
 *	i)	dbpath		database directory
 *	i)	db		GTAGS,GRTAGS,GSYMS
 *	r)			count of output lines
 */
int
search(const char *pattern, const char *root, const char *dbpath, int db)
{
	const char *p;
	int count = 0;
	FILE *op;
	GTOP *gtop;
	int flags = 0;
	STRBUF *sb = NULL;

	/*
	 * open tag file.
	 */
	gtop = gtags_open(dbpath, root, db, GTAGS_READ, 0);
	if (!(op = openfilter()))
		die("cannot open output filter.");
	/*
	 * search through tag file.
	 */
	if (nflag > 1)
		flags |= GTOP_NOSOURCE;
	if (iflag) {
		if (!isregex(pattern)) {
			sb = strbuf_open(0);
			strbuf_putc(sb, '^');
			strbuf_puts(sb, pattern);
			strbuf_putc(sb, '$');
			pattern = strbuf_value(sb);
		}
		flags |= GTOP_IGNORECASE;
	}
	if (Gflag)
		flags |= GTOP_BASICREGEX;
	for (p = gtags_first(gtop, pattern, flags); p; p = gtags_next(gtop)) {
		if (lflag) {
			const char *q;
			/* locate start point of a path */
			q = locatestring(p, "./", MATCH_FIRST);
			if (!locatestring(q, localprefix, MATCH_AT_FIRST))
				continue;
		}
		printtag(op, p);
		count++;
	}
	closefilter(op);
	if (sb)
		strbuf_close(sb);
	gtags_close(gtop);
	return count;
}
/*
 * ffformat: string copy with converting blank chars into %ff format.
 *
 *	o)	to	result
 *	i)	size	size of 'to' buffer
 *	i)	from	string
 */
void
ffformat(char *to, int size, const char *from)
{
	const char *p;
	char *e = to;

	for (p = from; *p; p++) {
		if (*p == '%' || *p == ' ' || *p == '\t') {
			if (size <= 3)
				break;
			snprintf(e, size, "%%%02x", *p);
			e += 3;
			size -= 3;
		} else {
			if (size <= 1)
				break;
			*e++ = *p;
			size--;
		}
	}
	*e = 0;
}
