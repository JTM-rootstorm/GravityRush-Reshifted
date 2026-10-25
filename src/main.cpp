// grr: Gravity Rush Remastered (PS4), natively.
//
//   grr [--game-dir DIR]
//
// DIR is the game's app0 (eboot.bin, sce_module/, arc/). Defaults to
// $GRR_GAME_DIR. Set GRR_TRACE=1 to log every HLE call.

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "orbis/process.h"
#include "runtime/log.h"

int main(int argc, char** argv) {
  std::string game_dir;
  if (const char* e = std::getenv("GRR_GAME_DIR")) game_dir = e;
  for (int i = 1; i < argc; i++) {
    if (!std::strcmp(argv[i], "--game-dir") && i + 1 < argc) {
      game_dir = argv[++i];
    } else {
      std::fprintf(stderr, "usage: %s [--game-dir DIR]\n", argv[0]);
      return 2;
    }
  }
  if (game_dir.empty()) {
    std::fprintf(stderr, "grr: pass --game-dir or set GRR_GAME_DIR\n");
    return 2;
  }
  while (game_dir.size() > 1 && game_dir.back() == '/') game_dir.pop_back();
  if (const char* t = std::getenv("GRR_TRACE")) recomp::g_trace = *t && *t != '0';

  recomp::orbis::Config config;
  config.eboot = game_dir + "/eboot.bin";
  struct stat st;
  if (stat(config.eboot.c_str(), &st) != 0) {
    std::fprintf(stderr, "grr: %s not found\n", config.eboot.c_str());
    return 2;
  }
  // The PRXs the eboot needs at start that the game ships, lifted with it
  // at these bases (tools/lift.sh), in initialisation order: libc needs
  // Fios2.
  config.prxs = {{game_dir + "/sce_module/libSceFios2.prx", 0x800000000},
                 {game_dir + "/sce_module/libc.prx", 0x810000000}};
  config.args = {"/app0/eboot.bin"};

  recomp::GameConfig game;
  game.id = "grr";
  return recomp::orbis::RunProcess(game, game_dir, config);
}
