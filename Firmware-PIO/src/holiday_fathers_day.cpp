#include "holiday_fathers_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setFathersPixel(MilestoneCtx &ctx, int col, int row,
                            bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setFathersPixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setFathersPixel(ctx, col, row);
  }
}

static void drawMug(MilestoneCtx &ctx, int left, int top, uint8_t frame) {
  const int cupTop = top + 3;
  const int cupBottom = top + 7;

  drawHorizontalLine(ctx, left + 1, left + 11, cupTop);
  drawHorizontalLine(ctx, left + 2, left + 10, cupTop + 1);
  drawHorizontalLine(ctx, left + 2, left + 10, cupBottom);
  drawVerticalLine(ctx, left + 1, cupTop + 1, cupBottom - 1);
  drawVerticalLine(ctx, left + 11, cupTop + 1, cupBottom - 1);

  setFathersPixel(ctx, left + 13, cupTop + 1);
  setFathersPixel(ctx, left + 15, cupTop + 2);
  setFathersPixel(ctx, left + 15, cupTop + 3);
  setFathersPixel(ctx, left + 13, cupBottom - 1);
  setFathersPixel(ctx, left + 14, cupTop + 1);
  setFathersPixel(ctx, left + 14, cupBottom - 1);

  int drift = frame % 4;
  setFathersPixel(ctx, left + 3 + (drift == 1 ? 1 : 0), top + 2);
  setFathersPixel(ctx, left + 4 + (drift == 2 ? 1 : 0), top + 1);
  setFathersPixel(ctx, left + 3 + (drift == 3 ? 1 : 0), top);

  setFathersPixel(ctx, left + 7 + (drift == 0 ? 1 : 0), top + 2);
  setFathersPixel(ctx, left + 8 + (drift == 1 ? 1 : 0), top + 1);
  setFathersPixel(ctx, left + 7 + (drift == 2 ? 1 : 0), top);

  setFathersPixel(ctx, left + 10 + (drift == 2 ? 1 : 0), top + 2);
  setFathersPixel(ctx, left + 9 + (drift == 3 ? 1 : 0), top + 1);
}

static void drawSparkles(MilestoneCtx &ctx, uint8_t frame) {
  for (int spark = 0; spark < 12; spark++) {
    int x = (spark * 7 + frame * 3) % ctx.width;
    int y = (spark * 5 + frame) % ctx.height;
    if ((spark + frame) % 3 != 0) {
      setFathersPixel(ctx, x, y);
    }
  }
}

static void drawWheel(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setFathersPixel(ctx, cx - 1, cy - 2);
  setFathersPixel(ctx, cx, cy - 2);
  setFathersPixel(ctx, cx + 1, cy - 2);
  setFathersPixel(ctx, cx - 2, cy - 1);
  setFathersPixel(ctx, cx + 2, cy - 1);
  setFathersPixel(ctx, cx - 2, cy);
  setFathersPixel(ctx, cx + 2, cy);
  setFathersPixel(ctx, cx - 2, cy + 1);
  setFathersPixel(ctx, cx + 2, cy + 1);
  setFathersPixel(ctx, cx - 1, cy + 2);
  setFathersPixel(ctx, cx, cy + 2);
  setFathersPixel(ctx, cx + 1, cy + 2);

  setFathersPixel(ctx, cx, cy);
  if (frame % 2 == 0) {
    setFathersPixel(ctx, cx - 1, cy);
    setFathersPixel(ctx, cx + 1, cy);
  } else {
    setFathersPixel(ctx, cx, cy - 1);
    setFathersPixel(ctx, cx, cy + 1);
  }
}

static void drawMotorcycle(MilestoneCtx &ctx, int left, uint8_t frame) {
  drawWheel(ctx, left + 5, 5, frame);
  drawWheel(ctx, left + 18, 5, frame + 1);

  drawHorizontalLine(ctx, left + 5, left + 18, 3);
  drawHorizontalLine(ctx, left + 8, left + 15, 2);
  setFathersPixel(ctx, left + 7, 4);
  setFathersPixel(ctx, left + 16, 4);
  setFathersPixel(ctx, left + 10, 1);
  setFathersPixel(ctx, left + 11, 1);
  setFathersPixel(ctx, left + 12, 2);

  setFathersPixel(ctx, left + 18, 3);
  setFathersPixel(ctx, left + 20, 2);
  setFathersPixel(ctx, left + 21, 2);

  setFathersPixel(ctx, left + 11, 0);
  setFathersPixel(ctx, left + 10, 0);
  setFathersPixel(ctx, left + 9, 1);
  setFathersPixel(ctx, left + 13, 1);

  if (frame % 3 == 0) {
    setFathersPixel(ctx, left - 1, 4);
    setFathersPixel(ctx, left - 3, 5);
  }
}

static void drawLetter(MilestoneCtx &ctx, int left, int top,
                       const uint8_t rows[5], uint8_t width) {
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < width; col++) {
      uint8_t mask = 1u << (width - 1 - col);
      if ((rows[row] & mask) != 0) {
        setFathersPixel(ctx, left + col, top + row);
      }
    }
  }
}

static void drawNumberOneDad(MilestoneCtx &ctx, int left, int top,
                             uint8_t frame) {
  static const uint8_t HASH_ROWS[] = {0b01010, 0b11111, 0b01010, 0b11111,
                                      0b01010};
  static const uint8_t ONE_ROWS[] = {0b010, 0b110, 0b010, 0b010, 0b111};
  static const uint8_t D_ROWS[] = {0b110, 0b101, 0b101, 0b101, 0b110};
  static const uint8_t A_ROWS[] = {0b010, 0b101, 0b111, 0b101, 0b101};

  drawLetter(ctx, left, top, HASH_ROWS, 5);
  drawLetter(ctx, left + 7, top, ONE_ROWS, 3);
  drawLetter(ctx, left + 13, top, D_ROWS, 3);
  drawLetter(ctx, left + 17, top, A_ROWS, 3);
  drawLetter(ctx, left + 21, top, D_ROWS, 3);

  if (frame % 2 == 0) {
    setFathersPixel(ctx, left - 2, top + 2);
    setFathersPixel(ctx, left + 25, top + 2);
  }
}

void runFathersDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 30; frame++) {
    milestoneClear(ctx);
    drawHorizontalLine(ctx, 0, ctx.width - 1, 7);
    int motoX = -22 + (frame * (ctx.width + 26)) / 29;
    drawMotorcycle(ctx, motoX, frame);
    milestoneFrameShow(ctx, 55, frame % 5);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawMug(ctx, ctx.cx - 8, 0, frame);
    milestoneFrameShow(ctx, 90, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawNumberOneDad(ctx, ctx.cx - 12, 1, frame);
    drawSparkles(ctx, frame);
    milestoneFrameShow(ctx, 75, frame % 2 == 0 ? 5 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawMotorcycle(ctx, ctx.cx - 11, flash);
    } else {
      drawNumberOneDad(ctx, ctx.cx - 12, 1, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 100 : 120,
                       flash % 2 == 0 ? 0 : 3);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Father's Day!");
}
