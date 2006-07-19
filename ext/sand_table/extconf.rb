require 'mkmf'

dir_config("sand_table")
have_library("c", "main")

create_makefile("sand_table")
