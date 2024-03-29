###############################################################################
# 
# configglue -- glue for your apps' configuration
# 
# A library for simple, DRY configuration of applications
# 
# (C) 2009--2010 by Canonical Ltd.
# originally by John R. Lenton <john.lenton@canonical.com>
# incorporating schemaconfig as configglue.pyschema
# schemaconfig originally by Ricardo Kirkner <ricardo.kirkner@canonical.com>
# 
# Released under the BSD License (see the file LICENSE)
# 
# For bug reports, support, and new releases: http://launchpad.net/configglue
# 
###############################################################################

import codecs
import collections
import copy
import os
import string

from ConfigParser import (NoSectionError, DEFAULTSECT,
    InterpolationMissingOptionError, NoOptionError,
    ConfigParser as BaseConfigParser)

from configglue.pyschema.options import DictConfigOption


CONFIG_FILE_ENCODING = 'utf-8'


class SchemaValidationError(Exception):
    pass


class SchemaConfigParser(BaseConfigParser, object):
    """A ConfigParser that validates against a Schema

    The way to use this class is:

    config = SchemaConfigParser(MySchema())
    config.read('mysystemconfig.cfg', 'mylocalconfig.cfg', ...)
    config.parse_all()
    ...
    profit!
    """
    def __init__(self, schema):
        super(SchemaConfigParser, self).__init__()
        # validate schema
        if not schema.is_valid():
            # TODO: add error details
            raise SchemaValidationError()
        self.schema = schema
        self._location = {}
        self.extra_sections = set()
        self._basedir = ''
        self._dirty = collections.defaultdict(
            lambda: collections.defaultdict(dict))

    def is_valid(self, report=False):
        valid = True
        errors = []
        try:
            # validate structure
            parsed_sections = set(self.sections())
            schema_sections = set(s.name for s in self.schema.sections())
            skip_sections = self.extra_sections
            if '__noschema__' in parsed_sections:
                skip_sections.add('__noschema__')
            schema_sections.update(skip_sections)
            sections_match = (parsed_sections == schema_sections)
            if not sections_match:
                error_msg = "Sections in configuration do not match schema: %s"
                unmatched_sections = list(parsed_sections - schema_sections)
                error_value = ', '.join(unmatched_sections)
                errors.append(error_msg % error_value)
            valid &= sections_match

            for name in parsed_sections:
                if name not in skip_sections:
                    if not self.schema.has_section(name):
                        # this should have been reported before
                        # so skip bogus section
                        continue

                    section = self.schema.section(name)
                    parsed_options = set(self.options(name))
                    schema_options = set(section.options())

                    fatal_options = set(opt.name for opt in schema_options
                                        if opt.fatal)
                    # all fatal options are included
                    fatal_included = parsed_options.issuperset(fatal_options)
                    if not fatal_included:
                        error_msg = ("Configuration missing required options"
                                     " for section '%s': %s")
                        error_value = ', '.join(list(fatal_options -
                                                     parsed_options))
                        errors.append(error_msg % (name, error_value))
                    valid &= fatal_included

                    # remaining parsed options are valid schema options
                    other_options = parsed_options - fatal_options
                    schema_opt_names = set(opt.name for opt in schema_options)

                    # add the default section special includes option
                    if name == '__main__':
                        schema_opt_names.add('includes')

                    schema_options = other_options.issubset(schema_opt_names)
                    if not schema_options:
                        error_msg = ("Configuration includes invalid options"
                                     " for section '%s': %s")
                        error_value = ', '.join(list(other_options -
                                                     schema_opt_names))
                        errors.append(error_msg % (name, error_value))
                    valid &= schema_options

            # structure validates, validate content
            self.parse_all()

        except Exception, e:
            if valid:
                errors.append(e)
                valid = False

        if report:
            return valid, errors
        else:
            return valid

    def items(self, section, raw=False, vars=None):
        """Return a list of tuples with (name, value) for each option
        in the section.

        All % interpolations are expanded in the return values, based on the
        defaults passed into the constructor, unless the optional argument
        `raw' is true.  Additional substitutions may be provided using the
        `vars' argument, which must be a dictionary whose contents overrides
        any pre-existing defaults.

        The section DEFAULT is special.
        """
        d = self._defaults.copy()
        try:
            d.update(self._sections[section])
        except KeyError:
            if section != DEFAULTSECT:
                raise NoSectionError(section)
        # Update with the entry specific variables
        if vars:
            for key, value in vars.items():
                d[self.optionxform(key)] = value
        options = d.keys()
        if "__name__" in options:
            options.remove("__name__")
        if raw:
            return [(option, d[option])
                    for option in options]
        else:
            items = []
            for option in options:
                try:
                    value = self._interpolate(section, option, d[option], d)
                except InterpolationMissingOptionError, e:
                    # interpolation failed, because key was not found in
                    # section. try other sections before bailing out
                    value = self._interpolate_value(section, option)
                    if value is None:
                        # this should be a string, so None indicates an error
                        raise e
                items.append((option, value))
            return items

    def values(self, section=None, parse=True):
        """Returns multiple values, in a dict.

        This method can return the value of multiple options in a single call,
        unlike get() that returns a single option's value.

        If section=None, return all options from all sections.
        If section is specified, return all options from that section only.

        Section is to be specified *by name*, not by
        passing in real ConfigSection objects.
        """
        values = collections.defaultdict(dict)
        if section is None:
            sections = self.schema.sections()
        else:
            sections = [self.schema.section(section)]

        for sect in sections:
            for opt in sect.options():
                values[sect.name][opt.name] = self.get(
                    sect.name, opt.name, parse=parse)
        if section is not None:
            return values[section]
        else:
            return values

    def read(self, filenames, already_read=None):
        """Like ConfigParser.read, but consider files we've already read."""
        if already_read is None:
            already_read = set()
        if isinstance(filenames, basestring):
            filenames = [filenames]
        read_ok = []
        for filename in filenames:
            path = os.path.join(self._basedir, filename)
            if path in already_read:
                continue
            try:
                fp = codecs.open(path, 'r', CONFIG_FILE_ENCODING)
            except IOError:
                continue
            self._read(fp, path, already_read=already_read)
            fp.close()
            read_ok.append(path)
        return read_ok

    def readfp(self, fp, filename=None):
        # wrap the StringIO so it can read encoded text
        decoded_fp = codecs.getreader(CONFIG_FILE_ENCODING)(fp)
        self._read(decoded_fp, filename)

    def _read(self, fp, fpname, already_read=None):
        # read file content
        self._update(fp, fpname)

        if already_read is None:
            already_read = set()
        already_read.add(fpname)

        if self.has_option('__main__', 'includes'):
            old_basedir, self._basedir = self._basedir, os.path.dirname(fpname)
            includes = self.get('__main__', 'includes')
            filenames = map(string.strip, includes)
            for filename in filenames:
                self.read(filename, already_read=already_read)
            self._basedir = old_basedir

            if filenames:
                # re-read the file to override included options with
                # local values
                fp.seek(0)
                self._update(fp, fpname)

    def _update(self, fp, fpname):
        # remember current values
        old_sections = copy.deepcopy(self._sections)
        # read in new file
        super(SchemaConfigParser, self)._read(fp, fpname)
        # update location of changed values
        self._update_location(old_sections, fpname)

    def _update_location(self, old_sections, filename):
        # keep list of valid options to include locations for
        option_names = map(lambda x: x.name, self.schema.options())

        # new values
        sections = self._sections

        # update locations
        for section, options in sections.items():
            old_section = old_sections.get(section)
            if old_section is not None:
                # update options in existing section
                for option, value in options.items():
                    valid_option = option in option_names
                    option_changed = (option not in old_section or
                                      value != old_section[option])
                    if valid_option and option_changed:
                        self._location[option] = filename
            else:
                # complete section is new
                for option, value in options.items():
                    valid_option = option in option_names
                    if valid_option:
                        self._location[option] = filename

    def parse(self, section, option, value):
        if section == '__main__':
            option_obj = getattr(self.schema, option, None)
        else:
            section_obj = getattr(self.schema, section, None)
            if section_obj is not None:
                option_obj = getattr(section_obj, option, None)
            else:
                raise NoSectionError(section)

        if option_obj is not None:
            kwargs = {}
            if option_obj.require_parser:
                kwargs = {'parser': self}

            # hook to save extra sections
            is_dict_option = isinstance(option_obj, DictConfigOption)
            is_dict_lines_option = (hasattr(option_obj, 'item') and
                isinstance(option_obj.item, DictConfigOption))
            is_default_value = unicode(option_obj.default) == value

            # avoid adding implicit sections for dict default value
            if ((is_dict_option or is_dict_lines_option) and
                not is_default_value):
                sections = value.split()
                self.extra_sections.update(set(sections))

                if is_dict_option:
                    base = option_obj
                else:
                    base = option_obj.item

                for name in sections:
                    nested = base.get_extra_sections(name, self)
                    self.extra_sections.update(set(nested))

            if is_default_value:
                value = option_obj.default
            else:
                try:
                    value = option_obj.parse(value, **kwargs)
                except ValueError, e:
                    raise ValueError("Invalid value '%s' for %s '%s' in"
                        " section '%s'. Original exception was: %s" %
                        (value, option_obj.__class__.__name__, option,
                         section, e))
        return value

    def parse_all(self):
        """Go through all sections and options attempting to parse each one.

        If any options are omitted from the config file, provide the
        default value from the schema.  If the option has fatal=True, raise
        an exception.
        """
        for section in self.schema.sections():
            for option in section.options():
                try:
                    self.get(section.name, option.name, raw=option.raw)
                except (NoSectionError, NoOptionError):
                    if option.fatal:
                        raise

    def locate(self, option=None):
        return self._location.get(option)

    def _get_interpolation_keys(self, section, option):
        def extract_keys(item):
            if isinstance(item, (list, tuple)):
                keys = map(extract_keys, item)
                keys = reduce(set.union, keys, set())
            else:
                keys = set(self._KEYCRE.findall(item))
            # remove invalid key
            if '' in keys:
                keys.remove('')
            return keys

        rawval = super(SchemaConfigParser, self).get(section, option, True)
        try:
            opt = self.schema.section(section).option(option)
            value = opt.parse(rawval, raw=True)
        except:
            value = rawval

        keys = extract_keys(value)
        return rawval, keys

    def _interpolate_value(self, section, option):
        rawval, keys = self._get_interpolation_keys(section, option)
        if not keys:
            # interpolation keys are not valid
            return

        values = {}
        # iterate over the other sections
        for key in keys:
            # we want the unparsed value
            try:
                value = self.get(section, key, parse=False)
            except (NoSectionError, NoOptionError):
                # value of key not found in config, so try in special
                # sections
                for section in ('__main__', '__noschema__'):
                    try:
                        value = super(SchemaConfigParser, self).get(section,
                                                                    key)
                        break
                    except:
                        continue
                else:
                    return
            values[key] = value

        # replace holders with values
        result = rawval % values

        assert isinstance(result, basestring)
        return result

    def _get_default(self, section, option):
        # mark the value as not initialized to be able to have a None default
        marker = object()
        value = marker

        # cater for 'special' sections
        if section == '__main__':
            opt = getattr(self.schema, option, None)
            if opt is not None and not opt.fatal:
                value = opt.default
        elif section == '__noschema__':
            value = super(SchemaConfigParser, self).get(section, option)
        else:
            try:
                opt = self.schema.section(section).option(option)
                if not opt.fatal:
                    value = opt.default
            except Exception:
                pass

        if value is marker:
            # value was not set, so either section or option was not found
            # or option was required (fatal set to True)
            #if self.schema.has_section(section):
            #    raise NoOptionError(option, section)
            #else:
            #    raise NoSectionError(section)
            return None
        else:
            # we want to return a non-parsed value
            # a unicode of the value is the closest we can get
            return unicode(value)

    def get(self, section, option, raw=False, vars=None, parse=True):
        try:
            # get option's raw mode setting
            try:
                section_obj = self.schema.section(section)
                option_obj = section_obj.option(option)
                raw = option_obj.raw or raw
            except:
                pass
            # value is defined entirely in current section
            value = super(SchemaConfigParser, self).get(section, option,
                                                        raw, vars)
        except InterpolationMissingOptionError, e:
            # interpolation key not in same section
            value = self._interpolate_value(section, option)
            if value is None:
                # this should be a string, so None indicates an error
                raise e
        except (NoSectionError, NoOptionError), e:
            # option not found in config, try to get its default value from
            # schema
            value = self._get_default(section, option)
            if value is None:
                raise

            # value found, so section and option exist
            # add it to the config
            if not self.has_section(section):
                # Don't call .add_section here because 2.6 complains
                # about sections called '__main__'
                self._sections[section] = {}
            self.set(section, option, value)

        if parse:
            value = self.parse(section, option, value)
        return value

    def set(self, section, option, value):
        super(SchemaConfigParser, self).set(section, option, value)
        filename = self.locate(option)
        self._dirty[filename][section][option] = value

    def save(self, fp=None):
        if fp is not None:
            if isinstance(fp, basestring):
                fp = open(fp, 'w')
            # write to a specific file
            encoded_fp = codecs.getwriter(CONFIG_FILE_ENCODING)(fp)
            self.write(encoded_fp)
        else:
            # write to the original files
            for filename, sections in self._dirty.items():

                parser = BaseConfigParser()
                parser.read(filename)

                for section, options in sections.items():
                    for option, value in options.items():
                        parser.set(section, option, value)

                # write to new file
                parser.write(open("%s.new" % filename, 'w'))
                # rename old file
                if os.path.exists(filename):
                    os.rename(filename, "%s.old" % filename)
                # rename new file
                os.rename("%s.new" % filename, filename)

