#include "holiday_july_fourth.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setJulyPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + col, on);
  }
}

static void setJulyFlagPixel(MilestoneCtx &ctx, int col, int row,
                             bool on = true) {
  setJulyPixel(ctx, ctx.width - 1 - col, row, on);
}

static void drawFlagWave(MilestoneCtx &ctx, uint8_t frame) {
  for (int row = 0; row < ctx.height; row++) {
    for (int col = 0; col < ctx.width; col++) {
      bool canton = row < 4 && col < 11;
      bool stripe = ((row + frame / 2) % 2) == 0;
      bool wave = ((col + frame + row * 2) % 9) < 5;
      bool star = canton && ((col + row * 3 + frame) % 5 == 0);

      if ((canton && star) || (!canton && stripe && wave)) {
        setJulyFlagPixel(ctx, col, row);
      }
    }
  }
}

static void drawRocket(MilestoneCtx &ctx, int x, int y, uint8_t frame) {
  setJulyPixel(ctx, x, y);
  setJulyPixel(ctx, x - 1, y + 1);
  setJulyPixel(ctx, x, y + 1);
  setJulyPixel(ctx, x + 1, y + 1);
  setJulyPixel(ctx, x, y + 2);

  if (frame % 2 == 0) {
    setJulyPixel(ctx, x, y + 3);
    setJulyPixel(ctx, x - 1, y + 4);
    setJulyPixel(ctx, x + 1, y + 4);
  } else {
    setJulyPixel(ctx, x - 1, y + 3);
    setJulyPixel(ctx, x + 1, y + 3);
  }
}

static void drawFireworkBurst(MilestoneCtx &ctx, int originX, int originY,
                              int frame, int particleCount, float speed) {
  setJulyPixel(ctx, originX, originY);
  for (int particle = 0; particle < particleCount; particle++) {
    float angle = particle * (6.283185f / particleCount) + frame * 0.12f;
    int px = originX + (int)(frame * cosf(angle) * speed);
    int py = originY + (int)(frame * sinf(angle) * speed * 0.75f);
    setJulyPixel(ctx, px, py, frame % 2 == 0 || particle % 2 == 0);
  }
}

void runJulyFourthHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawFlagWave(ctx, frame);
    milestoneFrameShow(ctx, 55, frame % 5);
  }

  for (int frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawRocket(ctx, ctx.width / 4, ctx.height - frame / 2 - 3, frame);
    drawRocket(ctx, 3 * ctx.width / 4, ctx.height - frame / 2 - 2, frame + 1);
    if (frame > 9) {
      drawFireworkBurst(ctx, ctx.width / 4, 2, frame - 9, 8, 0.75f);
      drawFireworkBurst(ctx, 3 * ctx.width / 4, 3, frame - 9, 8, 0.75f);
    }
    milestoneFrameShow(ctx, 48, frame % 5);
  }

  const int burstX[5] = {ctx.width / 6, ctx.width / 3, ctx.width / 2,
                         2 * ctx.width / 3, 5 * ctx.width / 6};
  const int burstY[5] = {2, 4, 1, 5, 3};
  for (int frame = 0; frame < 22; frame++) {
    milestoneClear(ctx);
    for (int burst = 0; burst < 5; burst++) {
      int localFrame = frame - burst * 2;
      if (localFrame >= 0) {
        drawFireworkBurst(ctx, burstX[burst], burstY[burst], localFrame, 10,
                          0.62f);
      }
    }
    uint8_t intensity = frame / 2;
    milestoneFrameShow(ctx, 42, intensity > 7 ? 7 : intensity);
  }

  for (int flash = 0; flash < 5; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawFlagWave(ctx, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 70 : 95,
                       flash % 2 == 0 ? 7 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy 4th of July!");
}
