#include "maze_hero_game.h"

#include "camera.h"
#include "fog_of_war.h"
#include "hero_ai.h"
#include "maze.h"
#include "play_world.h"
#include "renderer.h"

#include <esp_system.h>

namespace MazeHero {
namespace {

constexpr unsigned long RENDER_FRAME_MS = 45UL;
constexpr uint8_t RUNNER_ZOOM_CHANCE_PER_ROLL_PERCENT = 25;
constexpr unsigned long RUNNER_ZOOM_CHANCE_INTERVAL_MS = 6000UL;
constexpr unsigned long RUNNER_ZOOM_MIN_GAP_MS = 10000UL;
constexpr uint16_t RUNNER_ZOOM_TARGET_PERCENT = 350U;
constexpr unsigned long RUNNER_ZOOM_IN_MIN_MS = 1600UL;
constexpr unsigned long RUNNER_ZOOM_IN_MAX_MS = 2800UL;
constexpr unsigned long RUNNER_ZOOM_HOLD_MIN_MS = 2500UL;
constexpr unsigned long RUNNER_ZOOM_HOLD_MAX_MS = 5000UL;
constexpr unsigned long RUNNER_ZOOM_OUT_MIN_MS = 1600UL;
constexpr unsigned long RUNNER_ZOOM_OUT_MAX_MS = 2800UL;

enum class GameState : uint8_t { IntroAnimation, Playing, VictoryAnimation };
enum class MovementPhase : uint8_t { IdleAtCell, CrossingPassage };

struct HeroRenderPosition {
  HeroRenderPosition() : xQ8(0), yQ8(0) {}
  HeroRenderPosition(int32_t xQ8, int32_t yQ8) : xQ8(xQ8), yQ8(yQ8) {}

  int32_t xQ8;
  int32_t yQ8;
};

struct RunnerZoomState {
  bool active = false;
  unsigned long startMs = 0;
  unsigned long lastRollMs = 0;
  unsigned long lastEndMs = 0;
  unsigned long zoomInMs = 0;
  unsigned long holdMs = 0;
  unsigned long zoomOutMs = 0;
};

struct Game {
  Maze maze;
  FogOfWar fog;
  Camera camera;
  Renderer renderer;
  HeroAi ai;
  Coord hero;
  Coord pendingHero;
  int16_t heroWorldX = 0;
  int16_t heroWorldY = 0;
  GameState state = GameState::Playing;
  MovementPhase movementPhase = MovementPhase::IdleAtCell;
  bool seeded = false;
  uint8_t frame = 0;
  unsigned long lastMoveMs = 0;
  unsigned long moveDelayMs = 1;
  unsigned long lastRenderMs = 0;
  unsigned long introStartMs = 0;
  unsigned long victoryStartMs = 0;
  RunnerZoomState runnerZoom;
  ZoomView victoryStartView;
};

Game game;

bool heroAtPendingCellCenter() {
  return game.heroWorldX == cellCenterX(game.pendingHero) &&
         game.heroWorldY == cellCenterY(game.pendingHero);
}

int32_t worldQ8(int16_t value) { return (int32_t)value * 256L; }

uint8_t moveProgressQ8(unsigned long now) {
  if (game.moveDelayMs == 0) {
    return 255;
  }
  unsigned long elapsedMs = now - game.lastMoveMs;
  if (elapsedMs >= game.moveDelayMs) {
    return 255;
  }
  return (uint8_t)((elapsedMs * 255UL) / game.moveDelayMs);
}

int32_t lerpWorldQ8(int16_t from, int16_t to, uint8_t progress) {
  return worldQ8(from) + ((int32_t)(to - from) * 256L * progress) / 255L;
}

HeroRenderPosition currentHeroRenderPosition(unsigned long now) {
  if (game.movementPhase != MovementPhase::CrossingPassage) {
    return {worldQ8(game.heroWorldX), worldQ8(game.heroWorldY)};
  }

  int16_t nextWorldX =
      approachOnePixel(game.heroWorldX, cellCenterX(game.pendingHero));
  int16_t nextWorldY =
      approachOnePixel(game.heroWorldY, cellCenterY(game.pendingHero));
  uint8_t progress = moveProgressQ8(now);
  return {lerpWorldQ8(game.heroWorldX, nextWorldX, progress),
          lerpWorldQ8(game.heroWorldY, nextWorldY, progress)};
}

unsigned long randomHeroSpeedMs(const ProgramConfig &cfg);

void resetMoveTimer(const ProgramConfig &cfg, unsigned long now) {
  game.lastMoveMs = now;
  game.moveDelayMs = randomHeroSpeedMs(cfg);
}

void stepHeroTowardPendingCell() {
  game.heroWorldX =
      approachOnePixel(game.heroWorldX, cellCenterX(game.pendingHero));
  game.heroWorldY =
      approachOnePixel(game.heroWorldY, cellCenterY(game.pendingHero));
}

void completeCellArrival() {
  game.hero = game.pendingHero;
  game.fog.markHeroVisited(game.maze, game.hero);
  game.fog.revealFrom(game.maze, game.hero);
  game.movementPhase = MovementPhase::IdleAtCell;
}

unsigned long runnerZoomTotalMs() {
  return game.runnerZoom.zoomInMs + game.runnerZoom.holdMs +
         game.runnerZoom.zoomOutMs;
}

unsigned long randomRangeInclusive(unsigned long minValue,
                                   unsigned long maxValue) {
  if (maxValue < minValue) {
    maxValue = minValue;
  }
  return random(minValue, maxValue + 1);
}

PlayZoomFrame currentRunnerZoomFrame(unsigned long now) {
  PlayZoomFrame frame;
  if (!game.runnerZoom.active) {
    return frame;
  }

  frame.active = true;
  frame.elapsedMs = now - game.runnerZoom.startMs;
  frame.zoomInMs = game.runnerZoom.zoomInMs;
  frame.holdMs = game.runnerZoom.holdMs;
  frame.zoomOutMs = game.runnerZoom.zoomOutMs;
  frame.targetZoomPercent = RUNNER_ZOOM_TARGET_PERCENT;
  return frame;
}

void resetRunnerZoom(unsigned long now) {
  game.runnerZoom.active = false;
  game.runnerZoom.startMs = 0;
  game.runnerZoom.lastRollMs = now;
  game.runnerZoom.lastEndMs = now;
  game.runnerZoom.zoomInMs = 0;
  game.runnerZoom.holdMs = 0;
  game.runnerZoom.zoomOutMs = 0;
}

void updateRunnerZoom(unsigned long now) {
  if (!game.runnerZoom.active) {
    return;
  }

  if (now - game.runnerZoom.startMs >= runnerZoomTotalMs()) {
    game.runnerZoom.active = false;
    game.runnerZoom.lastEndMs = now;
    game.runnerZoom.lastRollMs = now;
  }
}

void maybeStartRunnerZoom(unsigned long now) {
  if (game.runnerZoom.active || sameCoord(game.hero, game.maze.finish())) {
    return;
  }
  if (now - game.runnerZoom.lastEndMs < RUNNER_ZOOM_MIN_GAP_MS) {
    return;
  }
  if (now - game.runnerZoom.lastRollMs < RUNNER_ZOOM_CHANCE_INTERVAL_MS) {
    return;
  }

  game.runnerZoom.lastRollMs = now;
  if (random(100) >= RUNNER_ZOOM_CHANCE_PER_ROLL_PERCENT) {
    return;
  }

  game.runnerZoom.active = true;
  game.runnerZoom.startMs = now;
  game.runnerZoom.zoomInMs =
      randomRangeInclusive(RUNNER_ZOOM_IN_MIN_MS, RUNNER_ZOOM_IN_MAX_MS);
  game.runnerZoom.holdMs =
      randomRangeInclusive(RUNNER_ZOOM_HOLD_MIN_MS, RUNNER_ZOOM_HOLD_MAX_MS);
  game.runnerZoom.zoomOutMs =
      randomRangeInclusive(RUNNER_ZOOM_OUT_MIN_MS, RUNNER_ZOOM_OUT_MAX_MS);
}

void startVictory(unsigned long now) {
  PlayZoomFrame zoomFrame = currentRunnerZoomFrame(now);
  game.victoryStartView =
      game.renderer.playZoomView(game.maze, game.camera,
                                 worldQ8(game.heroWorldX),
                                 worldQ8(game.heroWorldY), zoomFrame);
  resetRunnerZoom(now);
  game.state = GameState::VictoryAnimation;
  game.victoryStartMs = now;
  game.lastRenderMs = 0;
}

unsigned long randomHeroSpeedMs(const ProgramConfig &cfg) {
  unsigned long minSpeedMs =
      cfg.mazeHeroMinSpeedMs > 0 ? cfg.mazeHeroMinSpeedMs : 1UL;
  unsigned long maxSpeedMs = cfg.mazeHeroMaxSpeedMs >= minSpeedMs
                                 ? cfg.mazeHeroMaxSpeedMs
                                 : minSpeedMs;
  return random(minSpeedMs, maxSpeedMs + 1);
}

void startNewMaze(const ProgramConfig &cfg) {
  unsigned long now = millis();
  game.maze.generate(cfg.mazeMinWidth, cfg.mazeMaxWidth, cfg.mazeMinHeight,
                     cfg.mazeMaxHeight);
  game.hero = game.maze.start();
  game.pendingHero = game.hero;
  game.heroWorldX = cellCenterX(game.hero);
  game.heroWorldY = cellCenterY(game.hero);
  game.ai.reset();
  game.fog.reset(game.maze);
  game.fog.markHeroVisited(game.maze, game.hero);
  game.fog.revealFrom(game.maze, game.hero);
  game.camera.reset(game.maze, game.renderer.viewWidth(),
                    game.renderer.viewHeight(), game.hero);
  resetRunnerZoom(now);
  game.victoryStartView =
      game.renderer.playZoomView(game.maze, game.camera,
                                 worldQ8(game.heroWorldX),
                                 worldQ8(game.heroWorldY),
                                 currentRunnerZoomFrame(now));
  game.state = GameState::IntroAnimation;
  game.movementPhase = MovementPhase::IdleAtCell;
  game.frame = 0;
  game.lastMoveMs = now;
  game.moveDelayMs = randomHeroSpeedMs(cfg);
  game.lastRenderMs = now;
  game.introStartMs = now;
  game.victoryStartMs = 0;
  game.renderer.renderIntro(game.maze, game.fog, game.camera, game.hero, 0,
                            game.frame++);
}

void tickIntro(unsigned long now) {
  unsigned long elapsedMs = now - game.introStartMs;
  if (elapsedMs >= INTRO_TOTAL_MS) {
    game.state = GameState::Playing;
    game.lastMoveMs = now;
    game.lastRenderMs = 0;
    return;
  }

  if (now - game.lastRenderMs >= RENDER_FRAME_MS) {
    game.lastRenderMs = now;
    game.renderer.renderIntro(game.maze, game.fog, game.camera, game.hero,
                              elapsedMs, game.frame++);
  }
}

void tickPlaying(const ProgramConfig &cfg, unsigned long now) {
  updateRunnerZoom(now);

  if (game.movementPhase == MovementPhase::IdleAtCell &&
      sameCoord(game.hero, game.maze.finish())) {
    startVictory(now);
    return;
  }

  maybeStartRunnerZoom(now);

  if (now - game.lastMoveMs >= game.moveDelayMs) {
    if (game.movementPhase == MovementPhase::CrossingPassage) {
      stepHeroTowardPendingCell();
      resetMoveTimer(cfg, now);
      if (heroAtPendingCellCenter()) {
        completeCellArrival();
      }
    } else {
      Coord next;
      unsigned long decisionDelayMs = game.moveDelayMs;
      if (game.ai.chooseNextStep(game.maze, game.fog, game.hero, now,
                                 decisionDelayMs, next)) {
        game.pendingHero = next;
        game.movementPhase = MovementPhase::CrossingPassage;
        stepHeroTowardPendingCell();
        resetMoveTimer(cfg, now);
        // Follow the committed destination while AI/fog remain cell-boundary
        // driven until the visible hero reaches the next cell center.
        game.camera.updateTarget(game.maze, game.renderer.viewWidth(),
                                 game.renderer.viewHeight(), game.pendingHero);
      }
    }
  }

  if (now - game.lastRenderMs >= RENDER_FRAME_MS) {
    game.lastRenderMs = now;
    game.camera.stepTowardTarget();
    PlayZoomFrame zoomFrame = currentRunnerZoomFrame(now);
    HeroRenderPosition heroRender = currentHeroRenderPosition(now);
    game.renderer.renderPlaying(game.maze, game.fog, game.camera,
                                game.heroWorldX, game.heroWorldY,
                                heroRender.xQ8, heroRender.yQ8, zoomFrame,
                                game.frame++);
  }
}

void tickVictory(const ProgramConfig &cfg, unsigned long now) {
  unsigned long elapsedMs = now - game.victoryStartMs;
  if (elapsedMs >= VICTORY_TOTAL_MS) {
    startNewMaze(cfg);
    return;
  }

  if (now - game.lastRenderMs >= RENDER_FRAME_MS) {
    game.lastRenderMs = now;
    game.renderer.renderVictory(game.maze, game.fog, game.victoryStartView,
                                game.hero, elapsedMs, game.frame++);
  }
}

} // namespace

void start(const ProgramConfig &cfg) {
  if (!game.seeded) {
    randomSeed(esp_random());
    game.seeded = true;
  }
  Display.setIntensity(cfg.brightness > 15 ? 15 : cfg.brightness);
  startNewMaze(cfg);
}

void tick(const ProgramConfig &cfg) {
  unsigned long now = millis();

  switch (game.state) {
  case GameState::IntroAnimation:
    tickIntro(now);
    break;
  case GameState::Playing:
    tickPlaying(cfg, now);
    break;
  case GameState::VictoryAnimation:
    tickVictory(cfg, now);
    break;
  }
}

} // namespace MazeHero
