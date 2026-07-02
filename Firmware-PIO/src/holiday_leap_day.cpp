#include "holiday_leap_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setLeapPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setLeapPixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setLeapPixel(ctx, col, row);
  }
}

static void drawDigit(MilestoneCtx &ctx, int left, int top,
                      const uint8_t rows[5]) {
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      uint8_t mask = 1u << (2 - col);
      if ((rows[row] & mask) != 0) {
        setLeapPixel(ctx, left + col, top + row);
      }
    }
  }
}

static void drawBigDigit(MilestoneCtx &ctx, int left, int top,
                         const uint8_t rows[7]) {
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      uint8_t mask = 1u << (4 - col);
      if ((rows[row] & mask) != 0) {
        setLeapPixel(ctx, left + col, top + row);
      }
    }
  }
}

static void drawBig29(MilestoneCtx &ctx, int left, int top, uint8_t frame) {
  static const uint8_t BIG_2[] = {
      0b11110, 0b00001, 0b00001, 0b11110,
      0b10000, 0b10000, 0b11111,
  };
  static const uint8_t BIG_9[] = {
      0b11110, 0b10001, 0b10001, 0b11111,
      0b00001, 0b00001, 0b11110,
  };

  drawBigDigit(ctx, left, top + 1, BIG_2);
  drawBigDigit(ctx, left + 7, top + 1, BIG_9);

  // Small calendar-ring hint without shrinking the important "29".
  setLeapPixel(ctx, left + 1, top);
  setLeapPixel(ctx, left + 2, top);
  setLeapPixel(ctx, left + 10, top);
  setLeapPixel(ctx, left + 11, top);

  if (frame % 2 == 0) {
    setLeapPixel(ctx, left - 2, top + 3);
    setLeapPixel(ctx, left + 14, top + 3);
  }
}

static void drawFrog(MilestoneCtx &ctx, int cx, int groundRow, uint8_t frame) {
  int crouch = frame % 6 < 3 ? 0 : 1;
  int bodyY = groundRow - 2 - crouch;

  drawHorizontalLine(ctx, cx - 3, cx + 3, bodyY);
  drawHorizontalLine(ctx, cx - 4, cx + 4, bodyY + 1);
  drawHorizontalLine(ctx, cx - 2, cx + 2, bodyY - 1);

  setLeapPixel(ctx, cx - 2, bodyY - 2);
  setLeapPixel(ctx, cx + 2, bodyY - 2);
  setLeapPixel(ctx, cx - 2, bodyY - 1);
  setLeapPixel(ctx, cx + 2, bodyY - 1);

  setLeapPixel(ctx, cx - 5, groundRow);
  setLeapPixel(ctx, cx - 4, groundRow - 1);
  setLeapPixel(ctx, cx + 5, groundRow);
  setLeapPixel(ctx, cx + 4, groundRow - 1);

  if (frame % 4 == 0) {
    setLeapPixel(ctx, cx - 7, bodyY);
    setLeapPixel(ctx, cx + 7, bodyY);
  }
}

static void drawLeapArc(MilestoneCtx &ctx, uint8_t frame) {
  for (int dot = 0; dot < 8; dot++) {
    int x = 4 + dot * 5;
    int y = 6 - ((dot + frame / 2) % 5);
    setLeapPixel(ctx, x, y);
  }
}

static void drawPlusOne(MilestoneCtx &ctx, int left, int top) {
  static const uint8_t DIGIT_1[] = {0b010, 0b110, 0b010, 0b010, 0b111};

  setLeapPixel(ctx, left + 1, top + 2);
  drawHorizontalLine(ctx, left, left + 2, top + 2);
  drawVerticalLine(ctx, left + 1, top + 1, top + 3);
  drawDigit(ctx, left + 6, top, DIGIT_1);
}

static void drawExtraDaySparkles(MilestoneCtx &ctx, uint8_t frame) {
  for (int spark = 0; spark < 16; spark++) {
    int x = (spark * 7 + frame * 3) % ctx.width;
    int y = (spark * 5 + frame) % ctx.height;
    if ((spark + frame) % 3 != 0) {
      setLeapPixel(ctx, x, y);
    }
  }
}

void runLeapDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawBig29(ctx, ctx.cx - 6, 0, frame);
    milestoneFrameShow(ctx, 90, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 24; frame++) {
    milestoneClear(ctx);
    drawHorizontalLine(ctx, 0, ctx.width - 1, 7);
    drawLeapArc(ctx, frame);
    int frogX = 5 + (frame * (ctx.width - 10)) / 23;
    drawFrog(ctx, frogX, 7, frame);
    milestoneFrameShow(ctx, 60, frame % 5);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawPlusOne(ctx, ctx.cx - 5, 1);
    drawExtraDaySparkles(ctx, frame);
    milestoneFrameShow(ctx, 65, frame % 2 == 0 ? 6 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawBig29(ctx, ctx.cx - 6, 0, flash);
    } else {
      drawFrog(ctx, ctx.cx, 7, flash);
      drawPlusOne(ctx, ctx.cx + 10, 1);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 110 : 130,
                       flash % 2 == 0 ? 0 : 3);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Leap Day!");
}
