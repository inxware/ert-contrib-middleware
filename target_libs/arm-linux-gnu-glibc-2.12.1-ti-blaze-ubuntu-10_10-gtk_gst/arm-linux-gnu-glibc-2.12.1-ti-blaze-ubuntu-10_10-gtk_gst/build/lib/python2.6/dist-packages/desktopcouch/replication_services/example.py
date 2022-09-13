# Note that the __init__.py of this package must import this module for it to
# be found.  Plugin logic is not pretty, and not implemented yet.

# Required
name = "Example"
# Required; should include the words "cloud service" on the end.
description = "Example cloud service"

# Required
def is_active():
    """Can we deliver information?"""
    return False

# Required
def oauth_data():
    """OAuth information needed to replicate to a server.  This may be
    called from a subthread, so be sure not to violate any execution 
    idioms, like updating the GUI from a non-main thread."""
    return dict(consumer_key="", consumer_secret="", oauth_token="",
            oauth_token_secret="")
    # or to symbolize failure
    return None

# Access to this as a string fires off functions.
# Required
db_name_prefix = "http://host.required.example.com/a_prefix_if_necessary"
# You can be sure that access to this will always, always be through its
# __str__ method.
