#include "holiday_labor_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setLaborPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setLaborPixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setLaborPixel(ctx, col, row);
  }
}

static void drawHardHat(MilestoneCtx &ctx, int cx, int top, uint8_t reveal) {
  static const uint8_t HAT_WIDTH = 23;
  static const uint8_t HAT_HEIGHT = 7;
  static const uint32_t HAT_ROWS[] = {
      0b00000001110000000000000, 0b00000111111110000000000,
      0b00011111111111100000000, 0b00111111111111110000000,
      0b01111111111111111000000, 0b11111111111111111100000,
      0b00111111111111110000000,
  };

  int left = cx - HAT_WIDTH / 2;
  for (uint8_t row = 0; row < HAT_HEIGHT && row < reveal; row++) {
    for (uint8_t col = 0; col < HAT_WIDTH; col++) {
      uint32_t mask = 1UL << (HAT_WIDTH - 1 - col);
      if ((HAT_ROWS[row] & mask) != 0) {
        setLaborPixel(ctx, left + col, top + row);
      }
    }
  }

  if (reveal >= HAT_HEIGHT) {
    drawVerticalLine(ctx, cx, top + 1, top + 5);
    drawVerticalLine(ctx, cx - 5, top + 2, top + 5);
    drawVerticalLine(ctx, cx + 5, top + 2, top + 5);
  }
}

static void drawHammer(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  drawHorizontalLine(ctx, cx - 7, cx + 5, cy - 3);
  drawHorizontalLine(ctx, cx - 8, cx - 3, cy - 2);
  for (int step = 0; step < 8; step++) {
    setLaborPixel(ctx, cx - 2 + step, cy - 1 + step / 2);
  }
  if (frame % 2 == 0) {
    setLaborPixel(ctx, cx + 6, cy + 3);
    setLaborPixel(ctx, cx + 7, cy + 4);
  }
}

static void drawWrench(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  drawHorizontalLine(ctx, cx - 5, cx + 7, cy + 3);
  setLaborPixel(ctx, cx - 7, cy + 1);
  setLaborPixel(ctx, cx - 6, cy + 2);
  setLaborPixel(ctx, cx - 7, cy + 3);
  setLaborPixel(ctx, cx - 6, cy + 4);
  setLaborPixel(ctx, cx - 4, cy + 2);
  setLaborPixel(ctx, cx - 4, cy + 4);
  if (frame % 2 == 1) {
    setLaborPixel(ctx, cx + 8, cy + 2);
    setLaborPixel(ctx, cx + 8, cy + 4);
  }
}

static void drawWorkSpark(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setLaborPixel(ctx, cx, cy);
  setLaborPixel(ctx, cx - 1, cy);
  setLaborPixel(ctx, cx + 1, cy);
  setLaborPixel(ctx, cx, cy - 1);
  setLaborPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setLaborPixel(ctx, cx - 2, cy);
    setLaborPixel(ctx, cx + 2, cy);
  }
}

static void drawWorksite(MilestoneCtx &ctx, uint8_t frame) {
  drawHorizontalLine(ctx, 0, ctx.width - 1, 7);

  for (int beam = 0; beam < 4; beam++) {
    int x = 4 + beam * 7;
    drawVerticalLine(ctx, x, 2 + beam % 2, 7);
    if (beam > 0) {
      for (int step = 0; step < 7; step++) {
        setLaborPixel(ctx, x - 7 + step, 2 + step % 2 + beam % 2);
      }
    }
  }

  int hookX = 34 + (frame % 8);
  drawHorizontalLine(ctx, 30, 45, 1);
  drawVerticalLine(ctx, hookX, 1, 4);
  setLaborPixel(ctx, hookX - 1, 5);
  setLaborPixel(ctx, hookX, 5);
}

static void drawSun(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setLaborPixel(ctx, cx, cy);
  setLaborPixel(ctx, cx - 1, cy);
  setLaborPixel(ctx, cx + 1, cy);
  setLaborPixel(ctx, cx, cy - 1);
  setLaborPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setLaborPixel(ctx, cx - 2, cy);
    setLaborPixel(ctx, cx + 2, cy);
    setLaborPixel(ctx, cx, cy + 2);
  }
}

static void drawRestScene(MilestoneCtx &ctx, uint8_t frame) {
  drawHorizontalLine(ctx, 0, ctx.width - 1, 7);
  drawSun(ctx, ctx.width - 6, 2, frame);

  int chairX = ctx.cx - 8;
  drawHorizontalLine(ctx, chairX, chairX + 10, 5);
  drawVerticalLine(ctx, chairX + 2, 5, 7);
  drawVerticalLine(ctx, chairX + 9, 5, 7);
  setLaborPixel(ctx, chairX + 4, 4);
  setLaborPixel(ctx, chairX + 5, 3);
  setLaborPixel(ctx, chairX + 6, 4);

  if (frame % 2 == 0) {
    setLaborPixel(ctx, chairX + 14, 5);
    setLaborPixel(ctx, chairX + 15, 4);
    setLaborPixel(ctx, chairX + 16, 5);
  }
}

void runLaborDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t reveal = 1; reveal <= 7; reveal++) {
    milestoneClear(ctx);
    drawHardHat(ctx, ctx.cx, 0, reveal);
    milestoneFrameShow(ctx, 120, reveal);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawHardHat(ctx, ctx.cx, 0, 7);
    if (frame % 4 < 2) {
      drawWorkSpark(ctx, ctx.cx + 14, 3, frame);
    }
    milestoneFrameShow(ctx, 70, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawHammer(ctx, ctx.cx - 6, 3, frame);
    drawWrench(ctx, ctx.cx + 8, 2, frame);
    drawWorkSpark(ctx, ctx.cx, 3, frame);
    milestoneFrameShow(ctx, 75, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawWorksite(ctx, frame);
    milestoneFrameShow(ctx, 80, frame % 5);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawRestScene(ctx, frame);
    milestoneFrameShow(ctx, 95, frame % 2 == 0 ? 5 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawHardHat(ctx, ctx.cx, 0, 7);
    } else {
      drawRestScene(ctx, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 100 : 130,
                       flash % 2 == 0 ? 4 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Labor Day!");
}
