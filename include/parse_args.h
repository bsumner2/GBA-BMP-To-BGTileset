#ifndef _PARSE_ARGS_H_
#define _PARSE_ARGS_H_

#include <stdint.h>

struct opts {
  char *path_in,
       *path_out,
       *target_name;
  uint8_t bpp;
  
};


struct opts ParseArgs(int argc, char **argv);


#endif /* _PARSE_ARGS_H_ */
