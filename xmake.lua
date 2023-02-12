add_rules('mode.debug', 'mode.release')
set_languages('c23', 'c++17')
set_warnings('all', 'error')

add_defines('NDEBUG', '_GNU_SOURCE=1')

add_repositories("my-repo git@github.com:RunThem/My-xmake-repo.git")

add_requires('mpc', 'linenoise')

target('lisp', function()
  set_kind('binary')
  add_files('src/*.c')
  add_packages('mpc', 'linenoise')
end)
