#include "holiday_groundhog.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setGroundhogPixel(MilestoneCtx &ctx, int col, int row,
                              bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + col, on);
  }
}

static void drawBurrow(MilestoneCtx &ctx, int centerX, uint8_t sparkleFrame) {
  for (int col = 0; col < ctx.width; col++) {
    int centerDistance = col > centerX ? col - centerX : centerX - col;
    bool mound = centerDistance < 9 && ((col + sparkleFrame) % 3 != 0);
    if (mound || col % 2 == 0) {
      setGroundhogPixel(ctx, col, 7);
    }
    if (mound && centerDistance < 7) {
      setGroundhogPixel(ctx, col, 6);
    }
  }

  for (int col = centerX - 5; col <= centerX + 5; col++) {
    setGroundhogPixel(ctx, col, 7, false);
  }
}

static void drawSun(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setGroundhogPixel(ctx, cx, cy);
  setGroundhogPixel(ctx, cx - 1, cy);
  setGroundhogPixel(ctx, cx + 1, cy);
  setGroundhogPixel(ctx, cx, cy - 1);
  setGroundhogPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setGroundhogPixel(ctx, cx - 2, cy);
    setGroundhogPixel(ctx, cx + 2, cy);
    setGroundhogPixel(ctx, cx, cy + 2);
  }
}

static void drawGroundhog(MilestoneCtx &ctx, int centerX, int baseRow,
                          uint8_t peekRows, bool blink) {
  static const uint8_t HOG_WIDTH = 9;
  static const uint8_t HOG_HEIGHT = 6;
  static const uint16_t HOG_ROWS[] = {
      0b000111000, 0b001111100, 0b011111110,
      0b011111110, 0b001111100, 0b000111000,
  };
  static const uint16_t FACE_ROWS[] = {
      0b000000000, 0b000000000, 0b001010100,
      0b000000000, 0b000111000, 0b000000000,
  };

  int leftCol = centerX - HOG_WIDTH / 2;
  int topRow = baseRow - peekRows;
  for (int row = 0; row < HOG_HEIGHT; row++) {
    int displayRow = topRow + row;
    if (displayRow < 0 || displayRow > baseRow) {
      continue;
    }

    for (int col = 0; col < HOG_WIDTH; col++) {
      uint16_t mask = 1u << (HOG_WIDTH - 1 - col);
      if ((HOG_ROWS[row] & mask) == 0) {
        continue;
      }

      bool faceCutout = (FACE_ROWS[row] & mask) != 0;
      bool blinkingEye = blink && row == 2 && (col == 2 || col == 6);
      if (!faceCutout || blinkingEye) {
        setGroundhogPixel(ctx, leftCol + col, displayRow);
      }
    }
  }

  setGroundhogPixel(ctx, centerX - 3, topRow + 1);
  setGroundhogPixel(ctx, centerX + 3, topRow + 1);
}

static void drawShadow(MilestoneCtx &ctx, int startX, uint8_t frame) {
  for (int col = 0; col < 12; col++) {
    int x = startX + col;
    int y = 6 + ((col + frame) % 2);
    setGroundhogPixel(ctx, x, y);
  }
}

void runGroundhogHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    drawBurrow(ctx, ctx.cx, frame);
    drawSun(ctx, ctx.width - 4, 2, frame);
    milestoneFrameShow(ctx, 70, frame % 4);
  }

  for (uint8_t peek = 1; peek <= 6; peek++) {
    milestoneClear(ctx);
    drawBurrow(ctx, ctx.cx, peek);
    drawSun(ctx, ctx.width - 4, 2, peek);
    drawGroundhog(ctx, ctx.cx, 6, peek, false);
    milestoneFrameShow(ctx, 120, peek);
  }

  for (int look = 0; look < 6; look++) {
    milestoneClear(ctx);
    drawBurrow(ctx, ctx.cx, look);
    drawSun(ctx, ctx.width - 4 - look, 2, look);
    drawGroundhog(ctx, ctx.cx, 6, 6, look % 3 == 0);
    milestoneFrameShow(ctx, 130, look % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    drawBurrow(ctx, ctx.cx, frame);
    drawSun(ctx, ctx.width - 10 + frame / 2, 1, frame);
    drawGroundhog(ctx, ctx.cx, 6, 6, false);
    drawShadow(ctx, ctx.cx + 3, frame);
    uint8_t intensity = frame / 2;
    milestoneFrameShow(ctx, 75, intensity > 7 ? 7 : intensity);
  }

  for (int surprised = 0; surprised < 4; surprised++) {
    milestoneClear(ctx);
    drawBurrow(ctx, ctx.cx, surprised);
    drawGroundhog(ctx, ctx.cx, 6, surprised % 2 == 0 ? 6 : 4, false);
    drawShadow(ctx, ctx.cx + 3, surprised);
    setGroundhogPixel(ctx, ctx.cx - 8, 2);
    setGroundhogPixel(ctx, ctx.cx + 8, 2);
    milestoneFrameShow(ctx, 130, surprised % 2 == 0 ? 7 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Groundhog Day!");
}
