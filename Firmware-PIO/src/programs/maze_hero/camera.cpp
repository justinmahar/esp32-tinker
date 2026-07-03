#include "camera.h"

namespace MazeHero {
namespace {

constexpr uint8_t HERO_VIEW_PADDING_PX = 2;

int16_t approach(int16_t current, int16_t target) {
  if (current < target) {
    return current + 1;
  }
  if (current > target) {
    return current - 1;
  }
  return current;
}

int16_t mazePixelWidth(const Maze &maze) { return maze.width() * 2 + 1; }

int16_t mazePixelHeight(const Maze &maze) { return maze.height() * 2 + 1; }

int16_t heroPixelX(Coord hero) { return hero.x * 2 + 1; }

int16_t heroPixelY(Coord hero) { return hero.y * 2 + 1; }

} // namespace

void Camera::reset(const Maze &maze, uint16_t viewWidth, uint8_t viewHeight,
                   Coord hero) {
  offsetX = 0;
  offsetY = 0;
  targetX = 0;
  targetY = 0;
  updateTarget(maze, viewWidth, viewHeight, hero);
  offsetX = targetX;
  offsetY = targetY;
}

void Camera::updateTarget(const Maze &maze, uint16_t viewWidth,
                          uint8_t viewHeight, Coord hero) {
  int16_t heroX = heroPixelX(hero);
  int16_t heroY = heroPixelY(hero);

  if (viewWidth > HERO_VIEW_PADDING_PX * 2) {
    int16_t screenX = heroX - targetX;
    if (screenX >= (int16_t)viewWidth - HERO_VIEW_PADDING_PX) {
      targetX = heroX - ((int16_t)viewWidth - HERO_VIEW_PADDING_PX - 1);
    } else if (screenX < HERO_VIEW_PADDING_PX) {
      targetX = heroX - HERO_VIEW_PADDING_PX;
    }
  }

  if (viewHeight > HERO_VIEW_PADDING_PX * 2) {
    int16_t screenY = heroY - targetY;
    if (screenY >= (int16_t)viewHeight - HERO_VIEW_PADDING_PX) {
      targetY = heroY - ((int16_t)viewHeight - HERO_VIEW_PADDING_PX - 1);
    } else if (screenY < HERO_VIEW_PADDING_PX) {
      targetY = heroY - HERO_VIEW_PADDING_PX;
    }
  }

  targetX = clampX(maze, viewWidth, targetX);
  targetY = clampY(maze, viewHeight, targetY);
}

bool Camera::stepTowardTarget() {
  int16_t nextX = approach(offsetX, targetX);
  int16_t nextY = approach(offsetY, targetY);
  bool changed = nextX != offsetX || nextY != offsetY;
  offsetX = nextX;
  offsetY = nextY;
  return changed;
}

int16_t Camera::clampX(const Maze &maze, uint16_t viewWidth,
                       int16_t value) const {
  int16_t mazeWidth = mazePixelWidth(maze);
  int16_t maxOffset = mazeWidth > viewWidth ? mazeWidth - viewWidth : 0;
  if (value < 0) {
    return 0;
  }
  if (value > maxOffset) {
    return maxOffset;
  }
  return value;
}

int16_t Camera::clampY(const Maze &maze, uint8_t viewHeight,
                       int16_t value) const {
  int16_t mazeHeight = mazePixelHeight(maze);
  int16_t maxOffset = mazeHeight > viewHeight ? mazeHeight - viewHeight : 0;
  if (value < 0) {
    return 0;
  }
  if (value > maxOffset) {
    return maxOffset;
  }
  return value;
}

} // namespace MazeHero
