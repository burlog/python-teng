# -*- coding: utf-8 -*-
#
# LICENCE       MIT
#
# DESCRIPTION   Setup for teng package.
#
# AUTHOR        Michal Bukovsky <burlog@seznam.cz>
#

import os, sys
from distutils.core import setup
from distutils.core import Extension

# detect python version
version = []
if hasattr(sys.version_info, 'major'):
    version.append(sys.version_info.major)
    version.append(sys.version_info.minor)
else:
    version = sys.version_info[0:2]

# detect boost_python library name
pattern = 'ld -o /dev/null --allow-shlib-undefined -lXXX > /dev/null 2>&1'
boost_python = 'boost_python-py%d%d' % (version[0], version[1])
if os.system(pattern.replace('XXX', boost_python)) != 0:
    boost_python = 'boost_python%d%d' % (version[0], version[1])
    if os.system(pattern.replace('XXX', boost_python)) != 0:
        boost_python = 'boost_python-%d.%d' % (version[0], version[1])
        if os.system(pattern.replace('XXX', boost_python)) != 0:
            print('can\'t find boost_python library')
            sys.exit(1)
print('checking boost_python library name: ' + boost_python)

setup(name="teng",
      version="4.0.8",
      description="Teng python module written on boost python.",
      author="Michal Bukovsky",
      author_email="burlog@seznam.cz",
      packages=["teng"],
      license="MIT",
      ext_modules=[Extension("rawteng",
                             ["teng/rawteng.cc"],
                             extra_compile_args=["-W",
                                                 "-Wall",
                                                 "-g",
                                                 "-ggdb",
                                                 "-O0",
                                                 "-Wextra",
                                                 "-Wconversion",
                                                 "-std=c++14"],
                             libraries=[boost_python, "teng"])])

