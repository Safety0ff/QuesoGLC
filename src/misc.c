/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$ */

/* This file defines miscellaneous functions that are used throughout the lib */

#include <stdlib.h>
#include "ocontext.h"
#include "internal.h"
#include FT_LIST_H

/* QuesoGLC own allocation and memory management routines */

void* __glcMalloc(size_t size)
{
  return malloc(size);
}

void __glcFree(void *ptr)
{
  free(ptr);
}

void* __glcRealloc(void *ptr, size_t size)
{
  return realloc(ptr, size);
}

/* FT lists destructor */
void __glcListDestructor(FT_Memory inMemory, void *inData, void *inUser)
{
  __glcFree(inData);
}

/* Find a token in a list of tokens separated by 'separator' */
GLCchar* __glcFindIndexList(const GLCchar* inString, GLuint inIndex,
			    const GLCchar* inSeparator)
{
    GLuint i = 0;
    GLuint occurence = 0;
    char *s = (char *)inString;
    char *sep = (char *)inSeparator;
    
    if (!inIndex)
	return (GLCchar *)inString;
    
    /* TODO use Unicode instead of ASCII */
    for (i=0; i<=strlen(s); i++) {
	if (s[i] == *sep)
	    occurence++;
	if (occurence == inIndex) {
	    i++;
	    break;
	}
    }
    
    
    return (GLCchar *)&s[i];
}

/* Fetch a key in the DB :
 * Since NDBM is not thread-safe, we need to make sure that if several threads
 * try to access the DB simultaneously, there won't be any issue. In order to
 * fix this issue, a mutex is used although this is not very elegant. However,
 * the occurence of many simultaneous accesses from different threads to the
 * DB is very unlikely to happen.
 * Pros :
 * - Makes the library simpler since only one file is open at a given time.
 * - The cache is common to all threads which can be improve its efficiency
 *   if the different threads make access to the same location in the file.
 * Cons :
 * - On most modern OSes, the FS allows simultaneous reads on a single file
 *   and this workaround does not take advantage of this feature.
 * - Numerous cache misses can occur if 2 threads make access to different
 *   locations of the DB.
 */
datum __glcDBFetch(DBM *dbf, datum key)
{
  datum content;

  pthread_mutex_lock(&__glcCommonArea->dbMutex);
  content = dbm_fetch(dbf, key);
  pthread_mutex_unlock(&__glcCommonArea->dbMutex);
  return content;
}
