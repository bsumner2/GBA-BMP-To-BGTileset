#ifndef _HASHING_H_
#define _HASHING_H_
#include <stdint.h>
#include <stddef.h>

// Tile hashing
typedef struct _GBATile_HashTable GBATile_HashTable;


GBATile_HashTable *GBATileTableCreate(uint8_t bpp);


uint32_t HashTile_4bpp(uint8_t *tile);
uint32_t HashTile_8bpp(uint8_t *tile);

int GBATileTablePut(GBATile_HashTable *table, uint8_t *tile, 
    uint16_t *return_screenblock_entry);


/** 
 * \brief Dealloc the hashtable, but not the tiles in the table. Instead,
 * the tiles will be put into a contiguous array whose indices mark their
 * respective tile's TID.
 * */
int GBATileTableDealloc(GBATile_HashTable *table, uint8_t ***return_tileset);

/** 
 * \brief Use this to dealloc the hashtable and the tiles themselves. Use this
 * for cleanup when program exiting due to failure. */
void GBATileTableDestroy(GBATile_HashTable *table);

// Color table hashing

typedef struct _RGB_HashTable HexCode_HashTable;

HexCode_HashTable *HexCodeTableCreate(uint8_t bpp);

uint32_t HashRGB(uint32_t hexcode, uint8_t bpp);

int HexCodeTablePut(HexCode_HashTable *table, uint32_t hexcode);



uint32_t *HexCodeTableDealloc(HexCode_HashTable *table, size_t *return_pallen);
void HexCodeTableDestroy(HexCode_HashTable *table);


#endif  /* _HASHING_H_ */
