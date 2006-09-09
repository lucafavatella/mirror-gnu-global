/*
 * Copyright (c) 1996, 1997, 1998, 1999, 2000, 2006
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef _TAB_H_
#define _TAB_H_

#include <stdio.h>

void settabs(int);
void detab(FILE *, const char *);
void entab(char *);
size_t read_file_detabing(char *, size_t, FILE *, int *, int *);
void detab_replacing(FILE *op, const char *buf, const char *(*replace)(int c));


#endif /* ! _TAB_H_ */
