#include "holiday_coffee_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setCoffeePixel(MilestoneCtx &ctx, int col, int row,
                           bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setCoffeePixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setCoffeePixel(ctx, col, row);
  }
}

static void drawSteam(MilestoneCtx &ctx, int left, int top, uint8_t frame) {
  int drift = frame % 4;

  setCoffeePixel(ctx, left + 3 + (drift == 1 ? 1 : 0), top + 2);
  setCoffeePixel(ctx, left + 4 + (drift == 2 ? 1 : 0), top + 1);
  setCoffeePixel(ctx, left + 3 + (drift == 3 ? 1 : 0), top);

  setCoffeePixel(ctx, left + 8 + (drift == 0 ? 1 : 0), top + 2);
  setCoffeePixel(ctx, left + 9 + (drift == 1 ? 1 : 0), top + 1);
  setCoffeePixel(ctx, left + 8 + (drift == 2 ? 1 : 0), top);

  setCoffeePixel(ctx, left + 13 + (drift == 2 ? 1 : 0), top + 2);
  setCoffeePixel(ctx, left + 12 + (drift == 3 ? 1 : 0), top + 1);
}

static void drawMug(MilestoneCtx &ctx, int left, int top, uint8_t frame,
                    bool withSteam = true) {
  const int cupTop = top + 3;
  const int cupBottom = top + 7;

  drawHorizontalLine(ctx, left + 1, left + 15, cupTop);
  drawHorizontalLine(ctx, left + 2, left + 14, cupTop + 1);
  drawHorizontalLine(ctx, left + 2, left + 14, cupBottom);
  drawVerticalLine(ctx, left + 1, cupTop + 1, cupBottom - 1);
  drawVerticalLine(ctx, left + 15, cupTop + 1, cupBottom - 1);

  setCoffeePixel(ctx, left + 17, cupTop + 1);
  setCoffeePixel(ctx, left + 19, cupTop + 2);
  setCoffeePixel(ctx, left + 19, cupTop + 3);
  setCoffeePixel(ctx, left + 17, cupBottom - 1);
  setCoffeePixel(ctx, left + 18, cupTop + 1);
  setCoffeePixel(ctx, left + 18, cupBottom - 1);

  if (frame % 2 == 0) {
    drawHorizontalLine(ctx, left + 4, left + 12, cupTop + 2);
  }

  if (withSteam) {
    drawSteam(ctx, left, top, frame);
  }
}

static void drawBean(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setCoffeePixel(ctx, cx, cy - 1);
  setCoffeePixel(ctx, cx - 1, cy);
  setCoffeePixel(ctx, cx, cy);
  setCoffeePixel(ctx, cx + 1, cy);
  setCoffeePixel(ctx, cx, cy + 1);

  if (frame % 2 == 0) {
    setCoffeePixel(ctx, cx - 1, cy - 1);
    setCoffeePixel(ctx, cx + 1, cy + 1);
  } else {
    setCoffeePixel(ctx, cx + 1, cy - 1);
    setCoffeePixel(ctx, cx - 1, cy + 1);
  }
}

static void drawBeanRain(MilestoneCtx &ctx, uint8_t frame) {
  for (int bean = 0; bean < 7; bean++) {
    int x = (bean * 8 + frame * 2) % (ctx.width + 8) - 4;
    int y = (frame + bean * 3) % (ctx.height + 4) - 2;
    drawBean(ctx, x, y, frame + bean);
  }
}

static void drawCaffeineBurst(MilestoneCtx &ctx, int cx, int cy,
                              uint8_t frame) {
  setCoffeePixel(ctx, cx, cy);
  for (int spark = 0; spark < 12; spark++) {
    int dx = ((spark * 5 + frame) % 9) - 4;
    int dy = ((spark * 3 + frame) % 7) - 3;
    int distance = frame / 3 + 1;
    int x = cx + (dx * distance) / 4;
    int y = cy + (dy * distance) / 3;
    if ((spark + frame) % 3 != 0) {
      setCoffeePixel(ctx, x, y);
    }
  }
}

void runCoffeeDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawBeanRain(ctx, frame);
    milestoneFrameShow(ctx, 55, frame % 5);
  }

  for (uint8_t frame = 0; frame < 20; frame++) {
    milestoneClear(ctx);
    drawMug(ctx, ctx.cx - 10, 0, frame);
    milestoneFrameShow(ctx, 85, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawMug(ctx, ctx.cx - 10, 0, frame);
    drawBean(ctx, ctx.cx - 17 + frame * 2, 5 - frame % 3, frame);
    milestoneFrameShow(ctx, 65, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawMug(ctx, ctx.cx - 10, 0, frame, false);
    drawCaffeineBurst(ctx, ctx.cx, 3, frame);
    milestoneFrameShow(ctx, 45, frame % 2 == 0 ? 5 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawMug(ctx, ctx.cx - 10, 0, flash);
    } else {
      drawBeanRain(ctx, flash);
      drawCaffeineBurst(ctx, ctx.cx, 3, flash + 6);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 110 : 90,
                       flash % 2 == 0 ? 0 : 3);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy National Coffee Day!");
}
