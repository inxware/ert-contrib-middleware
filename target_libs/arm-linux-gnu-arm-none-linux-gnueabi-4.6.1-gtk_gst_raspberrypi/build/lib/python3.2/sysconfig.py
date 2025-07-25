"""Provide access to Python's configuration information.

"""
import sys
import os
from os.path import pardir, realpath

__all__ = [
    'get_config_h_filename',
    'get_config_var',
    'get_config_vars',
    'get_makefile_filename',
    'get_path',
    'get_path_names',
    'get_paths',
    'get_platform',
    'get_python_version',
    'get_scheme_names',
    'parse_config_h',
    ]

_INSTALL_SCHEMES = {
    'posix_prefix': {
        'stdlib': '{base}/lib/python{py_version_short}',
        'platstdlib': '{platbase}/lib/python{py_version_short}',
        'purelib': '{base}/lib/python{py_version_short}/site-packages',
        'platlib': '{platbase}/lib/python{py_version_short}/site-packages',
        'include':
            '{base}/include/python{py_version_short}{abiflags}',
        'platinclude':
            '{platbase}/include/python{py_version_short}{abiflags}',
        'scripts': '{base}/bin',
        'data': '{base}',
        },
    'posix_local': {
        'stdlib': '{base}/local/lib/python{py_version_short}',
        'platstdlib': '{platbase}/local/lib/python{py_version_short}',
        'purelib': '{base}/local/lib/python{py_version_short}/dist-packages',
        'platlib': '{platbase}/local/lib/python{py_version_short}/dist-packages',
        'include': '{base}/local/include/python{py_version_short}',
        'platinclude': '{platbase}/local/include/python{py_version_short}',
        'scripts': '{base}/local/bin',
        'data': '{base}/local',
        },
    'deb_system': {
        'stdlib': '{base}/lib/python{py_version_short}',
        'platstdlib': '{platbase}/lib/python{py_version_short}',
        'purelib': '{base}/lib/python3/dist-packages',
        'platlib': '{platbase}/lib/python3/dist-packages',
        'include': '{base}/include/python{py_version_short}',
        'platinclude': '{platbase}/include/python{py_version_short}',
        'scripts': '{base}/bin',
        'data': '{base}',
        },
    'posix_home': {
        'stdlib': '{base}/lib/python',
        'platstdlib': '{base}/lib/python',
        'purelib': '{base}/lib/python',
        'platlib': '{base}/lib/python',
        'include': '{base}/include/python',
        'platinclude': '{base}/include/python',
        'scripts': '{base}/bin',
        'data'   : '{base}',
        },
    'nt': {
        'stdlib': '{base}/Lib',
        'platstdlib': '{base}/Lib',
        'purelib': '{base}/Lib/site-packages',
        'platlib': '{base}/Lib/site-packages',
        'include': '{base}/Include',
        'platinclude': '{base}/Include',
        'scripts': '{base}/Scripts',
        'data'   : '{base}',
        },
    'os2': {
        'stdlib': '{base}/Lib',
        'platstdlib': '{base}/Lib',
        'purelib': '{base}/Lib/site-packages',
        'platlib': '{base}/Lib/site-packages',
        'include': '{base}/Include',
        'platinclude': '{base}/Include',
        'scripts': '{base}/Scripts',
        'data'   : '{base}',
        },
    'os2_home': {
        'stdlib': '{userbase}/lib/python{py_version_short}',
        'platstdlib': '{userbase}/lib/python{py_version_short}',
        'purelib': '{userbase}/lib/python{py_version_short}/site-packages',
        'platlib': '{userbase}/lib/python{py_version_short}/site-packages',
        'include': '{userbase}/include/python{py_version_short}',
        'scripts': '{userbase}/bin',
        'data'   : '{userbase}',
        },
    'nt_user': {
        'stdlib': '{userbase}/Python{py_version_nodot}',
        'platstdlib': '{userbase}/Python{py_version_nodot}',
        'purelib': '{userbase}/Python{py_version_nodot}/site-packages',
        'platlib': '{userbase}/Python{py_version_nodot}/site-packages',
        'include': '{userbase}/Python{py_version_nodot}/Include',
        'scripts': '{userbase}/Scripts',
        'data'   : '{userbase}',
        },
    'posix_user': {
        'stdlib': '{userbase}/lib/python{py_version_short}',
        'platstdlib': '{userbase}/lib/python{py_version_short}',
        'purelib': '{userbase}/lib/python{py_version_short}/site-packages',
        'platlib': '{userbase}/lib/python{py_version_short}/site-packages',
        'include': '{userbase}/include/python{py_version_short}',
        'scripts': '{userbase}/bin',
        'data'   : '{userbase}',
        },
    'osx_framework_user': {
        'stdlib': '{userbase}/lib/python',
        'platstdlib': '{userbase}/lib/python',
        'purelib': '{userbase}/lib/python/site-packages',
        'platlib': '{userbase}/lib/python/site-packages',
        'include': '{userbase}/include',
        'scripts': '{userbase}/bin',
        'data'   : '{userbase}',
        },
    }

_SCHEME_KEYS = ('stdlib', 'platstdlib', 'purelib', 'platlib', 'include',
                'scripts', 'data')
_PY_VERSION = sys.version.split()[0]
_PY_VERSION_SHORT = sys.version[:3]
_PY_VERSION_SHORT_NO_DOT = _PY_VERSION[0] + _PY_VERSION[2]
_PREFIX = os.path.normpath(sys.prefix)
_EXEC_PREFIX = os.path.normpath(sys.exec_prefix)
_CONFIG_VARS = None
_USER_BASE = None

def _safe_realpath(path):
    try:
        return realpath(path)
    except OSError:
        return path

if sys.executable:
    _PROJECT_BASE = os.path.dirname(_safe_realpath(sys.executable))
else:
    # sys.executable can be empty if argv[0] has been changed and Python is
    # unable to retrieve the real program name
    _PROJECT_BASE = _safe_realpath(os.getcwd())

if os.name == "nt" and "pcbuild" in _PROJECT_BASE[-8:].lower():
    _PROJECT_BASE = _safe_realpath(os.path.join(_PROJECT_BASE, pardir))
# PC/VS7.1
if os.name == "nt" and "\\pc\\v" in _PROJECT_BASE[-10:].lower():
    _PROJECT_BASE = _safe_realpath(os.path.join(_PROJECT_BASE, pardir, pardir))
# PC/AMD64
if os.name == "nt" and "\\pcbuild\\amd64" in _PROJECT_BASE[-14:].lower():
    _PROJECT_BASE = _safe_realpath(os.path.join(_PROJECT_BASE, pardir, pardir))

def is_python_build():
    for fn in ("Setup.dist", "Setup.local"):
        if os.path.isfile(os.path.join(_PROJECT_BASE, "Modules", fn)):
            return True
    return False

_PYTHON_BUILD = is_python_build()

if _PYTHON_BUILD:
    for scheme in ('posix_prefix', 'posix_home'):
        _INSTALL_SCHEMES[scheme]['include'] = '{srcdir}/Include'
        _INSTALL_SCHEMES[scheme]['platinclude'] = '{projectbase}/.'

def _subst_vars(s, local_vars):
    try:
        return s.format(**local_vars)
    except KeyError:
        try:
            return s.format(**os.environ)
        except KeyError as var:
            raise AttributeError('{%s}' % var)

def _extend_dict(target_dict, other_dict):
    target_keys = target_dict.keys()
    for key, value in other_dict.items():
        if key in target_keys:
            continue
        target_dict[key] = value

def _expand_vars(scheme, vars):
    res = {}
    if vars is None:
        vars = {}
    _extend_dict(vars, get_config_vars())

    for key, value in _INSTALL_SCHEMES[scheme].items():
        if os.name in ('posix', 'nt'):
            value = os.path.expanduser(value)
        res[key] = os.path.normpath(_subst_vars(value, vars))
    return res

def _get_default_scheme():
    if os.name == 'posix':
        # the default scheme for posix is posix_prefix
        return 'posix_prefix'
    return os.name

def _getuserbase():
    env_base = os.environ.get("PYTHONUSERBASE", None)
    def joinuser(*args):
        return os.path.expanduser(os.path.join(*args))

    # what about 'os2emx', 'riscos' ?
    if os.name == "nt":
        base = os.environ.get("APPDATA") or "~"
        return env_base if env_base else joinuser(base, "Python")

    if sys.platform == "darwin":
        framework = get_config_var("PYTHONFRAMEWORK")
        if framework:
            return env_base if env_base else joinuser("~", "Library", framework, "%d.%d"%(
                sys.version_info[:2]))

    return env_base if env_base else joinuser("~", ".local")


def _parse_makefile(filename, vars=None):
    """Parse a Makefile-style file.

    A dictionary containing name/value pairs is returned.  If an
    optional dictionary is passed in as the second argument, it is
    used instead of a new dictionary.
    """
    import re
    # Regexes needed for parsing Makefile (and similar syntaxes,
    # like old-style Setup files).
    _variable_rx = re.compile("([a-zA-Z][a-zA-Z0-9_]+)\s*=\s*(.*)")
    _findvar1_rx = re.compile(r"\$\(([A-Za-z][A-Za-z0-9_]*)\)")
    _findvar2_rx = re.compile(r"\${([A-Za-z][A-Za-z0-9_]*)}")

    if vars is None:
        vars = {}
    done = {}
    notdone = {}

    with open(filename, errors="surrogateescape") as f:
        lines = f.readlines()

    for line in lines:
        if line.startswith('#') or line.strip() == '':
            continue
        m = _variable_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            v = v.strip()
            # `$$' is a literal `$' in make
            tmpv = v.replace('$$', '')

            if "$" in tmpv:
                notdone[n] = v
            else:
                try:
                    v = int(v)
                except ValueError:
                    # insert literal `$'
                    done[n] = v.replace('$$', '$')
                else:
                    done[n] = v

    # do variable interpolation here
    variables = list(notdone.keys())

    # Variables with a 'PY_' prefix in the makefile. These need to
    # be made available without that prefix through sysconfig.
    # Special care is needed to ensure that variable expansion works, even
    # if the expansion uses the name without a prefix.
    renamed_variables = ('CFLAGS', 'LDFLAGS', 'CPPFLAGS')

    while len(variables) > 0:
        for name in tuple(variables):
            value = notdone[name]
            m = _findvar1_rx.search(value) or _findvar2_rx.search(value)
            if m is not None:
                n = m.group(1)
                found = True
                if n in done:
                    item = str(done[n])
                elif n in notdone:
                    # get it on a subsequent round
                    found = False
                elif n in os.environ:
                    # do it like make: fall back to environment
                    item = os.environ[n]

                elif n in renamed_variables:
                    if name.startswith('PY_') and name[3:] in renamed_variables:
                        item = ""

                    elif 'PY_' + n in notdone:
                        found = False

                    else:
                        item = str(done['PY_' + n])

                else:
                    done[n] = item = ""

                if found:
                    after = value[m.end():]
                    value = value[:m.start()] + item + after
                    if "$" in after:
                        notdone[name] = value
                    else:
                        try:
                            value = int(value)
                        except ValueError:
                            done[name] = value.strip()
                        else:
                            done[name] = value
                        variables.remove(name)

                        if name.startswith('PY_') \
                        and name[3:] in renamed_variables:

                            name = name[3:]
                            if name not in done:
                                done[name] = value


            else:
                # bogus variable reference (e.g. "prefix=$/opt/python");
                # just drop it since we can't deal
                done[name] = value
                variables.remove(name)

    # strip spurious spaces
    for k, v in done.items():
        if isinstance(v, str):
            done[k] = v.strip()

    # save the results in the global dictionary
    vars.update(done)
    return vars


def get_makefile_filename():
    """Return the path of the Makefile."""
    if _PYTHON_BUILD:
        return os.path.join(_PROJECT_BASE, "Makefile")
    return os.path.join(get_path('stdlib'),
                        'config-{}{}'.format(_PY_VERSION_SHORT, sys.abiflags),
                        'Makefile')


def _init_posix(vars):
    """Initialize the module as appropriate for POSIX systems."""
    # load the installed Makefile:
    makefile = get_makefile_filename()
    try:
        _parse_makefile(makefile, vars)
    except IOError as e:
        msg = "invalid Python installation: unable to open %s" % makefile
        if hasattr(e, "strerror"):
            msg = msg + " (%s)" % e.strerror
        raise IOError(msg)
    # load the installed pyconfig.h:
    config_h = get_config_h_filename()
    try:
        with open(config_h) as f:
            parse_config_h(f, vars)
    except IOError as e:
        msg = "invalid Python installation: unable to open %s" % config_h
        if hasattr(e, "strerror"):
            msg = msg + " (%s)" % e.strerror
        raise IOError(msg)
    # On AIX, there are wrong paths to the linker scripts in the Makefile
    # -- these paths are relative to the Python source, but when installed
    # the scripts are in another directory.
    if _PYTHON_BUILD:
        vars['LDSHARED'] = vars['BLDSHARED']

def _init_non_posix(vars):
    """Initialize the module as appropriate for NT"""
    # set basic install directories
    vars['LIBDEST'] = get_path('stdlib')
    vars['BINLIBDEST'] = get_path('platstdlib')
    vars['INCLUDEPY'] = get_path('include')
    vars['SO'] = '.pyd'
    vars['EXE'] = '.exe'
    vars['VERSION'] = _PY_VERSION_SHORT_NO_DOT
    vars['BINDIR'] = os.path.dirname(_safe_realpath(sys.executable))

#
# public APIs
#


def parse_config_h(fp, vars=None):
    """Parse a config.h-style file.

    A dictionary containing name/value pairs is returned.  If an
    optional dictionary is passed in as the second argument, it is
    used instead of a new dictionary.
    """
    import re
    if vars is None:
        vars = {}
    define_rx = re.compile("#define ([A-Z][A-Za-z0-9_]+) (.*)\n")
    undef_rx = re.compile("/[*] #undef ([A-Z][A-Za-z0-9_]+) [*]/\n")

    while True:
        line = fp.readline()
        if not line:
            break
        m = define_rx.match(line)
        if m:
            n, v = m.group(1, 2)
            try: v = int(v)
            except ValueError: pass
            vars[n] = v
        else:
            m = undef_rx.match(line)
            if m:
                vars[m.group(1)] = 0
    return vars

def get_config_h_filename():
    """Return the path of pyconfig.h."""
    if _PYTHON_BUILD:
        if os.name == "nt":
            inc_dir = os.path.join(_PROJECT_BASE, "PC")
        else:
            inc_dir = _PROJECT_BASE
    else:
        inc_dir = get_path('platinclude')
    return os.path.join(inc_dir, 'pyconfig.h')

def get_scheme_names():
    """Return a tuple containing the schemes names."""
    schemes = list(_INSTALL_SCHEMES.keys())
    schemes.sort()
    return tuple(schemes)

def get_path_names():
    """Return a tuple containing the paths names."""
    return _SCHEME_KEYS

def get_paths(scheme=_get_default_scheme(), vars=None, expand=True):
    """Return a mapping containing an install scheme.

    ``scheme`` is the install scheme name. If not provided, it will
    return the default scheme for the current platform.
    """
    if expand:
        return _expand_vars(scheme, vars)
    else:
        return _INSTALL_SCHEMES[scheme]

def get_path(name, scheme=_get_default_scheme(), vars=None, expand=True):
    """Return a path corresponding to the scheme.

    ``scheme`` is the install scheme name.
    """
    return get_paths(scheme, vars, expand)[name]

def get_config_vars(*args):
    """With no arguments, return a dictionary of all configuration
    variables relevant for the current platform.

    On Unix, this means every variable defined in Python's installed Makefile;
    On Windows and Mac OS it's a much smaller set.

    With arguments, return a list of values that result from looking up
    each argument in the configuration variable dictionary.
    """
    import re
    global _CONFIG_VARS
    if _CONFIG_VARS is None:
        _CONFIG_VARS = {}
        # Normalized versions of prefix and exec_prefix are handy to have;
        # in fact, these are the standard versions used most places in the
        # Distutils.
        _CONFIG_VARS['prefix'] = _PREFIX
        _CONFIG_VARS['exec_prefix'] = _EXEC_PREFIX
        _CONFIG_VARS['py_version'] = _PY_VERSION
        _CONFIG_VARS['py_version_short'] = _PY_VERSION_SHORT
        _CONFIG_VARS['py_version_nodot'] = _PY_VERSION[0] + _PY_VERSION[2]
        _CONFIG_VARS['base'] = _PREFIX
        _CONFIG_VARS['platbase'] = _EXEC_PREFIX
        _CONFIG_VARS['projectbase'] = _PROJECT_BASE
        try:
            _CONFIG_VARS['abiflags'] = sys.abiflags
        except AttributeError:
            # sys.abiflags may not be defined on all platforms.
            _CONFIG_VARS['abiflags'] = ''

        if os.name in ('nt', 'os2'):
            _init_non_posix(_CONFIG_VARS)
        if os.name == 'posix':
            _init_posix(_CONFIG_VARS)
        # Setting 'userbase' is done below the call to the
        # init function to enable using 'get_config_var' in
        # the init-function.
        _CONFIG_VARS['userbase'] = _getuserbase()

        if 'srcdir' not in _CONFIG_VARS:
            _CONFIG_VARS['srcdir'] = _PROJECT_BASE
        else:
            _CONFIG_VARS['srcdir'] = _safe_realpath(_CONFIG_VARS['srcdir'])


        # Convert srcdir into an absolute path if it appears necessary.
        # Normally it is relative to the build directory.  However, during
        # testing, for example, we might be running a non-installed python
        # from a different directory.
        if _PYTHON_BUILD and os.name == "posix":
            base = _PROJECT_BASE
            try:
                cwd = os.getcwd()
            except OSError:
                cwd = None
            if (not os.path.isabs(_CONFIG_VARS['srcdir']) and
                base != cwd):
                # srcdir is relative and we are not in the same directory
                # as the executable. Assume executable is in the build
                # directory and make srcdir absolute.
                srcdir = os.path.join(base, _CONFIG_VARS['srcdir'])
                _CONFIG_VARS['srcdir'] = os.path.normpath(srcdir)

        if sys.platform == 'darwin':
            kernel_version = os.uname()[2] # Kernel version (8.4.3)
            major_version = int(kernel_version.split('.')[0])

            if major_version < 8:
                # On Mac OS X before 10.4, check if -arch and -isysroot
                # are in CFLAGS or LDFLAGS and remove them if they are.
                # This is needed when building extensions on a 10.3 system
                # using a universal build of python.
                for key in ('LDFLAGS', 'BASECFLAGS',
                        # a number of derived variables. These need to be
                        # patched up as well.
                        'CFLAGS', 'PY_CFLAGS', 'BLDSHARED'):
                    flags = _CONFIG_VARS[key]
                    flags = re.sub('-arch\s+\w+\s', ' ', flags)
                    flags = re.sub('-isysroot [^ \t]*', ' ', flags)
                    _CONFIG_VARS[key] = flags
            else:
                # Allow the user to override the architecture flags using
                # an environment variable.
                # NOTE: This name was introduced by Apple in OSX 10.5 and
                # is used by several scripting languages distributed with
                # that OS release.
                if 'ARCHFLAGS' in os.environ:
                    arch = os.environ['ARCHFLAGS']
                    for key in ('LDFLAGS', 'BASECFLAGS',
                        # a number of derived variables. These need to be
                        # patched up as well.
                        'CFLAGS', 'PY_CFLAGS', 'BLDSHARED'):

                        flags = _CONFIG_VARS[key]
                        flags = re.sub('-arch\s+\w+\s', ' ', flags)
                        flags = flags + ' ' + arch
                        _CONFIG_VARS[key] = flags

                # If we're on OSX 10.5 or later and the user tries to
                # compiles an extension using an SDK that is not present
                # on the current machine it is better to not use an SDK
                # than to fail.
                #
                # The major usecase for this is users using a Python.org
                # binary installer  on OSX 10.6: that installer uses
                # the 10.4u SDK, but that SDK is not installed by default
                # when you install Xcode.
                #
                CFLAGS = _CONFIG_VARS.get('CFLAGS', '')
                m = re.search('-isysroot\s+(\S+)', CFLAGS)
                if m is not None:
                    sdk = m.group(1)
                    if not os.path.exists(sdk):
                        for key in ('LDFLAGS', 'BASECFLAGS',
                             # a number of derived variables. These need to be
                             # patched up as well.
                            'CFLAGS', 'PY_CFLAGS', 'BLDSHARED'):

                            flags = _CONFIG_VARS[key]
                            flags = re.sub('-isysroot\s+\S+(\s|$)', ' ', flags)
                            _CONFIG_VARS[key] = flags

    if args:
        vals = []
        for name in args:
            vals.append(_CONFIG_VARS.get(name))
        return vals
    else:
        return _CONFIG_VARS

def get_config_var(name):
    """Return the value of a single variable using the dictionary returned by
    'get_config_vars()'.

    Equivalent to get_config_vars().get(name)
    """
    return get_config_vars().get(name)

def get_platform():
    """Return a string that identifies the current platform.

    This is used mainly to distinguish platform-specific build directories and
    platform-specific built distributions.  Typically includes the OS name
    and version and the architecture (as supplied by 'os.uname()'),
    although the exact information included depends on the OS; eg. for IRIX
    the architecture isn't particularly important (IRIX only runs on SGI
    hardware), but for Linux the kernel version isn't particularly
    important.

    Examples of returned values:
       linux-i586
       linux-alpha (?)
       solaris-2.6-sun4u
       irix-5.3
       irix64-6.2

    Windows will return one of:
       win-amd64 (64bit Windows on AMD64 (aka x86_64, Intel64, EM64T, etc)
       win-ia64 (64bit Windows on Itanium)
       win32 (all others - specifically, sys.platform is returned)

    For other non-POSIX platforms, currently just returns 'sys.platform'.
    """
    import re
    if os.name == 'nt':
        # sniff sys.version for architecture.
        prefix = " bit ("
        i = sys.version.find(prefix)
        if i == -1:
            return sys.platform
        j = sys.version.find(")", i)
        look = sys.version[i+len(prefix):j].lower()
        if look == 'amd64':
            return 'win-amd64'
        if look == 'itanium':
            return 'win-ia64'
        return sys.platform

    if os.name != "posix" or not hasattr(os, 'uname'):
        # XXX what about the architecture? NT is Intel or Alpha,
        # Mac OS is M68k or PPC, etc.
        return sys.platform

    # Try to distinguish various flavours of Unix
    osname, host, release, version, machine = os.uname()

    # Convert the OS name to lowercase, remove '/' characters
    # (to accommodate BSD/OS), and translate spaces (for "Power Macintosh")
    osname = osname.lower().replace('/', '')
    machine = machine.replace(' ', '_')
    machine = machine.replace('/', '-')

    if osname[:5] == "linux":
        # At least on Linux/Intel, 'machine' is the processor --
        # i386, etc.
        # XXX what about Alpha, SPARC, etc?
        return  "%s-%s" % (osname, machine)
    elif osname[:5] == "sunos":
        if release[0] >= "5":           # SunOS 5 == Solaris 2
            osname = "solaris"
            release = "%d.%s" % (int(release[0]) - 3, release[2:])
            # We can't use "platform.architecture()[0]" because a
            # bootstrap problem. We use a dict to get an error
            # if some suspicious happens.
            bitness = {2147483647:"32bit", 9223372036854775807:"64bit"}
            machine += ".%s" % bitness[sys.maxsize]
        # fall through to standard osname-release-machine representation
    elif osname[:4] == "irix":              # could be "irix64"!
        return "%s-%s" % (osname, release)
    elif osname[:3] == "aix":
        return "%s-%s.%s" % (osname, version, release)
    elif osname[:6] == "cygwin":
        osname = "cygwin"
        rel_re = re.compile (r'[\d.]+')
        m = rel_re.match(release)
        if m:
            release = m.group()
    elif osname[:6] == "darwin":
        #
        # For our purposes, we'll assume that the system version from
        # distutils' perspective is what MACOSX_DEPLOYMENT_TARGET is set
        # to. This makes the compatibility story a bit more sane because the
        # machine is going to compile and link as if it were
        # MACOSX_DEPLOYMENT_TARGET.
        #
        cfgvars = get_config_vars()
        macver = cfgvars.get('MACOSX_DEPLOYMENT_TARGET')

        if 1:
            # Always calculate the release of the running machine,
            # needed to determine if we can build fat binaries or not.

            macrelease = macver
            # Get the system version. Reading this plist is a documented
            # way to get the system version (see the documentation for
            # the Gestalt Manager)
            try:
                f = open('/System/Library/CoreServices/SystemVersion.plist')
            except IOError:
                # We're on a plain darwin box, fall back to the default
                # behaviour.
                pass
            else:
                try:
                    m = re.search(
                            r'<key>ProductUserVisibleVersion</key>\s*' +
                            r'<string>(.*?)</string>', f.read())
                    if m is not None:
                        macrelease = '.'.join(m.group(1).split('.')[:2])
                    # else: fall back to the default behaviour
                finally:
                    f.close()

        if not macver:
            macver = macrelease

        if macver:
            release = macver
            osname = "macosx"

            if (macrelease + '.') >= '10.4.' and \
                    '-arch' in get_config_vars().get('CFLAGS', '').strip():
                # The universal build will build fat binaries, but not on
                # systems before 10.4
                #
                # Try to detect 4-way universal builds, those have machine-type
                # 'universal' instead of 'fat'.

                machine = 'fat'
                cflags = get_config_vars().get('CFLAGS')

                archs = re.findall('-arch\s+(\S+)', cflags)
                archs = tuple(sorted(set(archs)))

                if len(archs) == 1:
                    machine = archs[0]
                elif archs == ('i386', 'ppc'):
                    machine = 'fat'
                elif archs == ('i386', 'x86_64'):
                    machine = 'intel'
                elif archs == ('i386', 'ppc', 'x86_64'):
                    machine = 'fat3'
                elif archs == ('ppc64', 'x86_64'):
                    machine = 'fat64'
                elif archs == ('i386', 'ppc', 'ppc64', 'x86_64'):
                    machine = 'universal'
                else:
                    raise ValueError(
                       "Don't know machine value for archs=%r"%(archs,))

            elif machine == 'i386':
                # On OSX the machine type returned by uname is always the
                # 32-bit variant, even if the executable architecture is
                # the 64-bit variant
                if sys.maxsize >= 2**32:
                    machine = 'x86_64'

            elif machine in ('PowerPC', 'Power_Macintosh'):
                # Pick a sane name for the PPC architecture.
                # See 'i386' case
                if sys.maxsize >= 2**32:
                    machine = 'ppc64'
                else:
                    machine = 'ppc'

    return "%s-%s-%s" % (osname, release, machine)


def get_python_version():
    return _PY_VERSION_SHORT

def _print_dict(title, data):
    for index, (key, value) in enumerate(sorted(data.items())):
        if index == 0:
            print('{0}: '.format(title))
        print('\t{0} = "{1}"'.format(key, value))

def _main():
    """Display all information sysconfig detains."""
    print('Platform: "{0}"'.format(get_platform()))
    print('Python version: "{0}"'.format(get_python_version()))
    print('Current installation scheme: "{0}"'.format(_get_default_scheme()))
    print('')
    _print_dict('Paths', get_paths())
    print('')
    _print_dict('Variables', get_config_vars())

if __name__ == '__main__':
    _main()
