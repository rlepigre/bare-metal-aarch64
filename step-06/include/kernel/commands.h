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
