/*
 * cache.c - allocation/initialization/free routines for cache
 *
 * Copyright (C) 2001 Andreas Dilger
 * Copyright (C) 2003 Theodore Ts'o
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 * %End-Header%
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include "blkidP.h"
#include "env.h"

int blkid_debug_mask = 0;

/**
 * SECTION:cache
 * @title: Cache
 * @short_description: basic routines to work with libblkid cache
 *
 * Block device information is normally kept in a cache file blkid.tab and is
 * verified to still be valid before being returned to the user (if the user has
 * read permission on the raw block device, otherwise not).  The cache file also
 * allows unprivileged users (normally anyone other than root, or those not in the
 * "disk" group) to locate devices by label/id.  The standard location of the
 * cache file can be overridden by the environment variable BLKID_FILE.
 *
 * In situations where one is getting information about a single known device, it
 * does not impact performance whether the cache is used or not (unless you are
 * not able to read the block device directly).  If you are dealing with multiple
 * devices, use of the cache is highly recommended (even if empty) as devices will
 * be scanned at most one time and the on-disk cache will be updated if possible.
 * There is rarely a reason not to use the cache.
 *
 * In some cases (modular kernels), block devices are not even visible until after
 * they are accessed the first time, so it is critical that there is some way to
 * locate these devices without enumerating only visible devices, so the use of
 * the cache file is required in this situation.
 */

#if 0 /* ifdef CONFIG_BLKID_DEBUG */
static blkid_debug_dump_cache(int mask, blkid_cache cache)
{
	struct list_head *p;

	if (!cache) {
		printf("cache: NULL\n");
		return;
	}

	printf("cache: time = %lu\n", cache->bic_time);
	printf("cache: flags = 0x%08X\n", cache->bic_flags);

	list_for_each(p, &cache->bic_devs) {
		blkid_dev dev = list_entry(p, struct blkid_struct_dev, bid_devs);
		blkid_debug_dump_dev(dev);
	}
}
#endif

#ifdef CONFIG_BLKID_DEBUG
void blkid_init_debug(int mask)
{
	if (blkid_debug_mask & DEBUG_INIT)
		return;

	if (!mask)
	{
		char *dstr = getenv("LIBBLKID_DEBUG");

		if (!dstr)
			dstr = getenv("BLKID_DEBUG");	/* for backward compatibility */
		if (dstr)
			blkid_debug_mask = strtoul(dstr, 0, 0);
	} else
		blkid_debug_mask = mask;

	if (blkid_debug_mask)
		printf("libblkid: debug mask set to 0x%04x.\n", blkid_debug_mask);

	blkid_debug_mask |= DEBUG_INIT;
}
#endif

static const char *get_default_cache_filename(void)
{
	struct stat st;

	if (stat(BLKID_RUNTIME_TOPDIR, &st) == 0 && S_ISDIR(st.st_mode))
		return BLKID_CACHE_FILE;	/* cache in /run */

	return BLKID_CACHE_FILE_OLD;	/* cache in /etc */
}

/* returns allocated path to cache */
char *blkid_get_cache_filename(struct blkid_config *conf)
{
	char *filename;

	filename = safe_getenv("BLKID_FILE");
	if (filename)
		filename = blkid_strdup(filename);
	else if (conf)
		filename = blkid_strdup(conf->cachefile);
	else {
		struct blkid_config *c = blkid_read_config(NULL);
		if (!c)
			filename = blkid_strdup(get_default_cache_filename());
		else {
			filename = c->cachefile;  /* already allocated */
			c->cachefile = NULL;
			blkid_free_config(c);
		}
	}
	return filename;
}

/**
 * blkid_get_cache:
 * @cache: pointer to return cache handler
 * @filename: path to the cache file or NULL for the default path
 *
 * Allocates and initialize librray cache handler.
 *
 * Returns: 0 on success or number less than zero in case of error.
 */
int blkid_get_cache(blkid_cache *ret_cache, const char *filename)
{
	blkid_cache cache;

	blkid_init_debug(0);

	DBG(DEBUG_CACHE, printf("creating blkid cache (using %s)\n",
				filename ? filename : "default cache"));

	if (!(cache = (blkid_cache) calloc(1, sizeof(struct blkid_struct_cache))))
		return -BLKID_ERR_MEM;

	INIT_LIST_HEAD(&cache->bic_devs);
	INIT_LIST_HEAD(&cache->bic_tags);

	if (filename && !*filename)
		filename = NULL;
	if (filename)
		cache->bic_filename = blkid_strdup(filename);
	else
		cache->bic_filename = blkid_get_cache_filename(NULL);

	blkid_read_cache(cache);
	*ret_cache = cache;
	return 0;
}

/**
 * blkid_put_cache:
 * @cache: cache handler
 *
 * Saves changes to cache file.
 */
void blkid_put_cache(blkid_cache cache)
{
	if (!cache)
		return;

	(void) blkid_flush_cache(cache);

	DBG(DEBUG_CACHE, printf("freeing cache struct\n"));

	/* DBG(DEBUG_CACHE, blkid_debug_dump_cache(cache)); */

	while (!list_empty(&cache->bic_devs)) {
		blkid_dev dev = list_entry(cache->bic_devs.next,
					   struct blkid_struct_dev,
					    bid_devs);
		blkid_free_dev(dev);
	}

	while (!list_empty(&cache->bic_tags)) {
		blkid_tag tag = list_entry(cache->bic_tags.next,
					   struct blkid_struct_tag,
					   bit_tags);

		while (!list_empty(&tag->bit_names)) {
			blkid_tag bad = list_entry(tag->bit_names.next,
						   struct blkid_struct_tag,
						   bit_names);

			DBG(DEBUG_CACHE, printf("warning: unfreed tag %s=%s\n",
						bad->bit_name, bad->bit_val));
			blkid_free_tag(bad);
		}
		blkid_free_tag(tag);
	}

	blkid_free_probe(cache->probe);

	free(cache->bic_filename);
	free(cache);
}

/**
 * blkid_gc_cache:
 * @cache: cache handler
 *
 * Removes garbage (non-existing devices) from the cache.
 */
void blkid_gc_cache(blkid_cache cache)
{
	struct list_head *p, *pnext;
	struct stat st;

	if (!cache)
		return;

	list_for_each_safe(p, pnext, &cache->bic_devs) {
		blkid_dev dev = list_entry(p, struct blkid_struct_dev, bid_devs);
		if (stat(dev->bid_name, &st) < 0) {
			DBG(DEBUG_CACHE,
			    printf("freeing %s\n", dev->bid_name));
			blkid_free_dev(dev);
			cache->bic_flags |= BLKID_BIC_FL_CHANGED;
		} else {
			DBG(DEBUG_CACHE,
			    printf("Device %s exists\n", dev->bid_name));
		}
	}
}

#ifdef TEST_PROGRAM
int main(int argc, char** argv)
{
	blkid_cache cache = NULL;
	int ret;

	blkid_init_debug(DEBUG_ALL);

	if ((argc > 2)) {
		fprintf(stderr, "Usage: %s [filename] \n", argv[0]);
		exit(1);
	}

	if ((ret = blkid_get_cache(&cache, argv[1])) < 0) {
		fprintf(stderr, "error %d parsing cache file %s\n", ret,
			argv[1] ? argv[1] : blkid_get_cache_filename(NULL));
		exit(1);
	}
	if ((ret = blkid_get_cache(&cache, "/dev/null")) != 0) {
		fprintf(stderr, "%s: error creating cache (%d)\n",
			argv[0], ret);
		exit(1);
	}
	if ((ret = blkid_probe_all(cache) < 0))
		fprintf(stderr, "error probing devices\n");

	blkid_put_cache(cache);

	return ret;
}
#endif
