#include "holiday_valentines.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setValentinePixel(MilestoneCtx &ctx, int col, int row,
                              bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + col, on);
  }
}

static void drawHeart(MilestoneCtx &ctx, int leftCol, uint8_t size,
                      bool sparkle) {
  static const uint8_t HEART_WIDTH = 15;
  static const uint8_t HEART_HEIGHT = 8;
  static const uint16_t SMALL_HEART_ROWS[] = {
      0b000000000000000, 0b000011001100000, 0b000111111110000,
      0b000111111110000, 0b000011111100000, 0b000001111000000,
      0b000000110000000, 0b000000000000000,
  };
  static const uint16_t BIG_HEART_ROWS[] = {
      0b001110001110000, 0b011111011111000, 0b111111111111100,
      0b111111111111100, 0b011111111111000, 0b001111111110000,
      0b000111111100000, 0b000001110000000,
  };

  const uint16_t *rows = size > 0 ? BIG_HEART_ROWS : SMALL_HEART_ROWS;
  for (int row = 0; row < HEART_HEIGHT; row++) {
    for (int col = 0; col < HEART_WIDTH; col++) {
      uint16_t mask = 1u << (HEART_WIDTH - 1 - col);
      if ((rows[row] & mask) == 0) {
        continue;
      }
      bool twinkleHole = sparkle && row > 1 && row < 5 &&
                         ((row * 3 + col) % 11 == 0);
      if (!twinkleHole) {
        setValentinePixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawArrow(MilestoneCtx &ctx, int tipX, int y) {
  for (int col = tipX - 10; col <= tipX - 2; col++) {
    setValentinePixel(ctx, col, y);
  }
  setValentinePixel(ctx, tipX, y);
  setValentinePixel(ctx, tipX - 1, y - 1);
  setValentinePixel(ctx, tipX - 1, y + 1);
  setValentinePixel(ctx, tipX - 2, y - 2);
  setValentinePixel(ctx, tipX - 2, y + 2);

  setValentinePixel(ctx, tipX - 10, y - 1);
  setValentinePixel(ctx, tipX - 10, y + 1);
  setValentinePixel(ctx, tipX - 11, y - 2);
  setValentinePixel(ctx, tipX - 11, y + 2);
}

void runValentinesHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  int heartLeft = ctx.cx - 7;
  for (int beat = 0; beat < 4; beat++) {
    milestoneClear(ctx);
    drawHeart(ctx, heartLeft + 2, 0, false);
    milestoneFrameShow(ctx, 130, 0);

    milestoneClear(ctx);
    drawHeart(ctx, heartLeft, 1, beat % 2 == 0);
    milestoneFrameShow(ctx, 170, 0);
  }

  for (int tip = -2; tip <= ctx.width + 10; tip += 2) {
    milestoneClear(ctx);
    drawHeart(ctx, heartLeft, 1, false);
    drawArrow(ctx, tip, 3);
    milestoneFrameShow(ctx, 45, 0);
  }

  for (int pulse = 0; pulse < 5; pulse++) {
    milestoneClear(ctx);
    drawHeart(ctx, heartLeft, pulse % 2, pulse % 2 == 0);
    for (int spark = 0; spark < 8; spark++) {
      float angle = spark * 0.785398f + pulse * 0.45f;
      int px = ctx.cx + (int)((pulse + 2) * cosf(angle) * 1.1f);
      int py = 3 + (int)((pulse + 2) * sinf(angle) * 0.6f);
      setValentinePixel(ctx, px, py, pulse % 2 == 0 || spark % 2 == 0);
    }
    milestoneFrameShow(ctx, 95, pulse % 2 == 0 ? 5 : 0);
  }

  for (int shower = 0; shower < 14; shower++) {
    milestoneClear(ctx);
    drawHeart(ctx, heartLeft, 1, shower % 2 == 0);
    for (int drop = 0; drop < 6; drop++) {
      int x = (drop * 5 + shower * 2) % ctx.width;
      int y = (shower + drop * 2) % ctx.height;
      setValentinePixel(ctx, x, y);
      setValentinePixel(ctx, x + 1, y);
    }
    uint8_t intensity = shower / 2;
    milestoneFrameShow(ctx, 45, intensity > 7 ? 7 : intensity);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Valentine's Day!");
}
