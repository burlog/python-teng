# -*- coding: utf-8 -*-
#
# LICENCE       MIT
#
# DESCRIPTION   Teng module.
#
# AUTHOR        Michal Bukovsky <burlog@seznam.cz>
#

from collections.abc import Iterable, Mapping

from rawteng import Teng
from rawteng import listSupportedContentTypes
from rawteng import registerUdf
from rawteng import Fragment
from rawteng import Error
from rawteng import FileWriter
from rawteng import StringWriter

class IOWriter(object):
    def __init__(self, io):
        self.io = io

    def write(self, data):
        self.io.write(data)
        return 0

    def write_slice(self, data, f, t):
        self.io.write(data[f:t])
        return 0

    def flush(self):
        return self.io.flush()

    def dump(self):
        return ""

def _add_value(frag, name, value):
    if isinstance(value, Mapping):
        return _add_fragment(frag, name, value)
    elif isinstance(value, ("".__class__, u"".__class__)):
        return _add_variable(frag, name, value)
    elif isinstance(value, Iterable):
        return _add_fragments(frag, name, value)
    return _add_variable(frag, name, value)

def _add_fragment(parent, mapping_key, mapping):
    frag = parent._addFragment(mapping_key)
    for name, value in mapping.items():
        _add_value(frag, name, value)
    return frag

def _add_fragments(parent, name, iterable):
    for entry in iterable:
        if not isinstance(entry, Mapping):
            raise ValueError("Non mapping object for name '%s'" % name)
        _add_fragment(parent, name, entry)

def _add_variable(frag, name, value):
    frag.addVariable(name, value)

def _fragment_add_fragment(self, name, data):
    if not isinstance(data, Mapping):
        raise ValueError("The data object must be mapping")
    return _add_fragment(self, name, data)

Fragment.addFragment = _fragment_add_fragment
Fragment.addFragment.__doc__ = """
Add sub fragment to this fragment.

:param name: Name new fragment.
:param data: Fragment's variables and subfragmens.

:return: Created sub fragment (type Fragment).
"""

Fragment.addVariable.__doc__ = """
Add variable to this fragment.

:param name:  Name of variable.
:param value: Value of variable.
"""

def _make_error_entry(error_entry):
    return {
        "level": error_entry.level,
        "filename": error_entry.filename,
        "line": error_entry.line,
        "column": error_entry.column,
        "message": error_entry.message,
    }

def _create_data_root(self, data, encoding=None):
    if encoding and encoding.lower() != "utf-8":
        raise NotImplementedError("UTF-8 encoding is supported only")
    frag = self._createDataRoot()
    if isinstance(data, Mapping):
        for name, value in data.items():
            _add_value(frag, name, value)
        return frag
    elif isinstance(data, Iterable):
        for mapping in data:
            if not isinstance(mapping, Mapping):
                raise ValueError("The root iterable object must contain "
                                 "solely mapping objects")
            for name, value in mapping.items():
                _add_value(frag, name, value)
        return frag
    raise ValueError("The data param must be mapping object "
                     "or an iterable object of mapping objects")

def _generate_page(self, templateFilename="", skin="",
                   templateString="", dataDefinitionFilename="",
                   dictionaryFilename="", language="",
                   configFilename="", contentType="", data=None,
                   outputFilename="", outputFile="", encoding="utf-8"):
    # polish input
    if templateFilename and templateString:
        raise AttributeError("You can't supply both templateFilename "
                             "and templateString at the same time")
    if not templateFilename and not templateString:
        raise AttributeError("Supply one of templateFilename or "
                             "non empty templateString param")
    if language and not dictionaryFilename:
        raise AttributeError("Language param must go with "
                             "dictionaryFilename")
    if outputFilename and outputFile:
        raise AttributeError("You can't supply both outputFilename "
                             "and outputFile at the same time")
    if encoding and encoding.lower() != "utf-8":
        raise NotImplementedError("UTF-8 encoding is supported only")

    # create root if need
    if not isinstance(data, Fragment):
        data = self.createDataRoot(data)

    # prepare output and error gathering objects
    errors = Error()
    if outputFile: writer = IOWriter(outputFile)
    elif outputFilename: writer = FileWriter(outputFilename)
    else: writer = StringWriter()

    # generate page
    if templateFilename:
        status = self._generatePage(templateFilename, skin, dictionaryFilename,
                                    language, configFilename,
                                    contentType or self.defaultContentType,
                                    encoding, data, writer, errors)
    else:
        status = self._generatePage(templateString, dictionaryFilename,
                                    language, configFilename,
                                    contentType or self.defaultContentType,
                                    encoding, data, writer, errors)

    # just return
    return {
        "status": status,
        "output": writer.dump(),
        "errorLog": [_make_error_entry(e) for e in errors.entries()]
    }

def _dictionary_lookup(self, dictionaryFilename, language, key,
                       configFilename=""):
    return self._dictionaryLookup(configFilename, dictionaryFilename,
                                  language, key)

Teng.createDataRoot = _create_data_root
Teng.createDataRoot.__doc__ = """
Create root fragment.

:param data:      Root fragment's variables and subfragmens.
:param encoding:  Encoding for unicode conversions.
                  If ommitted use Teng's default.

:return: Created fragment (type Fragment).
"""

Teng.generatePage = _generate_page
Teng.generatePage.__doc__ = """
Generate page from template, dictionaries and data.

= 1. source is =

:param templateFilename:   Path to template.
:param skin:               Skin -- appended after last dot of filename.
                           (..../x.html + std -> ..../x.std.html)
                           Accepted only together with templateFilename.
or:
:param templateString:     Template.

= 2. output is =

:param outputFilename:     Path to output file.
or:
:param outputFile:         File object (must be open for writing)
                           or file-like object (must have write() method.
or:
Returned as item in result sucture (see below)

= 3. dictionaries are =

:param dictionaryFilename: File with language dictionary.
:param language:           Language variation.
                           Appended after last dot of filename.
                           (..../x.dict en -> ..../x.en.dict)
:param configFilename:     File with configuration.

= 4. and other arguments are =

:param contentType:        Content-type of template.
                           Use teng.listSupportedContentTypes()
                           for accepted values.
:param encoding:           Encoding of data. Used for conversion
                           of Unicode object to 8bit strings.

=5. Data are=
dict, list or Fragment data  Hierarchical data (see documentation).

Or above measn exclusive or! 'templateFilename' and 'templateString'
cannot be specified together. Arguments'outputFilename' and 'outputFile'
collide too. When no output specified result will be stored in
result structure.

:return: dict {
    string output         Result of page generation when no output specified.
    int status            Indicates error/no error during generation.
    tuple errorLog        Error log.
}
"""

Teng.dictionaryLookup = _dictionary_lookup
Teng.dictionaryLookup.__doc__ = """
Finds item in dictionary.

:param dictionaryFilename:  File with language dictionary.
:param language:            Language variation.
                            Appended after last dot of filename.
                            (..../x.dict en -> ..../x.en.dict)
:param key:                 The item name.
:param configFilename:      File with configuration.

:return: Value for given key or None.
"""

Teng.__doc__ = """
Create new teng engine

:param root:                Root path for relative paths.
:param encoding:            Default encoding for generatePage().
:param contentType:         Defautl contentType for generatePage().
:param templateCacheSize:   Specifies max number of templates in the cache.
:param dictionaryCacheSize: Specifies max number of dictionaries in the cache.
"""

listSupportedContentTypes.__doc__ = """
List content types supported by this engine.

Returns tuple of two-item tuples with name of content type and
comment on the content type.
"""

registerUdf.__doc__ = """
Registers user-defined function.

:param functionName: Name of the function in udf. namespace.
:param handler:      Callable object.

:return: True if function was registered.
"""

