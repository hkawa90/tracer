includes = include_directories('.')

tracer_srcs = files('logger.c', 'ringbuffer.c', 'finstrument.c', 'bt.c')
watchalloc_srcs = files('watchalloc.c', 'base_va.c')

c_compile_options = ['-g', '-Wall']
cpp_compile_options = ['-g', '-Wall', '-x cpp']

c_link_options = ['-L.']

cc = meson.get_compiler('c')

lib_pthread = cc.find_library('pthread', required: true)
lib_backtrace = cc.find_library('backtrace', required: true)
lib_dl = cc.find_library('dl', required: true)
lib_confuse = cc.find_library('confuse', required: true)
lib_uuid = cc.find_library('uuid', required: true)

external_lib_deps = [lib_pthread, lib_backtrace, lib_dl, lib_confuse, lib_uuid]
  
tracer_lib = shared_library(
  'tracer',
  tracer_srcs,
  include_directories: includes,
  c_args: c_compile_options,
  link_args: c_link_options,
  dependencies: external_lib_deps,
  install: false,
)

tracer_cpp_lib = shared_library(
  'tracer++',
  tracer_srcs,
  include_directories: includes,
  cpp_args: cpp_compile_options,
  link_args: c_link_options,
  dependencies: external_lib_deps,
  install: false,
)

watcher_lib= shared_library(
  'watcher',
  watchalloc_srcs,
  include_directories: includes,
  c_args: c_compile_options,
  link_args: c_link_options,
  dependencies: external_lib_deps,
  install: false,
)
