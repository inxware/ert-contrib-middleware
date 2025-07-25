/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM c:/builds/tinderbox/MozillaTrunk/WINNT_5.0_Clobber/mozilla/embedding/browser/webBrowser/nsIContextMenuListener.idl
 */

#ifndef __gen_nsIContextMenuListener_h__
#define __gen_nsIContextMenuListener_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIDOMEvent; /* forward declaration */

class nsIDOMNode; /* forward declaration */


/* starting interface:    nsIContextMenuListener */
#define NS_ICONTEXTMENULISTENER_IID_STR "3478b6b0-3875-11d4-94ef-0020183bf181"

#define NS_ICONTEXTMENULISTENER_IID \
  {0x3478b6b0, 0x3875, 0x11d4, \
    { 0x94, 0xef, 0x00, 0x20, 0x18, 0x3b, 0xf1, 0x81 }}

/**
 * An optional interface for embedding clients wishing to receive
 * notifications for context menu events (e.g. generated by
 * a user right-mouse clicking on a link). The embedder implements
 * this interface on the web browser chrome object associated
 * with the window that notifications are required for. When a context
 * menu event, the browser will call this interface if present.
 * 
 * @see nsIDOMNode
 * @see nsIDOMEvent
 *
 * @status FROZEN
 */
class NS_NO_VTABLE nsIContextMenuListener : public nsISupports {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ICONTEXTMENULISTENER_IID)

  /** Flag. No context. */
  enum { CONTEXT_NONE = 0U };

  /** Flag. Context is a link element. */
  enum { CONTEXT_LINK = 1U };

  /** Flag. Context is an image element. */
  enum { CONTEXT_IMAGE = 2U };

  /** Flag. Context is the whole document. */
  enum { CONTEXT_DOCUMENT = 4U };

  /** Flag. Context is a text area element. */
  enum { CONTEXT_TEXT = 8U };

  /** Flag. Context is an input element. */
  enum { CONTEXT_INPUT = 16U };

  /**
     * Called when the browser receives a context menu event (e.g. user is right-mouse
     * clicking somewhere on the document). The combination of flags, event and node
     * provided in the call indicate where and what was clicked on.
     *
     * The following table describes what context flags and node combinations are
     * possible.
     *
     * <TABLE>
     * <TR><TD><B>aContextFlag</B></TD><TD>aNode</TD></TR>
     * <TR><TD>CONTEXT_LINK</TD><TD>&lt;A&gt;</TD></TR>
     * <TR><TD>CONTEXT_IMAGE</TD><TD>&lt;IMG&gt;</TD></TR>
     * <TR><TD>CONTEXT_IMAGE | CONTEXT_LINK</TD><TD>&lt;IMG&gt;
     *       with an &lt;A&gt; as an ancestor</TD></TR>
     * <TR><TD>CONTEXT_INPUT</TD><TD>&lt;INPUT&gt;</TD></TR>
     * <TR><TD>CONTEXT_TEXT</TD><TD>&lt;TEXTAREA&gt;</TD></TR>
     * <TR><TD>CONTEXT_DOCUMENT</TD><TD>&lt;HTML&gt;</TD></TR>
     * </TABLE>
     *
     * @param aContextFlags Flags indicating the kind of context.
     * @param aEvent The DOM context menu event.
     * @param aNode The DOM node most relevant to the context.
     *
     * @return <CODE>NS_OK</CODE> always.
     */
  /* void onShowContextMenu (in unsigned long aContextFlags, in nsIDOMEvent aEvent, in nsIDOMNode aNode); */
  NS_IMETHOD OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode) = 0;

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSICONTEXTMENULISTENER \
  NS_IMETHOD OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSICONTEXTMENULISTENER(_to) \
  NS_IMETHOD OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode) { return _to OnShowContextMenu(aContextFlags, aEvent, aNode); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSICONTEXTMENULISTENER(_to) \
  NS_IMETHOD OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode) { return !_to ? NS_ERROR_NULL_POINTER : _to->OnShowContextMenu(aContextFlags, aEvent, aNode); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsContextMenuListener : public nsIContextMenuListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTEXTMENULISTENER

  nsContextMenuListener();

private:
  ~nsContextMenuListener();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsContextMenuListener, nsIContextMenuListener)

nsContextMenuListener::nsContextMenuListener()
{
  /* member initializers and constructor code */
}

nsContextMenuListener::~nsContextMenuListener()
{
  /* destructor code */
}

/* void onShowContextMenu (in unsigned long aContextFlags, in nsIDOMEvent aEvent, in nsIDOMNode aNode); */
NS_IMETHODIMP nsContextMenuListener::OnShowContextMenu(PRUint32 aContextFlags, nsIDOMEvent *aEvent, nsIDOMNode *aNode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIContextMenuListener_h__ */
