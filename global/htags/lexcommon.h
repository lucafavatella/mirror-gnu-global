/*
 * Copyright (c) 2004 Tama Communications Corporation
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
#ifndef _LEXCOMMON_H
#define _LEXCOMMON_H

#include "incop.h"
/*
 * The default action for line control.
 * These can be applicable to most languages.
 * You must define C_COMMENT, CPP_COMMENT SHELL_COMMENT, LITERAL, STRING
 * and PREPROCESSOR_LINE as %start values, even if they are not used.
 * It assumed that CPP_COMMENT and SHELL_COMMENT is one line comment.
 */
static int lineno;
static int begin_line;
/*
 * If you want newline to terminate string, set this variable to 1.
 */
static int newline_terminate_string = 0;

#define LINENO	lineno

#define DEFAULT_BEGIN_OF_FILE_ACTION {					\
        yyin = ip;							\
        yyrestart(yyin);						\
        lineno = 1;							\
        begin_line = 1;							\
}

#define DEFAULT_YY_USER_ACTION {					\
	if (begin_line) {						\
		put_begin_of_line(lineno);				\
		switch (YY_START) {					\
		case C_COMMENT:						\
		case CPP_COMMENT:					\
		case SHELL_COMMENT:					\
			echos(comment_begin);				\
			break;						\
		}							\
		begin_line = 0;						\
	}								\
}

#define DEFAULT_END_OF_LINE_ACTION {					\
	switch (YY_START) {						\
	case CPP_COMMENT:						\
	case SHELL_COMMENT:						\
		yy_pop_state();						\
		/* FALLTHROUGH */					\
	case C_COMMENT:							\
		echos(comment_end);					\
		break;							\
	case STRING:							\
	case LITERAL:							\
		if (newline_terminate_string)				\
			yy_pop_state();					\
		break;							\
	}								\
	if (YY_START == PREPROCESSOR_LINE)				\
		yy_pop_state();						\
	put_end_of_line(lineno);					\
	/* for the next line */						\
	lineno++;							\
	begin_line = 1;							\
}

#define DEFAULT_BACKSLASH_NEWLINE_ACTION {				\
	echoc('\\');							\
	switch (YY_START) {						\
	case CPP_COMMENT:						\
	case C_COMMENT:							\
	case SHELL_COMMENT:						\
		echos(comment_end);					\
		break;							\
	}								\
	put_end_of_line(lineno);					\
	/* for the next line */						\
	lineno++;							\
	begin_line = 1;							\
}

/*
 * Input.
 */
extern FILE *yyin;

/*
 * Output routine.
 */
extern void echoc(int);
extern void echos(const char *s);
extern char *generate_guide(int);
extern void put_anchor(char *, int, int);
extern void put_include_anchor(struct data *, char *);
extern void put_reserved_word(char *);
extern void put_macro(char *);
extern void unknown_preprocessing_directive(char *, int);
extern void unexpected_eof(int);
extern void unknown_yacc_directive(char *, int);
extern void missing_left(char *, int);
extern void put_char(int);
extern void put_string(char *);
extern void put_brace(char *);
extern void put_begin_of_line(int);
extern void put_end_of_line(int);

#endif /* ! _LEXCOMMON_H */
