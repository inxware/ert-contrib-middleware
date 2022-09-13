
# Just use the SHA module from the Python standard library

__revision__ = "$Id: SHA.py,v 1.4 2002/07/11 14:31:19 akuchling Exp $"

__all__ = ['new', 'digest_size']

try:
    # The md5 module is deprecated in Python 2.6, so use hashlib when possible.
    import hashlib
    def new(data=""):
        return hashlib.sha1(data)
    digest_size = new().digest_size

except ImportError:
    from sha import *
    import sha
    if hasattr(sha, 'digestsize'):
        digest_size = digestsize
        del digestsize
    del sha
