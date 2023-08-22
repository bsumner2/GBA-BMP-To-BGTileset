#include "pal_bmp.h"
#include "tiles.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



uint8_t *TileCpy(uint8_t *dest, uint8_t *src, uint8_t bpp) {
  uint64_t *destcast, *srccast;
  int lim = bpp, i;
  if (bpp&0xF3U || bpp==12)
    return NULL;
  if (!src)
    return NULL;
  if (!dest)
    dest = (uint8_t*)malloc(lim*8*sizeof(uint8_t));
  destcast = ((uint64_t*)dest), srccast = ((uint64_t*)src);
  for (i = 0; i < lim; ++i)
    destcast[i] = srccast[i];
  return dest;
}


void VFlipTile_4bpp(uint8_t *tile) {
  uint64_t *tcast = (uint64_t*)tile, tmp;
  int i, j;
  if (!tile)
    return;
  for (i = 0, j = 3; i < j; ++i, --j) {
    tmp = 
      (0x00000000FFFFFFFFUL&(tcast[i]>>32)) |
      (0xFFFFFFFF00000000UL&(tcast[i]<<32));
    tcast[i] = 
      (0x00000000FFFFFFFFUL&(tcast[j]>>32)) |
      (0xFFFFFFFF00000000UL&(tcast[j]<<32));
    tcast[j] = tmp;
  }
}

void VFlipTile_8bpp(uint8_t *tile) {
  uint64_t *tcast = (uint64_t*)tile, tmp;
  int i,j;
  if (!tile)
    return;
  for (i = 0, j = 7; i < j; ++i, --j) {
    tmp = tcast[i];
    tcast[i] = tcast[j];
    tcast[j] = tmp;
  }
}

void HFlipTile_4bpp(uint8_t *tile) {
  uint32_t *tcast = (uint32_t*)tile, tmp;
  int i, j;
  if (!tile)
    return;
  for (i = 0; i < 8; ++i) {
    tmp = tcast[i];
    tcast[i] = 0;
    for (j = 7; j > 3; --j)
      tcast[i] |=
        ((tmp>>((j<<3)-28))&(0xF0000000>>(j<<2))) | 
        ((tmp<<((j<<3)-28))&(0x0000000F<<(j<<2)));
  }
}

void HFlipTile_8bpp(uint8_t *tile) {
  uint8_t *row;
  int i, j;
  uint8_t tmp;
  if (!tile)
    return;
  for (i = 0; i < 8; ++i) {
    row = (tile + (i<<3));
    for (j = 0; j < 4; ++j) {
      tmp = row[j];
      row[j] = row[7-j];
      row[7-j] = tmp;
    }
  }
}


uint8_t *VFlipped_4bpp(uint8_t *tile) {
  uint32_t *ret, *tmp = (uint32_t*)tile;
  int i;
  if (!tile)
    return NULL;
  ret = (uint32_t*)malloc(sizeof(uint32_t)*8);
  /* 8x8 tile @ 4bpp => 256 bits total. 256/32 = 8 ; 8 4B words, can be repped 
   * as 8 uint32_t length array. * */
  for (i = 0; i < 4; ++i) {
    ret[i] = tmp[7-i];
    ret[7-i] = tmp[i];
  }
  return (uint8_t*)ret;
}


uint8_t *HFlipped_4bpp(uint8_t *tile) {
  uint8_t *ret, *retrow, *tilerow;
  int i, j, k;
  if (!tile)
    return NULL;
  ret = (uint8_t*)malloc(sizeof(uint8_t)*32);
  for (i = 0; i < 8; ++i) {
    retrow = (ret + (i<<2));
    tilerow = (tile + (i<<2));
    for (j = 0, k = 3; j < k; ++j, --k) {
      retrow[j] = (0xF0&(tilerow[k]<<4))|(0x0F&(tilerow[k]>>4));
      retrow[k] = (0xF0&(tilerow[j]<<4))|(0x0F&(tilerow[j]>>4));
    }
  }
  return ret;
}

uint8_t *VFlipped_8bpp(uint8_t *tile) {
  uint64_t *ret, *tmp = (uint64_t*)tile;
  int i;
  ret = (uint64_t*)malloc(sizeof(uint64_t)*8);
  for (i = 0; i < 4; ++i) {
    ret[i] = tmp[7-i];
    ret[7-i] = tmp[i];
  }
  return (uint8_t*)ret;
}

uint8_t *HFlipped_8bpp(uint8_t *tile) {
  uint8_t *ret = (uint8_t*)malloc(sizeof(uint8_t)*64), *retrow, *tilerow;
  int y, xl, xr;
  for (y = 0; y < 8; ++y) {
    retrow = (ret + (y<<3));
    tilerow = (tile + (y<<3));
    for (xl = 0, xr = 7; xl<xr; ++xl, --xr) {
      retrow[xl] = tilerow[xr];
      retrow[xr] = tilerow[xl];
    }
  }
  return ret;
}

int TileCmp_4bpp(uint8_t *comparand, uint8_t *comparator) {
  uint64_t *lhs, *rhs;
  int i;
  /* Check if directly equal */
  lhs = (uint64_t*)comparand, rhs = (uint64_t*)comparator;
  for (i = 0; i < 4; ++i)
    if (lhs[i]!=rhs[i])
      return 1;
  return 0;
}


int TileCmp_8bpp(uint8_t *comparand, uint8_t *comparator) {
  uint64_t *lhs, *rhs;
  int i;
  /* Check if directly equal */
  lhs = (uint64_t*)comparand, rhs = (uint64_t*)comparator;
  for (i = 0; i < 8; ++i)
    if (lhs[i]==rhs[i])
      continue;
    else
      return 1;
  return 0;
}

