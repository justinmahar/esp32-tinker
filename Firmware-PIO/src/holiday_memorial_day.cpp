#include "holiday_memorial_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setMemorialPixel(MilestoneCtx &ctx, int col, int row,
                             bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setMemorialPixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setMemorialPixel(ctx, col, row);
  }
}

static void drawFlagAtHalfStaff(MilestoneCtx &ctx, uint8_t frame,
                                bool outlineOnly = false) {
  const int poleX = 4;
  for (int row = 0; row < ctx.height; row++) {
    setMemorialPixel(ctx, poleX, row);
  }
  drawHorizontalLine(ctx, poleX - 1, poleX + 1, 0);
  drawHorizontalLine(ctx, poleX - 2, poleX + 2, 7);

  const int flagLeft = poleX + 1;
  const int flagTop = 2;
  const int flagWidth = 18;
  const int flagHeight = 5;

  for (int row = 0; row < flagHeight; row++) {
    for (int col = 0; col < flagWidth; col++) {
      bool edge = row == 0 || row == flagHeight - 1 || col == 0 ||
                  col == flagWidth - 1;
      bool canton = row < 3 && col < 7;
      bool stripe = row % 2 == 0;
      bool waveCut = ((col + frame / 2) % 7) == 0 && row == flagHeight - 1;
      bool star = canton && ((col + row * 2 + frame / 3) % 4 == 0);

      if (edge || (!outlineOnly && ((canton && star) || (stripe && !waveCut)))) {
        setMemorialPixel(ctx, flagLeft + col, flagTop + row);
      }
    }
  }
}

static void drawCrossMarker(MilestoneCtx &ctx, int cx, int groundRow) {
  drawVerticalLine(ctx, cx, groundRow - 5, groundRow);
  drawHorizontalLine(ctx, cx - 3, cx + 3, groundRow - 4);
  drawHorizontalLine(ctx, cx - 5, cx + 5, groundRow);
}

static void drawTombstone(MilestoneCtx &ctx, int left, int top) {
  static const uint8_t WIDTH = 9;
  static const uint8_t HEIGHT = 7;
  static const uint16_t TOMBSTONE_ROWS[] = {
      0b001111100, 0b011111110, 0b111000111, 0b110000011,
      0b110000011, 0b111111111, 0b111111111,
  };

  for (int row = 0; row < HEIGHT; row++) {
    for (int col = 0; col < WIDTH; col++) {
      uint16_t mask = 1u << (WIDTH - 1 - col);
      if ((TOMBSTONE_ROWS[row] & mask) != 0) {
        setMemorialPixel(ctx, left + col, top + row);
      }
    }
  }
}

static void drawRipText(MilestoneCtx &ctx, int left, int top) {
  static const uint8_t LETTER_WIDTH = 3;
  static const uint8_t LETTER_HEIGHT = 5;
  static const uint8_t R_ROWS[] = {0b110, 0b101, 0b110, 0b101, 0b101};
  static const uint8_t I_ROWS[] = {0b111, 0b010, 0b010, 0b010, 0b111};
  static const uint8_t P_ROWS[] = {0b110, 0b101, 0b110, 0b100, 0b100};
  const uint8_t *letters[] = {R_ROWS, I_ROWS, P_ROWS};

  for (int letter = 0; letter < 3; letter++) {
    int letterLeft = left + letter * 5;
    for (int row = 0; row < LETTER_HEIGHT; row++) {
      for (int col = 0; col < LETTER_WIDTH; col++) {
        uint8_t mask = 1u << (LETTER_WIDTH - 1 - col);
        if ((letters[letter][row] & mask) != 0) {
          setMemorialPixel(ctx, letterLeft + col, top + row);
        }
      }
    }
  }
}

static void drawLargePoppy(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setMemorialPixel(ctx, cx, cy);
  setMemorialPixel(ctx, cx - 1, cy);
  setMemorialPixel(ctx, cx + 1, cy);
  setMemorialPixel(ctx, cx, cy - 1);
  setMemorialPixel(ctx, cx, cy + 1);
  setMemorialPixel(ctx, cx - 2, cy);
  setMemorialPixel(ctx, cx + 2, cy);

  if (frame % 2 == 0) {
    setMemorialPixel(ctx, cx - 1, cy - 1);
    setMemorialPixel(ctx, cx + 1, cy - 1);
    setMemorialPixel(ctx, cx - 1, cy + 1);
    setMemorialPixel(ctx, cx + 1, cy + 1);
  }

  drawVerticalLine(ctx, cx, cy + 2, 7);
  setMemorialPixel(ctx, cx - 1, cy + 4);
  setMemorialPixel(ctx, cx + 1, cy + 5);
}

static void drawWreath(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  static const int RING_POINTS[][2] = {
      {0, -3}, {2, -2}, {3, 0}, {2, 2},
      {0, 3},  {-2, 2}, {-3, 0}, {-2, -2},
  };

  for (uint8_t point = 0; point < 8; point++) {
    bool twinkle = (point + frame) % 3 != 0;
    setMemorialPixel(ctx, cx + RING_POINTS[point][0],
                     cy + RING_POINTS[point][1], twinkle);
    if (point % 2 == 0) {
      setMemorialPixel(ctx, cx + RING_POINTS[point][0] / 2,
                       cy + RING_POINTS[point][1] / 2);
    }
  }

  setMemorialPixel(ctx, cx - 1, cy + 2);
  setMemorialPixel(ctx, cx, cy + 3);
  setMemorialPixel(ctx, cx + 1, cy + 2);
}

void runMemorialDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 10; frame++) {
    milestoneClear(ctx);
    int flagTop = frame < 5 ? frame : 5;
    drawVerticalLine(ctx, 4, 0, 7);
    drawHorizontalLine(ctx, 3, 5, 0);
    for (int row = 0; row < 3; row++) {
      for (int col = 0; col < 14; col++) {
        if (row == 0 || row == 2 || col == 0 || col == 13) {
          setMemorialPixel(ctx, 5 + col, flagTop + row);
        }
      }
    }
    milestoneFrameShow(ctx, 95, 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawFlagAtHalfStaff(ctx, frame);
    milestoneFrameShow(ctx, 95, frame % 2 == 0 ? 4 : 0);
  }

  for (uint8_t reveal = 0; reveal < 12; reveal++) {
    milestoneClear(ctx);
    drawHorizontalLine(ctx, 0, ctx.width - 1, 7);
    drawCrossMarker(ctx, ctx.cx - 8, 7);
    if (reveal > 3) {
      drawTombstone(ctx, ctx.cx + 3, 1);
    }
    milestoneFrameShow(ctx, 120, reveal / 2);
  }

  for (uint8_t frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    drawRipText(ctx, ctx.cx - 8, 1);
    drawLargePoppy(ctx, ctx.cx + 12, 2, frame);
    milestoneFrameShow(ctx, 130, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawFlagAtHalfStaff(ctx, frame, true);
    drawWreath(ctx, ctx.cx + 10, 3, frame);
    milestoneFrameShow(ctx, 110, frame % 2 == 0 ? 4 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Honoring Memorial Day");
}
