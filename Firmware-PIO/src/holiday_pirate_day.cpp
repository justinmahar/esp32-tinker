#include "holiday_pirate_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setPiratePixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawSkull(MilestoneCtx &ctx, int centerX, uint8_t revealRows,
                      uint8_t grinFrame) {
  static const uint8_t SKULL_WIDTH = 15;
  static const uint8_t SKULL_HEIGHT = 8;
  static const uint16_t SKULL_ROWS[] = {
      0b000011111100000, 0b000111111110000, 0b001101111011000,
      0b001101111011000, 0b000111111110000, 0b000011011100000,
      0b000001111000000, 0b000001010000000,
  };

  int leftCol = centerX - SKULL_WIDTH / 2;
  for (int row = 0; row < SKULL_HEIGHT; row++) {
    if (row >= revealRows) {
      continue;
    }

    for (int col = 0; col < SKULL_WIDTH; col++) {
      uint16_t mask = 1u << (SKULL_WIDTH - 1 - col);
      if ((SKULL_ROWS[row] & mask) == 0) {
        continue;
      }

      bool grinGap = row >= 5 && ((col + grinFrame) % 4 == 0);
      if (!grinGap || grinFrame % 2 == 0) {
        setPiratePixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawCrossbones(MilestoneCtx &ctx, int centerX, uint8_t frame) {
  for (int step = -9; step <= 9; step++) {
    int rowA = 4 + step / 3;
    int rowB = 4 - step / 3;
    setPiratePixel(ctx, centerX + step, rowA);
    setPiratePixel(ctx, centerX + step, rowB);
  }

  if (frame % 2 == 0) {
    setPiratePixel(ctx, centerX - 10, 1);
    setPiratePixel(ctx, centerX + 10, 1);
    setPiratePixel(ctx, centerX - 10, 7);
    setPiratePixel(ctx, centerX + 10, 7);
  }
}

static void drawShip(MilestoneCtx &ctx, int bowX, int waveFrame) {
  int mastX = bowX - 7;
  for (int row = 1; row <= 5; row++) {
    setPiratePixel(ctx, mastX, row);
  }

  for (int row = 1; row <= 4; row++) {
    for (int col = 1; col <= 5 - row; col++) {
      setPiratePixel(ctx, mastX + col, row);
    }
  }

  for (int col = bowX - 14; col <= bowX; col++) {
    int hullRow = 6 + ((col + waveFrame) % 2 == 0 ? 0 : 1);
    setPiratePixel(ctx, col, hullRow);
    if (col > bowX - 12 && col < bowX - 1) {
      setPiratePixel(ctx, col, 7);
    }
  }
  setPiratePixel(ctx, bowX + 1, 5);
}

static void drawTreasureChest(MilestoneCtx &ctx, int centerX, uint8_t lidFrame) {
  static const uint8_t CHEST_WIDTH = 17;
  static const uint8_t CHEST_HEIGHT = 7;
  static const uint32_t CHEST_ROWS[] = {
      0b00001111111110000, 0b00011111111111000, 0b00111111111111100,
      0b00100000100000100, 0b00111111111111100, 0b00101010101010100,
      0b00111111111111100,
  };

  int leftCol = centerX - CHEST_WIDTH / 2;
  for (int row = 0; row < CHEST_HEIGHT; row++) {
    int openLift = (lidFrame % 2 == 0 && row < 2) ? -1 : 0;
    for (int col = 0; col < CHEST_WIDTH; col++) {
      uint32_t mask = 1UL << (CHEST_WIDTH - 1 - col);
      if ((CHEST_ROWS[row] & mask) != 0) {
        setPiratePixel(ctx, leftCol + col, row + 1 + openLift);
      }
    }
  }

  if (lidFrame % 2 == 0) {
    for (int sparkle = 0; sparkle < 5; sparkle++) {
      setPiratePixel(ctx, leftCol + 4 + sparkle * 2, 1 + sparkle % 2);
    }
  }
}

static void drawGoldSparkle(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setPiratePixel(ctx, cx, cy);
  setPiratePixel(ctx, cx - 1, cy);
  setPiratePixel(ctx, cx + 1, cy);
  setPiratePixel(ctx, cx, cy - 1);
  setPiratePixel(ctx, cx, cy + 1);
  if (wide) {
    setPiratePixel(ctx, cx - 2, cy);
    setPiratePixel(ctx, cx + 2, cy);
  }
}

static void drawGoldBurst(MilestoneCtx &ctx, int originX, int originY,
                          uint8_t frame) {
  drawGoldSparkle(ctx, originX, originY, frame % 2 == 0);
  for (int spark = 0; spark < 10; spark++) {
    float angle = spark * 0.6283185f + frame * 0.18f;
    int px = originX + (int)(frame * cosf(angle) * 0.7f);
    int py = originY + (int)(frame * sinf(angle) * 0.5f);
    setPiratePixel(ctx, px, py, frame % 2 == 0 || spark % 2 == 0);
  }
}

void runPirateDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (int frame = 0; frame < ctx.width + 18; frame += 2) {
    milestoneClear(ctx);
    drawShip(ctx, frame, frame);
    milestoneFrameShow(ctx, 50, frame % 5);
  }

  for (uint8_t reveal = 1; reveal <= 8; reveal++) {
    milestoneClear(ctx);
    drawCrossbones(ctx, ctx.cx, reveal);
    drawSkull(ctx, ctx.cx, reveal, reveal);
    milestoneFrameShow(ctx, 110, reveal);
  }

  for (uint8_t grin = 0; grin < 10; grin++) {
    milestoneClear(ctx);
    drawCrossbones(ctx, ctx.cx, grin);
    drawSkull(ctx, ctx.cx, 8, grin);
    milestoneFrameShow(ctx, 90, grin % 2 == 0 ? 7 : 0);
  }

  for (uint8_t frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    drawTreasureChest(ctx, ctx.cx, frame);
    if (frame % 2 == 0) {
      drawGoldSparkle(ctx, ctx.cx - 10 + frame, 1 + frame % 5, true);
      drawGoldSparkle(ctx, ctx.cx + 10 - frame / 2, 2 + frame % 4, false);
    }
    milestoneFrameShow(ctx, 95, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawTreasureChest(ctx, ctx.cx, frame);
    drawGoldBurst(ctx, ctx.cx, 3, frame);
    uint8_t intensity = frame / 2;
    milestoneFrameShow(ctx, 45, intensity > 7 ? 7 : intensity);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawSkull(ctx, ctx.cx, 8, flash);
      drawTreasureChest(ctx, ctx.cx, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 55 : 95,
                       flash % 2 == 0 ? 7 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Ahoy! Talk Like a Pirate Day!");
}
