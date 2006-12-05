/*
 * Copyright (c) 2006 Tama Communications Corporation
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
#ifndef _POOL_H
#define _POOL_H

#include "obstack.h"

typedef struct {
	struct obstack obstack;		/* memory pool */
	char *first_object;		/* first object	(for reset) */
} POOL;

POOL *pool_open(void);
void *pool_malloc(POOL *, int);
char *pool_strdup(POOL *, const char *, int);
char *pool_strdup_withterm(POOL *, const char *, int);
void pool_reset(POOL *);
void pool_close(POOL *);

#endif /* ! _POOL_H */
