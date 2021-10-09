#include <stddef.h>
#include <stdbool.h>

// Functions from the C standard library.
// Check your local manpage for information.

char *strtok_r(char *str, const char *delim, char **saveptr){
  char *cur = str ? str : *saveptr;

  // In case we have nothing, just return.
  if(!cur) return NULL;

  // Skip characters that are not delimiters.
  while(1){
    if(*cur == '\0') return NULL;

    bool is_delim = false;
    for(const char *d = delim; *d != '\0'; d++){
      if(*cur == *d){
        is_delim = true;
        break;
      }
    }

    if(!is_delim) break;

    cur++;
  }

  // Now cur is the start of our token.
  char *tok = cur;

  // Find the next delimiter.
  while(1){
    // Our token goes to the end of the string.
    if(*cur == '\0'){
      *saveptr = NULL;
      return tok;
    }

    bool is_delim = false;
    for(const char *d = delim; *d != '\0'; d++){
      if(*cur == *d){
        is_delim = true;
        break;
      }
    }

    if(is_delim) break;

    cur++;
  }

  // Insert null-terminator for the token, set saveptr.
  *cur = '\0';
  *saveptr = cur + 1;

  return tok;
}

char *saveptr = NULL;

char *strtok(char *str, const char *delim){
  return strtok_r(str, delim, &saveptr);
}

int strcmp(const char *s1, const char *s2){
  while(*s1 != '\0' && *s2 != '\0'){
    unsigned char c1 = (unsigned char) *s1;
    unsigned char c2 = (unsigned char) *s2;

    int res = c2 - c1;
    if(res != 0) return res;

    s1++;
    s2++;
  }

  if(*s1 == *s2) return 0;

  if(*s1 == '\0') return -1;

  return 1;
}
