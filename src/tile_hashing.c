#include "tiles.h"
#include "hashing.h"
#include <md5.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct _HNode {
  uint8_t *pbuf;  /*!< Pixel buffer for the tile this HNode reps */
  struct _HNode *nxt;
  uint16_t tid;
} HashNode;

#define TABLE_SIZE 1567
#define MAX_TILE_CT 1024



struct _GBATile_HashTable {
  HashNode *table[TABLE_SIZE];
  int next_tid;
  uint8_t bpp;
};



uint32_t HashTile_4bpp(uint8_t *tile) {
  MD5_CTX ctx;
  uint8_t out[MD5_DIGEST_LENGTH];
  uint64_t *casted;
  MD5Init(&ctx);
  MD5Update(&ctx, tile, sizeof(uint8_t)<<5);
  MD5Final(out, &ctx);
  casted = ((uint64_t*)out);
  return
    (((casted[0]>>32) + casted[0]) ^ ((casted[1]>>32) + casted[1]))%TABLE_SIZE;
}

uint32_t HashTile_8bpp(uint8_t *tile) {
  MD5_CTX ctx;
  uint8_t out[MD5_DIGEST_LENGTH];
  uint64_t *casted;
  MD5Init(&ctx);
  MD5Update(&ctx, tile, sizeof(uint8_t)<<6);
  MD5Final(out, &ctx);
  casted = ((uint64_t*)out);
  return 
    (((casted[0]>>32) + casted[0]) ^ ((casted[1]>>32) + casted[1]))%TABLE_SIZE;
}


/** 
 * \details It is assumed that the tile has already been scaled into a 4bpp 
 * bitmap. It is also assumed that the palette bank idx has already been set
 * to the memory block pointed to by output param return_screenblock_entry.
 * */
static int HashPut__4(GBATile_HashTable *table, uint8_t *tile, 
    uint16_t *return_screenblock_entry) {
  HashNode *curr, *tmp_to_add = NULL;
  uint8_t *flipped;
  int hash_idx, orig_hashidx;
  if (!table || !tile)
    return -1;
  hash_idx = HashTile_4bpp(tile);
  orig_hashidx = hash_idx;
  curr = table->table[hash_idx];
  while (curr) {
    if (TileCmp_4bpp(tile, curr->pbuf)) {
      tmp_to_add = curr;
      curr = curr->nxt;
      continue;
    }
    *return_screenblock_entry |= curr->tid; 
    return 0;
  }

  flipped = VFlipped_4bpp(tile);
  hash_idx = HashTile_4bpp(flipped);
  curr = table->table[hash_idx];
  while (curr) {
    if (TileCmp_4bpp(flipped, curr->pbuf)) {
      curr = curr->nxt;
      continue;
    }
    free((void*)flipped);
    *return_screenblock_entry |= V_FLIP_FLAG|curr->tid;
    return 0;
  }

  free((void*)flipped);
  flipped = HFlipped_4bpp(tile);
  hash_idx = HashTile_4bpp(flipped);
  curr = table->table[hash_idx];
  while (curr) {
    if (TileCmp_4bpp(flipped, curr->pbuf)) {
      curr = curr->nxt;
      continue;
    }

    free((void*)flipped);
    *return_screenblock_entry |= H_FLIP_FLAG|curr->tid;
    return 0;
  }

  VFlipTile_4bpp(flipped);
  hash_idx = HashTile_4bpp(flipped);
  curr = table->table[hash_idx];
  while (curr) {
    if (TileCmp_4bpp(flipped, curr->pbuf)) {
      curr = curr->nxt;
      continue;
    }
    free((void*)flipped);
    *return_screenblock_entry |= V_FLIP_FLAG|H_FLIP_FLAG|curr->tid;
    return 0;
  }

  free((void*)flipped);
  if (table->next_tid >= MAX_TILE_CT)
    return -1;
  
  curr = tmp_to_add;
  tmp_to_add = (HashNode*)malloc(sizeof(HashNode));
  tmp_to_add->pbuf = TileCpy(NULL, tile, 4);
  tmp_to_add->nxt = NULL;
  tmp_to_add->tid = table->next_tid++;
  *return_screenblock_entry |= tmp_to_add->tid;
  
  if (!curr) {
    assert(table->table[orig_hashidx]==NULL);
    table->table[orig_hashidx] = tmp_to_add;
    return 0;
  }

  curr->nxt = tmp_to_add;
  return 0;
}




static int HashPut__8(GBATile_HashTable *table, uint8_t *tile, 
    uint16_t *return_screenblock_entry) {
  HashNode *curr, *tmp_to_add = NULL;
  uint8_t *tmp;
  int idx, origidx;
  if (!table || !tile)
    return -1;
  idx = HashTile_8bpp(tile);
  origidx = idx;
  curr = table->table[idx];
  while (curr) {
    if (TileCmp_8bpp(tile, curr->pbuf)) {
      tmp_to_add = curr;
      curr = curr->nxt;
      continue;
    }
    *return_screenblock_entry = curr->tid;
    return 0;
  }
  tmp = HFlipped_8bpp(tile);
  idx = HashTile_8bpp(tmp);
  curr = table->table[idx];
  while (curr) {
    if (TileCmp_8bpp(tmp, curr->pbuf)) {
      curr = curr->nxt;
      continue;
    }
    *return_screenblock_entry = curr->tid|H_FLIP_FLAG;
    free((void*)tmp);
    return 0;
  }

  free((void*)tmp);
  tmp = VFlipped_8bpp(tile);
  idx = HashTile_8bpp(tmp);
  curr = table->table[idx];
  while (curr) {
    if (TileCmp_8bpp(tmp, curr->pbuf)) {
      curr = curr->nxt;
      continue;
    }
    *return_screenblock_entry = curr->tid|V_FLIP_FLAG;
    free((void*)tmp);
    return 0;
  }

  HFlipTile_8bpp(tmp);
  idx = HashTile_8bpp(tmp);
  curr = table->table[idx];
  while (curr) {
    if (TileCmp_8bpp(tmp, curr->pbuf)) {
      curr = curr->nxt;
      continue;
    }
    *return_screenblock_entry = curr->tid|V_FLIP_FLAG|H_FLIP_FLAG;
    free((void*)tmp);
    return 0;
  }

  free((void*)tmp);
  if (table->next_tid>= MAX_TILE_CT)
    return -1;
  curr = tmp_to_add;
  tmp_to_add = (HashNode*)malloc(sizeof(HashNode));
  tmp_to_add->pbuf = (uint8_t*)malloc(sizeof(uint8_t)<<6);
  memmove((void*)(tmp_to_add->pbuf), (void*)tile, sizeof(uint8_t)<<6);
  tmp_to_add->tid = table->next_tid++;
  *return_screenblock_entry = tmp_to_add->tid;
  tmp_to_add->nxt = NULL;

  if (!curr) {
    assert(table->table[origidx]==NULL);
    table->table[origidx] = tmp_to_add;
    return 0;
  }
  /* ELSE add to tail of bucket at hash idx origidx. */
  curr->nxt = tmp_to_add;
  return 0;
}





int GBATileTablePut(GBATile_HashTable *table, uint8_t *tile, 
    uint16_t *return_screenblock_entry) {
  if (!table || !tile)
    return -1;
  
  return (table->bpp == 8) ? HashPut__8(table, tile, return_screenblock_entry) : HashPut__4(table, tile, return_screenblock_entry);
}

GBATile_HashTable *GBATileTableCreate(uint8_t bpp) {
  GBATile_HashTable *ret =
    (GBATile_HashTable*)calloc(1, sizeof(struct _GBATile_HashTable));
  /* If invalid bpp value given, default to an 8bpp tile map cfg.
   * */
  ret->bpp = (bpp&0xF3U || bpp==12) ? 8 : bpp;
  return ret;
}


int GBATileTableDealloc(GBATile_HashTable *table, uint8_t ***return_tileset) {
  uint8_t **tileset;
  HashNode *cur, *tmp_to_dealloc;
  int i, ret, tmp_tid_idx;
  if (!table)
    return -1;
  ret = table->next_tid;
  if (ret > MAX_TILE_CT || ret < 0)
    return -1;
  
  *return_tileset = (uint8_t**)calloc(ret, sizeof(void*));
  tileset = *return_tileset;
  
  for (i = 0; i < TABLE_SIZE; ++i) {
    if (NULL == (cur = table->table[i]))
      continue;
    table->table[i] = NULL;
    do {
      tmp_tid_idx = (int)cur->tid;
      assert((0 <= tmp_tid_idx) && (ret >= tmp_tid_idx));
      tileset[tmp_tid_idx] = cur->pbuf;
      tmp_to_dealloc = cur;
      cur = cur->nxt;
      free((void*)tmp_to_dealloc);
    } while (cur);
  }
  free((void*)table);
  return ret;
}

void GBATileTableDestroy(GBATile_HashTable *table) {
  HashNode *cur = NULL, *tmp = NULL;
  int i;
  if (!table)
    return;
  for (i=0; i < TABLE_SIZE; ++i) {
    if (NULL == (cur = table->table[i]))
      continue;
    table->table[i] = NULL;
    do {
      free((void*)(cur->pbuf));
      tmp = cur;
      cur = cur->nxt;
      free((void*)tmp);
    } while (cur);
  }
  free((void*)table);
}
