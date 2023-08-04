#ifndef _PAL_BMP_H_
#define _PAL_BMP_H_

#include <stddef.h>
#include <stdint.h>

typedef struct _Indexed_BMP {
  uint32_t *pal;  /*!< 24bpp (packed along 4B word boundaries) color table. 
                       (Fmt = RGBX | 8b per channel) */
  uint8_t *pbuf;  /*!< Pixel buffer. Made up of <bpp> bit-long pixel entries. */
  size_t pallen;  /*!< Length of pal in Bytes. */
  size_t pbuflen;  /*!< Length of pbuf in Bytes. */
  int width, height;  /*!< Dimensions of bmp (in pixels) */
  uint8_t bpp;  /*!< Amount of bits per pixel entry in pbuf */
} Paletted_Bitmap;


typedef struct _GBA_Compatible_BMP {
  uint16_t *pal;  /*!< 16bpp (packed along 2B hword boundaries) color table. 
                       (Fmt = XBGR | 4b per channel) */
  uint8_t *pbuf;
  size_t pallen;
  size_t pbuflen;
  int width, height;
  uint8_t bpp;
} GBACompatible_Paletted_Bitmap;



/**! 
 * \brief Create a Paletted_Bitmap object inst. given file path to bitmap.
 * \details Either store the information parsed from bitmap file path in a
 * dynamically-allocated Paletted_Bitmap obj pointer, or store it in the address
 * of a static reference to a bitmap that's passed through via dest param.
 * \param[out] dest If NULL, returned Paletted_Bitmap ptr will be malloc'd, 
 * otherwise just returns dest itself.
 * \param[in] srcpath File path to the bitmap whose data will be sourced for the
 * Paletted_Bitmap instance to return.
 * \return Either malloc'd Paletted_Bitmap pointer, or same pointer passed thru
 * via dest, if dest is a valid ptr.
 * */
Paletted_Bitmap *ParseBMP8BPP(Paletted_Bitmap *dest, const char *srcpath);

/** 
 * \brief Create new bitmap whose palette's color depth is downscaled to 16b,
 * instead of 24.
 * \details Takes param'd bitmap and returns a bitmap with same pixel indexes.
 * Only difference is the palette's color space only has 16b of depth,
 * as opposed to 24. This makes the data compatible with GBA's graphics 
 * rendering hardware. */
GBACompatible_Paletted_Bitmap *ConvertPaletteBitDepth(Paletted_Bitmap *bmp);

/** 
 * \brief If the struct itself was not dynamically allocated, only free the
 * buffers using this function.
 * */
void PalettedBitmapCloseBuffers(Paletted_Bitmap *bmp);

/** 
 * \brief If the struct itself was dynamically alloc'd along with its buffers.
 * */
void PalettedBitmapClose(Paletted_Bitmap *bmp);

void GBA_PalettedBMPClose(GBACompatible_Paletted_Bitmap *bmp);



#endif  /* _PAL_BMP_H_ */
