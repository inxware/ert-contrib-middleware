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
__FBSDID("$FreeBSD: src/lib/libarchive/test/test_fuzz.c,v 1.1 2008/12/06 07:08:08 kientzle Exp $");

/*
 * This was inspired by an ISO fuzz tester written by Michal Zalewski
 * and posted to the "vulnwatch" mailing list on March 17, 2005:
 *    http://seclists.org/vulnwatch/2005/q1/0088.html
 *
 * This test simply reads each archive image into memory, pokes
 * random values into it and runs it through libarchive.  It tries
 * to damage about 1% of each file and repeats the exercise 100 times
 * with each file.
 *
 * Unlike most other tests, this test does not verify libarchive's
 * responses other than to ensure that libarchive doesn't crash.
 *
 * Due to the deliberately random nature of this test, it may be hard
 * to reproduce failures.  Because this test deliberately attempts to
 * induce crashes, there's little that can be done in the way of
 * post-failure diagnostics.
 */

/* Because this works for any archive, I can just re-use the archives
 * developed for other tests.  I've not included all of the compressed
 * archives here, though; I don't want to spend all of my test time
 * testing zlib and bzlib. */
static const char *
files[] = {
	"test_fuzz_1.iso",
	"test_compat_bzip2_1.tbz",
	"test_compat_gtar_1.tar",
	"test_compat_tar_hardlink_1.tar",
	"test_compat_zip_1.zip",
	"test_read_format_gtar_sparse_1_17_posix10_modified.tar",
	"test_read_format_tar_empty_filename.tar",
	"test_read_format_zip.zip",
	NULL
};

DEFINE_TEST(test_fuzz)
{
	const char **filep;

	for (filep = files; *filep != NULL; ++filep) {
		struct archive_entry *ae;
		struct archive *a;
		char *rawimage, *image;
		size_t size;
		int i, r;

		extract_reference_file(*filep);
		rawimage = slurpfile(&size, *filep);
		assert(rawimage != NULL);
		image = malloc(size);
		assert(image != NULL);
		srand(time(NULL));

		assert((a = archive_read_new()) != NULL);
		assert(0 == archive_read_support_compression_all(a));
		assert(0 == archive_read_support_format_all(a));
		assert(0 == archive_read_open_memory(a, rawimage, size));
		r = archive_read_next_header(a, &ae);
		if (UnsupportedCompress(r, a)) {
			skipping("Skipping GZIP/BZIP2 compression check: "
			    "This version of libarchive was compiled "
			    "without gzip/bzip2 support");
			assert(0 == archive_read_close(a));
			assert(0 == archive_read_finish(a));
			continue;
		}
		assert(0 == r);
		if (r == ARCHIVE_OK) {
			char buff[20];

			r = archive_read_data(a, buff, 19);
			if (r < ARCHIVE_OK && strcmp(archive_error_string(a),
			    "libarchive compiled without deflate support (no libz)") == 0) {
				skipping("Skipping ZIP compression check: %s",
				    archive_error_string(a));
				assert(0 == archive_read_close(a));
				assert(0 == archive_read_finish(a));
				continue;
			}
		}
		assert(0 == archive_read_close(a));
		assert(0 == archive_read_finish(a));

		for (i = 0; i < 100; ++i) {
			int j, fd, numbytes;

			/* Fuzz < 1% of the bytes in the archive. */
			memcpy(image, rawimage, size);
			numbytes = (int)(rand() % (size / 100));
			for (j = 0; j < numbytes; ++j)
				image[rand() % size] = (char)rand();

			/* Save the messed-up image to a file.
			 * If we crash, that file will be useful. */
			fd = open("after.test.failure.send.this.file."
			    "to.libarchive.maintainers.with.system.details",
			    O_WRONLY | O_CREAT | O_TRUNC, 0744);
			write(fd, image, (off_t)size);
			close(fd);

			assert((a = archive_read_new()) != NULL);
			assertEqualIntA(a, ARCHIVE_OK,
			    archive_read_support_compression_all(a));
			assertEqualIntA(a, ARCHIVE_OK,
			    archive_read_support_format_all(a));

			if (0 == archive_read_open_memory(a, image, size)) {
				while(0 == archive_read_next_header(a, &ae)) {
					archive_read_data_skip(a);
				}
				archive_read_close(a);
				archive_read_finish(a);
			}
		}
		free(image);
		free(rawimage);
	}
}


