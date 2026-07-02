#include "holiday_thanksgiving.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setThanksgivingPixel(MilestoneCtx &ctx, int col, int row,
                                 bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static int thanksgivingAbs(int value) { return value < 0 ? -value : value; }

static void drawLine(MilestoneCtx &ctx, int x0, int y0, int x1, int y1) {
  int dx = thanksgivingAbs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -thanksgivingAbs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    setThanksgivingPixel(ctx, x0, y0);
    if (x0 == x1 && y0 == y1) {
      break;
    }

    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static void drawLeaf(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setThanksgivingPixel(ctx, cx, cy);
  setThanksgivingPixel(ctx, cx - 1, cy);
  setThanksgivingPixel(ctx, cx + 1, cy);
  setThanksgivingPixel(ctx, cx, cy - 1);
  setThanksgivingPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setThanksgivingPixel(ctx, cx - 1, cy - 1);
    setThanksgivingPixel(ctx, cx + 1, cy + 1);
  } else {
    setThanksgivingPixel(ctx, cx + 1, cy - 1);
    setThanksgivingPixel(ctx, cx - 1, cy + 1);
  }
}

static void drawTurkey(MilestoneCtx &ctx, int centerX, uint8_t featherCount,
                       uint8_t frame) {
  const int baseX = centerX - 2;
  const int baseY = 5;
  const int featherX[7] = {centerX - 12, centerX - 8, centerX - 4, centerX,
                           centerX + 4, centerX + 8, centerX + 12};
  const int featherY[7] = {2, 0, 1, 0, 1, 0, 2};

  for (uint8_t feather = 0; feather < featherCount && feather < 7; feather++) {
    drawLine(ctx, baseX, baseY, featherX[feather], featherY[feather]);
    drawLeaf(ctx, featherX[feather], featherY[feather], frame + feather);
  }

  for (int col = -3; col <= 3; col++) {
    setThanksgivingPixel(ctx, centerX + col, 4);
    setThanksgivingPixel(ctx, centerX + col, 5);
  }
  for (int col = -2; col <= 2; col++) {
    setThanksgivingPixel(ctx, centerX + col, 3);
    setThanksgivingPixel(ctx, centerX + col, 6);
  }

  int neckX = centerX + 4;
  int headX = centerX + 5 + frame % 2;
  setThanksgivingPixel(ctx, neckX, 3);
  setThanksgivingPixel(ctx, neckX, 4);
  setThanksgivingPixel(ctx, headX, 2);
  setThanksgivingPixel(ctx, headX + 1, 2);
  setThanksgivingPixel(ctx, headX, 3);
  setThanksgivingPixel(ctx, headX + 2, 3);
  setThanksgivingPixel(ctx, headX + 1, 4);

  setThanksgivingPixel(ctx, centerX - 2, 7);
  setThanksgivingPixel(ctx, centerX + 1, 7);
}

static void drawPilgrimHat(MilestoneCtx &ctx, int centerX, uint8_t frame) {
  static const uint8_t HAT_WIDTH = 17;
  static const uint8_t HAT_HEIGHT = 7;
  static const uint32_t HAT_ROWS[] = {
      0b00000111110000000, 0b00000111110000000, 0b00000111110000000,
      0b00011111111100000, 0b00111111111110000, 0b01111111111111000,
      0b00000011100000000,
  };

  int leftCol = centerX - HAT_WIDTH / 2;
  for (int row = 0; row < HAT_HEIGHT; row++) {
    for (int col = 0; col < HAT_WIDTH; col++) {
      uint32_t mask = 1UL << (HAT_WIDTH - 1 - col);
      if ((HAT_ROWS[row] & mask) != 0) {
        setThanksgivingPixel(ctx, leftCol + col, row);
      }
    }
  }

  if (frame % 2 == 0) {
    setThanksgivingPixel(ctx, centerX, 3);
    setThanksgivingPixel(ctx, centerX - 1, 3);
    setThanksgivingPixel(ctx, centerX + 1, 3);
  }
}

static void drawFallingLeaves(MilestoneCtx &ctx, uint8_t frame) {
  for (int leaf = 0; leaf < 8; leaf++) {
    int x = (leaf * ctx.width) / 8 + ((frame + leaf) % 3) - 1;
    int y = (frame + leaf * 2) % (ctx.height + 3) - 2;
    drawLeaf(ctx, x, y, frame + leaf);
  }
}

static void drawHarvestBurst(MilestoneCtx &ctx, int originX, int originY,
                             uint8_t frame) {
  drawLeaf(ctx, originX, originY, frame);
  for (int spark = 0; spark < 10; spark++) {
    float angle = spark * 0.6283185f + frame * 0.16f;
    int px = originX + (int)(frame * cosf(angle) * 0.7f);
    int py = originY + (int)(frame * sinf(angle) * 0.5f);
    setThanksgivingPixel(ctx, px, py, frame % 2 == 0 || spark % 2 == 0);
  }
}

void runThanksgivingHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawFallingLeaves(ctx, frame);
    milestoneFrameShow(ctx, 58, frame % 5);
  }

  for (uint8_t reveal = 1; reveal <= 7; reveal++) {
    milestoneClear(ctx);
    drawTurkey(ctx, ctx.cx, reveal, reveal);
    milestoneFrameShow(ctx, 135, reveal);
  }

  for (uint8_t gobble = 0; gobble < 12; gobble++) {
    milestoneClear(ctx);
    drawTurkey(ctx, ctx.cx, 7, gobble);
    if (gobble % 3 == 0) {
      drawLeaf(ctx, ctx.cx - 13 + gobble, 1 + gobble % 5, true);
    }
    milestoneFrameShow(ctx, 90, gobble % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 10; frame++) {
    milestoneClear(ctx);
    drawPilgrimHat(ctx, ctx.cx, frame);
    drawFallingLeaves(ctx, frame);
    milestoneFrameShow(ctx, 75, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawHarvestBurst(ctx, ctx.cx, 3, frame);
    if (frame < 10) {
      drawTurkey(ctx, ctx.cx, 7, frame);
    }
    uint8_t intensity = frame / 2;
    milestoneFrameShow(ctx, 45, intensity > 7 ? 7 : intensity);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawTurkey(ctx, ctx.cx, 7, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 55 : 95,
                       flash % 2 == 0 ? 7 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Thanksgiving!");
}
