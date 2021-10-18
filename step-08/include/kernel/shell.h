#pragma once

// Maximum number of characters for a command line.
#define CMD_BUF_SIZE 80

// Maximum number of arguments for a command.
#define ARGV_SIZE    10

// Shell prompt.
#define PROMPT       "> "

// Run an interactive shell (never returns).
void shell_main();
