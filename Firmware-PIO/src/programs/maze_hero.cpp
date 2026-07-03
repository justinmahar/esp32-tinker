#include "maze_hero.h"

#include "maze_hero/maze_hero_game.h"

void mazeHeroStart(const ProgramConfig &cfg) {
  MazeHero::start(cfg);
}

void mazeHeroTick(const ProgramConfig &cfg) {
  MazeHero::tick(cfg);
}
