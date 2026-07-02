#include "holiday_new_years_eve.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setNewYearPixel(MilestoneCtx &ctx, int col, int row,
                            bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawDigit(MilestoneCtx &ctx, int leftCol, uint8_t digit) {
  static const uint8_t DIGIT_WIDTH = 5;
  static const uint8_t DIGIT_HEIGHT = 7;
  static const uint8_t DIGITS[][DIGIT_HEIGHT] = {
      {0b11111, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b11111},
      {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
      {0b11110, 0b00001, 0b00001, 0b11110, 0b10000, 0b10000, 0b11111},
      {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110},
  };

  if (digit > 3) {
    return;
  }

  for (int row = 0; row < DIGIT_HEIGHT; row++) {
    for (int col = 0; col < DIGIT_WIDTH; col++) {
      uint8_t mask = 1u << (DIGIT_WIDTH - 1 - col);
      if ((DIGITS[digit][row] & mask) != 0) {
        setNewYearPixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawSparkle(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setNewYearPixel(ctx, cx, cy);
  setNewYearPixel(ctx, cx - 1, cy);
  setNewYearPixel(ctx, cx + 1, cy);
  setNewYearPixel(ctx, cx, cy - 1);
  setNewYearPixel(ctx, cx, cy + 1);
  if (wide) {
    setNewYearPixel(ctx, cx - 2, cy);
    setNewYearPixel(ctx, cx + 2, cy);
  }
}

static void drawBall(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setNewYearPixel(ctx, cx, cy - 1);
  setNewYearPixel(ctx, cx - 1, cy);
  setNewYearPixel(ctx, cx, cy);
  setNewYearPixel(ctx, cx + 1, cy);
  setNewYearPixel(ctx, cx, cy + 1);

  if (frame % 2 == 0) {
    setNewYearPixel(ctx, cx - 1, cy - 1);
    setNewYearPixel(ctx, cx + 1, cy - 1);
    setNewYearPixel(ctx, cx - 1, cy + 1);
    setNewYearPixel(ctx, cx + 1, cy + 1);
  }
}

static void drawBallDrop(MilestoneCtx &ctx, int ballY, uint8_t frame) {
  int towerX = ctx.cx;
  for (int row = 0; row < ctx.height; row++) {
    setNewYearPixel(ctx, towerX - 3, row, row % 2 == 0);
    setNewYearPixel(ctx, towerX + 3, row, row % 2 == 1);
  }

  for (int row = 0; row <= ballY && row < ctx.height; row++) {
    setNewYearPixel(ctx, towerX, row);
  }

  drawBall(ctx, towerX, ballY, frame);
}

static void drawConfetti(MilestoneCtx &ctx, uint8_t frame) {
  for (int piece = 0; piece < 18; piece++) {
    int x = (piece * 7 + frame * 3) % ctx.width;
    int y = (piece * 5 + frame) % ctx.height;
    bool dash = (piece + frame) % 3 == 0;

    setNewYearPixel(ctx, x, y);
    if (dash) {
      setNewYearPixel(ctx, x + 1, y);
    }
  }
}

void runNewYearsEveHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (int digit = 3; digit >= 1; digit--) {
    for (int blink = 0; blink < 2; blink++) {
      milestoneClear(ctx);
      drawDigit(ctx, ctx.cx - 2, digit);
      if (blink == 0) {
        drawSparkle(ctx, ctx.cx - 8, 1 + digit, true);
        drawSparkle(ctx, ctx.cx + 8, 6 - digit, false);
      }
      milestoneFrameShow(ctx, blink == 0 ? 230 : 90, blink == 0 ? 9 : 0);
    }
  }

  for (int frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    int ballY = 1 + (frame * 5) / 15;
    drawBallDrop(ctx, ballY, frame);
    if (frame % 3 == 0) {
      drawSparkle(ctx, ctx.cx - 10 + frame, 1 + frame % 5, frame % 2 == 0);
    }
    milestoneFrameShow(ctx, 62, frame / 2);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawBallDrop(ctx, 6, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 60 : 95,
                       flash % 2 == 0 ? 6 : 0);
  }

  const int burstX[5] = {ctx.width / 6, ctx.width / 3, ctx.width / 2,
                         2 * ctx.width / 3, 5 * ctx.width / 6};
  const int burstY[5] = {2, 5, 1, 4, 3};
  for (int frame = 0; frame < 24; frame++) {
    milestoneClear(ctx);
    for (int burst = 0; burst < 5; burst++) {
      int localFrame = frame - burst * 2;
      if (localFrame >= 0) {
        milestoneDrawFireworkBurst(ctx, burstX[burst], burstY[burst],
                                   localFrame, 10, 0.62f);
      }
    }
    milestoneFrameShow(ctx, 42, min(6, frame / 2));
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawConfetti(ctx, frame);
    if (frame % 2 == 0) {
      drawSparkle(ctx, ctx.cx, 3, true);
    }
    milestoneFrameShow(ctx, 48, frame % 2 == 0 ? 5 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy New Year!");
}
