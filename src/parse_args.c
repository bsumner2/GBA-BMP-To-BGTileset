#include "parse_args.h"
#include "outwriter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
// Args:
//      --
//      --
//      
static char buff[256];
static char *tmp, *tmp2;
#define ERRMSG "\x1b[1;34m[Error]:\x1b[0m "

void parse_arg(char *arg, char *nxt_arg, int *return_i, 
    struct opts *return_opts) {
  int j, i;
  if (!arg[0]) {
    fprintf(stderr, ERRMSG"Invalid argument given");
    exit(EXIT_FAILURE);
  }
  if (arg[0]!='-') {
    for (i = 1; arg[i]; ++i) {
      if (arg[i]!='.')
        continue;
      for (j = i+1; arg[j]; ++j)
        continue;
      if (j-i!=4) {
        fprintf(stderr, "%s is not a valid path\n", arg);
      }
      if ((*((int*) (arg + i))) == (*((int*)".bmp"))) {
        return_opts->path_in = arg;
        if (arg[i-1]=='/') {
          fputs(ERRMSG"/.bmp is not a valid path.\n", stderr);
          exit(EXIT_FAILURE);
        }
        for (j = i-2; j > -1; --j)
          if (arg[j]=='/') {
            ++j;
            break;
          }
        assert(j-i < 256);
        tmp = buff;
        for (; j < i; ++j)
          *(tmp++) = arg[j];
        return;
      } else {
        fprintf(stderr, ERRMSG"Invalid argument. %s is neither a valid argument"
            "\nnor a bitmap file path.\n", arg);
        exit(EXIT_FAILURE);
      }
    }
  }
  switch (arg[1]) {
    case 'h':
      printf("\x1b[1;34mHelp:\x1b[0m This program can take an index-based, "
          "8bpp, bitmap image,\nand create a GBA-compatible background tileset,"
          "palette, and respective map.\nBased on the bitmap provided, along "
          "some supplementary information, the program outputs\n"
          "C source and header file that can be used in GBA game development.\n"
          "\x1b[1;34mUsage:\n\t\x1b[32m%s \x1b[36m[bitmap path] \x1b[3m<opts> "
          "\n\x1b[23;34mOpts:\n"
          "\t-h:\x1b[0m The help menu.\n\x1b[1;34m"
          "\t-o:\x1b[0m Specify the output directory. If no output directory is"
          "specified, then\n\t    it will default to the current working "
          "directory.\x1b[1;34m\n"
          "\t-n:\x1b[0m Specify a different name for the outputted tilemap data.\n"
          "\t    This will change the outputted files' names as well as the names\n"
          "\t    of the data constants generated within the outputted source files."
          "\n\t    If this opt isn't used, the default name will be the input bmp\n"
          "\t    file's name.\n\x1b[1;34m"
          "\t-b:\x1b[0m Specify the bit depth. Valid bit depths are 4 and 8. (Default=8)\n",
          tmp2);
      exit(EXIT_SUCCESS);
      break;
    case 'o':
      return_opts->path_out = nxt_arg;
      ++return_i[0];
      break;
    case 'n':
      for (i=0; nxt_arg[i]; ++i) {
        if (isalpha(nxt_arg[i]) || nxt_arg[i]=='_')
          continue;
        fprintf(stderr, ERRMSG"%s is an Invalid name. Can only contain"
              "alphabetic characters and underscores.\n", nxt_arg);
        exit(EXIT_FAILURE);
      }
      return_opts->target_name = nxt_arg;
      ++return_i[0];
      break;
    case 'b':
      assert(!nxt_arg[1] && (*nxt_arg=='4' || *nxt_arg=='8'));
      return_opts->bpp = *nxt_arg - '0';
      ++return_i[0];
      break;
  }
}

static char *arg_strdup(const char *restrict src, int *return_len) {
  char *ret;
  int i;
  for (i = 0; src[i]; ++i)
    continue;
  ret = (char*)malloc(sizeof(char)*(i+1));
  if (!ret) {
    *return_len = -1;
    return NULL;
  }
  for (i=0; src[i]; ++i)
    ret[i] = src[i];
  ret[i] = '\0';
  *return_len = i;
  return ret;
}

static void clrbuff(void) {
  // buff is 256 bytes so can clear by overwriting buff typecasted to u64
  // over 4 indices.
  uint64_t *bufflong = (uint64_t*)buff;
  int i;
  for (i = 0; i < 4; ++i)
    bufflong[i] = 0ULL;
}


struct opts ParseArgs(int argc, char **argv) {
  tmp2 = argv[0];
  struct opts ret = {NULL, "./", buff, 8};
  char *outname;
  int i, j;
  for (i = 1; i < argc-1; ++i)
    parse_arg(argv[i], argv[i+1], &i, &ret);
  if ((*((unsigned short *) (argv[argc-1]))) == (*((unsigned short *) "-h"))) {
    parse_arg(argv[argc-1], NULL, NULL, NULL);
    exit(EXIT_SUCCESS);
  }


  if (ret.path_in==NULL) {
    fprintf(stderr, ERRMSG"An input bitmap file path was never given.\n"
        "Please try the command, \x1b[1;34m\"%s -h\"\x1b[0m to view the help"
        " dialogue.\n", argv[0]);
    exit(EXIT_FAILURE);
  }


  outname = arg_strdup(ret.target_name, &j);
  clrbuff();
  for (i = 0; ret.path_out[i]; ++i)
    continue;
  assert((i+j+3) < 255);
  if (ret.path_out[i-1]=='/')
    i = sprintf(buff, "%s%s", ret.path_out, outname);
  else
    i = sprintf(buff, "%s/%s", ret.path_out, outname);
  setup_outbuff_paths(buff, outname, i+2, j);
  free((void*)outname);

  return ret;
}
