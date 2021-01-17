cpp_compile_options = ['-g', '-Wall']
cpp_link_options = ['-L.']

cc = meson.get_compiler('cpp')

lib_CppUTest = cc.find_library('CppUTest', required: true)
lib_CppUTestExt = cc.find_library('CppUTestExt', required: true)

test_ringbuffer_exe = executable('test_ringbuffer', 
                                 ['test_ringbuffer.cpp', 'main.cpp'], 
                                 include_directories: includes,
                                 dependencies: [tracer_cpp_lib_dep, lib_CppUTest, lib_CppUTestExt], 
                                 cpp_args: cpp_compile_options, 
                                 link_args: cpp_link_options)
test('ringbuffer test', test_ringbuffer_exe)

test_finstrument_exe = executable('test_finstrument', 
                                  ['test_finstrument.cpp', 'main.cpp'],
                                  include_directories: includes,
                                  dependencies: [tracer_cpp_lib_dep, lib_CppUTest, lib_CppUTestExt], 
                                  cpp_args: cpp_compile_options, 
                                  link_args: c_link_options)
test('finstrument test', test_ringbuffer_exe)

