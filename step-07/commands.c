#include <types.h>
#include <string.h>
#include <util.h>
#include <bcm2837/uart1.h>
#include <kernel/commands.h>

int help(size_t argc, char **argv){
  if(argc > 1){
    uart1_printf("Error: \"%s\" does not expect arguments.\n", argv[0]);
    return 1;
  }

  uart1_printf("List of available commands:\n");
  cmd_descr *next_cmd = cmds;
  while(next_cmd->name){
    uart1_printf("- \"%s\": %s.\n", next_cmd->name, next_cmd->doc);
    next_cmd++;
  }

  return 0;
}

int echo(size_t argc, char **argv){
  for(size_t i = 1; i < argc; i++){
    uart1_printf("%s\n", argv[i]);
  }

  return 0;
}

int hexdump(size_t argc, char **argv){
  if(argc != 3){
    uart1_printf("Error: \"s\" expects two integer arguments.\n", argv[0]);
    return 1;
  }

  u64 addr; // Start address.
  u64 size; // Number of bytes.

  // Parse the first argument.
  char *end;
  if(argv[1][0] == '0' && argv[1][1] == 'x'){
    addr = strtou64(argv[1] + 2, &end, 16);
  } else {
    addr = strtou64(argv[1], &end, 10);
  }
  if(end){
    uart1_printf("Error: ARG1 should be a decimal or hex address.\n");
    return 1;
  }

  // Parse the second argument
  size = strtou64(argv[2], &end, 10);
  if(end){
    uart1_printf("Error: ARG2 should be a base 10 size.\n");
    return 1;
  }

  // Main logic start here.
  u64 nb_lines = size / 16 + (size % 16 ? 1 : 0);
  u64 line;
  u64 col;
  for(line = 0; line < nb_lines; line++){
    uart1_printf("%w: ", addr + line * 16);
    for(col = 0; col < 16; col += 2){
      char *b = (char *) (addr + line * 16 + col);
      if(line * 16 + col < size){
        uart1_printf("%b%b ", *b, *(b+1));
      } else {
        uart1_printf("     ");
      }
    }
    uart1_printf(" ");
    for(col = 0; col < 16; col++){
      if(line * 16 + col >= size) break;
      char *b = (char *) (addr + line * 16 + col);
      char c = *b;
      if(c < ' ' || c > '~') c = '.';
      uart1_printf("%c", c);
    }
    uart1_printf("\n");
  }

  return 0;
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
