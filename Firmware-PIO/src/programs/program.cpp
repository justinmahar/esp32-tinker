#include "program.h"

#include "fireworks.h"
#include "maze_hero.h"
#include "scroller.h"

ProgramId parseProgramId(const String &value) {
  if (value == "fireworks") {
    return ProgramId::Fireworks;
  }
  if (value == "maze_hero") {
    return ProgramId::MazeHero;
  }
  return ProgramId::Scroller;
}

const char *programIdToString(ProgramId id) {
  switch (id) {
  case ProgramId::Fireworks:
    return "fireworks";
  case ProgramId::MazeHero:
    return "maze_hero";
  case ProgramId::Scroller:
  default:
    return "scroller";
  }
}

void programStart(const ProgramConfig &cfg) {
  switch (cfg.program) {
  case ProgramId::Fireworks:
    fireworksStart(cfg);
    break;
  case ProgramId::MazeHero:
    mazeHeroStart(cfg);
    break;
  case ProgramId::Scroller:
  default:
    scrollerStart(cfg);
    break;
  }
}

void programTick(const ProgramConfig &cfg) {
  switch (cfg.program) {
  case ProgramId::Fireworks:
    fireworksTick(cfg);
    break;
  case ProgramId::MazeHero:
    mazeHeroTick(cfg);
    break;
  case ProgramId::Scroller:
  default:
    scrollerTick(cfg);
    break;
  }
}
