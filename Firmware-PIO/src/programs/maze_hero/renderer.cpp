#include "renderer.h"

#include "../program.h"

namespace MazeHero {
namespace {

MD_MAX72XX *matrix() { return Display.getGraphicObject(); }

constexpr int16_t CLOSEUP_CELL_PITCH = 10;
constexpr int16_t CLOSEUP_SAMPLE_RADIUS_CELLS = 10;
constexpr int16_t CLOSEUP_WALL_HALF_THICKNESS = 1;
constexpr uint16_t CLOSEUP_START_ZOOM_Q8 = 2U * 256U;
constexpr uint16_t CLOSEUP_HOLD_ZOOM_Q8 = 7U * 256U;

void beginFrame() {
  matrix()->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  matrix()->clear();
}

void endFrame() {
  matrix()->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix()->update();
}

void setPixel(int16_t row, int16_t col, bool on = true) {
  if (row < 0 || row >= DISPLAY_HEIGHT || col < 0 ||
      col >= matrix()->getColumnCount()) {
    return;
  }
  matrix()->setPoint(row, col, on);
}

void clearPixel(int16_t row, int16_t col) { setPixel(row, col, false); }

int16_t mazePixelWidth(const Maze &maze) { return maze.width() * 2 + 1; }

int16_t mazePixelHeight(const Maze &maze) { return maze.height() * 2 + 1; }

int16_t cellCenterX(Coord coord) { return coord.x * 2 + 1; }

int16_t cellCenterY(Coord coord) { return coord.y * 2 + 1; }

int16_t horizontalMargin(const Maze &maze, uint16_t viewWidth) {
  int16_t width = mazePixelWidth(maze);
  return width < viewWidth ? (viewWidth - width) / 2 : 0;
}

int16_t verticalMargin(const Maze &maze, uint8_t viewHeight) {
  int16_t height = mazePixelHeight(maze);
  return height < viewHeight ? (viewHeight - height) / 2 : 0;
}

int16_t screenX(const Maze &maze, const Camera &camera, int16_t worldX,
                uint16_t viewWidth) {
  return worldX - camera.x() + horizontalMargin(maze, viewWidth);
}

int16_t screenY(const Maze &maze, const Camera &camera, int16_t worldY,
                uint8_t viewHeight) {
  return worldY - camera.y() + verticalMargin(maze, viewHeight);
}

void drawWorldPixel(const Maze &maze, const Camera &camera, int16_t worldY,
                    int16_t worldX, uint16_t viewWidth, uint8_t viewHeight) {
  setPixel(screenY(maze, camera, worldY, viewHeight),
           screenX(maze, camera, worldX, viewWidth));
}

void drawHorizontalWall(const Maze &maze, const Camera &camera, int16_t worldY,
                        int16_t centerX, uint16_t viewWidth,
                        uint8_t viewHeight) {
  for (int16_t x = centerX - 1; x <= centerX + 1; x++) {
    drawWorldPixel(maze, camera, worldY, x, viewWidth, viewHeight);
  }
}

void drawVerticalWall(const Maze &maze, const Camera &camera, int16_t worldX,
                      int16_t centerY, uint16_t viewWidth,
                      uint8_t viewHeight) {
  for (int16_t y = centerY - 1; y <= centerY + 1; y++) {
    drawWorldPixel(maze, camera, y, worldX, viewWidth, viewHeight);
  }
}

void drawDiscoveredWalls(const Maze &maze, const FogOfWar &fog,
                         const Camera &camera, Coord coord,
                         uint16_t viewWidth, uint8_t viewHeight) {
  if (!fog.isDiscovered(maze, coord)) {
    return;
  }

  int16_t centerX = cellCenterX(coord);
  int16_t centerY = cellCenterY(coord);

  if (maze.hasWall(coord, Direction::North)) {
    drawHorizontalWall(maze, camera, centerY - 1, centerX, viewWidth,
                       viewHeight);
  }
  if (maze.hasWall(coord, Direction::East)) {
    drawVerticalWall(maze, camera, centerX + 1, centerY, viewWidth, viewHeight);
  }
  if (maze.hasWall(coord, Direction::South)) {
    drawHorizontalWall(maze, camera, centerY + 1, centerX, viewWidth,
                       viewHeight);
  }
  if (maze.hasWall(coord, Direction::West)) {
    drawVerticalWall(maze, camera, centerX - 1, centerY, viewWidth, viewHeight);
  }
}

uint8_t progressQ8(unsigned long elapsedMs, unsigned long durationMs) {
  if (durationMs == 0 || elapsedMs >= durationMs) {
    return 255;
  }
  return (uint8_t)((elapsedMs * 255UL) / durationMs);
}

uint8_t easeInOutQ8(uint8_t value) {
  uint32_t t = value;
  return (uint8_t)((t * t * (765UL - 2UL * t) + 32512UL) / 65025UL);
}

uint16_t lerpQ8(uint16_t from, uint16_t to, uint8_t progress) {
  int32_t delta = (int32_t)to - (int32_t)from;
  return (uint16_t)((int32_t)from + (delta * progress) / 255L);
}

int16_t closeupCellCenterX(Coord coord) {
  return (int16_t)coord.x * CLOSEUP_CELL_PITCH;
}

int16_t closeupCellCenterY(Coord coord) {
  return (int16_t)coord.y * CLOSEUP_CELL_PITCH;
}

int16_t screenToWorld(int16_t doubledScreenDelta, int16_t focus,
                      uint16_t zoomQ8) {
  return focus +
         (int32_t)doubledScreenDelta * CLOSEUP_CELL_PITCH * 128L / zoomQ8;
}

bool insideRange(int16_t value, int16_t start, int16_t end) {
  return value >= start && value <= end;
}

int16_t maxInt16(int16_t a, int16_t b) { return a > b ? a : b; }

int16_t minInt16(int16_t a, int16_t b) { return a < b ? a : b; }

int16_t lerpInt16(int16_t from, int16_t to, uint8_t progress) {
  int32_t delta = (int32_t)to - (int32_t)from;
  return (int16_t)((int32_t)from + (delta * progress) / 255L);
}

int16_t oldMazePixelToCloseup(int16_t oldWorld) {
  return ((oldWorld - 1) * CLOSEUP_CELL_PITCH) / 2;
}

int16_t playCameraFocusX(const Maze &maze, const Camera &camera,
                         uint16_t viewWidth) {
  int16_t oldWorldX =
      camera.x() + (int16_t)(viewWidth / 2) - horizontalMargin(maze, viewWidth);
  return oldMazePixelToCloseup(oldWorldX);
}

int16_t playCameraFocusY(const Maze &maze, const Camera &camera,
                         uint8_t viewHeight) {
  int16_t oldWorldY =
      camera.y() + (int16_t)(viewHeight / 2) - verticalMargin(maze, viewHeight);
  return oldMazePixelToCloseup(oldWorldY);
}

Coord focusCoord(const Maze &maze, int16_t focusX, int16_t focusY) {
  int16_t x = (focusX + CLOSEUP_CELL_PITCH / 2) / CLOSEUP_CELL_PITCH;
  int16_t y = (focusY + CLOSEUP_CELL_PITCH / 2) / CLOSEUP_CELL_PITCH;
  x = maxInt16(0, minInt16((int16_t)maze.width() - 1, x));
  y = maxInt16(0, minInt16((int16_t)maze.height() - 1, y));
  return {(uint8_t)x, (uint8_t)y};
}

bool insideHorizontalWall(int16_t worldX, int16_t worldY, int16_t centerX,
                          int16_t wallY) {
  return insideRange(worldY, wallY - CLOSEUP_WALL_HALF_THICKNESS,
                     wallY + CLOSEUP_WALL_HALF_THICKNESS) &&
         insideRange(worldX, centerX - CLOSEUP_CELL_PITCH / 2,
                     centerX + CLOSEUP_CELL_PITCH / 2);
}

bool insideVerticalWall(int16_t worldX, int16_t worldY, int16_t wallX,
                        int16_t centerY) {
  return insideRange(worldX, wallX - CLOSEUP_WALL_HALF_THICKNESS,
                     wallX + CLOSEUP_WALL_HALF_THICKNESS) &&
         insideRange(worldY, centerY - CLOSEUP_CELL_PITCH / 2,
                     centerY + CLOSEUP_CELL_PITCH / 2);
}

bool insideCloseupWallSegment(const Maze &maze, Coord coord, int16_t worldX,
                              int16_t worldY) {
  int16_t centerX = closeupCellCenterX(coord);
  int16_t centerY = closeupCellCenterY(coord);
  int16_t halfPitch = CLOSEUP_CELL_PITCH / 2;

  if (maze.hasWall(coord, Direction::North) &&
      insideHorizontalWall(worldX, worldY, centerX, centerY - halfPitch)) {
    return true;
  }
  if (maze.hasWall(coord, Direction::East) &&
      insideVerticalWall(worldX, worldY, centerX + halfPitch, centerY)) {
    return true;
  }
  if (maze.hasWall(coord, Direction::South) &&
      insideHorizontalWall(worldX, worldY, centerX, centerY + halfPitch)) {
    return true;
  }
  if (maze.hasWall(coord, Direction::West) &&
      insideVerticalWall(worldX, worldY, centerX - halfPitch, centerY)) {
    return true;
  }
  return false;
}

bool isCloseupWall(const Maze &maze, Coord center, int16_t worldX,
                   int16_t worldY) {
  int16_t startY = maxInt16(0, (int16_t)center.y - CLOSEUP_SAMPLE_RADIUS_CELLS);
  int16_t endY = minInt16((int16_t)maze.height() - 1,
                          (int16_t)center.y + CLOSEUP_SAMPLE_RADIUS_CELLS);
  int16_t startX = maxInt16(0, (int16_t)center.x - CLOSEUP_SAMPLE_RADIUS_CELLS);
  int16_t endX = minInt16((int16_t)maze.width() - 1,
                          (int16_t)center.x + CLOSEUP_SAMPLE_RADIUS_CELLS);

  for (int16_t y = startY; y <= endY; y++) {
    for (int16_t x = startX; x <= endX; x++) {
      Coord coord = {(uint8_t)x, (uint8_t)y};
      if (insideCloseupWallSegment(maze, coord, worldX, worldY)) {
        return true;
      }
    }
  }
  return false;
}

void drawCloseupMaze(const Maze &maze, int16_t focusX, int16_t focusY,
                     uint16_t zoomQ8, uint16_t width, uint8_t height) {
  int16_t centerCol2 = (int16_t)width - 1;
  int16_t centerRow2 = (int16_t)height - 1;
  Coord center = focusCoord(maze, focusX, focusY);

  for (uint8_t row = 0; row < height; row++) {
    int16_t screenY2 = (int16_t)row * 2 - centerRow2;
    int16_t worldY = screenToWorld(screenY2, focusY, zoomQ8);
    for (uint16_t col = 0; col < width; col++) {
      int16_t screenX2 = (int16_t)col * 2 - centerCol2;
      int16_t worldX = screenToWorld(screenX2, focusX, zoomQ8);
      if (isCloseupWall(maze, center, worldX, worldY)) {
        setPixel(row, col);
      }
    }
  }
}

int16_t worldToScreen(int16_t world, int16_t focus, uint16_t zoomQ8,
                      int16_t center2) {
  int16_t doubledScreenDelta =
      ((int32_t)(world - focus) * zoomQ8) / (CLOSEUP_CELL_PITCH * 128L);
  return (doubledScreenDelta + center2 + 1) / 2;
}

bool inCircle(int16_t row, int16_t col, int16_t centerRow, int16_t centerCol,
              int16_t radius) {
  if (radius <= 0) {
    return row == centerRow && col == centerCol;
  }
  int32_t dx = col - centerCol;
  int32_t dy = row - centerRow;
  int32_t radius2 = (int32_t)radius * radius;
  return dx * dx + dy * dy <= radius2;
}

void drawHeroFill(uint16_t width, uint8_t height, uint8_t radius) {
  int16_t centerCol = width / 2;
  int16_t centerRow = height / 2;
  for (uint8_t row = 0; row < height; row++) {
    for (uint16_t col = 0; col < width; col++) {
      if (inCircle(row, col, centerRow, centerCol, radius)) {
        setPixel(row, col);
      }
    }
  }
}

Direction exitDirection(const Maze &maze, Coord hero) {
  if (hero.y == 0) {
    return Direction::North;
  }
  if (hero.x == maze.width() - 1) {
    return Direction::East;
  }
  if (hero.y == maze.height() - 1) {
    return Direction::South;
  }
  return Direction::West;
}

void drawHeroAtScreen(int16_t centerCol, int16_t centerRow) {
  for (int16_t row = centerRow - 1; row <= centerRow + 1; row++) {
    for (int16_t col = centerCol - 1; col <= centerCol + 1; col++) {
      if (inCircle(row, col, centerRow, centerCol, 1)) {
        setPixel(row, col);
      }
    }
  }
}

void drawHeroAt(uint16_t width, uint8_t height, int16_t offsetX,
                int16_t offsetY) {
  drawHeroAtScreen(width / 2 + offsetX, height / 2 + offsetY);
}

void drawExitingHero(const Maze &maze, Coord hero, uint16_t width,
                     uint8_t height, uint8_t progress) {
  Direction dir = exitDirection(maze, hero);
  uint8_t eased = easeInOutQ8(progress);
  int16_t exitDistance =
      (dir == Direction::East || dir == Direction::West) ? width / 2 + 4
                                                         : height / 2 + 3;
  int16_t travel = ((int32_t)exitDistance * eased) / 255;
  drawHeroAt(width, height, directionDx(dir) * travel,
             directionDy(dir) * travel);
}

void drawEnteringHero(const Maze &maze, Coord hero, uint16_t width,
                      uint8_t height, uint8_t progress) {
  Direction dir = exitDirection(maze, hero);
  uint8_t eased = easeInOutQ8(progress);
  int16_t enterDistance =
      (dir == Direction::East || dir == Direction::West) ? width / 2 + 4
                                                         : height / 2 + 3;
  int16_t travel = ((int32_t)enterDistance * (255 - eased)) / 255;
  drawHeroAt(width, height, directionDx(dir) * travel,
             directionDy(dir) * travel);
}

} // namespace

uint16_t Renderer::viewWidth() const { return matrix()->getColumnCount(); }

void Renderer::renderPlaying(const Maze &maze, const FogOfWar &fog,
                             const Camera &camera, Coord hero,
                             uint8_t frame) const {
  uint16_t width = viewWidth();
  uint8_t height = viewHeight();
  beginFrame();

  for (uint8_t y = 0; y < maze.height(); y++) {
    for (uint8_t x = 0; x < maze.width(); x++) {
      Coord coord = {x, y};
      drawDiscoveredWalls(maze, fog, camera, coord, width, height);
    }
  }

  Coord finish = maze.finish();
  if (fog.isDiscovered(maze, finish) && (frame % 8) < 5) {
    int16_t row = screenY(maze, camera, cellCenterY(finish), height);
    int16_t col = screenX(maze, camera, cellCenterX(finish), width);
    setPixel(row, col);
  }

  int16_t heroRow = screenY(maze, camera, cellCenterY(hero), height);
  int16_t heroCol = screenX(maze, camera, cellCenterX(hero), width);
  setPixel(heroRow, heroCol);

  endFrame();
}

void Renderer::renderIntro(const Maze &maze, const Camera &camera, Coord hero,
                           unsigned long elapsedMs, uint8_t frame) const {
  (void)frame;
  uint16_t width = viewWidth();
  uint8_t height = viewHeight();
  int16_t heroFocusX = closeupCellCenterX(hero);
  int16_t heroFocusY = closeupCellCenterY(hero);

  beginFrame();
  if (elapsedMs < INTRO_ENTER_MS) {
    drawCloseupMaze(maze, heroFocusX, heroFocusY, CLOSEUP_HOLD_ZOOM_Q8, width,
                    height);
    drawEnteringHero(maze, hero, width, height,
                     progressQ8(elapsedMs, INTRO_ENTER_MS));
  } else {
    unsigned long zoomElapsed = elapsedMs - INTRO_ENTER_MS;
    uint8_t eased = easeInOutQ8(progressQ8(zoomElapsed, INTRO_ZOOM_OUT_MS));
    int16_t endFocusX = playCameraFocusX(maze, camera, width);
    int16_t endFocusY = playCameraFocusY(maze, camera, height);
    int16_t focusX = lerpInt16(heroFocusX, endFocusX, eased);
    int16_t focusY = lerpInt16(heroFocusY, endFocusY, eased);
    uint16_t zoomQ8 =
        lerpQ8(CLOSEUP_HOLD_ZOOM_Q8, CLOSEUP_START_ZOOM_Q8, eased);
    drawCloseupMaze(maze, focusX, focusY, zoomQ8, width, height);
    int16_t heroCol =
        worldToScreen(heroFocusX, focusX, zoomQ8, (int16_t)width - 1);
    int16_t heroRow =
        worldToScreen(heroFocusY, focusY, zoomQ8, (int16_t)height - 1);
    drawHeroAtScreen(heroCol, heroRow);
  }
  endFrame();
}

void Renderer::renderVictory(const Maze &maze, const Camera &camera, Coord hero,
                             unsigned long elapsedMs, uint8_t frame) const {
  (void)frame;
  uint16_t width = viewWidth();
  uint8_t height = viewHeight();
  int16_t heroFocusX = closeupCellCenterX(hero);
  int16_t heroFocusY = closeupCellCenterY(hero);
  int16_t focusX = heroFocusX;
  int16_t focusY = heroFocusY;

  beginFrame();
  if (elapsedMs < VICTORY_ZOOM_IN_MS) {
    uint8_t eased = easeInOutQ8(progressQ8(elapsedMs, VICTORY_ZOOM_IN_MS));
    int16_t startFocusX = playCameraFocusX(maze, camera, width);
    int16_t startFocusY = playCameraFocusY(maze, camera, height);
    focusX = lerpInt16(startFocusX, heroFocusX, eased);
    focusY = lerpInt16(startFocusY, heroFocusY, eased);
    uint16_t zoomQ8 =
        lerpQ8(CLOSEUP_START_ZOOM_Q8, CLOSEUP_HOLD_ZOOM_Q8, eased);
    drawCloseupMaze(maze, focusX, focusY, zoomQ8, width, height);
    int16_t heroCol =
        worldToScreen(heroFocusX, focusX, zoomQ8, (int16_t)width - 1);
    int16_t heroRow =
        worldToScreen(heroFocusY, focusY, zoomQ8, (int16_t)height - 1);
    drawHeroAtScreen(heroCol, heroRow);
  } else if (elapsedMs < VICTORY_ZOOM_IN_MS + VICTORY_HOLD_MS) {
    drawCloseupMaze(maze, heroFocusX, heroFocusY, CLOSEUP_HOLD_ZOOM_Q8, width,
                    height);
    drawHeroFill(width, height, 1);
  } else {
    unsigned long exitElapsed =
        elapsedMs - VICTORY_ZOOM_IN_MS - VICTORY_HOLD_MS;
    drawCloseupMaze(maze, heroFocusX, heroFocusY, CLOSEUP_HOLD_ZOOM_Q8, width,
                    height);
    drawExitingHero(maze, hero, width, height,
                    progressQ8(exitElapsed, VICTORY_EXIT_MS));
  }
  endFrame();
}

} // namespace MazeHero
