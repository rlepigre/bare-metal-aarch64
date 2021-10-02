# Relative path to the prefix where the compiler was installed.
COMPILER_PREFIX = ../compiler/prefix

# Prefix to use before all binutils, gcc and gdb commands.
BINROOT = ${COMPILER_PREFIX}/bin/aarch64-elf-

.PHONY: all
all:
	@${BINROOT}gcc --version | head -n 1
