#include "holiday_veterans_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setVeteransPixel(MilestoneCtx &ctx, int col, int row,
                             bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
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
        setVeteransPixel(ctx, col, row);
      }
    }
  }
}

static void drawSalute(MilestoneCtx &ctx, int centerX, uint8_t frame) {
  int headY = 1;
  setVeteransPixel(ctx, centerX, headY);
  setVeteransPixel(ctx, centerX - 1, headY + 1);
  setVeteransPixel(ctx, centerX, headY + 1);
  setVeteransPixel(ctx, centerX + 1, headY + 1);

  for (int row = 3; row <= 6; row++) {
    setVeteransPixel(ctx, centerX, row);
  }

  setVeteransPixel(ctx, centerX - 1, 4);
  setVeteransPixel(ctx, centerX - 2, 5);
  setVeteransPixel(ctx, centerX + 1, 3);
  setVeteransPixel(ctx, centerX + 2, 2);
  setVeteransPixel(ctx, centerX + 3, 2 + frame % 2);

  setVeteransPixel(ctx, centerX - 1, 7);
  setVeteransPixel(ctx, centerX + 1, 7);
}

static void drawStar(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setVeteransPixel(ctx, cx, cy);
  setVeteransPixel(ctx, cx - 1, cy);
  setVeteransPixel(ctx, cx + 1, cy);
  setVeteransPixel(ctx, cx, cy - 1);
  setVeteransPixel(ctx, cx, cy + 1);
  if (wide) {
    setVeteransPixel(ctx, cx - 2, cy);
    setVeteransPixel(ctx, cx + 2, cy);
  }
}

static void drawStarField(MilestoneCtx &ctx, uint8_t frame) {
  for (int star = 0; star < 12; star++) {
    int x = (star * 7 + frame * 2) % ctx.width;
    int y = (star * 5 + frame) % ctx.height;
    setVeteransPixel(ctx, x, y, (star + frame) % 3 != 0);
  }
}

static void drawHonorBurst(MilestoneCtx &ctx, int originX, int originY,
                           uint8_t frame) {
  drawStar(ctx, originX, originY, frame % 2 == 0);
  for (int spark = 0; spark < 10; spark++) {
    float angle = spark * 0.6283185f + frame * 0.15f;
    int px = originX + (int)(frame * cosf(angle) * 0.75f);
    int py = originY + (int)(frame * sinf(angle) * 0.5f);
    setVeteransPixel(ctx, px, py, frame % 2 == 0 || spark % 2 == 0);
  }
}

void runVeteransDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 20; frame++) {
    milestoneClear(ctx);
    drawFlagWave(ctx, frame);
    milestoneFrameShow(ctx, 60, frame % 5);
  }

  for (uint8_t frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    drawFlagWave(ctx, frame / 2);
    drawSalute(ctx, ctx.cx, frame);
    milestoneFrameShow(ctx, 105, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawSalute(ctx, ctx.cx, frame);
    drawStarField(ctx, frame);
    if (frame % 4 == 0) {
      drawStar(ctx, ctx.cx - 12 + frame, 1 + frame % 5, true);
    }
    milestoneFrameShow(ctx, 55, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawHonorBurst(ctx, ctx.cx, 3, frame);
    if (frame < 10) {
      drawSalute(ctx, ctx.cx, frame);
    }
    uint8_t intensity = frame / 2;
    milestoneFrameShow(ctx, 45, intensity > 7 ? 7 : intensity);
  }

  for (int flash = 0; flash < 3; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawFlagWave(ctx, flash);
    } else {
      drawSalute(ctx, ctx.cx, flash);
      drawStar(ctx, ctx.cx, 3, true);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 110 : 140,
                       flash % 2 == 0 ? 0 : 6);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Veterans Day!");
}
