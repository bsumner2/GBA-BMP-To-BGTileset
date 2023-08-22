#include "hashing.h"
#include <assert.h>
#include <md5.h>
#include <stdlib.h>
#include <stdio.h>

#define TABLE_SIZE 256
#define MAX_COLOR_ENTRIES 256
#define VALID_TABLE_RANGE_MASK 255

#define ERR_PREFIX "\x1b[1;31m[Error]:\x1b[0m "


typedef struct _Hex_HNode {
  struct _Hex_HNode *nxt;
  uint32_t hexcode;
  int pal_idx;
} HashNode;

struct _RGB_HashTable {
  HashNode *table[TABLE_SIZE];
  uint8_t keyset[MAX_COLOR_ENTRIES];
  int next_available_palidx, bpp, keyset_top;
}; 

uint32_t HashRGB(uint32_t hexcode, uint8_t bpp) {
  MD5_CTX context;
  uint8_t buff[MD5_DIGEST_LENGTH];
  uint32_t *tmp;
  uint32_t ret = 0;
  int i, tmp_len;
  assert(!(bpp&7U));
  MD5Init(&context);
  MD5Update(&context, ((uint8_t*) (&hexcode)), bpp>>3);
  MD5Final(buff, &context);
  tmp = (uint32_t*)buff;
  tmp_len = (MD5_DIGEST_LENGTH>>2);
  for (i=0;i<tmp_len; ++i)
    ret ^= tmp[i];
  ret = (ret&0x0000FFFF)^((ret&0xFFFF0000)>>16);
  return VALID_TABLE_RANGE_MASK&(ret^ret>>8);
}


HexCode_HashTable *HexCodeTableCreate(uint8_t bpp) {
  HexCode_HashTable *ret;
  if (bpp!=24 && bpp!=32)
    return NULL;
  ret = calloc(1, sizeof(HexCode_HashTable));
  ret->bpp = bpp;
  ret->keyset_top = -1;
  return ret;
}


int HexCodeTablePut(HexCode_HashTable *table, uint32_t hexcode) {
  HashNode *to_add, *tmp;
  int idx, palidx;
  if (!table) 
    return -1;
  idx = HashRGB(hexcode, table->bpp);
  if (!(tmp = table->table[idx])) {
    palidx = table->next_available_palidx++;
    if (palidx >= MAX_COLOR_ENTRIES)
      return -1;
    table->keyset[++table->keyset_top] = idx;
    tmp = malloc(sizeof(HashNode));
    tmp->nxt = NULL;
    tmp->hexcode = hexcode;
    tmp->pal_idx = palidx;
    table->table[idx] = tmp;
    return palidx;
  }
  
  while (tmp->nxt)
    if (tmp->hexcode==hexcode) return tmp->pal_idx;
    else tmp = tmp->nxt;
  
  if (tmp->hexcode==hexcode)
    return tmp->pal_idx;
  palidx = table->next_available_palidx++;
  if (palidx >= MAX_COLOR_ENTRIES)
    return -1;
  to_add = malloc(sizeof(HashNode));
  to_add->nxt = NULL;
  to_add->hexcode = hexcode;
  to_add->pal_idx = palidx;
  tmp->nxt = to_add;
  return palidx;
}

uint32_t *HexCodeTableDealloc(HexCode_HashTable *table, size_t *return_pallen) {
  HashNode **tbl, *cur, *tmp;
  uint32_t *ret;
  uint8_t *keys;
#if VALGRIND_BUILD
  int top, i;
#else
  int top;
#endif  /* VALGRIND_BUILD */

  if (!table || !return_pallen) {
    fprintf(stderr, ERR_PREFIX"Proper data wasn't passed through params.\n"
        "Table: %s\nPalette Length Return Address: %s\n", table?"Passed Through"
        :"Missing", return_pallen?"Passed Through":"Missing");
    return NULL;
  }
  top = table->keyset_top;
  tbl = table->table;
  keys = table->keyset;
  ret = calloc(256, sizeof(uint32_t));

  (*return_pallen) = sizeof(uint32_t)<<8;
  while (top > -1) {
    cur = tbl[keys[top]];
    tbl[keys[top--]] = NULL;
    while (cur) {
      tmp = cur->nxt;
      ret[cur->pal_idx] = cur->hexcode;
      free((void*)cur);
      cur = tmp;
    }
  }
#if VALGRIND_BUILD
  for (i = 0; i < TABLE_SIZE; ++i)
    if (table->table[i])
      fprintf(stderr, ERR_PREFIX"table[%d] was not dealloc'd.\n");
#endif  /* VALGRIND_BUILD */
  free((void*)table);
  return ret;
}

void HexCodeTableDestroy(HexCode_HashTable *table) {
  HashNode **tbl, *cur, *tmp;
  uint8_t *keys;
#if VALGRIND_BUILD
  int top, i;
#else
  int top;
#endif  /* VALGRIND_BUILD */
  if (!table)
    return;
  top = table->keyset_top;
  tbl = table->table;
  keys = table->keyset;
  while (top > -1) {
    cur = tbl[keys[top]];
    tbl[keys[top--]] = NULL;
    while (cur) {
      tmp = cur->nxt;
      free((void*)cur);
      cur = tmp;
    }
  }
#if VALGRIND_BUILD
  for (i = 0; i < TABLE_SIZE; ++i)
    if (table->table[i])
      fprintf(stderr, ERR_PREFIX"table[%d] was not dealloc'd.\n");
#endif  /* VALGRIND_BUILD */

  free((void*)table);
}


