# Relative path to the prefix where the compiler was installed.
COMPILER_PREFIX = ../compiler/prefix

# Prefix to use before all binutils, gcc and gdb commands.
# Use the following line to use the manually compiled version of binutils and gcc
# BINROOT = ${COMPILER_PREFIX}/bin/aarch64-elf-
# Use the following line when gcc and friends installed via apt
BINROOT = aarch64-linux-gnu-

# Name of the gdb command
# Use the following line to use the manually compiled version of gdb
GDB = ${BINROOT}gdb
# Use the following line to use gdb-multiarch
# GDB = gdb-multiarch

# Variable used to control the printing of commands.
# Printing is disabled by default (due to the "@").
# To enable command printing run "make Q= ..." instead of "make ...".
Q = @
