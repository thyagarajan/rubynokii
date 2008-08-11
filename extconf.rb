require 'mkmf'
abort 'need ruby.h' unless have_header("ruby.h")
dir_config('rubynokii')
have_library('gnokii')
create_makefile('rubynokii')
