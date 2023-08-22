#include "hashing.h"
#include "pal_bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ERR_PREFIX "\x1b[1;31m[Error]:\x1b[0m "

static Paletted_Bitmap *ParseBMPNonIndexed(Paletted_Bitmap *dest, 
    FILE *bmp, uint8_t *header, uint8_t mallocd_ret) {
  HexCode_HashTable *table;
  uint8_t *pdata;
  size_t pbuf_offset;
  size_t rowlen, padlen;
  uint32_t color;
  int i, j, bytes_per_px, row_off;
  uint8_t bpp, retidx;
  if (mallocd_ret) {
    if (dest==NULL)
      dest = (Paletted_Bitmap*)malloc(sizeof(Paletted_Bitmap));
    else
      mallocd_ret = 0;
  }
  pbuf_offset = (*(uint32_t*) (header + 10));
  bpp = header[28];
  printf("\x1b[1;34mBit Depth:\x1b[0m %d\n", bpp);
  bytes_per_px = bpp/8;
  dest->width = (*(int*) (header + 18));
  dest->height = (*(int*) (header + 22));
  if ((dest->width != 256 && dest->width != 512) ||
      (dest->height != 256 && dest->height != 512)) {
    fprintf(stderr, ERR_PREFIX"Bitmap size is not valid size for GBA tilemap\n"
        "Valid sizes are: \x1b[1;34m256x256\x1b[0m, \x1b[1;34m256x512\x1b[0m, "
        "\x1b[1;34m512x256\x1b[0m, and \x1b[1;34m512x512\x1b[0m\n");
    if (mallocd_ret)
      free((void*)dest);
    fclose(bmp);
    return NULL;
  }

  padlen = (((((size_t)bpp*dest->width) + 31)>>5)<<2)
    - (rowlen = (dest->width*bpp)>>3);
    
  if (0 > fseek(bmp, pbuf_offset, SEEK_SET)) {
    perror(ERR_PREFIX"Failed to seek bitmap data stream to pixel data.\n"
        "Details from fseek's set errno: ");
    if (mallocd_ret)
      free((void*)dest);
    fclose(bmp);
    return NULL;
  }
  if (!(table = HexCodeTableCreate(bpp))) {
    fprintf(stderr, ERR_PREFIX"Failed to allocate the hexcodes hash table.\n");
    if (mallocd_ret)
      free((void*)dest);
    fclose(bmp);
    return NULL;
  }
  pdata = (uint8_t*)calloc(dest->width*dest->height, sizeof(uint8_t));

  for (i=dest->height-1; i > -1; --i) {
    /* Since bitmap pixel arrays are stored upside-down for god knows what 
     * reason */
    row_off = i*dest->width;
    for (j=0;j<dest->width;++j) {
      color = 0;
      if ((unsigned)bytes_per_px != 
          fread(((void*) (&color)), 1, bytes_per_px, bmp)) {
        perror(ERR_PREFIX"Failed to parse color from pixel data in bitmap."
            "Details from errno set: ");
        free((void*)pdata);
        HexCodeTableDestroy(table);
        fclose(bmp);
        if (mallocd_ret)
          free((void*)dest);
        return NULL;
      }
      if (0 > (retidx = (uint8_t)HexCodeTablePut(table, color))) {
        free((void*)pdata);
        HexCodeTableDestroy(table);
        fclose(bmp);
        if (mallocd_ret)
          free((void*)dest);
        return NULL;
      }

      pdata[row_off + j] = retidx;
    }


    if (!padlen)
      continue;
    printf("\x1b[1;31mPadding Detected. Shouldn't need padding.\n\x1b[0m");
    if (0 > fseek(bmp, padlen, SEEK_CUR)) {
      perror(ERR_PREFIX"Could not seek file stream track to next line of "
          "pixel data.\nDetails from set errno: ");
      free((void*)pdata);
      HexCodeTableDestroy(table);
      if (mallocd_ret)
        free((void*)dest);
      fclose(bmp);
      return NULL;
    }
  }
  fclose(bmp);
  dest->pbuf = pdata;
  dest->pbuflen = dest->width*dest->height;
  dest->pal = HexCodeTableDealloc(table, &(dest->pallen));
  if (!(dest->pal)) {
    free((void*)pdata);
    HexCodeTableDestroy(table);
    if (mallocd_ret)
      free((void*)dest);
    return NULL;
  }
  dest->bpp = 8;
  return dest;
}



Paletted_Bitmap *ParseBMP8BPP(Paletted_Bitmap *dest, const char *srcpath) {
  FILE *bmp = fopen(srcpath, "r");
  uint8_t *hdr;
  size_t pdata_offset, lnlen, padlen;
  uint32_t hdrlen;
  int curr;
  uint8_t mallocret = (dest==NULL);
  if (!bmp) {
    perror(ERR_PREFIX"Could not open file given provided path."
        "Details from fopen errno: ");
    return NULL;
  }

  fseek(bmp, 14, SEEK_SET);
  /* Find length of DIB Header. */
  fread(((void*) &hdrlen), 4, 1, bmp);
  /* Add length of version-agnostic BMP General Header (14B) */
  hdrlen += 14;
  
  fseek(bmp, 0, SEEK_SET);
  hdr = malloc(hdrlen);

  if (hdrlen!=fread((void*)hdr, 1, hdrlen, bmp)) {
    perror(ERR_PREFIX"Could not get header data from file given."
        "Data could be corrupted. Details from fread errno: ");
    free((void*)hdr);
    fclose(bmp);
    return NULL;
  }
  /* Ensure that Bitmap header identifier field is valid to give insight as to
   * whether corruption has occurred. */
  if (0x4D42!=(*(uint16_t *) hdr)) {
    fprintf(stderr, ERR_PREFIX"Header identifier field is either an unsupported" 
        " value, or otherwise corrupted."
        "\n\x1b[1;34mExpected:\x1b[32m BM\x1b[34m"
        "\nRecieved:\x1b[32m %c%c\x1b[0m\n", (char)hdr[0], (char)hdr[1]);
    free((void*)hdr);
    fclose(bmp);
    return NULL;
  }
  pdata_offset = (*(uint32_t *) (hdr+10));

  if ((*(uint32_t *) (hdr + 30))) {
    fprintf(stderr, ERR_PREFIX"Unsupported Bitmap pixel data compression "
        "method.\nOnly supports raw bitmap pixel data (i.e.: no compression "
        "method)\n");
    free((void*)hdr);
    fclose(bmp);
    return NULL;
  }

  /* Make sure bitmap is an 8bpp paletted bitmap. */
  if (8 != hdr[28]) {
    if (hdr[28]==24 || hdr[28]==32)
      return ParseBMPNonIndexed(dest, bmp, hdr, mallocret);
    fprintf(stderr, ERR_PREFIX"Expected an 8bpp bitmap. Received a %ubpp "
        "bitmap\n", hdr[28]);
    free((void*)hdr);
    fclose(bmp);
    return NULL;
  }


  if (mallocret)
    dest = (Paletted_Bitmap*)malloc(sizeof(Paletted_Bitmap));

  /* Parse out general bitmap info. */

  dest->bpp = 8;

  dest->width = (*(int *) (hdr+18));
  dest->height = (*(int *) (hdr + 22));

  dest->pallen = (*(uint32_t *) (hdr + 46))*sizeof(uint32_t);
  if (1024UL != dest->pallen) {
    fprintf(stderr, ERR_PREFIX"Unexpected color palette table length.\n"
        "\x1b[1;34mExpected:\x1b[0m (256 possible colors) * (4 Bytes per color "
        "entry) = 1024\n\x1b[1;34mReceived:\x1b[0m %lu\n", dest->pallen);
    if (mallocret)
      free((void*)dest);
    free((void*)hdr);
    fclose(bmp);
    return NULL;
  }
  free((void*)hdr);
  hdr = NULL;
  dest->pbuflen = (size_t)(dest->width*dest->height);
  if (ftell(bmp)!=hdrlen) {
    fprintf(stderr, "\x1b[1;33m[Log]:\x1b[0m File buffer seek cursor was in an"
        "unexpected location.\n\x1b[34mExpected:\x1b[0m\t%u\n"
        "\x1b[1;34mReceived:\x1b[0m\t%lu\n", hdrlen, ftell(bmp));
    fseek(bmp, hdrlen, SEEK_SET);
  }

  dest->pal = (uint32_t*)malloc((dest->pallen));
  if (fread((void*)(dest->pal), 1, dest->pallen, bmp) != dest->pallen) {
    perror(ERR_PREFIX"Color table read error. Bitmap data may be malformed."
        "\nDetails from fread errno: ");
    free((void*)(dest->pal));
    if (mallocret)
      free((void*)dest);
    fclose(bmp);
    return NULL;
  }
  fseek(bmp, pdata_offset, SEEK_SET);
  /* 
   * If bpp guaranteed to be 8bpp, then lnlen is just width.
   * */
  lnlen = dest->width;
  padlen = lnlen&0x11UL;
  /* Reuse hdr, whose previous mem block dealloc'd, for our pixel buffer. */
  hdr = (uint8_t *)malloc(dest->pbuflen);
  dest->pbuf = hdr;
  for (curr = dest->height-1; curr > -1; --curr) {
    hdr = ((dest->pbuf) + (curr*lnlen));
    if (fread((void*)hdr, 1, lnlen, bmp) != lnlen) {
      perror(ERR_PREFIX"Could not read row from bitmap's pixel data."
          "\nDetails from fread errno: ");
      free((void*)(dest->pbuf));
      free((void*)(dest->pal));
      if (mallocret)
        free((void*)(dest));
      fclose(bmp);
      return NULL;
    }
    if (padlen)
      fseek(bmp, padlen, SEEK_CUR);
  }
  fclose(bmp);
  return dest;
}


void PalettedBitmapCloseBuffers(Paletted_Bitmap *bmp) {
  free((void*)(bmp->pal));
  free((void*)(bmp->pbuf));
}

void PalettedBitmapClose(Paletted_Bitmap *bmp) {
  PalettedBitmapCloseBuffers(bmp);
  free((void*)bmp);
}


void GBA_PalettedBMPClose(GBACompatible_Paletted_Bitmap *bmp) {
  free((void*)(bmp->pal));
  free((void*)(bmp->pbuf));
  free((void*)bmp);
}

GBACompatible_Paletted_Bitmap *ConvertPaletteBitDepth(Paletted_Bitmap *bmp) {
  GBACompatible_Paletted_Bitmap *ret;
  uint32_t col;
  uint16_t tmp;
  int i, num_cols;
  if (!bmp)
    return NULL;
  ret = (GBACompatible_Paletted_Bitmap*)
    malloc(sizeof(GBACompatible_Paletted_Bitmap));

  ret->width = bmp->width;
  ret->height = bmp->height;
  ret->bpp = bmp->bpp;
  ret->pal = malloc(256*sizeof(uint16_t));
  ret->pallen = 256*sizeof(uint16_t);
  assert((ret->pallen&1)==0);
  num_cols = 1<<(bmp->bpp);
  for (i = 0; i < num_cols; ++i) {
    col = bmp->pal[i];
    /*         BLUE             |           GREEN       |     RED           */
    tmp = (((col>>3)&0x1f)<<10) | (((col>>11)&0x1f)<<5) | ((col>>19)&0x1f);
    ret->pal[i] = tmp;
  }

  ret->pbuf = malloc((ret->pbuflen = bmp->pbuflen));
  memmove((void*)(ret->pbuf), (void*)(bmp->pbuf), ret->pbuflen);

  return ret;
}


