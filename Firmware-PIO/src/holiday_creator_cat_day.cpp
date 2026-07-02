#include "holiday_creator_cat_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setCreatorCatPixel(MilestoneCtx &ctx, int col, int row,
                               bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawHorizontalLine(MilestoneCtx &ctx, int x0, int x1, int row) {
  for (int col = x0; col <= x1; col++) {
    setCreatorCatPixel(ctx, col, row);
  }
}

static void drawVerticalLine(MilestoneCtx &ctx, int col, int y0, int y1) {
  for (int row = y0; row <= y1; row++) {
    setCreatorCatPixel(ctx, col, row);
  }
}

static void drawPlayButton(MilestoneCtx &ctx, int left, int top,
                           uint8_t frame, bool filled) {
  const int width = 22;
  const int height = 8;

  drawHorizontalLine(ctx, left + 2, left + width - 3, top);
  drawHorizontalLine(ctx, left + 2, left + width - 3, top + height - 1);
  drawVerticalLine(ctx, left, top + 2, top + height - 3);
  drawVerticalLine(ctx, left + width - 1, top + 2, top + height - 3);
  setCreatorCatPixel(ctx, left + 1, top + 1);
  setCreatorCatPixel(ctx, left + width - 2, top + 1);
  setCreatorCatPixel(ctx, left + 1, top + height - 2);
  setCreatorCatPixel(ctx, left + width - 2, top + height - 2);

  for (int row = 0; row < 6; row++) {
    int triWidth = row < 3 ? row + 1 : 6 - row;
    for (int col = 0; col < triWidth; col++) {
      setCreatorCatPixel(ctx, left + 9 + col, top + 1 + row);
      if (filled && frame % 2 == 0) {
        setCreatorCatPixel(ctx, left + 10 + col, top + 1 + row);
      }
    }
  }
}

static void drawCatFace(MilestoneCtx &ctx, int cx, uint8_t frame) {
  const int left = cx - 11;

  drawHorizontalLine(ctx, left + 5, left + 17, 1);
  drawHorizontalLine(ctx, left + 3, left + 19, 2);
  drawHorizontalLine(ctx, left + 2, left + 20, 3);
  drawHorizontalLine(ctx, left + 2, left + 20, 4);
  drawHorizontalLine(ctx, left + 3, left + 19, 5);
  drawHorizontalLine(ctx, left + 5, left + 17, 6);

  setCreatorCatPixel(ctx, left + 3, 0);
  setCreatorCatPixel(ctx, left + 4, 1);
  setCreatorCatPixel(ctx, left + 5, 2);
  setCreatorCatPixel(ctx, left + 19, 0);
  setCreatorCatPixel(ctx, left + 18, 1);
  setCreatorCatPixel(ctx, left + 17, 2);

  bool blink = frame % 8 == 6;
  if (blink) {
    drawHorizontalLine(ctx, left + 7, left + 9, 3);
    drawHorizontalLine(ctx, left + 13, left + 15, 3);
  } else {
    setCreatorCatPixel(ctx, left + 8, 3);
    setCreatorCatPixel(ctx, left + 14, 3);
  }

  setCreatorCatPixel(ctx, left + 11, 4);
  setCreatorCatPixel(ctx, left + 10, 5);
  setCreatorCatPixel(ctx, left + 12, 5);
  setCreatorCatPixel(ctx, left + 11, 6);

  drawHorizontalLine(ctx, left - 1, left + 6, 4);
  drawHorizontalLine(ctx, left + 16, left + 23, 4);
  drawHorizontalLine(ctx, left, left + 6, 5);
  drawHorizontalLine(ctx, left + 16, left + 22, 5);
}

static void drawPaw(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setCreatorCatPixel(ctx, cx, cy);
  setCreatorCatPixel(ctx, cx - 1, cy);
  setCreatorCatPixel(ctx, cx + 1, cy);
  setCreatorCatPixel(ctx, cx, cy + 1);
  if (wide) {
    setCreatorCatPixel(ctx, cx - 2, cy + 1);
    setCreatorCatPixel(ctx, cx + 2, cy + 1);
  }

  setCreatorCatPixel(ctx, cx - 2, cy - 2);
  setCreatorCatPixel(ctx, cx, cy - 2);
  setCreatorCatPixel(ctx, cx + 2, cy - 2);
}

static void drawPawTrail(MilestoneCtx &ctx, uint8_t frame) {
  for (int paw = 0; paw < 5; paw++) {
    int x = (paw * 9 + frame * 2) % (ctx.width + 12) - 6;
    int y = paw % 2 == 0 ? 5 : 3;
    drawPaw(ctx, x, y, (paw + frame) % 2 == 0);
  }
}

static void drawCreatorSparkles(MilestoneCtx &ctx, uint8_t frame) {
  for (int spark = 0; spark < 14; spark++) {
    int x = (spark * 7 + frame * 3) % ctx.width;
    int y = (spark * 5 + frame) % ctx.height;
    if ((spark + frame) % 3 != 0) {
      setCreatorCatPixel(ctx, x, y);
    }
  }
}

static void drawCatPlayCombo(MilestoneCtx &ctx, uint8_t frame) {
  int playLeft = ctx.cx - 11;
  drawPlayButton(ctx, playLeft, 0, frame, false);

  setCreatorCatPixel(ctx, playLeft + 3, 0);
  setCreatorCatPixel(ctx, playLeft + 4, 1);
  setCreatorCatPixel(ctx, playLeft + 5, 2);
  setCreatorCatPixel(ctx, playLeft + 18, 0);
  setCreatorCatPixel(ctx, playLeft + 17, 1);
  setCreatorCatPixel(ctx, playLeft + 16, 2);

  setCreatorCatPixel(ctx, playLeft + 7, 5);
  setCreatorCatPixel(ctx, playLeft + 15, 5);
  setCreatorCatPixel(ctx, playLeft + 11, 6);
  drawHorizontalLine(ctx, playLeft + 2, playLeft + 7, 6);
  drawHorizontalLine(ctx, playLeft + 15, playLeft + 20, 6);

  if (frame % 2 == 0) {
    drawPaw(ctx, playLeft + 27, 5, true);
  }
}

void runCreatorCatDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t grow = 0; grow < 10; grow++) {
    milestoneClear(ctx);
    drawPlayButton(ctx, ctx.cx - 11, 0, grow, grow > 4);
    milestoneFrameShow(ctx, 80, grow / 2);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawPlayButton(ctx, ctx.cx - 11, 0, frame, true);
    drawCreatorSparkles(ctx, frame);
    milestoneFrameShow(ctx, 55, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawCatFace(ctx, ctx.cx, frame);
    milestoneFrameShow(ctx, 95, frame % 8 == 6 ? 0 : 6);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawPawTrail(ctx, frame);
    milestoneFrameShow(ctx, 70, frame % 5);
  }

  for (uint8_t frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    drawCatPlayCombo(ctx, frame);
    drawCreatorSparkles(ctx, frame);
    milestoneFrameShow(ctx, 75, frame % 2 == 0 ? 5 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      drawCatFace(ctx, ctx.cx, flash);
    } else {
      drawCatPlayCombo(ctx, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 110 : 90,
                       flash % 2 == 0 ? 0 : 3);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Creator Cat Day!");
}
