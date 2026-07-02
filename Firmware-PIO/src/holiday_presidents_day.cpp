#include "holiday_presidents_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setPresidentsPixel(MilestoneCtx &ctx, int col, int row,
                               bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setPresidentsPixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setPresidentsPixel(ctx, col, row);
  }
}

static void drawFlagWave(MilestoneCtx &ctx, uint8_t frame) {
  for (int row = 0; row < ctx.height; row++) {
    for (int col = 0; col < ctx.width; col++) {
      bool canton = row < 4 && col < 11;
      bool stripe = ((row + frame / 2) % 2) == 0;
      bool wave = ((col + frame + row * 2) % 9) < 5;
      bool star = canton && ((col + row * 3 + frame) % 5 == 0);

      if ((canton && star) || (!canton && stripe && wave)) {
        setPresidentsPixel(ctx, col, row);
      }
    }
  }
}

static void drawTopHatProfile(MilestoneCtx &ctx, int cx, uint8_t frame) {
  drawHorizontalLine(ctx, cx - 10, cx + 10, 7);
  drawHorizontalLine(ctx, cx - 8, cx + 8, 6);
  drawHorizontalLine(ctx, cx - 5, cx + 5, 1);
  drawHorizontalLine(ctx, cx - 5, cx + 5, 2);
  drawVerticalLine(ctx, cx - 5, 2, 5);
  drawVerticalLine(ctx, cx + 5, 2, 5);
  drawHorizontalLine(ctx, cx - 6, cx + 6, 5);

  drawHorizontalLine(ctx, cx - 3, cx + 3, 3);
  drawHorizontalLine(ctx, cx - 4, cx + 4, 4);
  setPresidentsPixel(ctx, cx - 2, 4);
  setPresidentsPixel(ctx, cx + 2, 4);
  setPresidentsPixel(ctx, cx, 5);

  if (frame % 2 == 0) {
    setPresidentsPixel(ctx, cx - 8, 2);
    setPresidentsPixel(ctx, cx + 8, 2);
  }
}

static void drawColumns(MilestoneCtx &ctx, uint8_t frame) {
  drawHorizontalLine(ctx, ctx.cx - 16, ctx.cx + 16, 1);
  drawHorizontalLine(ctx, ctx.cx - 14, ctx.cx + 14, 2);
  drawHorizontalLine(ctx, ctx.cx - 18, ctx.cx + 18, 7);

  for (int column = -12; column <= 12; column += 6) {
    drawVerticalLine(ctx, ctx.cx + column, 3, 6);
  }

  if (frame % 2 == 0) {
    setPresidentsPixel(ctx, ctx.cx - 17, 0);
    setPresidentsPixel(ctx, ctx.cx + 17, 0);
  }
}

static void drawStar(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setPresidentsPixel(ctx, cx, cy);
  setPresidentsPixel(ctx, cx - 1, cy);
  setPresidentsPixel(ctx, cx + 1, cy);
  setPresidentsPixel(ctx, cx, cy - 1);
  setPresidentsPixel(ctx, cx, cy + 1);
  if (wide) {
    setPresidentsPixel(ctx, cx - 2, cy);
    setPresidentsPixel(ctx, cx + 2, cy);
  }
}

static void drawStarField(MilestoneCtx &ctx, uint8_t frame) {
  for (int star = 0; star < 14; star++) {
    int x = (star * 7 + frame * 2) % ctx.width;
    int y = (star * 5 + frame) % ctx.height;
    if ((star + frame) % 3 != 0) {
      setPresidentsPixel(ctx, x, y);
    }
  }
}

static void drawBigUSA(MilestoneCtx &ctx, int left, int top, uint8_t frame) {
  static const uint8_t U_ROWS[] = {0b101, 0b101, 0b101, 0b101, 0b111};
  static const uint8_t S_ROWS[] = {0b111, 0b100, 0b111, 0b001, 0b111};
  static const uint8_t A_ROWS[] = {0b010, 0b101, 0b111, 0b101, 0b101};
  const uint8_t *letters[] = {U_ROWS, S_ROWS, A_ROWS};

  for (int letter = 0; letter < 3; letter++) {
    int letterLeft = left + letter * 5;
    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 3; col++) {
        uint8_t mask = 1u << (2 - col);
        if ((letters[letter][row] & mask) != 0) {
          setPresidentsPixel(ctx, letterLeft + col, top + row);
        }
      }
    }
  }

  if (frame % 2 == 0) {
    drawStar(ctx, left + 18, top + 2, false);
  }
}

void runPresidentsDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawFlagWave(ctx, frame);
    milestoneFrameShow(ctx, 60, frame % 5);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawTopHatProfile(ctx, ctx.cx, frame);
    milestoneFrameShow(ctx, 95, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawColumns(ctx, frame);
    drawStarField(ctx, frame);
    milestoneFrameShow(ctx, 85, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawBigUSA(ctx, ctx.cx - 8, 1, frame);
    drawStarField(ctx, frame);
    milestoneFrameShow(ctx, 70, frame % 2 == 0 ? 6 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawFlagWave(ctx, flash);
    } else {
      drawTopHatProfile(ctx, ctx.cx, flash);
      drawStar(ctx, ctx.cx + 14, 2, true);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 95 : 130,
                       flash % 2 == 0 ? 0 : 5);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Presidents Day!");
}
