#ifndef _OUT_WRITER_H_
#define _OUT_WRITER_H_
#include <stdint.h>
#include "tiles.h"
uint8_t set_outputfilename(const char *fname);

uint8_t setup_outbuff_paths(char *fullpath, const char *basename, int fplen, 
    int base_len);

int WriteToFile(TileSet_Data *ref);


#endif  /* _OUT_WRITER_H_ */
