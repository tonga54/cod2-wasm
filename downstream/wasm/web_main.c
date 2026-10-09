#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "common_types.h"
#include <emscripten.h>

extern int WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int show);

int main(int argc, char **argv) {
  static char commandLine[4096];
  int i;
  if (EM_ASM_INT({ return new URLSearchParams(location.search).has('menuDebug'); }))
    setenv("MTRACE", "1", 1);
  if (EM_ASM_INT({ return new URLSearchParams(location.search).has('graphicsDebug'); }))
    setenv("GTRACE", "1", 1);
  if (EM_ASM_INT({ return new URLSearchParams(location.search).has('movementDebug'); }))
    setenv("PTRACE", "1", 1);
  for (i = 1; i < argc; ++i) {
    size_t used = strlen(commandLine);
    size_t needed = strlen(argv[i]) + (used ? 1 : 0);
    if (used + needed + 1 >= sizeof(commandLine)) {
      fprintf(stderr, "[cod2-wasm] command line exceeds %zu bytes\n", sizeof(commandLine));
      return 1;
    }
    if (used) strcat(commandLine, " ");
    strcat(commandLine, argv[i]);
  }
  /* Portable generated state must point at its real linear-memory owners. */
  extern unsigned char legacyHacks[32];
  extern unsigned char legacyHacksArray[1792];
  extern void *cmd_text;
  extern char cmd_texts[];
  *(void **)legacyHacks = legacyHacksArray;
  cmd_text = cmd_texts;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "[cod2-wasm] SDL initialization failed: %s\n", SDL_GetError());
    return 1;
  }
  fprintf(stdout, "[cod2-wasm] starting reconstructed native multiplayer client\n");
  return WinMain(0, 0, commandLine, 0);
}
