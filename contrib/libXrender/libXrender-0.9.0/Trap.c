/*
 * $Id: Trap.c,v 1.5 2003/04/24 03:29:15 nlevitt Exp $
 *
 * Copyright � 2002 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "Xrenderint.h"

void
XRenderCompositeTrapezoids (Display		*dpy,
			    int			op,
			    Picture		src,
			    Picture		dst,
			    _Xconst XRenderPictFormat	*maskFormat,
			    int			xSrc,
			    int			ySrc,
			    _Xconst XTrapezoid	*traps,
			    int			ntrap)
{
    XRenderExtDisplayInfo         *info = XRenderFindDisplay (dpy);
    xRenderTrapezoidsReq    *req;
    int			    n;
    long    		    len;
    unsigned long	    max_req = dpy->bigreq_size ? dpy->bigreq_size : dpy->max_request_size;

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    while (ntrap)
    {
	GetReq(RenderTrapezoids, req);
	req->reqType = info->codes->major_opcode;
	req->renderReqType = X_RenderTrapezoids;
	req->op = (CARD8) op;
	req->src = src;
	req->dst = dst;
	req->maskFormat = maskFormat ? maskFormat->id : 0;
	req->xSrc = xSrc;
	req->ySrc = ySrc;
	n = ntrap;
	len = ((long) n) * (SIZEOF (xTrapezoid) >> 2);
	if (len > (max_req - req->length)) {
	    n = (max_req - req->length) / (SIZEOF (xTrapezoid) >> 2);
	    len = ((long)n) * (SIZEOF (xTrapezoid) >> 2);
	}
	SetReqLen (req, len, len);
	len <<= 2;
	DataInt32 (dpy, (int *) traps, len);
	ntrap -= n;
	traps += n;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

