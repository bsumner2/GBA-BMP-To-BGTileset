#ifndef _TILES_H_
#define _TILES_H_

#include "pal_bmp.h"
#include <stdint.h>


#define H_FLIP_FLAG 0x0400
#define V_FLIP_FLAG 0x0800

#define TILE_SET_PAL_LEN 512


typedef struct _TileSet_Info {
  uint16_t *tilemap;  /*!< Tile map data. 
                           (Screen entry bitfields: 
                                PPPPVHTTTTTTTTTT.
                                P = palette bank no.
                                    (ignored for 8bpp tilemaps).
                                V = Vflip flag.
                                H = Hflip flag.
                                T = Tile ID (10b=at most, 1024 unique tiles).
                                )
                                */
  uint16_t *pal;  /*\< 16b-depth color palette. */
  uint8_t **tileset;  /*\< The set of tiles. Each u8 ptr is 1 tile's pbuf. */
  int tileset_len;  /*\< Number of tiles in tile set. */
  int bpp;  /*\< Bit depth of each tile. (only [4|8] bpp allowed). */
  int map_width;  /*\< Tile map width (in tiles). */
  int map_height;  /*\< Tile map height (in tiles). */
} TileSet_Data;

/** 
 * \brief Initialize a tileset data bookkeeping object,
 * pass by reference address, given the parameterized bitmap.
 * \param[out] ref_addr As the name suggests, pass address of static/local 
 * reference, and the data will be put in there.
 * \param[in] bmp The bitmap that the tileset and map will be generated from.
 * */
int TileSet_Data_Init(TileSet_Data *ref_addr, 
    GBACompatible_Paletted_Bitmap *bmp, uint8_t bpp);


/** 
 * \brief Dealloc all dynamic struct members. Does NOT dealloc the param'd ptr.
 * \param[out] ref_addr Address of a static TileSet_Data obj reference.
 * */
void TileSet_Data_Close(TileSet_Data *ref_addr);

/** 
 * \brief Copy a \<bpp\>bpp tile.
 * \param[out] dest Pointer to destination tile pixel data. If NULL, returned 
 * ptr will be a malloc'd tile; otherwise returned ptr will be dest.
 * \param[in] src The source information to copy into the return tile.
 * If this is NULL, fn returns NULL right away.
 * \param[in] bpp The bits per pixel of the tile. Determines how much
 * data needs to be copied, and size of returned tile if dest is NULL. If
 * bpp neither == 4 or 8, then returns NULL right away.
 * \return uint8_t* pbuf Will either be dest or a malloc'd ptr if the dest
 * parameter was passed through as NULL. Returns NULL if src is NULL.
 * */
uint8_t *TileCpy(uint8_t *dest, uint8_t *src, uint8_t bpp);



int TileSet_Data_Create(TileSet_Data *ref, const char *bitmap_path, 
    uint8_t bpp);




/** 
 * \brief compare two 4bpp tiles to see if they have the same index values.
 * \return 0 if perfect match. otherwise, 1.
 * */
int TileCmp_4bpp(uint8_t *comparand, uint8_t *comparator);
/** 
 * \brief compare two 8bpp tiles to see if they have the same index values.
 * \return 0 if perfect match. otherwise, 1.
 * */
int TileCmp_8bpp(uint8_t *comparand, uint8_t *comparator);


/** 
 * \brief Flip a 4bpp tile vertically, (in-place transformation).
 * */
void VFlipTile_4bpp(uint8_t *tile);
/** 
 * \brief Flip an 8bpp tile vertically, in-place transformation).
 * */
void VFlipTile_8bpp(uint8_t *tile);


/** 
 * \brief Flip a 4bpp tile horizontally, (in-place transformation).
 * */
void HFlipTile_4bpp(uint8_t *tile);
/** 
 * \brief Flip an 8bpp tile horizontally, (in-place transformation).
 * */
void HFlipTile_8bpp(uint8_t *tile);

/** 
 * \brief Create a malloc'd, vertically-flipped copy of the 4bpp tile
 * param'd in.
 * */
uint8_t *VFlipped_4bpp(uint8_t *tile);

/** 
 * \brief Create a malloc'd, vertically-flipped copy of the 8bpp tile
 * param'd in.
 * */
uint8_t *VFlipped_8bpp(uint8_t *tile);

/** 
 * \brief Create a malloc'd, horizontally-flipped copy of the 4bpp tile
 * param'd in.
 * */
uint8_t *HFlipped_4bpp(uint8_t *tile);

/** 
 * \brief Create a malloc'd, horizontally-flipped copy of the 8bpp tile
 * param'd in.
 * */
uint8_t *HFlipped_8bpp(uint8_t *tile);

#endif  /* _TILES_H_ */
