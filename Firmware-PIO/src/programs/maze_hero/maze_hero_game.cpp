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

enum class GameState : uint8_t { IntroAnimation, Playing, VictoryAnimation };
enum class MovementPhase : uint8_t { IdleAtCell, CrossingPassage };

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
};

Game game;

bool heroAtPendingCellCenter() {
  return game.heroWorldX == cellCenterX(game.pendingHero) &&
         game.heroWorldY == cellCenterY(game.pendingHero);
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

void startVictory(unsigned long now) {
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
  if (game.movementPhase == MovementPhase::IdleAtCell &&
      sameCoord(game.hero, game.maze.finish())) {
    startVictory(now);
    return;
  }

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
    game.renderer.renderPlaying(game.maze, game.fog, game.camera,
                                game.heroWorldX, game.heroWorldY,
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
    game.renderer.renderVictory(game.maze, game.fog, game.camera, game.hero,
                                elapsedMs, game.frame++);
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
