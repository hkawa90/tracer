cpp_compile_options = ['-g', '-Wall']
cpp_link_options = ['-L.']

cc = meson.get_compiler('c')

lib_gtest_main = cc.find_library('gtest_main', required: true)
lib_gtest = cc.find_library('gtest', required: true)
lib_pthread = cc.find_library('pthread', required: true)

test_ringbuffer_exe = executable('test_ringbuffer', 
                                 'test_ringbuffer.cpp', 
                                 include_directories: includes,
                                 dependencies: [tracer_cpp_lib_dep, lib_gtest_main, lib_gtest, lib_pthread], 
                                 cpp_args: cpp_compile_options, 
                                 link_args: cpp_link_options)
test('ringbuffer test', test_ringbuffer_exe)

test_finstrument_exe = executable('test_finstrument', 
                                  'test_finstrument.cpp',
                                  include_directories: includes,
                                  dependencies: [tracer_cpp_lib_dep, lib_gtest_main, lib_gtest, lib_pthread], 
                                  cpp_args: cpp_compile_options, 
                                  link_args: cpp_link_options)
test('finstrument test', test_ringbuffer_exe)

