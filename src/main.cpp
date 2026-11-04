// grr: Gravity Rush Remastered (PS4), natively.
//
//   grr --game-dir APP0 [--param-sfo FILE] [-- GAME ARGS...]
//
// APP0 is the game's app0 (eboot.bin, sce_module/, arc/); SwARc's PS4
// platform starts the eboot with the PRXs it ships and needs at start, at
// the bases tools/lift.sh lifted them at. Arguments after -- go to the
// game's main(), which reads its boot options there as from app0/args.txt
// (FINDINGS.md, "boot options").

#include "orbis/process.h"

int main(int argc, char** argv) {
  recomp::GameConfig game;
  game.id = "grr";
  return recomp::orbis::Main(game, argc, argv);
}
