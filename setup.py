# -*- coding: utf-8 -*-
#
# LICENCE       MIT
#
# DESCRIPTION   Setup for teng package.
#
# AUTHOR        Michal Bukovsky <burlog@seznam.cz>
#

import os, shlex, subprocess, sys
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


def pkg_config(*args):
    try:
        output = subprocess.check_output([
            'pkg-config', *args, 'libteng'
        ], text=True)
    except (OSError, subprocess.CalledProcessError):
        return []
    return shlex.split(output)


def append_unique(target, values):
    for value in values:
        if value not in target:
            target.append(value)


include_dirs = []
library_dirs = []
libraries = [boost_python]
extra_compile_args = ["-W",
                      "-Wall",
                      "-g",
                      "-ggdb",
                      "-O0",
                      "-Wextra",
                      "-Wconversion",
                      "-std=c++17"]
extra_link_args = []

for flag in pkg_config('--cflags'):
    if flag.startswith('-I'):
        append_unique(include_dirs, [flag[2:]])
    else:
        append_unique(extra_compile_args, [flag])

for flag in pkg_config('--static', '--libs'):
    if flag.startswith('-L'):
        append_unique(library_dirs, [flag[2:]])
    elif flag.startswith('-l'):
        append_unique(libraries, [flag[2:]])
    else:
        append_unique(extra_link_args, [flag])

setup(name="teng",
      version="4.0.8",
      description="Teng python module written on boost python.",
      author="Michal Bukovsky",
      author_email="burlog@seznam.cz",
      packages=["teng"],
      license="MIT",
      ext_modules=[Extension("rawteng",
                             ["teng/rawteng.cc"],
                             include_dirs=include_dirs,
                             library_dirs=library_dirs,
                             extra_compile_args=extra_compile_args,
                             extra_link_args=extra_link_args,
                             libraries=libraries)] )

