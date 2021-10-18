#include <string.h>
#include <bcm2837/uart1.h>
#include <kernel/commands.h>
#include <kernel/shell.h>

size_t to_argv(char *cmd, char **argv, size_t max_argc){
  size_t argc = 0;
  const char *delim = " \t\r\n";

  char *cur = strtok(cmd, delim);
  while(cur){
    if(argc < max_argc) argv[argc] = cur;
    argc++;
    cur = strtok(NULL, delim);
  }

  return argc;
}

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
