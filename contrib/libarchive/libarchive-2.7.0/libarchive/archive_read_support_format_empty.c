/*-
 * Copyright (c) 2003-2007 Tim Kientzle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "archive_platform.h"
__FBSDID("$FreeBSD: src/lib/libarchive/archive_read_support_format_empty.c,v 1.4 2008/12/06 06:45:15 kientzle Exp $");

#include "archive.h"
#include "archive_entry.h"
#include "archive_private.h"
#include "archive_read_private.h"

static int	archive_read_format_empty_bid(struct archive_read *);
static int	archive_read_format_empty_read_data(struct archive_read *,
		    const void **, size_t *, off_t *);
static int	archive_read_format_empty_read_header(struct archive_read *,
		    struct archive_entry *);
int
archive_read_support_format_empty(struct archive *_a)
{
	struct archive_read *a = (struct archive_read *)_a;
	int r;

	r = __archive_read_register_format(a,
	    NULL,
	    NULL,
	    archive_read_format_empty_bid,
	    NULL,
	    archive_read_format_empty_read_header,
	    archive_read_format_empty_read_data,
	    NULL,
	    NULL);

	return (r);
}


static int
archive_read_format_empty_bid(struct archive_read *a)
{
	//const void *h;
	ssize_t avail;

//	h = 
	__archive_read_ahead(a, 1, &avail);
	if (avail != 0)
		return (-1);
	return (1);
}

static int
archive_read_format_empty_read_header(struct archive_read *a,
    struct archive_entry *entry)
{
	(void)a; /* UNUSED */
	(void)entry; /* UNUSED */

	a->archive.archive_format = ARCHIVE_FORMAT_EMPTY;
	a->archive.archive_format_name = "Empty file";

	return (ARCHIVE_EOF);
}

static int
archive_read_format_empty_read_data(struct archive_read *a,
    const void **buff, size_t *size, off_t *offset)
{
	(void)a; /* UNUSED */
	(void)buff; /* UNUSED */
	(void)size; /* UNUSED */
	(void)offset; /* UNUSED */

	return (ARCHIVE_EOF);
}
