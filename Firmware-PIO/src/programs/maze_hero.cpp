#include "maze_hero.h"

static const char MAZE_HERO_MESSAGE[] = "Maze Hero";
static const unsigned int MAZE_HERO_SCROLL_SPEED_MS = 75;

void mazeHeroStart(const ProgramConfig &cfg) {
  (void)cfg;
  Display.displayClear();
  Display.setTextAlignment(PA_LEFT);
  Display.displayScroll(MAZE_HERO_MESSAGE, PA_LEFT, PA_SCROLL_LEFT,
                        MAZE_HERO_SCROLL_SPEED_MS);
}

void mazeHeroTick(const ProgramConfig &cfg) {
  (void)cfg;
  if (Display.displayAnimate()) {
    Display.displayReset();
  }
}
