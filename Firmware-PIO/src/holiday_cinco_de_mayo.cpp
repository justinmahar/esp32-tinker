#include "holiday_cinco_de_mayo.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setCincoPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawSombrero(MilestoneCtx &ctx, int centerX, uint8_t revealRows,
                         uint8_t sparkleFrame) {
  static const uint8_t HAT_WIDTH = 25;
  static const uint8_t HAT_HEIGHT = 8;
  static const uint32_t HAT_ROWS[] = {
      0b0000000000010000000000000, 0b0000000000111000000000000,
      0b0000000001111100000000000, 0b0000000011111110000000000,
      0b0000000111111111000000000, 0b0001111111111111111110000,
      0b0111111111111111111111100, 0b0011110000000000001111000,
  };

  int leftCol = centerX - HAT_WIDTH / 2;
  for (int row = 0; row < HAT_HEIGHT; row++) {
    if (row >= revealRows) {
      continue;
    }

    for (int col = 0; col < HAT_WIDTH; col++) {
      uint32_t mask = 1UL << (HAT_WIDTH - 1 - col);
      if ((HAT_ROWS[row] & mask) == 0) {
        continue;
      }

      bool sparkleHole = row >= 5 && ((col + sparkleFrame) % 7 == 0);
      if (!sparkleHole || sparkleFrame % 2 == 0) {
        setCincoPixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawMaraca(MilestoneCtx &ctx, int cx, int cy, int tilt,
                       uint8_t frame) {
  setCincoPixel(ctx, cx, cy - 1);
  setCincoPixel(ctx, cx - 1, cy);
  setCincoPixel(ctx, cx, cy);
  setCincoPixel(ctx, cx + 1, cy);
  setCincoPixel(ctx, cx, cy + 1);

  if (frame % 2 == 0) {
    setCincoPixel(ctx, cx - 1, cy - 1);
    setCincoPixel(ctx, cx + 1, cy + 1);
  }

  for (int step = 1; step <= 4; step++) {
    int handleX = cx + tilt * step / 2;
    int handleY = cy + 1 + step;
    setCincoPixel(ctx, handleX, handleY);
  }
}

static void drawMaracas(MilestoneCtx &ctx, uint8_t frame) {
  int shake = frame % 2 == 0 ? -1 : 1;
  drawMaraca(ctx, ctx.cx - 8 + shake, 2, -1, frame);
  drawMaraca(ctx, ctx.cx + 8 - shake, 2, 1, frame + 1);
}

static void drawConfetti(MilestoneCtx &ctx, uint8_t frame) {
  for (int piece = 0; piece < 20; piece++) {
    int x = (piece * 7 + frame * 3) % ctx.width;
    int y = (piece * 5 + frame) % ctx.height;
    bool dash = (piece + frame) % 3 == 0;

    setCincoPixel(ctx, x, y);
    if (dash) {
      setCincoPixel(ctx, x + 1, y);
    }
  }
}

static void drawBurst(MilestoneCtx &ctx, int originX, int originY, uint8_t frame) {
  setCincoPixel(ctx, originX, originY);
  for (int spark = 0; spark < 10; spark++) {
    float angle = spark * 0.6283185f + frame * 0.18f;
    int px = originX + (int)(frame * cosf(angle) * 0.75f);
    int py = originY + (int)(frame * sinf(angle) * 0.55f);
    setCincoPixel(ctx, px, py, frame % 2 == 0 || spark % 2 == 0);
  }
}

void runCincoDeMayoHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t reveal = 1; reveal <= 8; reveal++) {
    milestoneClear(ctx);
    drawSombrero(ctx, ctx.cx, reveal, reveal);
    milestoneFrameShow(ctx, 120, reveal);
  }

  for (uint8_t shake = 0; shake < 16; shake++) {
    milestoneClear(ctx);
    drawMaracas(ctx, shake);
    if (shake % 3 == 0) {
      drawConfetti(ctx, shake);
    }
    milestoneFrameShow(ctx, 65, shake % 2 == 0 ? 6 : 0);
  }

  const int burstX[4] = {ctx.width / 5, 2 * ctx.width / 5, 3 * ctx.width / 5,
                         4 * ctx.width / 5};
  const int burstY[4] = {2, 5, 1, 4};
  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawSombrero(ctx, ctx.cx, 8, frame);
    for (int burst = 0; burst < 4; burst++) {
      int localFrame = frame - burst * 2;
      if (localFrame >= 0) {
        drawBurst(ctx, burstX[burst], burstY[burst], localFrame);
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
      drawMaracas(ctx, flash);
      drawConfetti(ctx, flash * 3);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 55 : 95,
                       flash % 2 == 0 ? 7 : 0);
  }

  for (uint8_t frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    drawConfetti(ctx, frame);
    drawSombrero(ctx, ctx.cx, 8, frame);
    milestoneFrameShow(ctx, 48, frame % 2 == 0 ? 5 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Cinco de Mayo!");
}
