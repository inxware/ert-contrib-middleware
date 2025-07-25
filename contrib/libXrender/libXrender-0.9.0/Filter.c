/*
 * $Id: Filter.c,v 1.4 2003/06/08 17:56:37 keithp Exp $
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
 
XFilters *
XRenderQueryFilters (Display *dpy, Drawable drawable)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    XRenderInfo			*xri;
    xRenderQueryFiltersReq	*req;
    xRenderQueryFiltersReply	rep;
    XFilters			*filters;
    char			*name;
    char			len;
    int				i;
    long			nbytes, nbytesAlias, nbytesName;
    
    if (!RenderHasExtension (info))
	return 0;

    if (!XRenderQueryFormats (dpy))
	return 0;

    xri = info->info;
    if (xri->minor_version < 6)
	return 0;
    
    LockDisplay (dpy);
    GetReq (RenderQueryFilters, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderQueryFilters;
    req->drawable = drawable;
    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse))
    {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    /*
     * Compute total number of bytes for filter names
     */
    nbytes = (long)rep.length << 2;
    nbytesAlias = rep.numAliases * 2;
    if (rep.numAliases & 1)
	nbytesAlias += 2;
    nbytesName = nbytes - nbytesAlias;
    
    /*
     * Allocate one giant block for the whole data structure
     */
    filters = Xmalloc (sizeof (XFilters) +
		       rep.numFilters * sizeof (char *) +
		       rep.numAliases * sizeof (short) +
		       nbytesName);

    if (!filters)
    {
	_XEatData (dpy, (unsigned long) rep.length << 2);
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }

    /*
     * Layout:
     *	XFilters
     *	numFilters  char * pointers to filter names
     *	numAliases  short alias values
     *	nbytesName  char strings
     */
    
    filters->nfilter = rep.numFilters;
    filters->nalias = rep.numAliases;
    filters->filter = (char **) (filters + 1);
    filters->alias = (short *) (filters->filter + rep.numFilters);
    name = (char *) (filters->alias + rep.numAliases);

    /*
     * Read the filter aliases
     */
    _XRead16Pad (dpy, filters->alias, 2 * rep.numAliases);

    /*
     * Read the filter names
     */
    for (i = 0; i < rep.numFilters; i++)
    {
	int	l;
	_XRead (dpy, &len, 1);
	l = len & 0xff;
	filters->filter[i] = name;
	_XRead (dpy, name, l);
	name[l] = '\0';
	name += l + 1;
    }
    i = name - (char *) (filters->alias + rep.numAliases);

    if (i & 3)
	_XEatData (dpy, 4 - (i & 3));
    
    UnlockDisplay (dpy);
    return filters;
}

void
XRenderSetPictureFilter  (Display   *dpy,
			  Picture   picture,
			  char	    *filter,
			  XFixed    *params,
			  int	    nparams)
{
    XRenderExtDisplayInfo		*info = XRenderFindDisplay (dpy);
    xRenderSetPictureFilterReq	*req;
    int				nbytes = strlen (filter);

    RenderSimpleCheckExtension (dpy, info);
    LockDisplay(dpy);
    GetReq(RenderSetPictureFilter, req);
    req->reqType = info->codes->major_opcode;
    req->renderReqType = X_RenderSetPictureFilter;
    req->picture = picture;
    req->nbytes = nbytes;
    req->length += ((nbytes + 3) >> 2) + nparams;
    Data (dpy, filter, nbytes);
    Data32 (dpy, params, nparams << 2);
    UnlockDisplay(dpy);
    SyncHandle();
}
