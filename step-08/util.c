#include <stddef.h>
#include <types.h>
#include <util.h>

u64 strtou64(const char *nptr, char **endptr, int base){
  if(base < 2 || base > 36){
    *endptr = (char *) nptr;
    return 0;
  }

  u64 res = 0;
  size_t cur = 0;

  while(1){
    int digit;
    char next = nptr[cur];

    if('0' <= next && next <= '9'){
      digit = next - '0';
    } else if('a' <= next && next <= 'z'){
      digit = 10 + next - 'a';
    } else if('A' <= next && next <= 'A'){
      digit = 10 + next - 'A';
    } else {
      if(cur == 0){
        *endptr = (char *) nptr;
        return 0;
      } else {
        *endptr = next == '\0' ? NULL : (char *) &(nptr[cur]);
        return res;
      }
    }

    if(digit >= base){
      if(cur == 0){
        *endptr = (char *) nptr;
        return 0;
      } else {
        *endptr = (char *) &(nptr[cur]);
        return res;
      }
    }

    res = res * base + digit;
    cur++;
  }
}
