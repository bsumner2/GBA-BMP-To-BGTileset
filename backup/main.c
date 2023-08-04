#if 0
#include "outwriter.h"
#include "parse_args.h"
#include "tiles.h"
#include <stdio.h>
#include <SDL.h>
#include <stdlib.h>
#define ERR_PREFIX "\x1b[1;31m[Error]:\x1b[0m "

int main(int argc, char **argv) {
  SDL_Event ev;
  TileSet_Data data;
  char *fpath;
  SDL_Window *win;
  SDL_Renderer *ren;
  uint8_t *currtile;
  uint16_t flags, col;
  int width, height, tx, ty, x, y;
  uint8_t bpp, r, g, b, tmp, palidx;
  if (argc != 4) {
    fprintf(stderr, ERR_PREFIX"Need to specify bpp and path to bitmap of tile "
        "Via path.\n%s [bpp] [input bmp path] [output file name]", argv[0]);
    return EXIT_FAILURE;
  }
  
  if (!set_outputfilename(argv[3])) {
    fputs(ERR_PREFIX"Failed to set output file path.\n", stderr);
    return EXIT_FAILURE;
  }
  fpath = argv[2];
  bpp = (unsigned)(argv[1][0]-'0');

  if (bpp != 4 && bpp != 8) {
    fputs(ERR_PREFIX"Bits per pixel can only be 4 or 8.\n", stderr);
    return EXIT_FAILURE;
  }

  if (0 > TileSet_Data_Create(&data, fpath, bpp)) {
    fprintf(stderr, ERR_PREFIX"Try to figure out what went wrong using gdb ig.\n");
    return EXIT_FAILURE;
  }
  width  = (data.map_width<<3);
  height = (data.map_height<<3);

  if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
    fprintf(stderr, ERR_PREFIX"SDL couldn't init. Details: %s\n",
        SDL_GetError());
    TileSet_Data_Close(&data);
    return EXIT_FAILURE;
  }
  if (!(win = SDL_CreateWindow("Tilemap", SDL_WINDOWPOS_CENTERED, 
          SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN))) {
    fprintf(stderr, ERR_PREFIX"SDL couldn't open window. Details: %s\n",
        SDL_GetError());
    TileSet_Data_Close(&data);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  if (!(ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE))) {
    fprintf(stderr, ERR_PREFIX"SDL couldn't open renderer. Details: %s\n",
        SDL_GetError());
    TileSet_Data_Close(&data);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  for (ty = 0; ty < data.map_height; ++ty) {
    for (tx = 0; tx < data.map_width; ++tx) {
      flags = data.tilemap[ty*data.map_width + tx];
      currtile = TileCpy(NULL, data.tileset[(flags&0x03FFU)], data.bpp);
      if (flags&V_FLIP_FLAG) {
        if (data.bpp==4)
          VFlipTile_4bpp(currtile);
        else
          VFlipTile_8bpp(currtile);
      }
      if (flags&H_FLIP_FLAG) {
        if (data.bpp==4)
          HFlipTile_4bpp(currtile);
        else
          HFlipTile_8bpp(currtile);
      }
      palidx = (flags>>8)&0xF0U;
      for (y = 0; y < 8; ++y) {
        if (data.bpp==4) {
          for (x = 0; x < 4; ++x) {
            tmp = currtile[(y<<2)+x];
            col = data.pal[((tmp>>4)&0x0FU)|palidx];
            r = (col&0x1FU)<<3;
            g = ((col>>5)&0x1FU)<<3;
            b = ((col>>10)&0x1FU)<<3;
            SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
            SDL_RenderDrawPoint(ren, (tx<<3)+(x<<1), (ty<<3)+y);
            col = data.pal[(tmp&0x0FU)|palidx];
            r = (col&0x1FU)<<3;
            g = ((col>>5)&0x1FU)<<3;
            b = ((col>>10)&0x1FU)<<3;
            SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
            SDL_RenderDrawPoint(ren, ((tx<<3)+(x<<1))|1, (ty<<3)+y);
          } 
        } else {
          for (x = 0; x < 8; ++x) {
            tmp = currtile[(y<<3)+x];
            col = data.pal[tmp];
            r = (col&0x1FU)<<3;
            g = ((col>>5)&0x1FU)<<3;
            b = ((col>>10)&0x1FU)<<3;
            SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
            SDL_RenderDrawPoint(ren, (tx<<3)+x, (ty<<3)+y);
          }
        }
      }
      free((void*)currtile);
    }
  }




  SDL_RenderPresent(ren);
  tmp = 1;
  while (tmp) {
    while (SDL_PollEvent(&ev)) {
      if (ev.type!=SDL_QUIT)
        continue;
      tmp = 0;
      break;
    }
  }

  /** \TODO Write tile data GBADev C code and header to output buffer file. */
  
  WriteToFile(&data);
  TileSet_Data_Close(&data);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return EXIT_SUCCESS;
}

#else 
#include "parse_args.h"
#include <stdio.h>

int main(int argc, char **argv) {
  struct opts options = ParseArgs(argc, argv);

  printf("Infile: %s\nOutdir: %s\nTarget: %s\nBit depth: %u\n", options.path_in,
      options.path_out, options.target_name, options.bpp);
  
  

  return 0;

}

#endif 
