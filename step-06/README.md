Step 6: a minimal debugging shell
=================================

Although we can now interact with our kernel on the serial port, this is still
not very useful. For instance, we are not yet able to print any useful data on
the console: only characters and strings. Also, we do not have any way to make
queries like, say, asking for the contents of memory at some address.


Passing more initial state information to the kernel
----------------------------------------------------

Currently, our `kernel_entry` C function only has four arguments: a pointer to
the DTB (passed via register `x0`), and three `u64` initialised to `0` (passed
via registers `x1`, `x2` and `x3`). But what if we want to pass more arguments
to `kernel_entry`? The calling convention states that the first 8 arguments of
are passed via registers `x0` through `x7`, so all we have to do is change the
prototype of `kernel_entry` and make sure to set the values of these registers
to the relevant value before calling the function.

Coincidentally, `boot.S` currently uses all registers from `x0` to `x7` (well,
it does not directly touch registers `x0` to `x4`, but they are initialised by
the firmware code). However, we will not just make the `kernel_entry` function
take 8 arguments. Indeed, `x6` and `x7` are used when zeroing the BSS segment,
and they both have value `__bss_start` at the call to `kernel_entry`. However,
`x4` and `x5` have somewhat interesting values: the value of the former is the
address of our entry point (i.e., `0x80000`), and the latter gives the current
exception level (EL). We will thus feed these values to `kernel_entry`.
```c
void kernel_entry(void *dtb, u64 x1, u64 x2, u64 x3, u64 x4, u64 x5){
  // Silence GDB warnings about unused parameters.
  UNUSED(dtb); UNUSED(x1); UNUSED(x2); UNUSED(x3); UNUSED(x4); UNUSED(x5);

  ... // Same as before.
}
```
(At this point, we need more uses of `UNUSED` to avoid warnings.)


Validating the DTB pointer
--------------------------

As we have already noticed in a previous step, the value of `x0` upon entry in
QEMU is only a valid DTB pointer if we use the `-dtb` option. It would however
be tasteful to ensure that the first argument given to `kernel_entry` is valid
(as a DTB pointer) or `NULL`. To do this, we add the following to `boot.S`.
```gas
_start:
  // Check that x0 has a valid DTB pointer, otherwise set it to 0.
  ldr w5, [x0]          // Load a half-word from the address in x0 into w5.
  ldr w6, =0xedfe0dd0   // Put the (reversed) DTB magic number in w6.
  cmp w5, w6            // Compare the two.
  b.eq done_with_dtb    // If they are equal, x0 is a valid DTB pointer.
  mov x0, xzr           // If not we change the value of x0 to 0.
done_with_dtb:
```
This code is simple: it loads a half-word from where the DTB is supposed to be
in memory and puts it in register `w5`. The first four bytes of the DTB hold a
magic number (`0xdoodfeed` encoded in big-endian form), so we set the value of
register `w6` to this magic number (reversed as we are little-endians), and we
compare them.

**Note:** the half-word (32-bit) registers `w0` through `w30` access the lower
32 bits of (word) registers `x0` through `x30`. As a consequence, an operation
on `w0` will also change the value of `x0`. This is not a problem in our code:
indeed, the value of `x5` and `x6` is set before being used in the rest of the
code.


Printing system information
---------------------------

For now, the function provided by `uart1.c` cannot be easily used to print the
value of integer or addresses. However, there is no doubt that we will need to
print the value of such objects, e.g., for debugging. The most flexible way to
fill this gap is to implement a `printf`-like function.
```c
// This function behaves similarly to the standard printf function.
// The format string can contain the following format string directives:
// - "%%": print the "%" character,
// - "%s": print the given null-terminated string,
// - "%c": print the given character,
// - "%b": print the hexadecimal representation of the given character,
// - "%w": print the hexadecimal representation of the given word (u64),
// - "%h": print the hexadecimal representation of the given half-word (u34),
// - "%i": print the decimal representation of the given integer (int).
void uart1_printf(const char *format, ...);
```
This function takes as first argument a format string which can contain format
string directives (formed of `%` followed by a character), and then a variable
number of arguments (one for each non-`%%` directive), whose representation is
printed in a pre-determined instead of the corresponding directive.

The implementation of `uart1_printf` (found in `uart1.c`) is straight-forward,
and there is not much interesting to say about it. Just look at the code!

**Note:** the man-page for `stdarg.h` is a good source of information on how a
variable argument list works (use command `man 3 stdarg.h` to open it).

To test our new printing function, we can change `kernel_entry` as follows.
```c
void kernel_entry(void *dtb, u64 x1, u64 x2, u64 x3, u64 x4, u64 x5){
  // Initialise the UART, and print a first message.
  uart1_init();
  uart1_puts("********************************************\n");
  uart1_puts("*              Hello, World!!              *\n");
  uart1_puts("********************************************\n");

  // Print information about the environment.
  uart1_printf("Initial value of x1:     0x%w.\n", x1);
  uart1_printf("Initial value of x2:     0x%w.\n", x2);
  uart1_printf("Initial value of x3:     0x%w.\n", x3);
  uart1_printf("Initial entry point:     0x%w.\n", x4);
  uart1_printf("Initial exception level: EL%i.\n", (int) x5);
  uart1_printf("Address of the DTB:      ");
  if(dtb){
    uart1_printf("0x%w.\n", (u64) dtb);
  } else {
    uart1_printf("n/a\n");
  }

  ... // Same as before.
}
```
Note that we are now using all arguments, so `UNUSED` is not required anymore.


Writing a proper debugging shell
--------------------------------

To interact with the kernel, it would be really great to have a shell in which
we can run various commands (e.g., to query some data). Ideally, we would like
the notion of a "command" to be pretty general, so that we can define new ones
when we need to. Commands should be able to take arguments, and should give an
error status when they return. This sounds pretty familiar: like the signature
of the usual `main` function in C.

Starting from this, let use write the following to `include/kernel/command.h`.
```c
#pragma once

// Structure describing an availalbe "command".
typedef struct __cmd_descr {
  char *name;                   // Command name.
  char *doc;                    // Short command description.
  int (*func)(size_t, char **); // Command function.
} cmd_descr;

// A command function takes arguments argc and argv, as typical main function.
// The argc arguments gives the size of the argv array. The argv array holds a
// list of arguments, headed by the command name in argv[0].
//
// Command functions should return 0 on success, and a non-0 value on error.

// Array of all available commands (defined in "commands.c").
// It is terminated by a dummy entry whose fields are all NULL pointers.
extern cmd_descr cmds[];
```
As hinted by its name, the `struct` type `cmd_descr` represents a command. The
`name` field is meant to uniquely identify the command: it will be used as key
for command lookup in the shell implementation. Then, the `doc` field contains
a command description: it is used to print a list of available commands in the
implementation of the `help` command. Lastly, field `func` is a pointer to the
function implementing the command.

The shell itself is implemented by a function `shell_main`, whose prototype is
given in file `include/kernel/shell.h`.
```c
#pragma once

// Maximum number of characters for a command line.
#define CMD_BUF_SIZE 80

// Maximum number of arguments for a command.
#define ARGV_SIZE    10

// Shell prompt.
#define PROMPT       "> "

// Run an interactive shell (never returns).
void shell_main();
```
This file also defines a few constants: since we do not have an allocator yet,
everything array or buffer needs to be allocated statically, with a hard limit
on its size. This is the case for the buffer in which a line of input is read,
as already done in the previous step, and that will also be true for the array
containing the list of arguments to a command (i.e., `argv`).

The implementation of the shell is found in file `shell.c`.
```c
void shell_main(){
  while(1){
    // Buffer for the input.
    char cmd[CMD_BUF_SIZE];

    // Ask the user for input, and read a "line" of input on the UART.
    uart1_printf(PROMPT);
    size_t len = uart1_getline(cmd, CMD_BUF_SIZE);

    // Handle the case where the command is longer than the buffer.
    if(cmd[len - 1] != '\n'){
      size_t total = len;
      do {
        len = uart1_getline(cmd, CMD_BUF_SIZE);
        total += len;
      } while(cmd[len - 1] != '\n');

      uart1_printf("Error: command line formed of %i characters.\n", total-1);
      uart1_printf("You cannot use more than %i.\n", CMD_BUF_SIZE-2);
      continue;
    }

    // Turn the line into an argc/argv pair.
    char *argv[ARGV_SIZE];
    size_t argc = to_argv(cmd, argv, ARGV_SIZE);
    if(argc == 0) continue;
    if(argc > ARGV_SIZE){
      uart1_printf("Error: command formed of %i tokens.\n", argc);
      uart1_printf("You cannot use more than %i.\n", ARGV_SIZE);
      continue;
    }

    // Find the relevant command in the list.
    cmd_descr *d = cmds;
    while(d->name != NULL){
      if(strcmp(argv[0], d->name) == 0) break;
      d++;
    }

    // Fail if no matching command found.
    if(d->name == NULL){
      uart1_printf("Error: unknown command \"%s\".\n", argv[0]);
      uart1_printf("Use command \"help\" to get a list of commands.\n");
      continue;
    }

    int res = (d->func)(argc, argv);
    if(res != 0){
      uart1_printf("**Command exited with status %i.**\n", res);
    }
  }
}
```
As expected, the function runs an infinite loop. At each iteration, the prompt
is printed and a line of input is read into the `cmd` buffer. Now, in the case
where a full line was not read, the buffer is repeatedly emptied until we find
the end of line, and an error message is printed. Otherwise, we proceed to the
parsing of `cmd` into the `argv` array for the command. This is achieved using
the `to_argv` auxiliary function (see `shell.c`), itself implemented using the
`strtok` standard function. Since we are running code on bare metal, we do not
have access to the C standard library and its `string.h` header. We hence need
to implement our own (partial) version of `string.h` (see `include/string.h`),
and implement the corresponding functions (see `string.c`).

Having parsed the `cmd` buffer into an array of strings `argv` of size `argc`,
we can then look for the requested command (whose name is in `argv[0]`) in the
array of commands `cmds`. If a matching command is found, we can then call the
corresponding function with `argc` and `argv` as arguments, and check that the
command terminated without error by looking at the return value.

**Note:** to have the command entered by the user printed in the terminal, the
function `uart1_getc` need to be modified to print back the character it read.
It might be possible to avoid doing that by enabling a "local echo" option (in
QEMU, or by using a different tool like `screen` to read the serial port), but
for now this "hack" will have to do.


Command implementations
-----------------------

All that remains to do now is implement a few commands, and define `cmds` (the
array declared as `extern` in `include/kernel/command.h`). All of this is done
in file `command.c`, where three commands are defined: `help` (printing a list
of available commands), `echo` (printing its arguments) and `hexdump` (showing
the contents of memory).
```c
#include <types.h>
#include <string.h>
#include <util.h>
#include <bcm2837/uart1.h>
#include <kernel/commands.h>

int help(size_t argc, char **argv){
  ...
}

int echo(size_t argc, char **argv){
  for(size_t i = 1; i < argc; i++){
    uart1_printf("%s\n", argv[i]);
  }

  return 0;
}

int hexdump(size_t argc, char **argv){
  ...
}

// Fill in the list of commands (exposed by "include/kernel/commands.h").
cmd_descr cmds[] = {
  { .name = "help",
    .doc  = "list the available commands",
    .func = help },
  { .name = "echo",
    .doc  = "print each of its arguments",
    .func = echo },
  { .name = "hexdump",
    .doc  = "dump memory starting at ARG1 for ARG2 bytes",
    .func = hexdump },
  // Dummy list terminator.
  { .name = NULL, .doc = NULL, .func = NULL }
};
```

**Note:** the implementation of `hexdump` requires converting a string into an
integer. This is done using the `strtou64` function defined in `util.c`. It is
accessed via header `include/util.h`.

We can now run `make run` to test our new commands! Here are a few examples.
```
> help
List of available commands:
- "help": list the available commands.
- "echo": print each of its arguments.
- "hexdump": dump memory starting at ARG1 for ARG2 bytes.
> echo hello world
hello
world
> hexdump 0x80000 128
0000000000080000: 0500 40b9 a602 0018 bf00 066b 4000 0054  ..@........k@..T
0000000000080010: e003 1faa 4542 38d5 a510 43d3 0642 38d5  ....EB8...C..B8.
0000000000080020: c6f8 7f92 0642 18d5 c601 0058 df00 0091  .....B.....X....
0000000000080030: c601 0058 e701 0058 df00 07eb 8a00 0054  ...X...X.......T
0000000000080040: df00 00f9 c620 0091 fcff ff17 0101 0094  ..... ..........
0000000000080050: 5f20 03d5 ffff ff17 d00d feed 0000 0000  _ ..............
0000000000080060: 0000 0800 0000 0000 0040 0800 0000 0000  .........@......
0000000000080070: 0050 0800 0000 0000 fd7b bda9 fd03 0091  .P.......{......
```
