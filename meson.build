project('bfjit', ['c', 'cpp'], default_options : ['cpp_std=c++23', 'cpp_rtti=false'])

executable(
  'bfjit',
  'bfjit.cpp',
  dependencies : [
    dependency('fmt'),
    dependency('llvm'),
  ],
  install : true,
)
