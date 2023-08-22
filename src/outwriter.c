#include "tiles.h"
#include "outwriter.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>


#define PATH_PREFIX "/home/burton/dev/c/tilemap_creator/out/"
#define PATH_PREFIX_LEN 39
#define PATH_SUFFIX_LEN 2

#define WRITERBUFFLEN 256

#define BASENAME_BUFF_LEN 128

static char outfname_base[128], *pathbuff;
static int basename_len = -1, 
           path_strlen = -1;


uint8_t setup_outbuff_paths(char *fullpath, const char *basename, int fplen, 
    int base_len) {
  if (base_len > BASENAME_BUFF_LEN) {
    return 0;
  }
  basename_len = base_len;
  path_strlen = fplen;
  pathbuff = fullpath;
  ((uint64_t*)outfname_base)[0] = 0UL;
  ((uint64_t*)outfname_base)[1] = 0UL;
  strncpy(outfname_base, basename, base_len);
  pathbuff[fplen-2] = '.';
  pathbuff[fplen-1] = 'c';
  return 1;
}


static void WritePal(FILE *cfp, FILE *hfp, uint16_t *pal) {
  uint16_t *currblock, *currrow;
  int row, block, idx;

  fprintf(cfp, "const unsigned short %sPal[256] = {", outfname_base);
  fprintf(hfp, "#define %sPalLen 512\n", outfname_base);
  fprintf(hfp, "extern const unsigned short %sPal[256];\n\n", outfname_base);
  for (block = 0; block < 4; ++block) {
    currblock = (pal + (block<<6));
    fputc('\n', cfp);
    for (row = 0; row < 8; ++row) {
      currrow = (currblock + (row<<3));
      fputs("  ", cfp);
      for (idx = 0; idx < 8; ++idx)
        fprintf(cfp, "0x%04X,", currrow[idx]);
      fputc('\n', cfp);
    }
  }
  fputs("};\n\n", cfp);
}


static void WriteTile_4bpp(FILE *fp, uint16_t *tile) {
  int i, j;
  uint16_t tmp, l, r;
  for (i = 0; i < 2; ++i) {
    fputs("  ", fp);
    for (j = 0; j < 8; ++j) {
      /* Re-format data due to the GBA's Little Endianness.
       * Basically, we need to fix the pixels to where the most significant bit
       * of each byte is the offset of that byte's right pixel, instead of the
       * left, as would be the usual case.*/
      tmp = tile[((i<<3)|j)];
      l = (tmp>>8)&0x00FF;
      r = (tmp)&0x00FF;
      l = ((l>>4)&0x000F)|((l<<4)&0x00F0);
      r = ((r>>4)&0x000F)|((r<<4)&0x00F0);
      tmp = (l<<8)|r;
      fprintf(fp, "0x%04X,", tmp);
    }
    fputc('\n', fp);
  }
}


static void WriteTile_8bpp(FILE *fp, uint16_t *tile) {
  int i, j;
  for (i = 0; i < 4; ++i) {
    fputs("  ", fp);
    for (j = 0; j < 8; ++j)
      fprintf(fp, "0x%04X,", tile[(i<<3)+j]);
    fputc('\n', fp);
  }
}


static void WriteTiles(FILE *cfp, FILE *hfp, uint8_t **tiles, int tile_ct, 
    uint8_t bpp) {
  int i;
  fprintf(cfp, "const unsigned short %sTiles[%d] = {",
      outfname_base, tile_ct*(bpp<<2));
  fprintf(hfp, "#define %sTilesLen %d\n", outfname_base, tile_ct*(bpp<<3));
  fprintf(hfp, "extern const unsigned short %sTiles[%d];\n\n",
      outfname_base, tile_ct*(bpp<<2));
  if (bpp==4) {
    for (i = 0; i < tile_ct; ++i) {
      if (!(i&3))
        fputc('\n', cfp);
      WriteTile_4bpp(cfp, (uint16_t*)tiles[i]);
    }
  } else {
    for (i = 0; i < tile_ct; ++i) {
      if (!(i&1))
        fputc('\n', cfp);
      WriteTile_8bpp(cfp, ((uint16_t*)(tiles[i])));
    }
  }

  fputs("};\n\n", cfp);
}

void WriteMap(FILE *cfp, FILE *hfp, uint16_t *map, int mapsize) {
  uint16_t *currblock, *currrow; 
  int numblocks = mapsize>>6, row, block, idx;

  fprintf(cfp, "const unsigned short %sMap[%d] = {", outfname_base, mapsize);
  fprintf(hfp, "#define %sMapLen %d\n", outfname_base, mapsize<<1);
  fprintf(hfp, "extern const unsigned short %sMap[%d];\n\n", outfname_base, 
      mapsize);
  for (block = 0; block < numblocks; ++block) {
    currblock = (map + (block<<6));
    fputc('\n', cfp);
    for (row = 0; row < 8; ++row) {
      currrow = (currblock + (row<<3));
      fputs("  ", cfp);
      for (idx = 0; idx < 8; ++idx)
        fprintf(cfp, "0x%04X,", currrow[idx]);
      fputc('\n', cfp);
    }
  }
  fputs("};\n\n", cfp);


}

static void print_head_info(FILE *fp, TileSet_Data *ref) {
  fprintf(fp, 
          "//---------------------------------------------------------------\n"
          "// Auto-gen'd tile info by Tilemap BMP Parser Program by Burt S.\n"
          "// Feel free to use my program at your leisure. Src and build at:\n"
          "// https://github.com/bsumner2\n"
          "// ==============================================================\n"
          "// BMP Parsed Info:\n"
          "// \tTilemap Width: %d\n// \tTilemap Height: %d\n"
          "// \tTiles In Set: %d\n// \tBit Depth: %d\n"
          "//---------------------------------------------------------------\n",
          ref->map_width, ref->map_height, ref->tileset_len, ref->bpp);

}

int WriteToFile(TileSet_Data *ref) {
  char buff[256] = {0};
  FILE *src_out;
  FILE *header_out;
  int i;
  if (path_strlen==-1)
    return -1;
  if (!ref)
    return -1;
#if 0
  spf_ret = sprintf(buff, PATH_PREFIX"%s.c", outfname_base);
  if (1 < (path_strlen - spf_ret)) {
    fprintf(stderr, "\x1b[1;31m[Error]:\x1b[0m Expected sprintf on buff which"
        "now contains the string, \x1b[1;32m\"%s\"\x1b[0m,\nto return either "
        "\x1b[1;33m%d\x1b[0m or \x1b[1;33m%d\x1b[0m. Instead, received: "
        "\x1b[1;34m%d\x1b[0m\n", buff, path_strlen, path_strlen-1, 
        (path_strlen - spf_ret));
    return -1;
  }
#endif
  if (!(src_out = fopen(pathbuff, "w+"))) {
    perror("\x1b[1;31m[Error]:\x1b[0m Could not create file at given path.\n"
        "Details from fopen's errno: ");
    fprintf(stderr, "\x1b[1;31mPath tried: \x1b[32m\"%s\"\x1b[0m.\n", pathbuff);
    return -1;
  }

  printf("\x1b[1;33mSrc Outfiles' paths:\x1b[34m %s\x1b[0m\n", pathbuff);
  
  print_head_info(src_out, ref);

  pathbuff[path_strlen-1] = 'h';
  if (!(header_out = fopen(pathbuff, "w+"))) {
    perror("\x1b[1;31m[Error]:\x1b[0m Could not create file at given path.\n"
        "Details from fopen's errno: ");
    fprintf(stderr, "\x1b[1;31mPath tried: \x1b[32m\"%s\"\x1b[0m.\n", pathbuff);
    return -1;
  }

  print_head_info(header_out, ref);

  printf("\x1b[1;33mHeader Outfiles' paths:\x1b[34m %s\x1b[0m\n", pathbuff);


  for (i = 0; i < basename_len; ++i)
    buff[i] = toupper(outfname_base[i]);
  fprintf(header_out, "#ifndef _%s_H_\n#define _%s_H_\n", buff, buff);
  
  WritePal(src_out, header_out, ref->pal);
  
  WriteTiles(src_out, header_out, ref->tileset, ref->tileset_len, ref->bpp); 
  
  WriteMap(src_out, header_out, ref->tilemap, (ref->map_width)*(ref->map_height));
   
  fprintf(header_out, "#endif  /* _%s_H_ */\n", buff);


  fclose(src_out);

  fclose(header_out);

  return 0;
}
