/*
 * Copyright (c) 1996, 1997, 1998, 1999
 *             Shigio Yamaguchi. All rights reserved.
 * Copyright (c) 1999, 2000
 *             Tama Communications Corporation. All rights reserved.
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
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "die.h"
#include "dbop.h"
#include "gtagsop.h"
#include "makepath.h"
#include "pathop.h"

static DBOP	*dbop;
static int	_nextkey;
static int	_mode;
static int	opened;
static int	created;

/*
 * pathopen: open gpath tag file
 *
 *	i)	mode	0: read only
 *			1: create
 *			2: modify
 *	r)		0: normal
 *			-1: error
 */
int
pathopen(dbpath, mode)
const char *dbpath;
int	mode;
{
	char	*p;

	assert(opened == 0);
	/*
	 * We create GPATH just first time.
	 */
	_mode = mode;
	if (mode == 1 && created)
		mode = 0;
	dbop = dbop_open(makepath(dbpath, dbname(GPATH), NULL), mode, 0644, 0);
	if (dbop == NULL)
		return -1;
	if (mode == 1)
		_nextkey = 1;
	else {
		if (!(p = dbop_get(dbop, NEXTKEY)))
			die("nextkey not found in GPATH.");
		_nextkey = atoi(p);
	}
	opened = 1;
	return 0;
}
/*
 * pathput: put path name
 *
 *	i)	path	path name
 */
void
pathput(path)
const char *path;
{
	char	buf[10];

	assert(opened == 1);
	if (_mode == 1 && created)
		return;
	if (dbop_get(dbop, path) != NULL)
		return;
#ifdef HAVE_SNPRINTF
	snprintf(buf, sizeof(buf), "%d", _nextkey++);
#else
	sprintf(buf, "%d", _nextkey++);
#endif /* HAVE_SNPRINTF */
	dbop_put(dbop, path, buf);
	dbop_put(dbop, buf, path);
}
/*
 * path2id: convert path into id
 *
 *	i)	path	path name
 *	r)		path id
 */
int
path2id(path)
const char *path;
{
	char	*id;

	assert(opened == 1);
	id = dbop_get(dbop, path);
	return (id != NULL) ? atoi(id) : 0;
}
/*
 * id2path: convert id into path
 *
 *	i)	id	path id
 *	r)		path name
 */
char	*
id2path(id)
int	id;
{
	char	key[80];
	assert(opened == 1);
#ifdef HAVE_SNPRINTF
	snprintf(key, sizeof(key), "%d", id);
#else
	sprintf(key, "%d", id);
#endif /* HAVE_SNPRINTF */
	return dbop_get(dbop, key);
}
/*
 * pathdel: delete specified path record
 *
 *	i)	path	path name
 */
void
pathdel(path)
const char *path;
{
	char	*id;

	assert(opened == 1);
	assert(_mode == 2);
	assert(path[0] == '.' && path[1] == '/');
	id = dbop_get(dbop, path);
	if (id == NULL)
		return;
	dbop_del(dbop, id);
	dbop_del(dbop, path);
}
/*
 * nextkey: return next key
 *
 *	r)		next id
 */
int
nextkey(void)
{
	assert(_mode != 1);
	return _nextkey;
}
/*
 * pathclose: close gpath tag file
 */
void
pathclose(void)
{
	char	buf[10];

	assert(opened == 1);
	opened = 0;
	if (_mode == 1 && created) {
		dbop_close(dbop);
		return;
	}
#ifdef HAVE_SNPRINTF
	snprintf(buf, sizeof(buf), "%d", _nextkey);
#else
	sprintf(buf, "%d", _nextkey);
#endif /* HAVE_SNPRINTF */
	if (_mode == 1 || _mode == 2)
		dbop_put(dbop, NEXTKEY, buf);
	dbop_close(dbop);
	if (_mode == 1)
		created = 1;
}
