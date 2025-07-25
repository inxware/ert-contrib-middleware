/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM c:/builds/tinderbox/MozillaTrunk/WINNT_5.0_Clobber/mozilla/netwerk/base/public/nsIChannel.idl
 */

#ifndef __gen_nsIChannel_h__
#define __gen_nsIChannel_h__


#ifndef __gen_nsIRequest_h__
#include "nsIRequest.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIURI; /* forward declaration */

class nsIInterfaceRequestor; /* forward declaration */

class nsIInputStream; /* forward declaration */

class nsIStreamListener; /* forward declaration */


/* starting interface:    nsIChannel */
#define NS_ICHANNEL_IID_STR "c63a055a-a676-4e71-bf3c-6cfa11082018"

#define NS_ICHANNEL_IID \
  {0xc63a055a, 0xa676, 0x4e71, \
    { 0xbf, 0x3c, 0x6c, 0xfa, 0x11, 0x08, 0x20, 0x18 }}

/**
 * The nsIChannel interface allows clients to construct "GET" requests for
 * specific protocols, and manage them in a uniform way.  Once a channel is
 * created (via nsIIOService::newChannel), parameters for that request may
 * be set by using the channel attributes, or by QI'ing to a subclass of
 * nsIChannel for protocol-specific parameters.  Then, the URI can be fetched
 * by calling nsIChannel::open or nsIChannel::asyncOpen.
 *
 * After a request has been completed, the channel is still valid for accessing
 * protocol-specific results.  For example, QI'ing to nsIHttpChannel allows
 * response headers to be retrieved for the corresponding http transaction. 
 *
 * @status FROZEN
 */
class NS_NO_VTABLE nsIChannel : public nsIRequest {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ICHANNEL_IID)

  /**
     * The original URI used to construct the channel. This is used in the case
     * of a redirect or URI "resolution" (e.g. resolving a resource: URI to a
     * file: URI) so that the original pre-redirect URI can still be obtained. 
     *
     * NOTE: this is distinctly different from the http Referer (referring URI),
     * which is typically the page that contained the original URI (accessible
     * from nsIHttpChannel).
     */
  /* attribute nsIURI originalURI; */
  NS_IMETHOD GetOriginalURI(nsIURI * *aOriginalURI) = 0;
  NS_IMETHOD SetOriginalURI(nsIURI * aOriginalURI) = 0;

  /**
     * The URI corresponding to the channel.  Its value is immutable.
     */
  /* readonly attribute nsIURI URI; */
  NS_IMETHOD GetURI(nsIURI * *aURI) = 0;

  /**
     * The owner, corresponding to the entity that is responsible for this
     * channel.  Used by the security manager to grant or deny privileges to
     * mobile code loaded from this channel.
     *
     * NOTE: this is a strong reference to the owner, so if the owner is also
     * holding a strong reference to the channel, care must be taken to 
     * explicitly drop its reference to the channel.
     */
  /* attribute nsISupports owner; */
  NS_IMETHOD GetOwner(nsISupports * *aOwner) = 0;
  NS_IMETHOD SetOwner(nsISupports * aOwner) = 0;

  /**
     * The notification callbacks for the channel.  This is set by clients, who
     * wish to provide a means to receive progress, status and protocol-specific 
     * notifications.  If this value is NULL, the channel implementation may use
     * the notification callbacks from its load group.
     * 
     * Interfaces commonly requested include: nsIProgressEventSink, nsIPrompt,
     * and nsIAuthPrompt.
     *
     * NOTE: channels generally query this attribute when they are opened and
     * retain references to the resulting interfaces until they are done.  A
     * channel may ignore changes to the notificationCallbacks attribute after
     * it has been opened.  This rule also applies to notificationCallbacks
     * queried from the channel's loadgroup.
     */
  /* attribute nsIInterfaceRequestor notificationCallbacks; */
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks) = 0;
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks) = 0;

  /**
     * Transport-level security information (if any) corresponding to the channel.
     */
  /* readonly attribute nsISupports securityInfo; */
  NS_IMETHOD GetSecurityInfo(nsISupports * *aSecurityInfo) = 0;

  /**
     * The MIME type of the channel's content if available. 
     * 
     * NOTE: the content type can often be wrongly specified (e.g., wrong file
     * extension, wrong MIME type, wrong document type stored on a server, etc.),
     * and the caller most likely wants to verify with the actual data.
     *
     * Setting contentType before the channel has been opened provides a hint
     * to the channel as to what the MIME type is.  The channel may ignore this
     * hint in deciding on the actual MIME type that it will report.
     *
     * Setting contentType after onStartRequest has been fired or after open()
     * is called will override the type determined by the channel.
     *
     * Setting contentType between the time that asyncOpen() is called and the
     * time when onStartRequest is fired has undefined behavior at this time.
     *
     * The value of the contentType attribute is a lowercase string.  A value
     * assigned to this attribute will be parsed and normalized as follows:
     *  1- any parameters (delimited with a ';') will be stripped.
     *  2- if a charset parameter is given, then its value will replace the
     *     the contentCharset attribute of the channel.
     *  3- the stripped contentType will be lowercased.
     * Any implementation of nsIChannel must follow these rules.
     */
  /* attribute ACString contentType; */
  NS_IMETHOD GetContentType(nsACString & aContentType) = 0;
  NS_IMETHOD SetContentType(const nsACString & aContentType) = 0;

  /**
     * The character set of the channel's content if available and if applicable.
     * This attribute only applies to textual data.
     *
     * The value of the contentCharset attribute is a mixedcase string.
     */
  /* attribute ACString contentCharset; */
  NS_IMETHOD GetContentCharset(nsACString & aContentCharset) = 0;
  NS_IMETHOD SetContentCharset(const nsACString & aContentCharset) = 0;

  /**
     * The length of the data associated with the channel if available.  A value
     * of -1 indicates that the content length is unknown.
     */
  /* attribute long contentLength; */
  NS_IMETHOD GetContentLength(PRInt32 *aContentLength) = 0;
  NS_IMETHOD SetContentLength(PRInt32 aContentLength) = 0;

  /**
     * Synchronously open the channel.
     *
     * @return blocking input stream to the channel's data.
     *
     * NOTE: nsIChannel implementations are not required to implement this
     * method.  Moreover, since this method may block the calling thread, it
     * should not be called on a thread that processes UI events.
     */
  /* nsIInputStream open (); */
  NS_IMETHOD Open(nsIInputStream **_retval) = 0;

  /**
     * Asynchronously open this channel.  Data is fed to the specified stream
     * listener as it becomes available.
     *
     * @param aListener the nsIStreamListener implementation
     * @param aContext an opaque parameter forwarded to aListener's methods
     */
  /* void asyncOpen (in nsIStreamListener aListener, in nsISupports aContext); */
  NS_IMETHOD AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext) = 0;

  /**************************************************************************
     * Channel specific load flags:
     *
     * Bits 21-31 are reserved for future use by this interface or one of its
     * derivatives (e.g., see nsICachingChannel).
     */
/**
     * Set (e.g., by the docshell) to indicate whether or not the channel
     * corresponds to a document URI.
     */
  enum { LOAD_DOCUMENT_URI = 65536U };

  /** 
     * If the end consumer for this load has been retargeted after discovering 
     * it's content, this flag will be set:
     */
  enum { LOAD_RETARGETED_DOCUMENT_URI = 131072U };

  /**
     * This flag is set to indicate that onStopRequest may be followed by
     * another onStartRequest/onStopRequest pair.  This flag is, for example,
     * used by the multipart/replace stream converter.
     */
  enum { LOAD_REPLACE = 262144U };

  /**
     * Set (e.g., by the docshell) to indicate whether or not the channel
     * corresponds to an initial document URI load (e.g., link click).
     */
  enum { LOAD_INITIAL_DOCUMENT_URI = 524288U };

  /**
     * Set (e.g., by the URILoader) to indicate whether or not the end consumer
     * for this load has been determined.
     */
  enum { LOAD_TARGETED = 1048576U };

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSICHANNEL \
  NS_IMETHOD GetOriginalURI(nsIURI * *aOriginalURI); \
  NS_IMETHOD SetOriginalURI(nsIURI * aOriginalURI); \
  NS_IMETHOD GetURI(nsIURI * *aURI); \
  NS_IMETHOD GetOwner(nsISupports * *aOwner); \
  NS_IMETHOD SetOwner(nsISupports * aOwner); \
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks); \
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks); \
  NS_IMETHOD GetSecurityInfo(nsISupports * *aSecurityInfo); \
  NS_IMETHOD GetContentType(nsACString & aContentType); \
  NS_IMETHOD SetContentType(const nsACString & aContentType); \
  NS_IMETHOD GetContentCharset(nsACString & aContentCharset); \
  NS_IMETHOD SetContentCharset(const nsACString & aContentCharset); \
  NS_IMETHOD GetContentLength(PRInt32 *aContentLength); \
  NS_IMETHOD SetContentLength(PRInt32 aContentLength); \
  NS_IMETHOD Open(nsIInputStream **_retval); \
  NS_IMETHOD AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext); \

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSICHANNEL(_to) \
  NS_IMETHOD GetOriginalURI(nsIURI * *aOriginalURI) { return _to GetOriginalURI(aOriginalURI); } \
  NS_IMETHOD SetOriginalURI(nsIURI * aOriginalURI) { return _to SetOriginalURI(aOriginalURI); } \
  NS_IMETHOD GetURI(nsIURI * *aURI) { return _to GetURI(aURI); } \
  NS_IMETHOD GetOwner(nsISupports * *aOwner) { return _to GetOwner(aOwner); } \
  NS_IMETHOD SetOwner(nsISupports * aOwner) { return _to SetOwner(aOwner); } \
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks) { return _to GetNotificationCallbacks(aNotificationCallbacks); } \
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks) { return _to SetNotificationCallbacks(aNotificationCallbacks); } \
  NS_IMETHOD GetSecurityInfo(nsISupports * *aSecurityInfo) { return _to GetSecurityInfo(aSecurityInfo); } \
  NS_IMETHOD GetContentType(nsACString & aContentType) { return _to GetContentType(aContentType); } \
  NS_IMETHOD SetContentType(const nsACString & aContentType) { return _to SetContentType(aContentType); } \
  NS_IMETHOD GetContentCharset(nsACString & aContentCharset) { return _to GetContentCharset(aContentCharset); } \
  NS_IMETHOD SetContentCharset(const nsACString & aContentCharset) { return _to SetContentCharset(aContentCharset); } \
  NS_IMETHOD GetContentLength(PRInt32 *aContentLength) { return _to GetContentLength(aContentLength); } \
  NS_IMETHOD SetContentLength(PRInt32 aContentLength) { return _to SetContentLength(aContentLength); } \
  NS_IMETHOD Open(nsIInputStream **_retval) { return _to Open(_retval); } \
  NS_IMETHOD AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext) { return _to AsyncOpen(aListener, aContext); } \

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSICHANNEL(_to) \
  NS_IMETHOD GetOriginalURI(nsIURI * *aOriginalURI) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOriginalURI(aOriginalURI); } \
  NS_IMETHOD SetOriginalURI(nsIURI * aOriginalURI) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOriginalURI(aOriginalURI); } \
  NS_IMETHOD GetURI(nsIURI * *aURI) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetURI(aURI); } \
  NS_IMETHOD GetOwner(nsISupports * *aOwner) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOwner(aOwner); } \
  NS_IMETHOD SetOwner(nsISupports * aOwner) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOwner(aOwner); } \
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetNotificationCallbacks(aNotificationCallbacks); } \
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetNotificationCallbacks(aNotificationCallbacks); } \
  NS_IMETHOD GetSecurityInfo(nsISupports * *aSecurityInfo) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetSecurityInfo(aSecurityInfo); } \
  NS_IMETHOD GetContentType(nsACString & aContentType) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentType(aContentType); } \
  NS_IMETHOD SetContentType(const nsACString & aContentType) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentType(aContentType); } \
  NS_IMETHOD GetContentCharset(nsACString & aContentCharset) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentCharset(aContentCharset); } \
  NS_IMETHOD SetContentCharset(const nsACString & aContentCharset) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentCharset(aContentCharset); } \
  NS_IMETHOD GetContentLength(PRInt32 *aContentLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetContentLength(aContentLength); } \
  NS_IMETHOD SetContentLength(PRInt32 aContentLength) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetContentLength(aContentLength); } \
  NS_IMETHOD Open(nsIInputStream **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Open(_retval); } \
  NS_IMETHOD AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext) { return !_to ? NS_ERROR_NULL_POINTER : _to->AsyncOpen(aListener, aContext); } \

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsChannel : public nsIChannel
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHANNEL

  nsChannel();

private:
  ~nsChannel();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsChannel, nsIChannel)

nsChannel::nsChannel()
{
  /* member initializers and constructor code */
}

nsChannel::~nsChannel()
{
  /* destructor code */
}

/* attribute nsIURI originalURI; */
NS_IMETHODIMP nsChannel::GetOriginalURI(nsIURI * *aOriginalURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannel::SetOriginalURI(nsIURI * aOriginalURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIURI URI; */
NS_IMETHODIMP nsChannel::GetURI(nsIURI * *aURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISupports owner; */
NS_IMETHODIMP nsChannel::GetOwner(nsISupports * *aOwner)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannel::SetOwner(nsISupports * aOwner)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIInterfaceRequestor notificationCallbacks; */
NS_IMETHODIMP nsChannel::GetNotificationCallbacks(nsIInterfaceRequestor * *aNotificationCallbacks)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannel::SetNotificationCallbacks(nsIInterfaceRequestor * aNotificationCallbacks)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsISupports securityInfo; */
NS_IMETHODIMP nsChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute ACString contentType; */
NS_IMETHODIMP nsChannel::GetContentType(nsACString & aContentType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannel::SetContentType(const nsACString & aContentType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute ACString contentCharset; */
NS_IMETHODIMP nsChannel::GetContentCharset(nsACString & aContentCharset)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannel::SetContentCharset(const nsACString & aContentCharset)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long contentLength; */
NS_IMETHODIMP nsChannel::GetContentLength(PRInt32 *aContentLength)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsChannel::SetContentLength(PRInt32 aContentLength)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIInputStream open (); */
NS_IMETHODIMP nsChannel::Open(nsIInputStream **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void asyncOpen (in nsIStreamListener aListener, in nsISupports aContext); */
NS_IMETHODIMP nsChannel::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIChannel_h__ */
