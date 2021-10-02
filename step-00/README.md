Step 0: installing a cross-compiler
===================================

Before we can compile our project, we must first install a cross-compiler. The
instructions given here will help you install the relevent versions of the GCC
compiler and Binutils. They are based on the instructions found on the "osdev"
wiki, which can be found [here](https://wiki.osdev.org/GCC_Cross-Compiler). We
will also install a corresponding version of GDB, that will come in handy when
we start running our code using QEMU.

**Note:** if you plan to compile and run the code that is provided in the next
steps from this repository, then you must need to follow the instruction given
below. Be especially careful with the naming of directories!

Preparing for the installation
------------------------------

We will assume that the commands are run at the root of your development repo.
Everything will be done under a new directory called `compiler`, which you may
want to add to your `.gitignore` (line `compiler/`).
```sh
# Create the new directory and move there.
mkdir compiler
cd compiler

# Download and extract the sources of Binutils.
wget https://ftp.gnu.org/gnu/binutils/binutils-2.37.tar.xz
tar -xf binutils-2.37.tar.xz

# Download and extract the sources of GCC.
wget https://ftp.gnu.org/gnu/gcc/gcc-11.2.0/gcc-11.2.0.tar.xz
tar -xf gcc-11.2.0.tar.xz

# Download and extract the sources of GDB.
wget https://ftp.gnu.org/gnu/gdb/gdb-11.1.tar.xz
tar -xf gdb-11.1.tar.xz
```


Compiling and installing
------------------------

Assuming you are still inside the `compiler` directory, we now need to run the
following commands to define a couple of environment variables.
```sh
export PREFIX="$PWD/prefix"     # Installation prefix.
export TARGET=aarch64-elf       # Target architecture.
export PATH="$PREFIX/bin:$PATH" # Put (not yet) installed stuff in the PATH.
```

We can now compile and install everything.
```sh
# Compiling and installing Binutils.
mkdir build-binutils
cd build-binutils
../binutils-2-37/configure --target=$TARGET --prefix="$PREFIX" \
  --with-sysroot --disable-nls --disable-werror
make -j 8  # Running with 8 jobs, but feel free to change that.
make install
cd ..
rm -r build-binutils binutils-2-37

# Compiling and installing GCC (needs the above extension of PATH).
mkdir build-gcc
cd build-gcc
../gcc-11.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls \
  --enable-languages=c --without-headers
make -j 8 all-gcc
make -j 8 all-target-libgcc
make install-gcc
make install-target-libgcc
cd ..
rm -r build-gcc gcc-11.2.0

# Compiling and installing GDB (needs the above extension of PATH).
mkdir build-gdb
cd build-gdb
../gdb-11.1/configure --target=$TARGET --prefix="$PREFIX"
make -j 8
make install
cd ..
rm -r build-gdb gdb-11.1
cd ..
```

Everything should now be ready to start compiling our project.


Notes on PATH and Makefile
--------------------------

There is no need to permanently extend you path with our `compiler/prefix/bin`
directory. Instead, you can define `COMPILER_PREFIX` and `BINROOT` variable as
follows, at the top of your `Makefile`.
```make
# Relative path to the prefix where the compiler was installed.
COMPILER_PREFIX = compiler/prefix

# Prefix to use before all binutils, gcc and gdb commands.
BINROOT = ${COMPILER_PREFIX}/bin/aarch64-elf-
```
You may need to adapt the definition of `COMPILER_PREFIX` depending on how you
installed Binutils, GCC and GDB. In this repository, `COMPILER_PREFIX` will be
defined as `../compiler/prefix` since our `Makefile` is not at the root of the
repository, but under a `step_XX` directory.

With the above, you should be able to call commands such as `${BINROOT}gcc` or
`${BINROOT}ld`, build of an usual command name prefixed with `${BINROOT}`. The
following target can be added to your `Makefile` to try this.
```make
.PHONY: all
all:
	@${BINROOT}gcc --version | head -n 1
```
Running `make` should output the line `aarch64-elf-gcc (GCC) 11.2.0`.
