#include "pal_bmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ERR_PREFIX "\x1b[1;31m[Error]:\x1b[0m "
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
  /* Make sure bitmap is an 8bpp paletted bitmap. */
  if (8 != hdr[28]) {
    fprintf(stderr, ERR_PREFIX"Expected an 8bpp bitmap. Received a %ubpp "
        "bitmap\n", hdr[28]);
    free((void*)hdr);
    fclose(bmp);
    return NULL;
  }

  if ((*(uint32_t *) (hdr + 30))) {
    fprintf(stderr, ERR_PREFIX"Unsupported Bitmap pixel data compression "
        "method.\nOnly supports raw bitmap pixel data (i.e.: no compression "
        "method)\n");
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


