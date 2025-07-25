/*
 * exsltconfig.h: compile-time version information for the EXSLT library
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_EXSLTCONFIG_H__
#define __XML_EXSLTCONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LIBEXSLT_DOTTED_VERSION:
 *
 * the version string like "1.2.3"
 */
#define LIBEXSLT_DOTTED_VERSION "1.1.26"

/**
 * LIBEXSLT_VERSION:
 *
 * the version number: 1.2.3 value is 10203
 */
#define LIBEXSLT_VERSION 815

/**
 * LIBEXSLT_VERSION_STRING:
 *
 * the version number string, 1.2.3 value is "10203"
 */
#define LIBEXSLT_VERSION_STRING "815"

/**
 * LIBEXSLT_VERSION_EXTRA:
 *
 * extra version information, used to show a CVS compilation
 */
#define	LIBEXSLT_VERSION_EXTRA ""

/**
 * WITH_CRYPTO:
 *
 * Whether crypto support is configured into exslt
 */
#if 1
#define EXSLT_CRYPTO_ENABLED
#endif

/**
 * ATTRIBUTE_UNUSED:
 *
 * This macro is used to flag unused function parameters to GCC
 */
#ifdef __GNUC__
#ifdef HAVE_ANSIDECL_H
#include <ansidecl.h>
#endif
#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED __attribute__((unused))
#endif
#else
#define ATTRIBUTE_UNUSED
#endif

#ifdef __cplusplus
}
#endif

#endif /* __XML_EXSLTCONFIG_H__ */
