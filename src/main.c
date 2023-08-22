#include "SDL_keycode.h"
#include "SDL_render.h"
#include "SDL_video.h"
#include "outwriter.h"
#include "parse_args.h"
#include "tiles.h"
#include <stdio.h>
#include <SDL.h>
#include <stdlib.h>
#define ERR_PREFIX "\x1b[1;31m[Error]:\x1b[0m "

#define VALGRIND_BUILD 0

#if !VALGRIND_BUILD
static void DisplayTileMap(TileSet_Data *data, SDL_Window **window,
    SDL_Renderer **renderer) {
  SDL_Rect drw_rct;
  SDL_Window *win;
  SDL_Renderer *ren;
  uint8_t *currtile;
  uint16_t flags, col;
  int width, height, tx, ty, x, y;
  uint8_t r, g, b, tmp, palidx;
  width = (data->map_width<<4);
  height = (data->map_height<<4);

  if (!(win = SDL_CreateWindow("Tilemap", SDL_WINDOWPOS_CENTERED, 
          SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN))) {
    fprintf(stderr, ERR_PREFIX"SDL couldn't open window. Details: %s\n",
        SDL_GetError());
    TileSet_Data_Close(data);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }


  if (!(ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE))) {
    fprintf(stderr, ERR_PREFIX"SDL couldn't open renderer. Details: %s\n",
        SDL_GetError());
    TileSet_Data_Close(data);
    SDL_DestroyWindow(win);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }
  if (*renderer != NULL)
    SDL_DestroyRenderer(*renderer);
  if (*window != NULL)
    SDL_DestroyWindow(*window);
  *renderer = ren;
  *window = win;
  drw_rct.h = 2;
  drw_rct.w = 2;
  for (ty = 0; ty < data->map_height; ++ty) {
    for (tx = 0; tx < data->map_width; ++tx) {
      flags = data->tilemap[ty*data->map_width + tx];
      currtile = TileCpy(NULL, data->tileset[(flags&0x03FFU)], data->bpp);
      if (flags&V_FLIP_FLAG) {
        if (data->bpp==4)
          VFlipTile_4bpp(currtile);
        else
          VFlipTile_8bpp(currtile);
      }
      if (flags&H_FLIP_FLAG) {
        if (data->bpp==4)
          HFlipTile_4bpp(currtile);
        else
          HFlipTile_8bpp(currtile);
      }
      palidx = (flags>>8)&0xF0U;
      for (y = 0; y < 8; ++y) {

        drw_rct.y = ((ty<<3)+y)<<1;
        if (data->bpp==4) {
          for (x = 0; x < 4; ++x) {
            tmp = currtile[(y<<2)+x];
            col = data->pal[((tmp>>4)&0x0FU)|palidx];
            r = (col&0x1FU)<<3;
            g = ((col>>5)&0x1FU)<<3;
            b = ((col>>10)&0x1FU)<<3;
            SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
            drw_rct.x = ((tx<<3)+(x<<1))<<1;
            SDL_RenderDrawRect(ren, &drw_rct);
            col = data->pal[(tmp&0x0FU)|palidx];
            r = (col&0x1FU)<<3;
            g = ((col>>5)&0x1FU)<<3;
            b = ((col>>10)&0x1FU)<<3;
            SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
            drw_rct.x += 2;
            SDL_RenderFillRect(ren, &drw_rct);
          }
        } else {
          for (x = 0; x < 8; ++x) {
            tmp = currtile[(y<<3)+x];
            col = data->pal[tmp];
            r = (col&0x1FU)<<3;
            g = ((col>>5)&0x1FU)<<3;
            b = ((col>>10)&0x1FU)<<3;
            SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
            drw_rct.x = ((tx<<3)+x)<<1;
            SDL_RenderFillRect(ren, &drw_rct);
          }
        }
      }
      free((void*)currtile);
    }
  }




  SDL_RenderPresent(ren);

}

static void DisplayTiles(TileSet_Data *data, SDL_Window **window,
    SDL_Renderer **renderer, uint8_t currpb, uint8_t modeswitch) {
  SDL_Rect rect = {0, 0, 4, 4};
  SDL_Window *win;
  SDL_Renderer *ren;
  uint8_t *currtile;
  int i, xoff = 0, yoff = 0, x, y;
  uint16_t col;
  uint8_t r, g, b, palidx;
  
  if (modeswitch) {
    if (*renderer != NULL)
      SDL_DestroyRenderer(*renderer);
    if (*window != NULL)
      SDL_DestroyWindow(*window);

    if (!(win = SDL_CreateWindow("Tileset", SDL_WINDOWPOS_CENTERED, 
            SDL_WINDOWPOS_CENTERED, 1024, 1024, SDL_WINDOW_VULKAN))) {
      fprintf(stderr, ERR_PREFIX"SDL couldn't open window. Details: %s\n",
          SDL_GetError());
      TileSet_Data_Close(data);
      SDL_Quit();
      exit(EXIT_FAILURE);
    }

    if (!(ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE))) {
      fprintf(stderr, ERR_PREFIX"SDL couldn't open renderer. Details: %s\n",
      SDL_GetError());
      TileSet_Data_Close(data);
      SDL_DestroyWindow(win);
      SDL_Quit();
      exit(EXIT_FAILURE);
    }
    
    *renderer = ren;
    *window = win;
  } else {
    win = *window;
    ren = *renderer;
    SDL_RenderClear(ren);
  }
  currpb<<=4;
  currpb&=0xF0;



  for (i = 0; i < data->tileset_len; ++i, xoff += 32) {
    if (i && !(i&0x1F)) {
      yoff += 32;
      xoff = 0;
    }

    currtile = data->tileset[i];
    if (data->bpp == 4) {
      for (y = 0; y < 8; ++y) {
        rect.y = yoff + (y<<2);
        for (x = 0; x < 4; ++x) {
          palidx = currtile[(y<<2) + x];
          col = data->pal[((palidx>>4)&0x0F)|currpb];
          r = (col&0x001F)<<3;
          g = (col&0x03E0)>>2;
          b = (col&0x7C00)>>7;
          rect.x = xoff + (x<<3);
          SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
          SDL_RenderFillRect(ren, &rect);

          col = data->pal[((palidx)&0x0F)|currpb];
          r = (col&0x001F)<<3;
          g = (col&0x03E0)>>2;
          b = (col&0x7C00)>>7; 
          rect.x += 4;
          SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
          SDL_RenderFillRect(ren, &rect);
        }
      }
    } else {
      for (y = 0; y < 8; ++y) {
        rect.y = yoff + (y<<2);
        for (x=0; x<8; ++x) {
          palidx = currtile[(y<<3)+x];
          col = data->pal[palidx];
          r = (col&0x001F)<<3;
          g = (col&0x03E0)>>2;
          b = (col&0x7C00)>>7;
          rect.x = xoff + (x<<2);
          SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
          SDL_RenderFillRect(ren, &rect);
        }
      }
    }
  }

  SDL_RenderPresent(ren);
}

static void DisplayPalette(TileSet_Data *data, SDL_Window **window,
    SDL_Renderer **renderer) {
  SDL_Window *win;
  SDL_Renderer *ren;
  uint16_t *palrow;
  SDL_Rect rct = {0,0, 16, 16};
  int i, j;
  uint16_t col;
  uint8_t r, g, b;
  
  if (*renderer != NULL)
    SDL_DestroyRenderer(*renderer);
  if (*window != NULL)
    SDL_DestroyWindow(*window);
  if (!(win = SDL_CreateWindow("Palette", SDL_WINDOWPOS_CENTERED, 
          SDL_WINDOWPOS_CENTERED, 256, 256, SDL_WINDOW_VULKAN))) {
      fprintf(stderr, ERR_PREFIX"SDL couldn't open window. Details: %s\n",
          SDL_GetError());
      TileSet_Data_Close(data);
      SDL_Quit();
      exit(EXIT_FAILURE);
  }

  if (!(ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE))) {
      fprintf(stderr, ERR_PREFIX"SDL couldn't open renderer. Details: %s\n",
      SDL_GetError());
      TileSet_Data_Close(data);
      SDL_DestroyWindow(win);
      SDL_Quit();
      exit(EXIT_FAILURE);
  }

  *renderer = ren;
  *window = win;


  for (i = 0; i < 16; ++i) {
    palrow = ((data->pal) + (rct.y = i<<4));
    for (j = 0; j < 16; ++j) {
      col = palrow[j];
      rct.x = j<<4;
      r = (col&0x001F)<<3;
      g = (col&0x03E0)>>2;
      b = (col&0x7C00)>>7;
      SDL_SetRenderDrawColor(ren, r, g, b, 0xFF);
      SDL_RenderFillRect(ren, &rct);
      SDL_SetRenderDrawColor(ren, 0xFF, 0xFF, 0xFF, 0xFF);
      SDL_RenderDrawRect(ren, &rct);
    }
  }
  SDL_RenderPresent(ren);
}








#define MODE_MAP 1U
#define MODE_PAL 2U
#define MODE_TILES 4U
#define VALID_MODES_MASK 7U  /* Derived from MODE_MAP|MODE_PAL|MODE_TILES =
                              * 1|2|4 = 0b001|0b010|0b100 = 0b111 = 7 */


static uint8_t handle_keypress(SDL_Event ev, uint8_t mode_bf, uint8_t *palno) {
  switch (ev.key.keysym.sym) {
    case SDLK_LEFT:
      mode_bf<<=1;
      if (!(mode_bf&VALID_MODES_MASK))
        mode_bf = MODE_MAP;
      printf("Mode bitfield = %d\n", mode_bf);
      return mode_bf;
    case SDLK_RIGHT:
      mode_bf>>=1;
      if (!(mode_bf&VALID_MODES_MASK))
        mode_bf = MODE_TILES;
      return mode_bf;
    case SDLK_UP:
      if (mode_bf==MODE_TILES && palno!=NULL)
        *palno = ((*palno)-1)&0xFF;
      return 0;
    case SDLK_DOWN:
      if (mode_bf==MODE_TILES && palno!=NULL)
        *palno = ((*palno)+1)&0xFF;
      return 0;
    default:
      return 0;
  }
}
#endif

int main(int argc, char **argv) {
#if !VALGRIND_BUILD
  SDL_Event ev;
  TileSet_Data data;
  struct opts options = ParseArgs(argc, argv);
  SDL_Window *win = NULL;
  SDL_Renderer *ren = NULL;
  uint8_t tmp, mode_bfield = MODE_MAP, currpal, tmp2, tmp3;
  uint8_t key_currently_down = 0;
#else
  TileSet_Data data;
  struct opts options = ParseArgs(argc, argv);
#endif
  if (0 > TileSet_Data_Create(&data, options.path_in, options.bpp)) {
    fprintf(stderr, ERR_PREFIX"Try to figure out what went wrong using gdb ig.\n");
    return EXIT_FAILURE;
  }
#if !VALGRIND_BUILD
  if (0 > SDL_Init(SDL_INIT_EVERYTHING)) {
    fprintf(stderr, ERR_PREFIX"SDL couldn't init. Details: %s\n",
        SDL_GetError());
    TileSet_Data_Close(&data);
    return EXIT_FAILURE;
  }

  DisplayTileMap(&data, &win, &ren); 
  
  tmp = 1;
  while (tmp) {
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
        case SDL_QUIT:
          tmp = 0;
          break;
        case SDL_KEYDOWN:
          if (key_currently_down)
            break;
          tmp3 = currpal;
          tmp2 = handle_keypress(ev, mode_bfield, 
              (data.bpp&4) ? &currpal : NULL);
          if (tmp2) {
            key_currently_down = 1;
            mode_bfield = tmp2;
            switch (mode_bfield) {
              case MODE_MAP:
                DisplayTileMap(&data, &win, &ren);
                break;
              case MODE_PAL:
                DisplayPalette(&data, &win, &ren);
                break;
              case MODE_TILES:
                DisplayTiles(&data, &win, &ren, (currpal = 0), 1);
                break;
            }
            break;
          } else {
            if (currpal!=tmp3)
              DisplayTiles(&data, &win, &ren, currpal, 0);
            break;
          }
          break;
        case SDL_KEYUP:
          if (!key_currently_down)
            break;
          switch (ev.key.keysym.sym) {
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_UP:
            case SDLK_DOWN:
              key_currently_down = 0;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
    }
  }
  /** \TODO Write tile data GBADev C code and header to output buffer file. */
  
  WriteToFile(&data);
  TileSet_Data_Close(&data);
  if (ren)
    SDL_DestroyRenderer(ren);
  if (win)
    SDL_DestroyWindow(win);
  SDL_Quit();
#else
  WriteToFile(&data);
  TileSet_Data_Close(&data);
#endif
  return EXIT_SUCCESS;
}
