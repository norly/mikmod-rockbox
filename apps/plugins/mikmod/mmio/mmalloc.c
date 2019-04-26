/*	MikMod sound library
	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: mmalloc.c,v 1.1.1.1 2004/01/21 01:36:35 raph Exp $

  Dynamic memory routines

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"


size_t mem_offset = 0;
char *mallocbuf = NULL;
size_t mallocbuflen = 0;


size_t mikmod_get_mbuffsize(size_t *buff)
{
	return buff[-1];
}


void mikmod_prepare_malloc(char *buff, int bufsize)
{
	mallocbuf = buff;
	mallocbuflen = bufsize;
	mem_offset = 0;
}

long mikmod_get_malloc_usage(void)
{
	return mem_offset;
}

int mikmod_abs(int num)
{
	if (num < 0)
		return (num * -1);
	else
		return (num);
}

void* mikmod_memset(char *buf, int val, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++)
	{
		buf[i] = val;
	}
	return buf;
}

/* 'Poor man's malloc' taken from rockbox codeclib.c */
void* mikmod_malloc(size_t size)
{
    void* x;

	if (!mallocbuf)
		return NULL;
    if (mem_offset + (long)size + 4 > mallocbuflen)
        return NULL;

	mem_offset += 4;

    x = &mallocbuf[mem_offset];
	((size_t*)x)[-1] = size;

	mem_offset += (size + 3) & ~3; /* Keep memory 32-bit aligned */


	memset(x, 0, size);

    return(x);
}

void* mikmod_calloc(size_t nmemb, size_t size)
{
    void *x;
    x = mikmod_malloc(nmemb * size);
    if (x == NULL)
        return NULL;
    //memset(x, 0, nmemb*size); /* already done in mikmod_malloc() */
    return(x);
}

void mikmod_free(void* ptr) {
    (void)ptr;
}

void* mikmod_realloc(void* ptr, size_t size)
{
    void* x;
    //(void)ptr;
    x = mikmod_malloc(size);
	if (ptr != NULL)
	{
		if (mikmod_get_mbuffsize(ptr) < size)
		{
			memcpy(x, ptr, mikmod_get_mbuffsize(ptr));
		}
		else
		{
			memcpy(x, ptr, size);
		}
	}
    return(x);
}

unsigned int mikmod_strlen(const char *string)
{
	unsigned int i;
	for (i = 0; string[i] != 0; i++);
	return i;
}

char* mikmod_strdup(const char *srcbuf)
{
	char *newbuf;
	unsigned int i, len;

	len = mikmod_strlen(srcbuf);
	newbuf = mikmod_malloc(len + 1);
	
	if (newbuf)
	{
		for (i = 0; i <= len; i++)
			newbuf[i] = srcbuf[i];
	}
	return newbuf;
}

char* mikmod_strncat(char *dest, const char *src, size_t count)
{
	size_t i, j;
	j = mikmod_strlen(dest);

	for(i = 0; i < count; i++)
	{
		if (src[i] == 0)
			break;
		dest[i + j] = src[i];
	}
	return dest;
}

int mikmod_memcmp(const char *buf1, const char *buf2, size_t count)
{
	size_t i;

	for(i = 0; i < count; i++)
	{
		if (buf1[i] > buf2[i])
			return 1;
		if (buf1[i] < buf2[i])
			return -1;
	}
	return 0;
}

char* mikmod_strstr(char *str, char *search)
{
	size_t i, j, k;

	if (!mikmod_strlen(search))
		return str;

	j = mikmod_strlen(str);
	k = mikmod_strlen(search);

	for (i = 0; i < (j - k); i++)
	{
		if (!mikmod_memcmp(str, search, k))
			return &str[i];
	}
	return NULL;
}

int mikmod_toupper(int character)
{
	if ((character > 96) && (character < 123))
		return (character - 32);
	else
		return character;
}

int mikmod_isalnum(int character)
{
	if ((character > 96) && (character < 123))
		return character;
	if ((character > 64) && (character < 91))
		return character;
	if ((character > 47) && (character < 58))
		return character;
	return 0;
}

int mikmod_isdigit(char c)
{
    return (c >= '0') && (c <= '9');
}


/****************************************************
 * 'Original' MikMod code goes here
 *
 ****************************************************/

/* Same as malloc, but sets error variable _mm_error when fails */
void* _mm_malloc(size_t size)
{
	void *d;

	//if(!(d=mikmod_malloc(size))) {
	if(!(d=mikmod_malloc(size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	memset(d, 0, size);
	return d;
}

/* Same as calloc, but sets error variable _mm_error when fails */
void* _mm_calloc(size_t nitems,size_t size)
{
	void *d;
   
	if(!(d=mikmod_calloc(nitems,size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return d;
}

/* ex:set ts=4: */
