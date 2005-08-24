/*
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003
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

#include <assert.h>
#include <fcntl.h>
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

#include "char.h"
#include "dbop.h"
#include "die.h"
#include "locatestring.h"
#include "strbuf.h"
#include "strlimcpy.h"
#include "test.h"

int print_statistics = 0;
/*
 * dbop_open: open db database.
 *
 *	i)	path	database name
 *	i)	mode	0: read only, 1: create, 2: modify
 *	i)	perm	file permission
 *	i)	flags
 *			DBOP_DUP: allow duplicate records.
 *			DBOP_REMOVE: remove on closed.
 *	r)		descripter for dbop_xxx()
 */
DBOP *
dbop_open(path, mode, perm, flags)
	const char *path;
	int mode;
	int perm;
	int flags;
{
	DB *db;
	int rw = 0;
	DBOP *dbop;
	BTREEINFO info;

	/*
	 * setup arguments.
	 */
	switch (mode) {
	case 0:
		rw = O_RDONLY;
		break;
	case 1:
		rw = O_RDWR|O_CREAT|O_TRUNC;
		break;
	case 2:
		rw = O_RDWR;
		break;
	default:
		assert(0);
	}
	memset(&info, 0, sizeof(info));
	if (flags & DBOP_DUP)
		info.flags |= R_DUP;
	info.psize = DBOP_PAGESIZE;
	/*
	 * accept user's request but needs 0.5MB at least.
	 */
	if (getenv("GTAGSCACHE") != NULL)
		info.cachesize = atoi(getenv("GTAGSCACHE"));
	if (info.cachesize < 500000)
		info.cachesize = 500000;

	/*
	 * if unlink do job normally, those who already open tag file can use
	 * it until closing.
	 */
	if (mode == 1 && test("f", path))
		(void)unlink(path);
	db = dbopen(path, rw, 0600, DB_BTREE, &info);
	if (!db)
		return NULL;
	if (!(dbop = (DBOP *)malloc(sizeof(DBOP))))
		die("short of memory.");
	strlimcpy(dbop->dbname, path, sizeof(dbop->dbname));
	dbop->db	= db;
	dbop->openflags	= flags;
	dbop->perm	= (mode == 1) ? perm : 0;
	dbop->lastdat	= NULL;

	return dbop;
}
/*
 * dbop_get: get data by a key.
 *
 *	i)	dbop	descripter
 *	i)	name	name
 *	r)		pointer to data
 */
const char *
dbop_get(dbop, name)
	DBOP *dbop;
	const char *name;
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;

	key.data = (char *)name;
	key.size = strlen(name)+1;

	status = (*db->get)(db, &key, &dat, 0);
	dbop->lastdat	= (char *)dat.data;
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
		die("cannot read from database.");
	case RET_SPECIAL:
		return (NULL);
	}
	return(dat.data);
}
/*
 * dbop_put: put data by a key.
 *
 *	i)	dbop	descripter
 *	i)	name	key
 *	i)	data	data
 */
void
dbop_put(dbop, name, data)
	DBOP *dbop;
	const char *name;
	const char *data;
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;
	int len;

	if (!(len = strlen(name)))
		die("primary key size == 0.");
	if (len > MAXKEYLEN)
		die("primary key too long.");
	key.data = (char *)name;
	key.size = strlen(name)+1;
	dat.data = (char *)data;
	dat.size = strlen(data)+1;

	status = (*db->put)(db, &key, &dat, 0);
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
	case RET_SPECIAL:
		die("cannot write to database.");
	}
}
/*
 * dbop_delete: delete record by path name.
 *
 *	i)	dbop	descripter
 *	i)	path	path name
 */
void
dbop_delete(dbop, path)
	DBOP *dbop;
	const char *path;
{
	DB *db = dbop->db;
	DBT key;
	int status;

	if (path) {
		key.data = (char *)path;
		key.size = strlen(path)+1;
		status = (*db->del)(db, &key, 0);
	} else
		status = (*db->del)(db, &key, R_CURSOR);
	if (status == RET_ERROR)
		die("cannot delete record.");
}
/*
 * dbop_update: update record.
 *
 *	i)	dbop	descripter
 *	i)	key	key
 *	i)	dat	data
 */
void
dbop_update(dbop, key, dat)
	DBOP *dbop;
	const char *key;
	const char *dat;
{
	dbop_put(dbop, key, dat);
}
/*
 * dbop_first: get first record. 
 * 
 *	i)	dbop	dbop descripter
 *	i)	name	key value or prefix
 *			!=NULL: indexed read by key
 *			==NULL: sequential read
 *	i)	preg	compiled regular expression if any.
 *	i)	flags	following dbop_next call take over this.
 *			DBOP_KEY	read key part
 *			DBOP_PREFIX	prefix read
 *					only valied when sequential read
 *	r)		data
 */
const char *
dbop_first(dbop, name, preg, flags)
	DBOP *dbop;
	const char *name;
	regex_t *preg;
	int	flags;
{
	DB *db = dbop->db;
	DBT key, dat;
	int status;

	dbop->preg = preg;
	if (flags & DBOP_PREFIX && !name)
		flags &= ~DBOP_PREFIX;
	if (name) {
		if (strlen(name) > MAXKEYLEN)
			die("primary key too long.");
		strlimcpy(dbop->key, name, sizeof(dbop->key));
		key.data = (char *)name;
		key.size = strlen(name);
		/*
		 * includes NULL character unless prefix read.
		 */
		if (!(flags & DBOP_PREFIX))
			key.size++;
		dbop->keylen = key.size;
		for (status = (*db->seq)(db, &key, &dat, R_CURSOR);
			status == RET_SUCCESS;
			status = (*db->seq)(db, &key, &dat, R_NEXT)) {
			if (flags & DBOP_PREFIX) {
				if (strncmp((char *)key.data, dbop->key, dbop->keylen))
					return NULL;
			} else {
				if (strcmp((char *)key.data, dbop->key))
					return NULL;
			}
			if (preg && regexec(preg, (char *)key.data, 0, 0, 0) != 0)
				continue;
			break;
		}
	} else {
		dbop->keylen = dbop->key[0] = 0;
		for (status = (*db->seq)(db, &key, &dat, R_FIRST);
			status == RET_SUCCESS;
			status = (*db->seq)(db, &key, &dat, R_NEXT)) {
			if (*((char *)key.data) == ' ')	/* meta record */
				continue;
			if (preg && regexec(preg, (char *)key.data, 0, 0, 0) != 0)
				continue;
			break;
		}
	}
	dbop->lastdat	= (char *)dat.data;
	switch (status) {
	case RET_SUCCESS:
		break;
	case RET_ERROR:
		die("dbop_first failed.");
	case RET_SPECIAL:
		return (NULL);
	}
	dbop->ioflags = flags;
	if (flags & DBOP_KEY) {
		strlimcpy(dbop->prev, (char *)key.data, sizeof(dbop->prev));
		return (char *)key.data;
	}
	return ((char *)dat.data);
}
/*
 * dbop_next: get next record. 
 * 
 *	i)	dbop	dbop descripter
 *	r)		data
 *
 * Db_next always skip meta records.
 */
const char *
dbop_next(dbop)
	DBOP *dbop;
{
	DB *db = dbop->db;
	int flags = dbop->ioflags;
	DBT key, dat;
	int status;

	while ((status = (*db->seq)(db, &key, &dat, R_NEXT)) == RET_SUCCESS) {
		assert(dat.data != NULL);
		if (flags & DBOP_KEY && *((char *)key.data) == ' ')
			continue;
		else if (*((char *)dat.data) == ' ')
			continue;
		if (flags & DBOP_KEY) {
			if (!strcmp(dbop->prev, (char *)key.data))
				continue;
			if (strlen((char *)key.data) > MAXKEYLEN)
				die("primary key too long.");
			strlimcpy(dbop->prev, (char *)key.data, sizeof(dbop->prev));
		}
		dbop->lastdat	= (char *)dat.data;
		if (flags & DBOP_PREFIX) {
			if (strncmp((char *)key.data, dbop->key, dbop->keylen))
				return NULL;
		} else if (dbop->keylen) {
			if (strcmp((char *)key.data, dbop->key))
				return NULL;
		}
		if (dbop->preg && regexec(dbop->preg, (char *)key.data, 0, 0, 0) != 0)
			continue;
		return (flags & DBOP_KEY) ? (char *)key.data : (char *)dat.data;
	}
	if (status == RET_ERROR)
		die("dbop_next failed.");
	return NULL;
}
/*
 * dbop_lastdat: get last data
 * 
 *	i)	dbop	dbop descripter
 *	r)		last data
 */
const char *
dbop_lastdat(dbop)
	DBOP *dbop;
{
	return dbop->lastdat;
}
/*
 * dbop_close: close db
 * 
 *	i)	dbop	dbop descripter
 */
void
dbop_close(dbop)
	DBOP *dbop;
{
	DB *db = dbop->db;

#ifdef USE_DB185_COMPAT
	(void)db->close(db);
#else
	(void)db->close(db, (dbop->openflags & DBOP_REMOVE) ? 1 : 0);
#endif
	if (dbop->openflags & DBOP_REMOVE)
		(void)unlink(dbop->dbname);
	else if (dbop->perm && chmod(dbop->dbname, dbop->perm) < 0)
		die("cannot change file mode.");
	(void)free(dbop);
}
