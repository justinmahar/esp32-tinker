#include "holiday_april_fools.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setAprilPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawQuestionMark(MilestoneCtx &ctx, int leftCol, int topRow,
                             bool sparkle) {
  static const uint8_t MARK_WIDTH = 7;
  static const uint8_t MARK_HEIGHT = 8;
  static const uint8_t MARK_ROWS[] = {
      0b0111100, 0b1000010, 0b0000010, 0b0001100,
      0b0011000, 0b0000000, 0b0011000, 0b0011000,
  };

  for (int row = 0; row < MARK_HEIGHT; row++) {
    for (int col = 0; col < MARK_WIDTH; col++) {
      uint8_t mask = 1u << (MARK_WIDTH - 1 - col);
      if ((MARK_ROWS[row] & mask) == 0) {
        continue;
      }

      bool hole = sparkle && ((row * 3 + col) % 7 == 0);
      if (!hole) {
        setAprilPixel(ctx, leftCol + col, topRow + row);
      }
    }
  }
}

static void drawJesterHat(MilestoneCtx &ctx, int centerX, uint8_t frame) {
  static const uint8_t HAT_WIDTH = 21;
  static const uint8_t HAT_HEIGHT = 8;
  static const uint32_t HAT_ROWS[] = {
      0b000000000010000000000, 0b000010000111000010000,
      0b000111001111100111000, 0b001111111111111111100,
      0b000011111111111110000, 0b000001111111111100000,
      0b000000111111111000000, 0b000000011111110000000,
  };

  int leftCol = centerX - HAT_WIDTH / 2;
  for (int row = 0; row < HAT_HEIGHT; row++) {
    int wobble = ((frame + row) % 4 == 0) ? 1 : 0;
    for (int col = 0; col < HAT_WIDTH; col++) {
      uint32_t mask = 1UL << (HAT_WIDTH - 1 - col);
      if ((HAT_ROWS[row] & mask) != 0) {
        setAprilPixel(ctx, leftCol + col + wobble, row);
      }
    }
  }

  setAprilPixel(ctx, leftCol + 2 + frame % 2, 1);
  setAprilPixel(ctx, leftCol + HAT_WIDTH - 3 - frame % 2, 1);
}

static void drawGlitchBars(MilestoneCtx &ctx, uint8_t frame) {
  for (int bar = 0; bar < 7; bar++) {
    int y = (bar * 3 + frame) % ctx.height;
    int start = (bar * 11 + frame * 5) % ctx.width;
    int length = 3 + ((bar + frame) % 7);
    for (int col = 0; col < length; col++) {
      setAprilPixel(ctx, start + col, y, (col + frame) % 3 != 0);
    }
  }
}

static void drawPrankFace(MilestoneCtx &ctx, uint8_t frame) {
  int cx = ctx.cx;
  int grinY = 5;
  setAprilPixel(ctx, cx - 6, 2);
  setAprilPixel(ctx, cx - 5, 2 + frame % 2);
  setAprilPixel(ctx, cx + 5, 2);
  setAprilPixel(ctx, cx + 6, 2 + (frame + 1) % 2);

  for (int col = -5; col <= 5; col++) {
    bool toothGap = (col + frame) % 4 == 0;
    setAprilPixel(ctx, cx + col, grinY + (col < 0 ? 0 : 1), !toothGap);
  }
  setAprilPixel(ctx, cx - 6, grinY);
  setAprilPixel(ctx, cx + 6, grinY + 1);
}

static void drawConfetti(MilestoneCtx &ctx, uint8_t frame) {
  for (int piece = 0; piece < 16; piece++) {
    int x = (piece * 9 + frame * 4) % ctx.width;
    int y = (piece * 5 + frame) % ctx.height;
    setAprilPixel(ctx, x, y);
    if ((piece + frame) % 3 == 0) {
      setAprilPixel(ctx, x + 1, y);
    }
  }
}

void runAprilFoolsHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    drawQuestionMark(ctx, ctx.cx - 12 + frame % 3, 0, frame % 2 == 0);
    drawQuestionMark(ctx, ctx.cx + 5 - frame % 3, 0, frame % 2 == 1);
    drawGlitchBars(ctx, frame);
    milestoneFrameShow(ctx, 65, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawJesterHat(ctx, ctx.cx, frame);
    if (frame % 3 == 0) {
      drawConfetti(ctx, frame);
    }
    milestoneFrameShow(ctx, 75, frame % 6);
  }

  for (uint8_t frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    drawPrankFace(ctx, frame);
    if (frame % 2 == 0) {
      drawQuestionMark(ctx, ctx.cx - 17, 0, false);
      drawQuestionMark(ctx, ctx.cx + 11, 0, false);
    }
    milestoneFrameShow(ctx, 90, frame % 2 == 0 ? 6 : 0);
  }

  for (int flash = 0; flash < 5; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawGlitchBars(ctx, flash * 3);
      drawPrankFace(ctx, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 45 : 85,
                       flash % 2 == 0 ? 7 : 0);
  }

  for (uint8_t frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    drawConfetti(ctx, frame);
    drawJesterHat(ctx, ctx.cx, frame);
    milestoneFrameShow(ctx, 48, frame % 2 == 0 ? 5 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "April Fools!");
}
