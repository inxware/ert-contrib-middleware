@ GStreamer
@
@ Copyright (C) 2009 Texas Instruments, Inc - http://www.ti.com/
@
@ Description: NEON/VFP accelerated functions for armv7 architecture
@  Created on: Nov 27, 2009
@      Author: Rob Clark <rob@ti.com>
@
@ This library is free software; you can redistribute it and/or
@ modify it under the terms of the GNU Library General Public
@ License as published by the Free Software Foundation; either
@ version 2 of the License, or (at your option) any later version.
@
@ This library is distributed in the hope that it will be useful,
@ but WITHOUT ANY WARRANTY; without even the implied warranty of
@ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
@ Library General Public License for more details.
@
@ You should have received a copy of the GNU Library General Public
@ License along with this library; if not, write to the
@ Free Software Foundation, Inc., 59 Temple Place - Suite 330,
@ Boston, MA 02111-1307, USA.

       .fpu neon
       .text

       .align
       .global stride_copy_zip2
       .type   stride_copy_zip2, %function
@void
@stride_copy_zip2 (guchar *new_buf, guchar *orig_buf1, guchar *orig_buf2, gint sz)
@{
@@@@ note: r0-r3, q0-3, and q8-q15 do not need to be preserved
stride_copy_zip2:
@ interleave remaining >= 16 bytes:
       pld [r1, #64]
       pld [r2, #64]
       cmp r3, #16
       blt stride_copy_zip2_2
stride_copy_zip2_1:
       vld1.8 {q8}, [r1]!
       vld1.8 {q9}, [r2]!

       vzip.8 q8, q9

       pld [r1, #64]
       vst1.8 {q8,q9}, [r0]!
       pld [r2, #64]
       sub r3, r3, #16

       cmp r3, #16
       bge stride_copy_zip2_1
@ interleave remaining >= 8 bytes:
stride_copy_zip2_2:
       cmp r3, #8
       blt stride_copy_zip2_3

       vld1.8 {d16}, [r1]!
       vld1.8 {d17}, [r2]!

       vzip.8 d16, d17

       vst1.8 {d16,d17}, [r0]!
       sub r3, r3, #8

@ interleave remaining < 8 bytes:
stride_copy_zip2_3:
@XXX
       bx lr
@}

       .align
       .global stride_copy
       .type   stride_copy, %function
@void
@stride_copy (guchar *new_buf, guchar *orig_buf, gint sz)
@{
@@@@ note: r0-r3, q0-3, and q8-q15 do not need to be preserved
stride_copy:
@ copy remaining >= 64 bytes:
       pld [r1, #64]
       cmp r2, #64
       blt stride_copy_2
stride_copy_1:
       vld1.8 {q8-q9},  [r1]!
       sub r2, r2, #64
       vld1.8 {q10-q11},[r1]!
       vst1.8 {q8-q9},  [r0]!
       pld [r1, #64]
       cmp r2, #64
       vst1.8 {q10-q11},[r0]!
       bge stride_copy_1
@ copy remaining >= 32 bytes:
stride_copy_2:
       cmp r2, #32
       blt stride_copy_3
       vld1.8 {q8-q9}, [r1]!
       sub r2, r2, #32
       vst1.8 {q8-q9}, [r0]!
@ copy remaining >= 16 bytes:
stride_copy_3:
       cmp r2, #16
       blt stride_copy_4
       vld1.8 {q8}, [r1]!
       sub r2, r2, #16
       vst1.8 {q8}, [r0]!
@ copy remaining >= 8 bytes:
stride_copy_4:
       cmp r2, #8
       blt stride_copy_5
       vld1.8 {d16}, [r1]!
       sub r2, r2, #8
       vst1.8 {d16}, [r0]!
@ copy remaining < 8 bytes:
stride_copy_5:
@XXX
       bx lr
@}

