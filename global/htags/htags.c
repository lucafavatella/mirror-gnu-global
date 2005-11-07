/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005
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
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/file.h>
#endif
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <time.h>

#include "getopt.h"
#include "regex.h"
#include "global.h"
#include "anchor.h"
#include "cache.h"
#include "common.h"
#include "htags.h"
#include "incop.h"
#include "path2url.h"
#include "const.h"

void src2html(const char *, const char *, int);
int makedupindex(void);
int makedefineindex(const char *, int, STRBUF *);
int makefileindex(const char *, STRBUF *);
void makeincludeindex();

#if defined(_WIN32) && !defined(__CYGWIN__)
#define mkdir(path,mode) mkdir(path)
#define link(one,two) (-1)
#endif

/*
 * Global data.
 */
int w32 = W32;				/* Windows32 environment	*/
const char *www = "http://www.gnu.org/software/global/";
int file_count = 0;
int sep = '/';
const char *save_config;
const char *save_argv;

char cwdpath[MAXPATHLEN];
char dbpath[MAXPATHLEN];
char distpath[MAXPATHLEN];
char gtagsconf[MAXPATHLEN];
char datadir[MAXPATHLEN];

char sort_path[MAXFILLEN];
char gtags_path[MAXFILLEN];
char global_path[MAXFILLEN];
const char *null_device = NULL_DEVICE;
const char *tmpdir = "/tmp";

/*
 * options
 */
int aflag;				/* --alphabet(-a) option	*/
int cflag;				/* --compact(-c) option		*/
int fflag;				/* --form(-f) option		*/
int Fflag;				/* --frame(-F) option		*/
int nflag;				/* --line-number(-n) option	*/
int gflag;				/* --gtags(-g) option		*/
int Sflag;				/* --secure-cgi(-S) option	*/
int qflag;
int vflag;				/* --verbose(-v) option		*/
int wflag;				/* --warning(-w) option		*/
int debug;				/* --debug option		*/

int show_help;				/* --help command		*/
int show_version;			/* --version command		*/
int caution;				/* --caution option		*/
int dynamic;				/* --dynamic(-D) option		*/
int symbol;				/* --symbol(-s) option		*/
int statistics;				/* --statistics option		*/

int copy_files;				/* 1: copy tag files		*/
int no_map_file;			/* 1: doesn't make map file	*/
int no_order_list;			/* 1: doesn't use order list	*/
int other_files;			/* 1: list other files		*/
int enable_grep;			/* 1: enable grep		*/
int enable_idutils;			/* 1: enable idutils		*/
int enable_xhtml;			/* 1: enable XHTML		*/

const char *action_value;
const char *id_value;
const char *cgidir;
const char *main_func = "main";
const char *cvsweb_url;
const char *cvsweb_cvsroot;
const char *gtagslabel;
const char *title;
const char *xhtml_version = "1.0";
const char *insert_header;		/* --insert-header=<file>	*/
const char *insert_footer;		/* --insert-footer=<file>	*/

/*
 * Constant values.
 */
const char *title_define_index = "DEFINITIONS";
const char *title_file_index = "FILES";
const char *title_included_from = "INCLUDED FROM";
/*
 * Function header items.
 */
const char *anchor_label[] = {
	"&lt;",
	"&gt;",
	"^",
	"v",
	"top",
	"bottom",
	"index",
	"help"
};
const char *anchor_icons[] = {
	"left",
	"right",
	"first",
	"last",
	"top",
	"bottom",
	"index",
	"help"
};
const char *anchor_comment[] = {
	"previous",
	"next",
	"first",
	"last",
	"top",
	"bottom",
	"index",
	"help"
};
const char *anchor_msg[] = {
	"Previous definition.",
	"Next definition.",
	"First definition in this file.",
	"Last definition in this file.",
	"Top of this file.",
	"Bottom of this file.",
	"Return to index page.",
	"You are seeing now."
};
const char *back_icon = "back";
const char *dir_icon  = "dir";
const char *c_icon = "c";
const char *file_icon = "text";

/*
 * Configuration parameters.
 */
int ncol = 4;				/* columns of line number	*/
int tabs = 8;				/* tab skip			*/
char stabs[8];				/* tab skip (string)		*/
int full_path = 0;			/* file index format		*/
int map_file = 1;			/* 1: create MAP file		*/
const char *icon_list = NULL;		/* use icon list		*/
const char *icon_suffix = "png";	/* icon suffix (jpg, png etc)	*/
const char *icon_spec = "border='0' align='top'";/* parameter in IMG tag*/
const char *prolog_script = NULL;	/* include script at first	*/
const char *epilog_script = NULL;	/* include script at last	*/
int show_position = 0;			/* show current position	*/
int table_list = 0;			/* tag list using table tag	*/
int colorize_warned_line = 0;		/* colorize warned line		*/
const char *script_alias = "/cgi-bin";	/* script alias of WWW server	*/
const char *gzipped_suffix = "ghtml";	/* suffix of gzipped html file	*/
const char *normal_suffix = "html";	/* suffix of normal html file	*/
const char *HTML;
const char *action = "cgi-bin/global.cgi";/* default action		*/
const char *saction;			/* safe action			*/
const char *id = NULL;			/* id (default non)		*/
int cgi = 1;				/* 1: make cgi-bin/		*/
int definition_header=NO_HEADER;	/* (NO|BEFORE|RIGHT|AFTER)_HEADER */
const char *htags_options = NULL;
const char *include_file_suffixes = "h,hxx,hpp,H,inc.php";
static const char *langmap = DEFAULTLANGMAP;

static struct option const long_options[] = {
        {"alphabet", no_argument, NULL, 'a'},
        {"compact", no_argument, NULL, 'c'},
        {"dynamic", no_argument, NULL, 'D'},
        {"dbpath", required_argument, NULL, 'd'},
        {"form", no_argument, NULL, 'f'},
        {"frame", no_argument, NULL, 'F'},
        {"gtags", no_argument, NULL, 'g'},
        {"main-func", required_argument, NULL, 'm'},
        {"line-number", no_argument, NULL, 'n'},
        {"other", no_argument, NULL, 'o'},
        {"symbol", no_argument, NULL, 's'},
        {"secure-cgi", required_argument, NULL, 'S'},
        {"title", required_argument, NULL, 't'},
        {"verbose", no_argument, NULL, 'v'},
        {"warning", no_argument, NULL, 'w'},
        {"xhtml", no_argument, NULL, 'x'},

        /* long name only */
        {"action", required_argument, NULL, 1},
        {"cvsweb", required_argument, NULL, 1},
        {"cvsweb-cvsroot", required_argument, NULL, 1},
        {"caution", no_argument, &caution, 1},
        {"debug", no_argument, &debug, 1},
        {"gtagsconf", required_argument, NULL, 1},
        {"gtagslabel", required_argument, NULL, 1},
        {"id", required_argument, NULL, 1},
        {"nocgi", no_argument, NULL, 1},
        {"no-map-file", no_argument, &no_map_file, 1},
        {"insert-header", required_argument, NULL, 1},
        {"insert-footer", required_argument, NULL, 1},
        {"statistics", no_argument, &statistics, 1},
        {"version", no_argument, &show_version, 1},
        {"help", no_argument, &show_help, 1},
        { 0 }
};

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
/*
 * Htags catch signal even if the parent ignore it.
 */
void
clean(void)
{
	unload_gpath();
	cache_close();
}
/*
 * Signal handler.
 *
 * This handler is set up in signal_setup().
 */
static void
suddenly(int signo)
{
        signo = 0;      /* to satisfy compiler */

	clean();
	exit(1);
}

/*
 * Setup signal hander.
 */
static void
signal_setup(void)
{
        signal(SIGINT, suddenly);
        signal(SIGTERM, suddenly);
#ifdef SIGHUP
        signal(SIGHUP, suddenly);
#endif
#ifdef SIGQUIT
        signal(SIGQUIT, suddenly);
#endif
}

/*
 * generate_file: generate file with replacing macro.
 *
 *	i)	dist	directory where the file should be created
 *	i)	file	file name
 */
static void
generate_file(const char *dist, const char *file)
{
	regex_t preg;
	regmatch_t pmatch[2];
	STRBUF *sb = strbuf_open(0);
	FILE *ip, *op;
	char *_;
	int i;
        struct map {
                const char *name;
                const char *value;
        } tab[] = {
		/* dynamic initialization */
                {"@page_begin@", NULL},
                {"@page_end@", NULL},
                {"@lineno_anchor@", NULL},

		/* static initialization */
                {"@body_begin@", body_begin},
                {"@body_end@", body_end},
                {"@title_begin@", title_begin},
                {"@title_end@", title_end},
                {"@error_begin@", error_begin},
                {"@error_end@", error_end},
                {"@message_begin@", message_begin},
                {"@message_end@", message_end},
                {"@verbatim_begin@", verbatim_begin},
                {"@verbatim_end@", verbatim_end},
                {"@global_path@", global_path},
                {"@gtags_path@", gtags_path},
                {"@normal_suffix@", normal_suffix},
                {"@tabs@", stabs},
                {"@hr@", hr},
                {"@br@", br},
                {"@HTML@", HTML},
                {"@action@", action},
                {"@null_device@", null_device},
                {"@globalpath@", global_path},
                {"@gtagspath@", gtags_path},
        };
	int tabsize = sizeof(tab) / sizeof(struct map);

	tab[0].value = gen_page_begin("Result", SUBDIR);
	tab[1].value = gen_page_end();
	tab[2].value = gen_name_string("L$.");
	/*
	 * construct regular expression.
	 */
	strbuf_putc(sb, '(');
	for (i = 0; i < tabsize; i++) {
		strbuf_puts(sb, tab[i].name);
		strbuf_putc(sb, '|');
	}
	strbuf_unputc(sb, '|');
	strbuf_putc(sb, ')');
	if (regcomp(&preg, strbuf_value(sb), REG_EXTENDED) != 0)
		die("cannot compile regular expression.");
	/*
	 * construct skeleton file name in the system datadir directory.
	 */
	strbuf_reset(sb);
	strbuf_sprintf(sb, "%s/gtags/%s.tmpl", datadir, file);
	ip = fopen(strbuf_value(sb), "r");
	if (!ip)
		die("skeleton file '%s' not found.", strbuf_value(sb));
	op = fopen(makepath(dist, file, NULL), "w");
	if (!op)
		die("cannot create file '%s'.", file);
	strbuf_reset(sb);
	/*
	 * Read templete file and evaluate macros.
	 */
	while ((_ = strbuf_fgets(sb, ip, STRBUF_NOCRLF)) != NULL) {
		const char *p;

		/* Pick up macro name */
		for (p = _; !regexec(&preg, p, 2, pmatch, 0); p += pmatch[0].rm_eo) {
			const char *start = p + pmatch[0].rm_so;
			int length = pmatch[0].rm_eo - pmatch[0].rm_so;

			/* print before macro */
			for (i = 0; i < pmatch[0].rm_so; i++)
				fputc(p[i], op);
			for (i = 0; i < tabsize; i++)
				if (!strncmp(start, tab[i].name, length))
					break;
			if (i >= tabsize)
				die("something wrong.");
			/* print macro value */
			if (i < tabsize) {
				const char *q;
				/*
				 * Double quote should be quoted using '\\'.
				 */
				for (q = tab[i].value; *q; q++) {
					if (*q == '"')
						fputc('\\', op);
					else if (*q == '\n')
						fputc('\\', op);
					fputc(*q, op);
				}
			}
		}
		fputs_nl(p, op);
	}
	fclose(op);
	fclose(ip);
	strbuf_close(sb);
	regfree(&preg);
	file_count++;
}

/*
 * makeprogram: make CGI program
 */
static void
makeprogram(const char *cgidir, const char *file)
{
	generate_file(cgidir, file);
}
/*
 * makebless: make bless.sh file.
 */
static void
makebless(const char *file)
{
	const char *save = action;
	action = saction;
	generate_file(distpath, file);
	action = save;
}
/*
 * makeghtml: make ghtml.cgi file.
 *
 *	i)	cgidir	directory where the file should be created
 *	i)	file	file name
 */
static void
makeghtml(const char *cgidir, const char *file)
{
	FILE *op;

	op = fopen(makepath(cgidir, file, NULL), "w");
	if (!op)
		die("cannot make unzip script.");
	fputs_nl("#!/bin/sh", op);
	fputs_nl("echo \"content-type: text/html\"", op);
	fputs_nl("echo", op);
	fprintf(op, "gzip -S %s -d -c \"$PATH_TRANSLATED\"\n", HTML);
        fclose(op);
}
/*
 * makerebuild: make rebuild script
 */
static void
makerebuild(const char *file)
{
	FILE *op;

	op = fopen(makepath(distpath, file, NULL), "w");
	if (!op)
		die("cannot make rebuild script.");
	fputs_nl("#!/bin/sh", op);
	fputs_nl("#", op);
	fputs_nl("# rebuild.sh: rebuild hypertext with the previous context.", op);
	fputs_nl("#", op);
	fputs_nl("# Usage:", op);
	fputs_nl("#\t% sh rebuild.sh", op);
	fputs_nl("#", op);
	fprintf(op, "cd %s && GTAGSCONF='%s' htags%s\n", cwdpath, save_config, save_argv);
        fclose(op);
}
/*
 * makehelp: make help file
 */
static void
makehelp(const char *file)
{
	const char **label = icon_list ? anchor_comment : anchor_label;
	const char **icons = anchor_icons;
	const char **msg   = anchor_msg;
	int n, last = 7;
	FILE *op;

	op = fopen(makepath(distpath, file, NULL), "w");
	if (!op)
		die("cannot make help file.");
	fputs_nl(gen_page_begin("HELP", TOPDIR), op);
	fputs_nl(body_begin, op);
	fputs(header_begin, op);
	fputs("Usage of Links", op);
	fputs_nl(header_end, op);
	if (!icon_list)
		fputs(verbatim_begin, op);
	fputs("/* ", op);
	for (n = 0; n <= last; n++) {
		if (icon_list) {
			fputs(gen_image(CURRENT, icons[n], label[n]), op);
			if (n < last)
				fputc(' ', op);
		} else {
			fprintf(op, "[%s]", label[n]);
		}
	}
	if (show_position)
		fprintf(op, "[+line file]");
	fputs(" */", op);
	if (!icon_list)
		fputs_nl(verbatim_end, op);
	else
		fputc('\n', op);
	fputs_nl(define_list_begin, op);
	for (n = 0; n <= last; n++) {
		fputs(define_term_begin, op);
		if (icon_list) {
			fputs(gen_image(CURRENT, icons[n], label[n]), op);
		} else {
			fprintf(op, "[%s]", label[n]);
		}
		fputs(define_term_end, op);
		fputs(define_desc_begin, op);
		fputs(msg[n], op);
		fputs_nl(define_desc_end, op);
	}
	if (show_position) {
		fputs(define_term_begin, op);
		fputs("[+line file]", op);
		fputs(define_term_end, op);
		fputs(define_desc_begin, op);
		fputs("Current position (line number and file name).", op);
		fputs_nl(define_desc_end, op);
	}
	fputs_nl(define_list_end, op);
	fputs_nl(body_end, op);
	fputs_nl(gen_page_end(), op);
	fclose(op);
	file_count++;
}
/*
 * makesearchpart: make search part
 *
 *	i)	$action	action url
 *	i)	$id	hidden variable
 *	i)	$target	target
 *	r)		html
 */
static char *
makesearchpart(const char *action, const char *id, const char *target)
{
	STATIC_STRBUF(sb);

	strbuf_clear(sb);
	strbuf_puts(sb, header_begin);
	if (Fflag)
		strbuf_puts(sb, gen_href_begin(NULL, "search", normal_suffix, NULL));
	strbuf_puts(sb, "SEARCH");
	if (Fflag)
		strbuf_puts(sb, gen_href_end());
	strbuf_puts_nl(sb, header_end);
	if (!target) {
		strbuf_puts(sb, "Please input object name and select [Search]. POSIX's regular expression is allowed.");
		strbuf_puts_nl(sb, br);
	}
	strbuf_puts_nl(sb, gen_form_begin(target));
	strbuf_puts_nl(sb, gen_input("pattern", NULL, NULL));
	if (id == NULL)
		id = "";
	strbuf_puts_nl(sb, gen_input("id", id, "hidden"));
	strbuf_puts_nl(sb, gen_input(NULL, "Search", "submit"));
	strbuf_puts(sb, gen_input(NULL, "Reset", "reset"));
	strbuf_puts_nl(sb, br);
	strbuf_puts(sb, gen_input_radio("type", "definition", 1, "Retrieve the definition place of the specified symbol."));
	strbuf_puts_nl(sb, target ? "Def" : "Definition");
	strbuf_puts(sb, gen_input_radio("type", "reference", 0, "Retrieve the reference place of the specified symbol."));
	strbuf_puts_nl(sb, target ? "Ref" : "Reference");
	if (test("f", makepath(dbpath, dbname(GSYMS), NULL))) {
		strbuf_puts(sb, gen_input_radio("type", "symbol", 0, "Retrieve the place of the specified symbol is used."));
		strbuf_puts_nl(sb, target ? "Sym" : "Other symbol");
	}
	strbuf_puts(sb, gen_input_radio("type", "path", 0, "Look for path name which matches to the specified pattern."));
	strbuf_puts(sb, target ? "Path" : "Path name");
	strbuf_puts_nl(sb, br);
	if (enable_grep) {
		strbuf_puts(sb, gen_input_radio("type", "grep", 0, "Retrieve lines which matches to the specified pattern."));
		strbuf_puts_nl(sb, target ? "Grep" : "Grep pattern");
	}
	if (enable_idutils && test("f", makepath(dbpath, "ID", NULL))) {
		strbuf_puts(sb, gen_input_radio("type", "idutils", 0, "Retrieve lines which matches to the specified pattern using idutils(1)."));
		strbuf_puts_nl(sb, target ? "Id" : "Id pattern");
	}
	strbuf_puts(sb, gen_input_checkbox("icase", "1", "Ignore case distinctions in the pattern."));
	strbuf_puts_nl(sb, target ? "Icase" : "Ignore case");
	if (other_files) {
		strbuf_puts(sb, gen_input_checkbox("other", "1", "Files other than the source code are also retrieved."));
		strbuf_puts_nl(sb, target ? "Other" : "Other files");
	}
	strbuf_puts_nl(sb, gen_form_end());
	return strbuf_value(sb);
}
/*
 * makeindex: make index file
 *
 *	i)	file	file name
 *	i)	title	title of index file
 *	i)	index	common part
 */
static void
makeindex(const char *file, const char *title, const char *index)
{
	FILE *op;

	op = fopen(makepath(distpath, file, NULL), "w");
	if (!op)
		die("cannot make file '%s'.", file);
	if (Fflag) {
		fputs_nl(gen_page_frameset_begin(title), op);
		fputs_nl(gen_frameset_begin("cols='200,*'"), op);
		if (fflag) {
			fputs_nl(gen_frameset_begin("rows='33%,33%,*'"), op);
			fputs_nl(gen_frame("search", makepath(NULL, "search", normal_suffix)), op);
		} else {
			fputs_nl(gen_frameset_begin("rows='50%,*'"), op);
		}
		/*
		 * id='xxx' for XHTML
		 * name='xxx' for HTML
		 */
		fputs_nl(gen_frame("defines", makepath(NULL, "defines", normal_suffix)), op);
		fputs_nl(gen_frame("files", makepath(NULL, "files", normal_suffix)), op);
		fputs_nl(gen_frameset_end(), op);
		fputs_nl(gen_frame("mains", makepath(NULL, "mains", normal_suffix)), op);
		fputs_nl(noframes_begin, op);
		fputs_nl(body_begin, op);
		fputs(index, op);
		fputs_nl(body_end, op);
		fputs_nl(noframes_end, op);
		fputs_nl(gen_frameset_end(), op);
		fputs_nl(gen_page_end(), op);
	} else {
		fputs_nl(gen_page_begin(title, TOPDIR), op);
		fputs_nl(body_begin, op);
		if (insert_header)
			fputs(gen_insert_header(TOPDIR), op);
		fputs(index, op);
		if (insert_footer)
			fputs(gen_insert_footer(TOPDIR), op);
		fputs_nl(body_end, op);
		fputs_nl(gen_page_end(), op);
	}
	fclose(op);
	file_count++;
}
/*
 * makemainindex: make main index
 *
 *	i)	file	file name
 *	i)	index	common part
 */
static void
makemainindex(const char *file, const char *index)
{
	FILE *op;

	op = fopen(makepath(distpath, file, NULL), "w");
	if (!op)
		die("cannot make file '%s'.", file);
	fputs_nl(gen_page_begin(title, TOPDIR), op);
	fputs_nl(body_begin, op);
	if (insert_header)
		fputs(gen_insert_header(TOPDIR), op);
	fputs(index, op);
	if (insert_footer)
		fputs(gen_insert_footer(TOPDIR), op);
	fputs_nl(body_end, op);
	fputs_nl(gen_page_end(), op);
	fclose(op);
	file_count++;
}
/*
 * makesearchindex: make search html
 *
 *	i)	file	file name
 */
static void
makesearchindex(const char *file)
{
	FILE *op;

	op = fopen(makepath(distpath, file, NULL), "w");
	if (!op)
		die("cannot create file '%s'.", file);
	fputs_nl(gen_page_begin("SEARCH", TOPDIR), op);
	fputs_nl(body_begin, op);
	fputs(makesearchpart(action, id, "mains"), op);
	fputs_nl(body_end, op);
	fputs_nl(gen_page_end(), op);
	fclose(op);
	file_count++;
}
/*
 * makehtaccess: make .htaccess skeleton file.
 */
static void
makehtaccess(const char *file)
{
	FILE *op;

	op = fopen(makepath(distpath, file, NULL), "w");
	if (!op)
		die("cannot make .htaccess skeleton file.");
	fputs_nl("#", op);
	fputs_nl("# Skeleton file for .htaccess -- This file was generated by htags(1).", op);
	fputs_nl("#", op);
	fputs_nl("# Htags have made gzipped hypertext because you specified -c option.", op);
	fputs_nl("# If your browser doesn't decompress gzipped hypertext, you will need to", op);
	fputs_nl("# setup your http server to treat this hypertext as gzipped files first.", op);
	fputs_nl("# There are many way to do it, but one of the method is to put .htaccess", op);
	fputs_nl("# file in 'HTML' directory.", op);
	fputs_nl("#", op);
	fputs_nl("# Please rewrite '/cgi-bin/ghtml.cgi' to the true value in your web site.", op);
	fputs_nl("#", op);
	fprintf(op, "AddHandler htags-gzipped-html %s\n", gzipped_suffix);
	fputs_nl("Action htags-gzipped-html /cgi-bin/ghtml.cgi", op);
	fclose(op);
}
/*
 * makehtml: make html files
 *
 *	i)	total	number of files.
 */
static void
makehtml(int total)
{
	GFIND *gp;
	FILE *source_stream, *anchor_stream;
	const char *path;
	int count = 0;
	STRBUF *sb = strbuf_open(0);

	/*
	 * Create two same streams. The two have each independent file pointer.
	 *      source_stream: for src2html()
	 *      anchor_stream: for anchor_load().
	 */
	source_stream = tmpfile();
	anchor_stream = tmpfile();
	gp = gfind_open(dbpath, NULL, other_files && !dynamic);
	while ((path = gfind_read(gp)) != NULL) {
		if (gp->type == GPATH_OTHER)
			fputc(' ', source_stream);
		fputs(path, source_stream);
		fputc('\n', source_stream);
		if (gp->type == GPATH_OTHER)
			fputc(' ', anchor_stream);
		fputs(path, anchor_stream);
		fputc('\n', anchor_stream);
	}
	gfind_close(gp);
	rewind(source_stream);
	/*
	 * Prepare anchor stream for anchor_load().
	 */
	anchor_prepare(anchor_stream);
	/*
	 * For each path in the source stream, convert the path into HTML file.
	 */
	while ((path = strbuf_fgets(sb, source_stream, STRBUF_NOCRLF)) != NULL) {
		char html[MAXPATHLEN];
		int notsource = 0;

		if (*path == ' ') {
			if (!other_files)
				continue;
			path++;
			if (test("b", path)) {
				if (wflag)
					warning("'%s' is binary file. (skipped)", path + 2);
				continue;
			}
			notsource = 1;
		} else {
			/*
			 * load tags belonging to the path.
			 * The path must be start "./".
			 */
			anchor_load(path);
		}
		count++;
		path += 2;		/* remove './' at the head */
		message(" [%d/%d] converting %s", count, total, path);
		snprintf(html, sizeof(html), "%s/%s/%s.%s", distpath, SRCS, path2fid(path), HTML);
		src2html(path, html, notsource);
	}
	strbuf_close(sb);
}
/*
 * copy file.
 */
static void
copyfile(const char *from, const char *to)
{
	int ip, op, size;
	char buf[8192];

	ip = open(from, O_RDONLY);
	if (ip < 0)
		die("cannot open input file '%s'.", from);
	op = open(to, O_WRONLY|O_CREAT|O_TRUNC, 0775);
	if (op < 0)
		die("cannot create output file '%s'.", to);
	while ((size = read(ip, buf, sizeof(buf))) != 0) {
		if (size < 0)
			die("file read error.");
		if (write(op, buf, size) != size)
			die("file write error.");
	}
	close(op);
	close(ip);
}
/*
 * duplicate file.
 * By default, htags uses link system call without making a copy.
 */
static void
duplicatefile(const char *file, const char *from, const char *to)
{
	char from_path[MAXPATHLEN];
	char to_path[MAXPATHLEN];

	snprintf(from_path, sizeof(from_path), "%s/%s", from, file);
	snprintf(to_path, sizeof(to_path), "%s/%s", to, file);
        if (copy_files) {
                copyfile(from_path, to_path);
	} else {
                if (link(from_path, to_path) < 0)
                        copyfile(from_path, to_path);
	}
}
/*
 * makecommonpart: make a common part for mains.html and index.html
 *
 *	i)	title
 *	i)	defines
 *	i)	files
 *	r)	index	common part
 */
static char *
makecommonpart(const char *title, const char *defines, const char *files)
{
	FILE *ip;
	STRBUF *sb = strbuf_open(0);
	STRBUF *ib = strbuf_open(0);
	char command[MAXFILLEN];
	const char *_;

	strbuf_puts(sb, title_begin);
	strbuf_puts(sb, title);
	strbuf_puts_nl(sb, title_end);
	strbuf_puts_nl(sb, gen_div_begin("right"));
	strbuf_sprintf(sb, "Last updated %s%s\n", now(), br);
	strbuf_sprintf(sb, "This hypertext was generated by %sGLOBAL-%s%s.%s\n",
		gen_href_begin_with_title_target(NULL, www, NULL, NULL, "Go to the GLOBAL project page.", "_top"),
		get_version(),
		gen_href_end(),
		br);
	strbuf_puts_nl(sb, gen_div_end());
	strbuf_puts_nl(sb, hr);
	if (caution) {
		strbuf_puts_nl(sb, caution_begin);
		strbuf_sprintf(sb, "<font size='+2' color='red'>CAUTION</font>%s\n", br);
		strbuf_sprintf(sb, "This hypertext consist of %d files.\n", file_count);
		strbuf_puts_nl(sb, "Please don't download whole hypertext using hypertext copy tools.");
		strbuf_puts_nl(sb, "Our network cannot afford such traffic.");
		strbuf_puts_nl(sb, "Instead, you can generate same thing in your computer using");
		strbuf_puts(sb, gen_href_begin_with_title_target(NULL, www, NULL, NULL, NULL, "_top"));
		strbuf_puts(sb, "GLOBAL source code tag system");
		strbuf_puts_nl(sb, gen_href_end());
		strbuf_puts_nl(sb, "Thank you.");
		strbuf_puts_nl(sb, caution_end);
		strbuf_sprintf(sb, "\n%s\n", hr);
	}
	if (fflag) {
		strbuf_puts(sb, makesearchpart(action, id, NULL));
		strbuf_puts_nl(sb, hr);
	}
	strbuf_sprintf(sb, "%sMAINS%s\n", header_begin, header_end);

	snprintf(command, sizeof(command), "%s -nx %s | %s -k 1,1 -k 3,3 -k 2,2n", global_path, main_func, POSIX_SORT);
        ip = popen(command, "r");
        if (!ip)
                die("cannot execute command '%s'.", command);
	strbuf_puts_nl(sb, gen_list_begin());
	while ((_ = strbuf_fgets(ib, ip, STRBUF_NOCRLF)) != NULL) {
		strbuf_puts_nl(sb, gen_list_body(SRCS, _));
	}
	strbuf_puts_nl(sb, gen_list_end());
	if (pclose(ip) != 0)
		die("cannot execute command '%s'.", command);
	strbuf_puts_nl(sb, hr);
	if (aflag && !Fflag) {
		strbuf_puts(sb, header_begin);
		strbuf_puts(sb, title_define_index);
		strbuf_puts_nl(sb, header_end);
		strbuf_puts(sb, defines);
	} else {
		strbuf_puts(sb, header_begin);
		strbuf_puts(sb, gen_href_begin(NULL, "defines", normal_suffix, NULL));
		strbuf_puts(sb, title_define_index);
		strbuf_puts(sb, gen_href_end());
		strbuf_puts_nl(sb, header_end);
	}
	strbuf_puts_nl(sb, hr);
	if (Fflag) {
		strbuf_puts(sb, header_begin);
		strbuf_puts(sb, gen_href_begin(NULL, "files", normal_suffix, NULL));
		strbuf_puts(sb, title_file_index);
		strbuf_puts(sb, gen_href_end());
		strbuf_puts_nl(sb, header_end);
	} else {
		strbuf_puts(sb, header_begin);
		strbuf_puts(sb, title_file_index);
		strbuf_puts_nl(sb, header_end);
		if (!no_order_list)
			strbuf_puts_nl(sb, list_begin);
		strbuf_puts(sb, files);
		if (!no_order_list) {
			strbuf_puts_nl(sb, list_end);
		} else {
			strbuf_puts_nl(sb, br);
		}
		strbuf_puts_nl(sb, hr);
	}
	strbuf_close(ib);

	return strbuf_value(sb);
	/* doesn't close string buffer */
}
/*
 * basic check.
 */
static void
basic_check(void)
{
	const char *p;

	/*
	 * COMMAND EXISTENCE CHECK
	 */
	if (!(p = usable(POSIX_SORT)))
		die("%s command required but not found.", POSIX_SORT);
	strlimcpy(sort_path, p, sizeof(sort_path));
	if (!(p = usable("gtags")))
		die("gtags command required but not found.");
	strlimcpy(gtags_path, p, sizeof(gtags_path));
	if (!(p = usable("global")))
		die("global command required but not found.");
	strlimcpy(global_path, p, sizeof(global_path));
	/*
	 * Temporary directory.
	 */
	if ((p = getenv("TMPDIR")) == NULL)
		p = getenv("TMP");
	if (p != NULL && test("d", p))
		tmpdir = p;
}
/*
 * load configuration variables.
 */
static void
configuration(int argc, char **argv)
{
	STRBUF *sb = strbuf_open(0);
	int i, n;
	char *p, *q;

	/*
	 * Setup the GTAGSCONF and the GTAGSLABEL environment variable
	 * according to the --gtagsconf and --gtagslabel option.
	 */
	{
		char *confpath = NULL;
		char *label = NULL;
		char *opt_gtagsconf = "--gtagsconf";
		char *opt_gtagslabel = "--gtagslabel";

		for (i = 1; i < argc; i++) {
			if ((p = locatestring(argv[i], opt_gtagsconf, MATCH_AT_FIRST))) {
				if (*p == '\0') {
					if (++i >= argc)
						die("%s needs an argument.", opt_gtagsconf);
					confpath = argv[i];
				} else {
					if (*p++ == '=' && *p)
						confpath = p;
				}
			} else if ((p = locatestring(argv[i], opt_gtagslabel, MATCH_AT_FIRST))) {
				if (*p == '\0') {
					if (++i >= argc)
						die("%s needs an argument.", opt_gtagslabel);
					label = argv[i];
				} else {
					if (*p++ == '=' && *p)
						label = p;
				}
			}
		}
		if (confpath) {
			char real[MAXPATHLEN];

			if (!test("f", confpath))
				die("%s file not found.", opt_gtagsconf);
			if (!realpath(confpath, real))
				die("cannot get absolute path of %s file.", opt_gtagsconf);
			set_env("GTAGSCONF", real);
		}
		if (label)
			set_env("GTAGSLABEL", label);
	}
	/*
	 * Config variables.
	 */
	strbuf_reset(sb);
	if (!getconfs("datadir", sb))
		die("cannot get datadir directory name.");
	strlimcpy(datadir, strbuf_value(sb), sizeof(datadir));
	if (getconfn("ncol", &n)) {
		if (n < 1 || n > 10)
			warning("parameter 'ncol' ignored becase the value (=%d) is too large or too small.", n);
		else
			ncol = n;
	}
	if (getconfn("tabs", &n)) {
		if (n < 1 || n > 32)
			warning("parameter 'tabs' ignored becase the value (=%d) is too large or too small.", n);
		else
			tabs = n;
	}
	snprintf(stabs, sizeof(stabs), "%d", tabs);
	strbuf_reset(sb);
	if (getconfs("gzipped_suffix", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		gzipped_suffix = p;
	}
	strbuf_reset(sb);
	if (getconfs("normal_suffix", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		normal_suffix = p;
	}
	strbuf_reset(sb);
	if (getconfs("definition_header", sb)) {
		p = strbuf_value(sb);
		if (!strcmp(p, "no"))
			definition_header = NO_HEADER;
		else if (!strcmp(p, "before"))
			definition_header = BEFORE_HEADER;
		else if (!strcmp(p, "right"))
			definition_header = RIGHT_HEADER;
		else if (!strcmp(p, "after"))
			definition_header = AFTER_HEADER;
	}
	if (getconfb("other_files"))
		other_files = 1;
	if (getconfb("enable_grep"))
		enable_grep = 1;
	if (getconfb("enable_idutils"))
		enable_idutils = 1;
	if (getconfb("full_path"))
		full_path = 1;
	if (getconfb("table_list"))
		table_list = 1;
	if (getconfb("no_order_list"))
		no_order_list = 1;
	if (getconfb("copy_files"))
		copy_files = 1;
	if (getconfb("no_map_file"))
		no_map_file = 1;
	strbuf_reset(sb);
	if (getconfs("icon_list", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		icon_list = p;
	}
	strbuf_reset(sb);
	if (getconfs("icon_spec", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		icon_spec = p;
	}
	strbuf_reset(sb);
	if (getconfs("icon_suffix", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		icon_suffix = p;
	}
	strbuf_reset(sb);
	if (getconfs("prolog_script", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		prolog_script = p;
	}
	strbuf_reset(sb);
	if (getconfs("epilog_script", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		epilog_script = p;
	}
	if (getconfb("show_position"))
		show_position = 1;
	if (getconfb("colorize_warned_line"))
		colorize_warned_line = 1;
	strbuf_reset(sb);
	if (getconfs("script_alias", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		/* remove the last '/' */
		q = p + strlen(p) - 1;
		if (*q == '/')
			*q = '\0';
		script_alias = p;
	}
	if (getconfb("symbols"))	/* for backward compatibility */
		symbol = 1;
	if (getconfb("symbol"))
		symbol = 1;
	if (getconfb("dynamic"))
		dynamic = 1;
	strbuf_reset(sb);
	if (getconfs("body_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("body_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			body_begin = p;
			body_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("table_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("table_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			table_begin = p;
			table_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("title_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("title_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			title_begin = p;
			title_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("comment_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("comment_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			comment_begin = p;
			comment_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("sharp_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("sharp_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			sharp_begin = p;
			sharp_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("brace_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("brace_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			brace_begin = p;
			brace_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("reserved_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("reserved_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			reserved_begin = p;
			reserved_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("position_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("position_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			position_begin = p;
			position_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("warned_line_begin", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		strbuf_reset(sb);
		if (getconfs("warned_line_end", sb)) {
			q = strdup(strbuf_value(sb));
			if (q == NULL)
				die("short of memory.");
			warned_line_begin = p;
			warned_line_end = q;
		} else {
			free(p);
		}
	}
	strbuf_reset(sb);
	if (getconfs("include_file_suffixes", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		include_file_suffixes = p;
	}
	strbuf_reset(sb);
	if (getconfs("langmap", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		langmap = p;
	}
	strbuf_reset(sb);
	if (getconfs("xhtml_version", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		xhtml_version = p;
	}
	/* insert htags_options into the head of ARGSV array. */
	strbuf_reset(sb);
	if (getconfs("htags_options", sb)) {
		p = strdup(strbuf_value(sb));
		if (p == NULL)
			die("short of memory.");
		htags_options = p;
	}
	strbuf_close(sb);
}
/*
 * save_environment: save configuration data and arguments.
 */
static void
save_environment(int argc, char **argv)
{
	char command[MAXFILLEN];
	STRBUF *sb = strbuf_open(0);
	STRBUF *save_c = strbuf_open(0);
	STRBUF *save_a = strbuf_open(0);
	int i;
	const char *p;
	FILE *ip;

	/*
	 * save config values.
	 */
	snprintf(command, sizeof(command), "%s --config", gtags_path);
	if ((ip = popen(command, "r")) == NULL)
		die("cannot execute '%s'.", command);
	while (strbuf_fgets(sb, ip, STRBUF_NOCRLF) != NULL) {
		for (p = strbuf_value(sb); *p; p++) {
			if (*p == '\'') {
				strbuf_putc(save_c, '\'');
				strbuf_putc(save_c, '"');
				strbuf_putc(save_c, '\'');
				strbuf_putc(save_c, '"');
				strbuf_putc(save_c, '\'');
			} else
				strbuf_putc(save_c, *p);
		}
	}
	if (pclose(ip) != 0)
		die("cannot execute '%s'.", command);
	strbuf_close(sb);
	save_config = strbuf_value(save_c);
	/* doesn't close string buffer for save config. */
	/* strbuf_close(save_c); */

	/*
	 * save arguments.
	 */
	{
		char *opt_gtagsconf = "--gtagsconf";

		for (i = 1; i < argc; i++) {
			char *blank;

			/*
			 * skip --gtagsconf because it is already read
			 * as config value.
			 */
			if ((p = locatestring(argv[i], opt_gtagsconf, MATCH_AT_FIRST))) {
				if (*p == '\0')
					i++;
				continue;
			}
			blank = locatestring(argv[i], " ", MATCH_FIRST);
			strbuf_putc(save_a, ' ');
			if (blank)
				strbuf_putc(save_a, '\'');
			strbuf_puts(save_a, argv[i]);
			if (blank)
				strbuf_putc(save_a, '\'');
		}
	}
	save_argv = strbuf_value(save_a);
	/* doesn't close string buffer for save arguments. */
	/* strbuf_close(save_a); */
}

char **
append_options(int *argc, char **argv)
{

	STRBUF *sb = strbuf_open(0);
	const char *p, *opt = strdup(htags_options);
	int count = 1;
	int quote = 0;
	const char **newargv;
	int i = 0, j = 1;

	if (!opt)
		die("Short of memory.");
	for (p = opt; *p && isspace(*p); p++)
		;
	for (; *p; p++) {
		int c = *p;

		if (quote) {
			if (quote == c)
				quote = 0;
			else
				strbuf_putc(sb, c);
		} else if (c == '\\') {
			strbuf_putc(sb, c);
		} else if (c == '\'' || c == '"') {
			quote = c;
		} else if (isspace(c)) {
			strbuf_putc(sb, '\0');
			count++;
			while (*p && isspace(*p))
				p++;
			p--;
		} else {
			strbuf_putc(sb, *p);
		}
	}
	newargv = (const char **)malloc(sizeof(char *) * (*argc + count + 1));
	if (!newargv)
		die("Short of memory.");
	newargv[i++] = argv[0];
	p = strbuf_value(sb);
	while (count--) {
		newargv[i++] = p;
		p += strlen(p) + 1;
	}
	while (j < *argc)
		newargv[i++] = argv[j++];
	newargv[i] = NULL;
	argv = (char **)newargv;
	*argc = i;
#ifdef DEBUG
	for (i = 0; i < *argc; i++)
		fprintf(stderr, "argv[%d] = '%s'\n", i, argv[i]);
#endif
	/* doesn't close string buffer. */

	return argv;
}
int
main(int argc, char **argv)
{
	const char *path, *av = NULL;
	int func_total, file_total;
        char arg_dbpath[MAXPATHLEN];
	const char *index = NULL;
	int optchar;
        int option_index = 0;
	time_t start_time, end_time, start_all_time, end_all_time,
		T_makedupindex, T_makedefineindex, T_makefileindex,
		T_makeincludeindex, T_makehtml, T_all;

	arg_dbpath[0] = 0;
	basic_check();
	configuration(argc, argv);
	setup_langmap(langmap);
	save_environment(argc, argv);

	/*
 	 * insert htags_options at the head of argv.
	 */
	if (htags_options)
		argv = append_options(&argc, argv);

	while ((optchar = getopt_long(argc, argv, "acd:DfFgm:noqsS:t:vwx", long_options, &option_index)) != EOF) {
		switch (optchar) {
                case 0:
                case 1:
			if (!strcmp("action", long_options[option_index].name))
                                action_value = optarg;
			else if (!strcmp("nocgi", long_options[option_index].name))
                                cgi = 0;
			else if (!strcmp("id", long_options[option_index].name))
                                id_value = optarg;
			else if (!strcmp("cvsweb", long_options[option_index].name))
                                cvsweb_url = optarg;
			else if (!strcmp("cvsweb-cvsroot", long_options[option_index].name))
                                cvsweb_cvsroot = optarg;
			else if (!strcmp("gtagsconf", long_options[option_index].name))
				;	/*  --gtagsconf is estimated only once. */
			else if (!strcmp("gtagslabel", long_options[option_index].name))
				;	/* --gtagslabel is estimated only once. */
			else if (!strcmp("insert-header", long_options[option_index].name))
				insert_header = optarg;
			else if (!strcmp("insert-footer", long_options[option_index].name))
				insert_footer = optarg;
                        break;
                case 'a':
                        aflag++;
                        break;
                case 'c':
                        cflag++;
                        break;
                case 'd':
			strlimcpy(arg_dbpath, optarg, sizeof(arg_dbpath));
                        break;
                case 'D':
			dynamic = 1;
                        break;
                case 'f':
                        fflag++;
                        break;
                case 'F':
                        Fflag++;
                        break;
                case 'm':
			main_func = optarg;
                        break;
                case 'n':
                        nflag++;
                        break;
                case 'g':
                        gflag++;
                        break;
                case 'o':
			other_files = 1;
                        break;
                case 's':
			symbol = 1;
                        break;
                case 'S':
			Sflag++;
			cgidir = optarg;
                        break;
                case 't':
			title = optarg;
                        break;
                case 'q':
                        qflag++;
                        break;
                case 'v':
                        vflag++;
			setverbose();
                        break;
                case 'w':
                        wflag++;
                        break;
		case 'x':
			enable_xhtml = 1;
                        break;
                default:
                        usage();
                        break;
		}
	}
	if (insert_header && !test("fr", insert_header))
		die("page header file '%s' not found.", insert_header);
	if (insert_footer && !test("fr", insert_footer))
		die("page footer file '%s' not found.", insert_footer);
	if (no_map_file)
		map_file = 0;
        argc -= optind;
        argv += optind;
        if (!av)
                av = (argc > 0) ? *argv : NULL;

	if (debug)
		setdebug();
        if (qflag) {
                setquiet();
		vflag = 0;
	}
	/*
	 * If the --xhtml option is specified then all HTML tags which
	 * are defined in configuration file are ignored. Instead, you can
	 * customize XHTML tag using style sheet (See 'style.css').
	 */
	if (enable_xhtml)
		setup_xhtml();
	/*
	 * If copy_files is true then htags copy tag files instead of linking.
	 * Since Windows 32 environment doesn't have link system call
	 * we set copy_files true.
	 */
	if (w32)
		copy_files = 1;
        if (show_version)
                version(av, vflag);
        if (show_help)
                help();

	if (gflag) {
		STRBUF *sb = strbuf_open(0);

		strbuf_puts(sb, gtags_path);
		if (vflag)
			strbuf_puts(sb, " -v");
		if (wflag)
			strbuf_puts(sb, " -w");
		if (enable_idutils)
			strbuf_puts(sb, " -I");
		if (arg_dbpath[0]) {
			strbuf_putc(sb, ' ');
			strbuf_puts(sb, arg_dbpath);
		}
		if (system(strbuf_value(sb)))
			die("cannot execute gtags(1) command.");
		strbuf_close(sb);
	}
	/*
	 * get dbpath.
	 */
	if (!getcwd(cwdpath, sizeof(cwdpath)))
		die("cannot get current directory.");
	if (arg_dbpath[0])
		strlimcpy(dbpath, arg_dbpath, sizeof(dbpath));
	else
		strlimcpy(dbpath, cwdpath, sizeof(dbpath));

	if (cflag && !usable("gzip")) {
		warning("'gzip' command not found. -c option ignored.");
		cflag = 0;
	}
	if (!title) {
		char *p = strrchr(cwdpath, sep);
		title = p ? p + 1 : cwdpath;
	}
	if (dynamic && Sflag)
		die("Current implementation doesn't allow both -D(--dynamic) and the -S(--secure-cgi).");
	if (icon_list && !test("f", icon_list))
		die("icon_list '%s' not found.", icon_list);
	/*
	 * decide directory in which we make hypertext.
	 */
	if (av) {
		char realpath[MAXPATHLEN];

		if (!test("dw", av))
			die("'%s' is not writable directory.", av);
		if (chdir(av) < 0)
			die("directory '%s' not found.", av);
		if (!getcwd(realpath, sizeof(realpath)))
			die("cannot get current directory");
		if (chdir(cwdpath) < 0)
			die("cannot return to original directory.");
		snprintf(distpath, sizeof(distpath), "%s/HTML", realpath);
	} else {
		snprintf(distpath, sizeof(distpath), "%s/HTML", cwdpath);
	}
	{
		static char buf[MAXBUFLEN];
		snprintf(buf, sizeof(buf), "%s/global.cgi", script_alias);
		saction = buf;
	}
	if (Sflag) {
		action = saction;
		id = distpath;
	}
	/* --action, --id overwrite Sflag's value. */
	if (action_value) {
		action = action_value;
	}
	if (id_value) {
		id = id_value;
	}
	if (!test("r", makepath(dbpath, dbname(GTAGS), NULL)) || !test("r", makepath(dbpath, dbname(GRTAGS), NULL)))
		die("GTAGS and/or GRTAGS not found. Htags needs both of them.");
	if (symbol && !test("r", makepath(dbpath, dbname(GSYMS), NULL)))
		die("-s(--symbol) option needs GSYMS tag file.");
	/*
	 * make dbpath absolute.
	 */
	{
		char buf[MAXPATHLEN];
		if (realpath(dbpath, buf) == NULL)
			die("cannot get realpath of dbpath.");
		strlimcpy(dbpath, buf, sizeof(dbpath));
	}
	/*
	 * The older version (4.8.7 or former) of GPATH doesn't have files
         * other than source file. The oflag requires new version of GPATH.
	 */
	if (other_files) {
		GFIND *gp = gfind_open(dbpath, NULL, 0);
		if (gp->version < 2)
			die("GPATH is old format. Please remake it by invoking gtags(1).");
		gfind_close(gp);
	}
	/*
	 * for global(1) and gtags(1).
	 */
	set_env("GTAGSROOT", cwdpath);
	set_env("GTAGSDBPATH", dbpath);
	set_env("GTAGSLIBPATH", "");
	/*
	 * Use C locale in order to avoid the degradation of performance
	 * by internationalized sort command.
	 */
	set_env("LC_ALL", "C");
	/*
	 * check directories
	 */
	if (fflag || cflag || dynamic) {
		if (cgidir && !test("d", cgidir))
			die("'%s' not found.", cgidir);
		if (!Sflag) {
			static char buf[MAXPATHLEN];
			snprintf(buf, sizeof(buf), "%s/cgi-bin", distpath);
			cgidir = buf;
		}
	} else {
		Sflag = 0;
		cgidir = 0;
	}
	/*------------------------------------------------------------------
	 * MAKE FILES
	 *------------------------------------------------------------------
	 *       HTML/cgi-bin/global.cgi ... CGI program (1)
	 *       HTML/cgi-bin/ghtml.cgi  ... unzip script (1)
	 *       HTML/.htaccess          ... skeleton of .htaccess (1)
	 *       HTML/help.html          ... help file (2)
	 *       HTML/R/                 ... references (3)
	 *       HTML/D/                 ... definitions (3)
	 *       HTML/search.html        ... search index (4)
	 *       HTML/defines.html       ... definitions index (5)
	 *       HTML/defines/           ... definitions index (5)
	 *       HTML/files/             ... file index (6)
	 *       HTML/index.html         ... index file (7)
	 *       HTML/mains.html         ... main index (8)
	 *       HTML/null.html          ... main null html (8)
	 *       HTML/S/                 ... source files (9)
	 *       HTML/I/                 ... include file index (9)
	 *       HTML/rebuild.sh         ... rebuild script (10)
	 *       HTML/style.css          ... style sheet (11)
	 *------------------------------------------------------------------
	 */
	/* for clean up */
	signal_setup();
	sethandler(clean);

        HTML = (cflag) ? gzipped_suffix : normal_suffix;

	message("[%s] Htags started", now());
	start_all_time = time(NULL);
	/*
	 * (#) check if GTAGS, GRTAGS is the latest.
	 */
	if (!w32) {
		/* UNDER CONSTRUCTION */
	}
	/*
	 * (0) make directories
	 */
	message("[%s] (0) making directories ...", now());
	{
		int mode = 0775;
		char *dirs[] = {"files", "defines", SRCS, DEFS, REFS, INCS, INCREFS, SYMS};

		int i, lim = sizeof(dirs) / sizeof(char *);

		if (!test("d", distpath)) {
			if (mkdir(distpath, 0777) < 0)
				die("cannot make directory '%s'.", distpath);
		}
		for (i = 0; i < lim; i++) {
			char *p = dirs[i];
			/*
			 * dynamic mode doesn't require these directories.
			 */
			if (dynamic)
				if (!strcmp(p, DEFS) || !strcmp(p, REFS) || !strcmp(p, SYMS))
					continue;
			if (!symbol)
				if (!strcmp(p, SYMS))
					continue;
			path = makepath(distpath, p, NULL);
			if (!test("d", path))
				if (mkdir(path, mode))
					die("cannot make '%s' directory.", p);
		}
		if (cgi && (fflag || cflag || dynamic)) {
			path = makepath(distpath, "cgi-bin", NULL);
			if (!test("d", path))
				if (mkdir(path, mode) < 0)
					die("cannot make cgi-bin directory.");
		}
	}
	/*
	 * (1) make CGI program
	 */
	if (cgi && (fflag || dynamic)) {
		int i;

		if (cgidir) {
			message("[%s] (1) making CGI program ...", now());
			makeprogram(cgidir, "global.cgi");
			if (chmod(makepath(cgidir, "global.cgi", NULL), 0755) < 0)
				die("cannot chmod CGI program.");
		}
		/*
		 * Always make bless.sh.
		 * Don't grant execute permission to bless script.
		 */
		makebless("bless.sh");
		if (chmod(makepath(distpath, "bless.sh", NULL), 0640) < 0)
			die("cannot chmod bless script.");
		/*
		 * replace G*** tag files.
		 */
		for (i = GPATH; i < GTAGLIM; i++) {
			const char *name = dbname(i);

			if (test("f", makepath(dbpath, name, NULL))) {
				char path[MAXPATHLEN];

				snprintf(path, sizeof(path), "%s/cgi-bin/%s", distpath, name);
				unlink(path);
				snprintf(path, sizeof(path), "%s/cgi-bin", distpath);
				duplicatefile(name, dbpath, path);
			}
		}
	} else {
		message("[%s] (1) making CGI program ...(skipped)", now());
	}
	if (cgi && cflag) {
		makehtaccess(".htaccess");
		if (chmod(makepath(distpath, ".htaccess", NULL), 0644) < 0)
			die("cannot chmod .htaccess skeleton.");
		if (cgidir) {
			makeghtml(cgidir, "ghtml.cgi");
			if (chmod(makepath(cgidir, "ghtml.cgi", NULL), 0644) < 0)
				die("cannot chmod unzip script.");
		}
	}
	/*
	 * (2) make help file
	 */
	message("[%s] (2) making help.html ...", now());
	makehelp("help.html");
	/*
	 * (#) load GPATH
	 */
	load_gpath(dbpath);

	/*
	 * (3) make function entries (D/ and R/)
	 *     MAKING TAG CACHE
	 */
	message("[%s] (3) making duplicate entries ...", now());
	cache_open();
	start_time = time(NULL);
	func_total = makedupindex();
	end_time = time(NULL);
	message("Total %d functions.", func_total);
	T_makedupindex = end_time - start_time;
	/*
	 * (4) search index. (search.html)
	 */
	if (Fflag && fflag) {
		message("[%s] (4) making search index ...", now());
		makesearchindex("search.html");
	}
	{
		STRBUF *defines = strbuf_open(0);
		STRBUF *files = strbuf_open(0);

		/*
		 * (5) make function index (defines.html and defines/)
		 *     PRODUCE @defines
		 */
		message("[%s] (5) making function index ...", now());
		start_time = time(NULL);
		func_total = makedefineindex("defines.html", func_total, defines);
		end_time = time(NULL);
		message("Total %d functions.", func_total);
		T_makedefineindex = end_time - start_time;
		/*
		 * (6) make file index (files.html and files/)
		 *     PRODUCE @files, %includes
		 */
		message("[%s] (6) making file index ...", now());
		init_inc();
		start_time = time(NULL);
		file_total = makefileindex("files.html", files);
		end_time = time(NULL);
		message("Total %d files.", file_total);
		T_makefileindex = end_time - start_time;
		file_count += file_total;
		/*
		 * [#] make include file index.
		 */
		message("[%s] (#) making include file index ...", now());
		start_time = time(NULL);
		makeincludeindex();
		end_time = time(NULL);
		T_makeincludeindex = end_time - start_time;
		/*
		 * [#] make a common part for mains.html and index.html
		 *     USING @defines @files
		 */
		message("[%s] (#) making a common part ...", now());
		index = makecommonpart(title, strbuf_value(defines), strbuf_value(files));

		strbuf_close(defines);
		strbuf_close(files);
	}
	/*
	 * (7)make index file (index.html)
	 */
	message("[%s] (7) making index file ...", now());
	makeindex("index.html", title, index);
	/*
	 * (8) make main index (mains.html)
	 */
	message("[%s] (8) making main index ...", now());
	makemainindex("mains.html", index);
	/*
	 * (9) make HTML files (SRCS/)
	 *     USING TAG CACHE, %includes and anchor database.
	 */
	message("[%s] (9) making hypertext from source code ...", now());
	start_time = time(NULL);
	makehtml(file_total);
	end_time = time(NULL);
	T_makehtml = end_time - start_time;
	/*
	 * (10) rebuild script. (rebuild.sh)
	 *
	 * Don't grant execute permission to rebuild script.
	 */
	makerebuild("rebuild.sh");
	if (chmod(makepath(distpath, "rebuild.sh", NULL), 0640) < 0)
		die("cannot chmod rebuild script.");
	/*
	 * (11) style sheet file (style.css)
	 */
	if (enable_xhtml) {
		char src[MAXPATHLEN], dst[MAXPATHLEN];
		snprintf(src, sizeof(src), "%s/gtags/style.css", datadir);
		snprintf(dst, sizeof(dst), "%s/style.css", distpath);
		copyfile(src, dst);
	}
	end_all_time = time(NULL);
	message("[%s] Done.", now());
	T_all = end_all_time - start_all_time;
	if (vflag && cgi && (cflag || fflag)) {
		message("\n[Information]\n");
		if (cflag) {
			message(" Your system may need to be setup to decompress *.%s files.", gzipped_suffix);
			message(" This can be done by having your browser compiled with the relevant");
			message(" options, or by configuring your http server to treat these as");
			message(" gzipped files. (Please see 'HTML/.htaccess')\n");
		}
		if (fflag || dynamic) {
			const char *path = (*action == '/') ? makepath("DOCUMENT_ROOT", action, NULL) : makepath("HTML", action, NULL);

			message(" You need to setup http server so that %s", path);
			message(" is executed as a CGI script. (DOCUMENT_ROOT means WWW server's data root.)\n");
		}
		message(" Good luck!\n");
	}
	/* This is not supported. */
	if (icon_list && test("f", icon_list)) {
		char command[MAXFILLEN];
		snprintf(command, sizeof(command), "tar xzf %s -C %s", icon_list, distpath);
		system(command);
	}
	/*
	 * Print statistics information.
	 */
	if (statistics) {
		setverbose();
		message("- Elapsed time of making duplicate entries ............ %10ld seconds.", T_makedupindex);
		message("- Elapsed time of making function index ............... %10ld seconds.", T_makedefineindex);
		message("- Elapsed time of making file index ................... %10ld seconds.", T_makefileindex);
		message("- Elapsed time of making include file index ........... %10ld seconds.", T_makeincludeindex);
		message("- Elapsed time of making hypertext .................... %10ld seconds.", T_makehtml);
		message("- The entire elapsed time ............................. %10ld seconds.", T_all);
	}
	clean();
	return 0;
}
