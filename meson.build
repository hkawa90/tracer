project('tracer', 'c', 'cpp')

# void subdir(dir_name, ...)
## Enters the specified subdirectory and executes the meson.build file in it.
subdir('src')

tracer_lib_dep = declare_dependency(link_with : tracer_lib)
tracer_cpp_lib_dep = declare_dependency(link_with : tracer_cpp_lib)
watcher_lib_dep = declare_dependency(link_with : watcher_lib)

subdir('test')
#subdir('example/c/')

