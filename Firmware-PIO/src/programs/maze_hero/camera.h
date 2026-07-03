#pragma once

#include "maze.h"
#include "types.h"

namespace MazeHero {

class Camera {
public:
  void reset(const Maze &maze, uint16_t viewWidth, uint8_t viewHeight,
             Coord hero);
  void updateTarget(const Maze &maze, uint16_t viewWidth, uint8_t viewHeight,
                    Coord hero);
  bool stepTowardTarget();

  int16_t x() const { return offsetX; }
  int16_t y() const { return offsetY; }

private:
  int16_t offsetX = 0;
  int16_t offsetY = 0;
  int16_t targetX = 0;
  int16_t targetY = 0;

  int16_t clampX(const Maze &maze, uint16_t viewWidth, int16_t value) const;
  int16_t clampY(const Maze &maze, uint8_t viewHeight, int16_t value) const;
};

} // namespace MazeHero
