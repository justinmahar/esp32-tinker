#include "holiday_easter.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setEasterPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setEasterPixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setEasterPixel(ctx, col, row);
  }
}

static void drawEgg(MilestoneCtx &ctx, int cx, int top, uint8_t frame,
                    bool decorated = true) {
  static const uint8_t EGG_WIDTH = 15;
  static const uint8_t EGG_HEIGHT = 8;
  static const uint16_t EGG_ROWS[] = {
      0b000011100000000, 0b000111110000000, 0b001111111000000,
      0b011111111100000, 0b011111111100000, 0b001111111000000,
      0b000111110000000, 0b000011100000000,
  };

  int left = cx - EGG_WIDTH / 2;
  for (int row = 0; row < EGG_HEIGHT; row++) {
    for (int col = 0; col < EGG_WIDTH; col++) {
      uint16_t mask = 1u << (EGG_WIDTH - 1 - col);
      if ((EGG_ROWS[row] & mask) == 0) {
        continue;
      }

      bool stripeHole = decorated && (row == 2 || row == 5) &&
                        ((col + frame) % 3 == 0);
      bool zigzagHole = decorated && row == 4 && ((col + frame) % 4 == 0);
      if (!stripeHole && !zigzagHole) {
        setEasterPixel(ctx, left + col, top + row);
      }
    }
  }
}

static void drawCrackedEgg(MilestoneCtx &ctx, int cx, uint8_t frame) {
  drawEgg(ctx, cx, 0, frame, false);
  for (int step = 0; step < 9; step++) {
    int x = cx - 4 + step;
    int y = 3 + (step % 2);
    setEasterPixel(ctx, x, y, false);
    setEasterPixel(ctx, x, y + 1, false);
  }

  if (frame % 2 == 0) {
    setEasterPixel(ctx, cx - 8, 2);
    setEasterPixel(ctx, cx + 8, 2);
  }
}

static void drawChick(MilestoneCtx &ctx, int cx, int baseRow, uint8_t frame) {
  int top = baseRow - 6;
  drawHorizontalLine(ctx, cx - 4, cx + 4, top + 2);
  drawHorizontalLine(ctx, cx - 5, cx + 5, top + 3);
  drawHorizontalLine(ctx, cx - 5, cx + 5, top + 4);
  drawHorizontalLine(ctx, cx - 3, cx + 3, top + 5);
  drawHorizontalLine(ctx, cx - 2, cx + 2, top + 6);

  setEasterPixel(ctx, cx - 2, top + 2);
  setEasterPixel(ctx, cx + 2, top + 2);
  setEasterPixel(ctx, cx + 5, top + 3);
  setEasterPixel(ctx, cx + 6, top + 3);
  setEasterPixel(ctx, cx + 5, top + 4);

  if (frame % 2 == 0) {
    setEasterPixel(ctx, cx - 6, top + 4);
    setEasterPixel(ctx, cx + 7, top + 4);
  }
}

static void drawBunnyHead(MilestoneCtx &ctx, int cx, uint8_t frame) {
  int earLift = frame % 6 < 3 ? 0 : 1;

  drawVerticalLine(ctx, cx - 5, 0, 3 - earLift);
  drawVerticalLine(ctx, cx - 4, 1, 4 - earLift);
  drawVerticalLine(ctx, cx + 4, 1, 4 - earLift);
  drawVerticalLine(ctx, cx + 5, 0, 3 - earLift);

  drawHorizontalLine(ctx, cx - 7, cx + 7, 4);
  drawHorizontalLine(ctx, cx - 8, cx + 8, 5);
  drawHorizontalLine(ctx, cx - 6, cx + 6, 6);
  drawHorizontalLine(ctx, cx - 4, cx + 4, 7);

  setEasterPixel(ctx, cx - 3, 5);
  setEasterPixel(ctx, cx + 3, 5);
  setEasterPixel(ctx, cx, 6);
  setEasterPixel(ctx, cx - 1, 7);
  setEasterPixel(ctx, cx + 1, 7);
}

static void drawSmallEgg(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setEasterPixel(ctx, cx, cy - 1);
  setEasterPixel(ctx, cx - 1, cy);
  setEasterPixel(ctx, cx, cy);
  setEasterPixel(ctx, cx + 1, cy);
  setEasterPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setEasterPixel(ctx, cx - 1, cy + 1);
    setEasterPixel(ctx, cx + 1, cy - 1);
  }
}

static void drawEggTrail(MilestoneCtx &ctx, uint8_t frame) {
  for (int egg = 0; egg < 6; egg++) {
    int x = (egg * 9 + frame * 2) % (ctx.width + 8) - 4;
    int y = egg % 2 == 0 ? 5 : 6;
    drawSmallEgg(ctx, x, y, frame + egg);
  }
}

static void drawSpringSparkles(MilestoneCtx &ctx, uint8_t frame) {
  for (int spark = 0; spark < 14; spark++) {
    int x = (spark * 7 + frame * 3) % ctx.width;
    int y = (spark * 5 + frame) % ctx.height;
    if ((spark + frame) % 3 != 0) {
      setEasterPixel(ctx, x, y);
    }
  }
}

void runEasterHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawEgg(ctx, ctx.cx, 0, frame);
    milestoneFrameShow(ctx, 80, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    drawCrackedEgg(ctx, ctx.cx, frame);
    milestoneFrameShow(ctx, 100, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawChick(ctx, ctx.cx, 7, frame);
    drawSpringSparkles(ctx, frame);
    milestoneFrameShow(ctx, 80, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawBunnyHead(ctx, ctx.cx, frame);
    milestoneFrameShow(ctx, 95, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawEggTrail(ctx, frame);
    drawBunnyHead(ctx, ctx.cx - 14 + frame / 2, frame);
    milestoneFrameShow(ctx, 70, frame % 5);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawEgg(ctx, ctx.cx, 0, flash);
    } else {
      drawBunnyHead(ctx, ctx.cx, flash);
      drawSmallEgg(ctx, ctx.cx + 14, 5, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 100 : 120,
                       flash % 2 == 0 ? 0 : 3);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Easter!");
}
