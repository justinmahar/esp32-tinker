#include "holiday_earth_day.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static void setEarthPixel(MilestoneCtx &ctx, int col, int row, bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + (ctx.width - 1 - col), on);
  }
}

static void drawGlobe(MilestoneCtx &ctx, int centerX, uint8_t frame) {
  static const uint8_t GLOBE_WIDTH = 17;
  static const uint8_t GLOBE_HEIGHT = 8;
  static const uint32_t OUTLINE_ROWS[] = {
      0b00000111111000000, 0b00011000000110000, 0b00100000000001000,
      0b01000000000000100, 0b01000000000000100, 0b00100000000001000,
      0b00011000000110000, 0b00000111111000000,
  };
  static const uint32_t INTERIOR_ROWS[] = {
      0b00000111111000000, 0b00011111111110000, 0b00111111111111000,
      0b01111111111111100, 0b01111111111111100, 0b00111111111111000,
      0b00011111111110000, 0b00000111111000000,
  };
  static const uint32_t LAND_ROWS[] = {
      0b00000000000000000, 0b00000110000000000, 0b00001111000110000,
      0b00000111001110000, 0b00011000111100000, 0b00111000011000000,
      0b00000001110000000, 0b00000000000000000,
  };

  int leftCol = centerX - GLOBE_WIDTH / 2;
  int landShift = frame % GLOBE_WIDTH;
  for (int row = 0; row < GLOBE_HEIGHT; row++) {
    for (int col = 0; col < GLOBE_WIDTH; col++) {
      uint32_t outlineMask = 1UL << (GLOBE_WIDTH - 1 - col);
      int landCol = (col + landShift) % GLOBE_WIDTH;
      uint32_t landMask = 1UL << (GLOBE_WIDTH - 1 - landCol);
      bool outline = (OUTLINE_ROWS[row] & outlineMask) != 0;
      bool insideGlobe = (INTERIOR_ROWS[row] & outlineMask) != 0;
      bool land = insideGlobe && (LAND_ROWS[row] & landMask) != 0;

      if (outline || land) {
        setEarthPixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawSprout(MilestoneCtx &ctx, int baseX, uint8_t growRows,
                       uint8_t frame) {
  for (uint8_t step = 0; step < growRows && step < 5; step++) {
    setEarthPixel(ctx, baseX, 7 - step);
  }

  if (growRows >= 3) {
    setEarthPixel(ctx, baseX - 1, 5);
    setEarthPixel(ctx, baseX - 2, 4);
    setEarthPixel(ctx, baseX - 3, 4 + frame % 2);
  }
  if (growRows >= 4) {
    setEarthPixel(ctx, baseX + 1, 4);
    setEarthPixel(ctx, baseX + 2, 3);
    setEarthPixel(ctx, baseX + 3, 3 + (frame + 1) % 2);
  }
}

static void drawRecycleArrow(MilestoneCtx &ctx, int cx, int cy, uint8_t phase) {
  static const int POINTS = 12;
  for (int point = 0; point < POINTS; point++) {
    float angle = (point + phase) * (6.283185f / POINTS);
    int x = cx + (int)(cosf(angle) * 9.0f);
    int y = cy + (int)(sinf(angle) * 3.0f);
    setEarthPixel(ctx, x, y);
  }

  float headAngle = phase * (6.283185f / POINTS);
  int hx = cx + (int)(cosf(headAngle) * 9.0f);
  int hy = cy + (int)(sinf(headAngle) * 3.0f);
  setEarthPixel(ctx, hx - 1, hy);
  setEarthPixel(ctx, hx, hy - 1);
  setEarthPixel(ctx, hx + 1, hy);
}

static void drawSparkle(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setEarthPixel(ctx, cx, cy);
  setEarthPixel(ctx, cx - 1, cy);
  setEarthPixel(ctx, cx + 1, cy);
  setEarthPixel(ctx, cx, cy - 1);
  setEarthPixel(ctx, cx, cy + 1);
  if (wide) {
    setEarthPixel(ctx, cx - 2, cy);
    setEarthPixel(ctx, cx + 2, cy);
  }
}

static void drawSeedRain(MilestoneCtx &ctx, uint8_t frame) {
  for (int seed = 0; seed < 8; seed++) {
    int x = (seed * ctx.width) / 8 + ((frame + seed) % 3) - 1;
    int y = (frame + seed * 2) % (ctx.height + 3) - 2;
    setEarthPixel(ctx, x, y);
    if ((seed + frame) % 2 == 0) {
      setEarthPixel(ctx, x + 1, y + 1);
    }
  }
}

void runEarthDayHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (uint8_t frame = 0; frame < 20; frame++) {
    milestoneClear(ctx);
    drawGlobe(ctx, ctx.cx, frame);
    milestoneFrameShow(ctx, 62, frame % 5);
  }

  for (uint8_t grow = 1; grow <= 5; grow++) {
    milestoneClear(ctx);
    drawSeedRain(ctx, grow);
    drawSprout(ctx, ctx.cx, grow, grow);
    milestoneFrameShow(ctx, 150, grow);
  }

  for (uint8_t frame = 0; frame < 16; frame++) {
    milestoneClear(ctx);
    drawSprout(ctx, ctx.cx, 5, frame);
    drawRecycleArrow(ctx, ctx.cx, 4, frame);
    milestoneFrameShow(ctx, 55, frame % 2 == 0 ? 6 : 0);
  }

  for (uint8_t frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    drawGlobe(ctx, ctx.cx, frame);
    drawSparkle(ctx, ctx.cx - 12 + frame * 2, 2 + frame % 4,
                frame % 2 == 0);
    drawSparkle(ctx, ctx.cx + 12 - frame, 5 - frame % 3, frame % 2 == 1);
    milestoneFrameShow(ctx, 60, frame % 2 == 0 ? 5 : 0);
  }

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      milestoneFillAll(ctx);
    } else {
      drawGlobe(ctx, ctx.cx, flash);
      drawSprout(ctx, ctx.cx, 5, flash);
    }
    milestoneFrameShow(ctx, flash % 2 == 0 ? 55 : 95,
                       flash % 2 == 0 ? 7 : 0);
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy Earth Day!");
}
