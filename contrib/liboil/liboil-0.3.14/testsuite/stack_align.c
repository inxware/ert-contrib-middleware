/*
 * Copyright (c) 2004 David A. Schleef <ds@schleef.org>
 * Copyright (c) 2005 Eric Anholt <anholt@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <liboil/liboil.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <liboil/liboilprototype.h>
#include <liboil/liboiltest.h>
#include <liboil/liboilcpu.h>

int verbose = 1;

/* Amount by which results of different types are allowed to deviate from the
 * reference.
 */
#define INT_EPSILON 1
#define FLOAT_EPSILON 0.0001

void
dump_array (void *data, void *ref_data, OilType type, int pre_n, int stride,
    int post_n)
{
  int i, j;
  int s2 = oil_type_sizeof (type);
  double x;

#define DUMP(type, format, int) do { \
  for(i=0;i<post_n;i++){ \
    float epsilon = (int) ? INT_EPSILON : FLOAT_EPSILON; \
    printf("    "); \
    for(j=0;j<pre_n;j++){ \
      x = fabs(OIL_GET(data, i*stride + j*s2, type) - \
          OIL_GET(ref_data, i*stride + j*s2, type)); \
      if (x > epsilon) { \
        printf("*" format "* (" format ") ", \
	    OIL_GET(data, i*stride + j*s2, type), \
	    OIL_GET(ref_data, i*stride + j*s2, type)); \
      } else { \
        printf(" " format " ", OIL_GET(data, i*stride + j*s2, type)); \
      } \
    } \
    printf("\n"); \
  } \
} while(0)

  switch(type) {
    case OIL_TYPE_s8p:
    case OIL_TYPE_u8p:
      DUMP(int8_t, "0x%02" PRIx8, 1);
      break;
    case OIL_TYPE_s16p:
    case OIL_TYPE_u16p:
      DUMP(uint16_t, "0x%04" PRIx16, 1);
      break;
    case OIL_TYPE_s32p:
    case OIL_TYPE_u32p:
      DUMP(uint32_t, "0x%08" PRIx32, 1);
      break;
    case OIL_TYPE_f32p:
      DUMP(float, "%g", 0);
      break;
    case OIL_TYPE_s64p:
    case OIL_TYPE_u64p:
      DUMP(uint64_t, "0x%016" PRIx64, 1);
      break;
    case OIL_TYPE_f64p:
      DUMP(double, "%g", 0);
      break;
    default:
      break;
  }
}

void
dump_source (OilTest *test)
{
  int i;
  for(i=0;i<OIL_ARG_LAST;i++){
    OilParameter *p = &test->params[i];
    if (p->is_pointer) {
      if (p->direction == 'i' || p->direction == 's') {
        printf ("  %s:\n", p->parameter_name);
        dump_array (p->src_data + p->test_header,
            p->src_data + p->test_header,
            p->type, p->pre_n, p->stride, p->post_n);
      }
    }
  }
}

void
dump_dest_ref (OilTest *test)
{
  int i;
  for(i=0;i<OIL_ARG_LAST;i++){
    OilParameter *p = &test->params[i];
    if (p->is_pointer) {
      if (p->direction == 'd') {
        printf ("  %s:\n", p->parameter_name);
        dump_array (p->ref_data + p->test_header,
            p->ref_data + p->test_header,
            p->type, p->pre_n, p->stride, p->post_n);
      }
    }
  }
}

int
test_difference (void *data, void *ref_data, OilType type, int pre_n, int stride,
    int post_n)
{
  int i, j;
  int s2 = oil_type_sizeof (type);
  double x;

#define CHECK(type, is_int) do { \
  float epsilon = (is_int) ? INT_EPSILON : FLOAT_EPSILON; \
  for(i=0;i<post_n;i++){ \
    for(j=0;j<pre_n;j++){ \
      x = fabs(OIL_GET(data, i*stride + j*s2, type) - \
          OIL_GET(ref_data, i*stride + j*s2, type)); \
      if (x > epsilon) { \
        return 1; \
      } \
    } \
  } \
  return 0; \
} while(0)

  switch(type) {
    case OIL_TYPE_s8p:
      CHECK(int8_t, 1);
      break;
    case OIL_TYPE_u8p:
      CHECK(uint8_t, 1);
      break;
    case OIL_TYPE_s16p:
      CHECK(int16_t, 1);
      break;
    case OIL_TYPE_u16p:
      CHECK(uint16_t, 1);
      break;
    case OIL_TYPE_s32p:
      CHECK(int32_t, 1);
      break;
    case OIL_TYPE_u32p:
      CHECK(uint32_t, 1);
      break;
    case OIL_TYPE_s64p:
      CHECK(int64_t, 1);
      break;
    case OIL_TYPE_u64p:
      CHECK(uint64_t, 1);
      break;
    case OIL_TYPE_f32p:
      CHECK(float, 0);
      break;
    case OIL_TYPE_f64p:
      CHECK(double, 0);
      break;
    default:
      return 1;
  }
}

int
check_test (OilTest *test)
{
  int i, failed = 0;
  for(i=0;i<OIL_ARG_LAST;i++){
    OilParameter *p = &test->params[i];
    if (p->is_pointer) {
      if (p->direction == 'i' || p->direction == 'd') {
	if (!test_difference(p->test_data + p->test_header,
            p->ref_data + p->test_header,
            p->type, p->pre_n, p->stride, p->post_n))
	  continue;
        printf (" Failure in %s (marked by *, ref in ()):\n",
	    p->parameter_name);
        dump_array (p->test_data + p->test_header,
            p->ref_data + p->test_header,
            p->type, p->pre_n, p->stride, p->post_n);
	failed = 1;
      }
    }
  }
  return failed;
}

void print_align(void *ptr)
{
  int i[4];

  if (verbose)printf("stack addr %p\n", &i);
}

OilFunctionClass *realign_klass;
int realign_align;
int realign_return;

void realign(int align)
{
#ifdef HAVE_GCC_ASM
#ifdef HAVE_I386
  __asm__ __volatile__ (
      "  sub %%ebx, %%esp\n"
      "  call check_class_with_alignment\n"
      "  add %%ebx, %%esp\n"
      :: "b" (align)
  );
#endif
#ifdef HAVE_AMD64
  __asm__ __volatile__ (
      "  sub %%rbx, %%rsp\n"
      "  call check_class_with_alignment\n"
      "  add %%rbx, %%rsp\n"
      :: "b" (align)
  );
#endif
#endif
}

void check_class_with_alignment (void)
{
  OilFunctionClass *klass = realign_klass;
  int align = realign_align;
  int test_failed = 0;
  OilTest *test;
  OilFunctionImpl *impl;

  test = oil_test_new(klass);

  oil_test_set_iterations(test, 1);

  impl = klass->reference_impl;
  oil_test_check_impl (test, impl);

  for (impl = klass->first_impl; impl; impl = impl->next) {
    if (impl == klass->reference_impl)
      continue;
    if (oil_impl_is_runnable (impl)) {
      if (!oil_test_check_impl (test, impl)) {
        printf ("impl %s with align %d\n", impl->name, align);
        printf("dests for %s:\n", klass->name);
        dump_dest_ref(test);
        printf("sources for %s:\n", klass->name);
        dump_source(test);
      }
    }
  }
  oil_test_free(test);

  realign_return = test_failed;
}

int check_class(OilFunctionClass *klass)
{
  OilTest *test;
  int failed = 0;
  int align;
  int step = 4;

  oil_class_optimize (klass);

  if(verbose) printf("checking class %s\n", klass->name);
  
  test = oil_test_new(klass);

#ifdef HAVE_AMD64
  step = 16;
#endif

  for (align = 0; align <= 32; align += step) {
    printf("  alignment %d\n", align);
    realign_klass = klass;
    realign_align = align;
    realign(align);
    failed |= realign_return;
  }
  oil_test_free (test);

  return failed;
}

int main (int argc, char *argv[])
{
  int failed = 0;
  int i, n;

  oil_init ();

  n = oil_class_get_n_classes ();
  for (i = 0; i < n; i++) {
    OilFunctionClass *klass = oil_class_get_by_index(i);
    failed |= check_class(klass);
  }

  return failed;
}
