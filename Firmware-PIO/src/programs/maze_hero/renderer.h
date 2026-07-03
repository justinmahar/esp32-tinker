#pragma once

#include "camera.h"
#include "fog_of_war.h"
#include "maze.h"
#include "types.h"

namespace MazeHero {

constexpr unsigned long INTRO_PAUSE_MS = 450UL;
constexpr unsigned long INTRO_ENTER_MS = 900UL;
constexpr unsigned long INTRO_ZOOM_OUT_MS = 1000UL;
constexpr unsigned long INTRO_TOTAL_MS =
    INTRO_PAUSE_MS + INTRO_ENTER_MS + INTRO_ZOOM_OUT_MS;

constexpr unsigned long VICTORY_ZOOM_IN_MS = 1000UL;
constexpr unsigned long VICTORY_HOLD_MS = 450UL;
constexpr unsigned long VICTORY_EXIT_MS = 1100UL;
constexpr unsigned long VICTORY_TOTAL_MS =
    VICTORY_ZOOM_IN_MS + VICTORY_HOLD_MS + VICTORY_EXIT_MS;

struct ZoomView {
  ZoomView() : focusX(0), focusY(0), zoomQ8(0) {}
  ZoomView(int16_t focusX, int16_t focusY, uint16_t zoomQ8)
      : focusX(focusX), focusY(focusY), zoomQ8(zoomQ8) {}

  int16_t focusX;
  int16_t focusY;
  uint16_t zoomQ8;
};

struct PlayZoomFrame {
  PlayZoomFrame()
      : active(false), elapsedMs(0), zoomInMs(0), holdMs(0), zoomOutMs(0),
        targetZoomPercent(100) {}

  bool active;
  unsigned long elapsedMs;
  unsigned long zoomInMs;
  unsigned long holdMs;
  unsigned long zoomOutMs;
  uint16_t targetZoomPercent;
};

class Renderer {
public:
  uint16_t viewWidth() const;
  uint8_t viewHeight() const { return DISPLAY_HEIGHT; }

  ZoomView playZoomView(const Maze &maze, const Camera &camera,
                        int16_t heroWorldX, int16_t heroWorldY,
                        const PlayZoomFrame &zoomFrame) const;
  void renderPlaying(const Maze &maze, const FogOfWar &fog,
                     const Camera &camera, int16_t heroWorldX,
                     int16_t heroWorldY, const PlayZoomFrame &zoomFrame,
                     uint8_t frame) const;
  void renderIntro(const Maze &maze, const FogOfWar &fog, const Camera &camera,
                   Coord hero, unsigned long elapsedMs, uint8_t frame) const;
  void renderVictory(const Maze &maze, const FogOfWar &fog,
                     const ZoomView &startView, Coord hero,
                     unsigned long elapsedMs, uint8_t frame) const;
};

} // namespace MazeHero
