#include "holiday_halloween.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setHolidayPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + col, on);
  }
}

static void drawBat(MilestoneCtx &ctx, int x, int y, uint8_t flapFrame) {
  bool wingsUp = flapFrame % 2 == 0;
  setHolidayPixel(ctx, x + 3, y + 1);
  setHolidayPixel(ctx, x + 4, y + 1);
  setHolidayPixel(ctx, x + 2, y + 2);
  setHolidayPixel(ctx, x + 5, y + 2);

  if (wingsUp) {
    setHolidayPixel(ctx, x, y);
    setHolidayPixel(ctx, x + 1, y + 1);
    setHolidayPixel(ctx, x + 6, y + 1);
    setHolidayPixel(ctx, x + 7, y);
  } else {
    setHolidayPixel(ctx, x, y + 2);
    setHolidayPixel(ctx, x + 1, y + 2);
    setHolidayPixel(ctx, x + 6, y + 2);
    setHolidayPixel(ctx, x + 7, y + 2);
  }
}

static void drawPumpkin(MilestoneCtx &ctx, int leftCol, uint8_t revealRows,
                        uint8_t faceFrame) {
  static const uint8_t PUMPKIN_WIDTH = 17;
  static const uint8_t PUMPKIN_HEIGHT = 8;
  static const uint32_t PUMPKIN_ROWS[] = {
      0b00000001000000000, 0b00011111111110000, 0b00111111111111000,
      0b01111111111111100, 0b11111111111111110, 0b01111111111111100,
      0b00111111111111000, 0b00011111111110000,
  };
  static const uint32_t FACE_ROWS[] = {
      0b00000000000000000, 0b00000000000000000, 0b00011000000110000,
      0b00111000000111000, 0b00000011110000000, 0b00001100000011000,
      0b00000111111100000, 0b00000000000000000,
  };
  static const uint32_t FACE_ALT_ROWS[] = {
      0b00000000000000000, 0b00000000000000000, 0b00010000000010000,
      0b00111000000111000, 0b00000001100000000, 0b00001000000001000,
      0b00000111111100000, 0b00000000000000000,
  };

  for (int row = 0; row < PUMPKIN_HEIGHT; row++) {
    if (row >= revealRows) {
      continue;
    }

    uint32_t faceMask = faceFrame % 2 == 0 ? FACE_ROWS[row] : FACE_ALT_ROWS[row];
    for (int col = 0; col < PUMPKIN_WIDTH; col++) {
      uint32_t mask = 1UL << (PUMPKIN_WIDTH - 1 - col);
      if ((PUMPKIN_ROWS[row] & mask) == 0 || (faceMask & mask) != 0) {
        continue;
      }
      setHolidayPixel(ctx, leftCol + col, row);
    }
  }

  setHolidayPixel(ctx, leftCol + 8, 0);
  setHolidayPixel(ctx, leftCol + 8, 1);
}

void runHalloweenHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (int frame = 0; frame < 24; frame++) {
    milestoneClear(ctx);
    drawBat(ctx, ctx.width - frame - 2, 0, frame);
    drawBat(ctx, frame - 8, 4, frame + 1);
    if (frame % 4 == 0) {
      setHolidayPixel(ctx, random(ctx.width), random(ctx.height));
    }
    milestoneFrameShow(ctx, 55, frame % 4);
  }

  int pumpkinLeft = ctx.cx - 8;
  for (uint8_t reveal = 1; reveal <= 8; reveal++) {
    milestoneClear(ctx);
    drawPumpkin(ctx, pumpkinLeft, reveal, 0);
    milestoneFrameShow(ctx, 95, reveal);
  }

  for (int flicker = 0; flicker < 8; flicker++) {
    milestoneClear(ctx);
    drawPumpkin(ctx, pumpkinLeft, 8, flicker);
    if (flicker % 2 == 0) {
      drawBat(ctx, 1, 0, flicker);
      drawBat(ctx, ctx.width - 9, 0, flicker + 1);
    }
    milestoneFrameShow(ctx, 115, flicker % 2 == 0 ? 8 : 0);
  }

  for (int burst = 0; burst < 12; burst++) {
    milestoneClear(ctx);
    drawPumpkin(ctx, pumpkinLeft, 8, burst);
    for (int spark = 0; spark < 10; spark++) {
      float angle = spark * 0.628318f + burst * 0.22f;
      int px = ctx.cx + (int)(burst * cosf(angle) * 0.95f);
      int py = 4 + (int)(burst * sinf(angle) * 0.55f);
      setHolidayPixel(ctx, px, py, burst % 2 == 0 || spark % 3 == 0);
    }
    milestoneFrameShow(ctx, 42, min(7, (int)burst));
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Halloween!");
}
