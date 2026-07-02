#include "holiday_pi_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setPiPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawDigit(MilestoneCtx &ctx, int leftCol, int topRow,
                      uint8_t digit) {
  static const uint8_t DIGIT_WIDTH = 5;
  static const uint8_t DIGIT_HEIGHT = 7;
  static const uint8_t DIGITS[][DIGIT_HEIGHT] = {
      {0b11111, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b11111},
      {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
      {0b11110, 0b00001, 0b00001, 0b11110, 0b10000, 0b10000, 0b11111},
      {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110},
      {0b10010, 0b10010, 0b10010, 0b11111, 0b00010, 0b00010, 0b00010},
  };

  if (digit > 4) {
    return;
  }

  for (int row = 0; row < DIGIT_HEIGHT; row++) {
    for (int col = 0; col < DIGIT_WIDTH; col++) {
      uint8_t mask = 1u << (DIGIT_WIDTH - 1 - col);
      if ((DIGITS[digit][row] & mask) != 0) {
        setPiPixel(ctx, leftCol + col, topRow + row);
      }
    }
  }
}

static void drawPiSymbol(MilestoneCtx &ctx, int centerX, uint8_t revealRows,
                         uint8_t sparkleFrame) {
  static const uint8_t PI_WIDTH = 17;
  static const uint8_t PI_HEIGHT = 8;
  static const uint32_t PI_ROWS[] = {
      0b00000000000000000, 0b00111111111111100, 0b00111111111111100,
      0b00001110001110000, 0b00001110001110000, 0b00001110001110000,
      0b00011100011100000, 0b00111000001110000,
  };

  int leftCol = centerX - PI_WIDTH / 2;
  for (int row = 0; row < PI_HEIGHT; row++) {
    if (row >= revealRows) {
      continue;
    }

    for (int col = 0; col < PI_WIDTH; col++) {
      uint32_t mask = 1UL << (PI_WIDTH - 1 - col);
      if ((PI_ROWS[row] & mask) == 0) {
        continue;
      }

      bool sparkleHole = row > 1 && ((row * 5 + col + sparkleFrame) % 13 == 0);
      if (!sparkleHole || sparkleFrame % 2 == 0) {
        setPiPixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawDecimalPoint(MilestoneCtx &ctx, int col, int row,
                             uint8_t frame) {
  setPiPixel(ctx, col, row);
  if (frame % 2 == 0) {
    setPiPixel(ctx, col + 1, row);
  }
}

static void drawPiDigits(MilestoneCtx &ctx, uint8_t frame) {
  int left = ctx.cx - 10;
  drawDigit(ctx, left, 0, 3);
  drawDecimalPoint(ctx, left + 6, 6, frame);
  drawDigit(ctx, left + 8, 0, 1);
  drawDigit(ctx, left + 14, 0, 4);
}

static void drawDigitRain(MilestoneCtx &ctx, uint8_t frame) {
  const uint8_t digits[] = {3, 1, 4, 1, 0, 4};
  for (int stream = 0; stream < 6; stream++) {
    int x = (stream * ctx.width) / 6 + (stream % 2);
    int y = (frame + stream * 3) % (ctx.height + 7) - 7;
    drawDigit(ctx, x, y, digits[(stream + frame / 3) % 6]);
  }
}

static void drawOrbitingDots(MilestoneCtx &ctx, uint8_t frame) {
  for (int dot = 0; dot < 10; dot++) {
    float angle = dot * 0.6283185f + frame * 0.22f;
    int px = ctx.cx + (int)(cosf(angle) * 13.0f);
    int py = 3 + (int)(sinf(angle) * 3.0f);
    setPiPixel(ctx, px, py);
    if ((dot + frame) % 3 == 0) {
      setPiPixel(ctx, px + 1, py);
    }
  }
}

void runPiDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawDigitRain(ctx, frame);
    milestoneFrameShow(ctx, 55, frame % 5);
  }

  for (uint8_t reveal = 1; reveal <= 8; reveal++) {
    milestoneClear(ctx);
    drawPiSymbol(ctx, ctx.cx, reveal, reveal);
    milestoneFrameShow(ctx, 115, reveal);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawPiSymbol(ctx, ctx.cx, 8, frame);
    drawOrbitingDots(ctx, frame);
    milestoneFrameShow(ctx, 50, frame % 2 == 0 ? 5 : 0);
  }

  for (uint8_t frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    drawPiDigits(ctx, frame);
    if (frame % 2 == 0) {
      drawOrbitingDots(ctx, frame);
    }
    milestoneFrameShow(ctx, 90, frame % 2 == 0 ? 7 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawPiDigits(ctx, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 55 : 95,
                       flash % 2 == 0 ? 7 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Pi Day!");
}
