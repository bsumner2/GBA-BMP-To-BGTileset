#include "pal_bmp.h"
#include "tiles.h"
#include "hashing.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int internal_TileSetInit_8bpp(TileSet_Data *obj,
    GBACompatible_Paletted_Bitmap *bmp);

static int internal_TileSetInit_4bpp(TileSet_Data *obj,
    GBACompatible_Paletted_Bitmap *bmp);

static uint8_t *TileBuff_4bpp(uint8_t *dest, GBACompatible_Paletted_Bitmap *bmp, 
    int tx, int ty, uint16_t *return_palbank_number);

static uint8_t *TileBuff_8bpp(uint8_t *dest, GBACompatible_Paletted_Bitmap *bmp, 
    int tx, int ty);

int TileSet_Data_Create(TileSet_Data *ref, const char *bitmap_path, 
    uint8_t bpp) {
  Paletted_Bitmap *tmp;
  GBACompatible_Paletted_Bitmap *bmp;
  if (!ref || !bitmap_path || !(bpp==4 || bpp==8))
    return -1;

  tmp = ParseBMP8BPP(NULL, bitmap_path);
  if (!tmp)
    return -1;
  bmp = ConvertPaletteBitDepth(tmp);
  PalettedBitmapClose(tmp);
  
  if (0 > TileSet_Data_Init(ref, bmp, bpp)) {
    GBA_PalettedBMPClose(bmp);
    return -1;
  }
  GBA_PalettedBMPClose(bmp);
  return 0;
}

int TileSet_Data_Init(TileSet_Data *obj,
    GBACompatible_Paletted_Bitmap *bmp, uint8_t bpp) {
  uint8_t parse_failure_flag = 0;
  if (!obj || !bmp || bpp&0xF3U || bpp==12)
    return -1;
  if (bmp->bpp != 8)
    return -1;
  obj->bpp = bpp;

  if (bmp->width != 256 && bmp->width != 512)
    return -1;
  if (bmp->height != 256 && bmp->height != 512)
    return -1;
  
  obj->map_height = (bmp->height)>>3;
  obj->map_width = (bmp->width)>>3;
  obj->tilemap = NULL;
  obj->tileset = NULL;
  assert(TILE_SET_PAL_LEN == bmp->pallen);
  switch (bpp) {
    case 4:
      if (0 > internal_TileSetInit_4bpp(obj, bmp))
        parse_failure_flag = 1;
      break;
    case 8:
      if (0 > internal_TileSetInit_8bpp(obj, bmp))
        parse_failure_flag = 1;
      break;
    default:
      return -1;
  }
  if (parse_failure_flag) {
    if (obj->tileset)
      free((void*)(obj->tileset));
    if (obj->tilemap)
      free((void*)(obj->tilemap));
    return -1;
  }
  
  obj->pal = (uint16_t*)malloc(bmp->pallen);
  memmove((void*)(obj->pal), (void*)(bmp->pal), bmp->pallen);
  return 0;
}

void TileSet_Data_Close(TileSet_Data *ref) {
  int i, tilect;
  free((void*)(ref->pal));
  tilect = ref->tileset_len;
  for (i = 0; i < tilect; ++i)
    free((void*)(ref->tileset[i]));
  free((void*)(ref->tileset));
  free((void*)(ref->tilemap));

  // We dont free the TileSet_Data instance, itself, since the interface calls
  // for any use to be with a static object (not dynamically alloc'd). As such 
  // if the instance is a heap inst, then it's up to the end user to free it
  // themselves after calling this function to close the buffers.
}


int internal_TileSetInit_4bpp(TileSet_Data *obj, 
    GBACompatible_Paletted_Bitmap *bmp) {
  uint8_t buff[32];
  GBATile_HashTable *table = NULL;
  int y,x;
  uint16_t palno = 0;
  if (!obj || !bmp)
    return -1;
  obj->tilemap = 
    (uint16_t*)malloc(sizeof(uint16_t)*(obj->map_width)*(obj->map_height));
  if (!(table = GBATileTableCreate(4)))
    return -1;
  for (y = 0; y < obj->map_height; ++y) {
    for (x = 0; x < obj->map_width; ++x) {
      if (!TileBuff_4bpp(buff, bmp, x, y, &palno)) {
        GBATileTableDestroy(table);
        return -1;
      }
      if (0 > GBATileTablePut(table, buff, &palno)) {
        GBATileTableDestroy(table);
        return -1;
      }
      obj->tilemap[y*(obj->map_width) + x] = palno;
    }
  }

  obj->tileset_len = GBATileTableDealloc(table, &(obj->tileset));
  if (0 > obj->tileset_len) {
    GBATileTableDestroy(table);
    return -1;
  }
  return 0;
}

int internal_TileSetInit_8bpp(TileSet_Data *obj, 
    GBACompatible_Paletted_Bitmap *bmp) {
  uint8_t buff[64];
  GBATile_HashTable *table = NULL;
  int y, x;
  uint16_t sbe;
  if (!obj || !bmp)
    return 01;
  obj->tilemap = 
    (uint16_t*)malloc(sizeof(uint16_t)*(obj->map_width)*(obj->map_height));
  if (!(table = GBATileTableCreate(8)))
    return -1;
  for (y = 0; y < obj->map_height; ++y) {
    for (x = 0; x < obj->map_width; ++x) {
      if (!TileBuff_8bpp(buff, bmp, x, y)) {
        GBATileTableDestroy(table);
        return -1;
      }
      if (0 > GBATileTablePut(table, buff, &sbe)) {
        GBATileTableDestroy(table);
        return -1;
      }
      obj->tilemap[y*(obj->map_width)+x] = sbe;
    }
  }

  obj->tileset_len = GBATileTableDealloc(table, &(obj->tileset));
  if (0 > obj->tileset_len) {
    GBATileTableDestroy(table);
    return -1;
  }

  return 0;


}

static uint8_t *offsetted_pbuf_row(GBACompatible_Paletted_Bitmap *bmp,
    int x_offset, int y_offset) {
  if (!bmp)
    return NULL;
  return ((bmp->pbuf) + (y_offset*(bmp->width) + x_offset));
}

static inline uint16_t get_palbank_number(uint8_t _8bpp_pixel) {
  return _8bpp_pixel>>4;
}

uint8_t *TileBuff_4bpp(uint8_t *dest, GBACompatible_Paletted_Bitmap *bmp,
    int tx, int ty, uint16_t *return_palbank_number) {
  uint8_t *offsetrow, *destrow;
  int y, x, yoff, xoff;
  uint16_t palbankno;
  if (!dest || !bmp || !return_palbank_number)
    return NULL;
  yoff = ty<<3, xoff = tx<<3;
  if (ty < 0 || tx < 0 || yoff > (bmp->height-8) || xoff > (bmp->width-8))
    return NULL;
  palbankno = get_palbank_number(*offsetted_pbuf_row(bmp, xoff, yoff));
  for (y = 0; y < 8; ++y) {
    if (!(offsetrow = offsetted_pbuf_row(bmp, xoff, y + yoff)))
      return NULL;
    destrow = (dest + (y<<2));
    for (x = 0; x < 8; x += 2) {
      if (get_palbank_number(offsetrow[x]) != palbankno ||
          get_palbank_number(offsetrow[x+1]) != palbankno)
        return NULL;
      destrow[x>>1] = ((offsetrow[x]<<4)&0xF0)|(0x0F&offsetrow[x+1]);
    }
  }
  *return_palbank_number = palbankno<<12;
  return dest;
}

uint8_t *TileBuff_8bpp(uint8_t *dest, GBACompatible_Paletted_Bitmap *bmp, 
    int tx, int ty) {
  uint64_t *offsetrow, *destrow;
  int y, yoff, xoff;
  if (!dest || !bmp)
    return NULL;
  yoff = ty<<3, xoff = tx<<3;
  if (ty < 0 || tx < 0 || xoff > (bmp->width-8) || yoff > (bmp->height-8))
    return NULL;
  for (y = 0; y < 8; ++y) {
    if (!(offsetrow = (uint64_t*)offsetted_pbuf_row(bmp, xoff, y + yoff)))
      return NULL;
    destrow = (uint64_t*)(dest + (y<<3));
    destrow[0] = offsetrow[0];
  }

  return dest;
}
