project(
  'szn-python3-teng',
  'cpp',
  version : '4.0.8',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'buildtype=debugoptimized',
    'wrap_mode=nofallback'
  ]
)

py3 = import('python').find_installation('python3')

ext_dependencies = [
  dependency('boost', modules: ['python3']),
  dependency('libteng', version: '>= 5.0.8')
]

pyext = py3.extension_module(
  'rawteng',
  sources: ['teng/rawteng.cc'],
  dependencies: ext_dependencies,
  install: true,
)

# Install the Python module files separately
py3.install_sources(
  ['teng/__init__.py'],
  subdir: 'teng'
)

