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
#include "test.h"
__FBSDID("$FreeBSD: src/lib/libarchive/test/test_read_compress_program.c,v 1.3 2008/09/01 05:38:33 kientzle Exp $");

static unsigned char archive[] = {
31,139,8,0,222,'C','p','C',0,3,211,'c',160,'=','0','0','0','0','7','5','U',
0,210,134,230,166,6,200,'4',28,'(',24,26,24,27,155,24,152,24,154,27,155,')',
24,24,26,152,154,25,'2','(',152,210,193,'m',12,165,197,'%',137,'E','@',167,
148,'d',230,226,'U','G','H',30,234,15,'8','=',10,'F',193,'(',24,5,131,28,
0,0,29,172,5,240,0,6,0,0};

DEFINE_TEST(test_read_compress_program)
{
	int r;
	struct archive_entry *ae;
	struct archive *a;
	const char *extprog;

	/*
	 * First, test handling when a non-existent compression
	 * program is requested.
	 */
	assert((a = archive_read_new()) != NULL);
	r = archive_read_support_compression_program(a, "nonexistent");
	if (r == ARCHIVE_FATAL) {
		skipping("archive_read_support_compression_program() "
		    "unsupported on this platform");
		return;
	}
	assertEqualIntA(a, ARCHIVE_OK, r);
	assertEqualIntA(a, ARCHIVE_OK,
	    archive_read_support_format_all(a));
	assertEqualIntA(a, ARCHIVE_OK,
	    archive_read_open_memory(a, archive, sizeof(archive)));
	assertEqualIntA(a, ARCHIVE_FATAL,
	    archive_read_next_header(a, &ae));
	assertEqualIntA(a, ARCHIVE_WARN, archive_read_close(a));
	assertEqualInt(ARCHIVE_OK, archive_read_finish(a));

	/*
	 * If we have "gzip -d", try using that.
	 */
	if ((extprog = external_gzip_program(1)) == NULL) {
		skipping("There is no gzip uncompression "
		    "program in this platform");
		return;
	}
	assert((a = archive_read_new()) != NULL);
	assertEqualIntA(a, ARCHIVE_OK,
	    archive_read_support_compression_none(a));
	assertEqualIntA(a, ARCHIVE_OK,
	    archive_read_support_compression_program(a, extprog));
	assertEqualIntA(a, ARCHIVE_OK,
	    archive_read_support_format_all(a));
	assertEqualIntA(a, ARCHIVE_OK,
	    archive_read_open_memory(a, archive, sizeof(archive)));
	assertEqualIntA(a, ARCHIVE_OK,
	    archive_read_next_header(a, &ae));
	assertEqualInt(archive_compression(a), ARCHIVE_COMPRESSION_PROGRAM);
	assertEqualInt(archive_format(a), ARCHIVE_FORMAT_TAR_USTAR);
	assertEqualIntA(a, ARCHIVE_OK, archive_read_close(a));
	assertEqualInt(ARCHIVE_OK, archive_read_finish(a));
}


