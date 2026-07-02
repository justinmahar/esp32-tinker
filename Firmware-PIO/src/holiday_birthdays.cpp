#include "holiday_birthdays.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setBirthdayPixel(MilestoneCtx &ctx, int col, int row,
                             bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawLetter(MilestoneCtx &ctx, int leftCol,
                       const uint8_t rows[7]) {
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 5; col++) {
      if ((rows[row] & (1 << (4 - col))) != 0) {
        setBirthdayPixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawHbd(MilestoneCtx &ctx, uint8_t frame) {
  static const uint8_t H_ROWS[7] = {
      0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001,
  };
  static const uint8_t B_ROWS[7] = {
      0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110,
  };
  static const uint8_t D_ROWS[7] = {
      0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110,
  };

  int leftCol = ctx.cx - 8;
  drawLetter(ctx, leftCol, H_ROWS);
  drawLetter(ctx, leftCol + 6, B_ROWS);
  drawLetter(ctx, leftCol + 12, D_ROWS);

  for (int col = 0; col < ctx.width; col += 4) {
    if (((col / 4) + frame) % 2 == 0) {
      setBirthdayPixel(ctx, col, 7);
    }
  }
}

static void drawCake(MilestoneCtx &ctx, int centerX, uint8_t revealRows,
                     uint8_t frame) {
  int bottomLeft = centerX - 11;
  int bottomRight = centerX + 11;
  int topLeft = centerX - 7;
  int topRight = centerX + 7;

  auto revealed = [revealRows](int row) { return row >= 8 - revealRows; };

  if (revealed(7)) {
    for (int col = bottomLeft - 2; col <= bottomRight + 2; col++) {
      setBirthdayPixel(ctx, col, 7);
    }
  }

  if (revealed(6)) {
    for (int col = bottomLeft; col <= bottomRight; col++) {
      setBirthdayPixel(ctx, col, 6);
    }
  }

  if (revealed(5)) {
    for (int col = bottomLeft; col <= bottomRight; col++) {
      if ((col + frame) % 4 != 0) {
        setBirthdayPixel(ctx, col, 5);
      }
    }
  }

  if (revealed(4)) {
    for (int col = bottomLeft; col <= bottomRight; col++) {
      setBirthdayPixel(ctx, col, 4);
    }
  }

  if (revealed(3)) {
    for (int col = topLeft; col <= topRight; col++) {
      setBirthdayPixel(ctx, col, 3);
    }
    setBirthdayPixel(ctx, topLeft - 1, 4);
    setBirthdayPixel(ctx, topRight + 1, 4);
  }

  if (revealRows < 7) {
    return;
  }

  const int candleX[3] = {centerX - 6, centerX, centerX + 6};
  for (int i = 0; i < 3; i++) {
    setBirthdayPixel(ctx, candleX[i], 1);
    setBirthdayPixel(ctx, candleX[i], 2);
    setBirthdayPixel(ctx, candleX[i] - 1, 2);
    setBirthdayPixel(ctx, candleX[i] + 1, 2);

    int flicker = (frame + i) % 2 == 0 ? -1 : 1;
    setBirthdayPixel(ctx, candleX[i], 0);
    setBirthdayPixel(ctx, candleX[i] + flicker, 1);
  }
}

static void drawBalloon(MilestoneCtx &ctx, int cx, int cy, int stringTilt,
                        uint8_t frame) {
  setBirthdayPixel(ctx, cx, cy - 1);
  setBirthdayPixel(ctx, cx - 1, cy);
  setBirthdayPixel(ctx, cx, cy);
  setBirthdayPixel(ctx, cx + 1, cy);
  setBirthdayPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setBirthdayPixel(ctx, cx - 1, cy - 1);
    setBirthdayPixel(ctx, cx + 1, cy + 1);
  }

  for (int step = 1; step <= 4; step++) {
    setBirthdayPixel(ctx, cx + stringTilt * step / 2, cy + 1 + step);
  }
}

static void drawBalloons(MilestoneCtx &ctx, uint8_t frame) {
  int bob = frame % 4 < 2 ? 0 : 1;
  drawBalloon(ctx, 3, 2 + bob, 1, frame);
  drawBalloon(ctx, ctx.width - 4, 2 - bob, -1, frame + 1);
}

static void drawConfetti(MilestoneCtx &ctx, uint8_t frame) {
  for (int piece = 0; piece < 18; piece++) {
    int x = (piece * 7 + frame * 4) % ctx.width;
    int y = (piece * 5 + frame) % ctx.height;
    setBirthdayPixel(ctx, x, y);
    if ((piece + frame) % 3 == 0) {
      setBirthdayPixel(ctx, x + 1, y);
    }
  }
}

static void drawSparkBurst(MilestoneCtx &ctx, int originX, int originY,
                           uint8_t frame) {
  setBirthdayPixel(ctx, originX, originY);
  for (int spark = 0; spark < 10; spark++) {
    float angle = spark * 0.6283185f + frame * 0.2f;
    int px = originX + (int)(frame * cosf(angle) * 0.65f);
    int py = originY + (int)(frame * sinf(angle) * 0.5f);
    setBirthdayPixel(ctx, px, py, frame % 2 == 0 || spark % 2 == 0);
  }
}

void runBirthdayHolidayAnimation(MD_Parola &display, const char *message) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    drawHbd(ctx, frame);
    milestoneFrameShow(ctx, 75, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t reveal = 1; reveal <= 8; reveal++) {
    milestoneClear(ctx);
    drawCake(ctx, ctx.cx, reveal, reveal);
    milestoneFrameShow(ctx, 120, reveal);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawCake(ctx, ctx.cx, 8, frame);
    drawBalloons(ctx, frame);
    milestoneFrameShow(ctx, 65, frame % 2 == 0 ? 6 : 0);
  }

  const int burstX[4] = {ctx.width / 5, 2 * ctx.width / 5, 3 * ctx.width / 5,
                         4 * ctx.width / 5};
  const int burstY[4] = {1, 5, 2, 4};
  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawConfetti(ctx, frame);
    for (int burst = 0; burst < 4; burst++) {
      int localFrame = frame - burst * 2;
      if (localFrame >= 0) {
        drawSparkBurst(ctx, burstX[burst], burstY[burst], localFrame);
      }
    }
    uint8_t intensity = frame / 2;
    milestoneFrameShow(ctx, 45, intensity > 7 ? 7 : intensity);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawCake(ctx, ctx.cx, 8, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 55 : 95,
                       flash % 2 == 0 ? 7 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, message);
}
