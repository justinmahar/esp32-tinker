#include "holiday_christmas.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void setChristmasPixel(MilestoneCtx &ctx, int col, int row,
                              bool on = true) {
  if (col >= 0 && col < ctx.width && row >= 0 && row < ctx.height) {
    ctx.matrix->setPoint(row, ctx.colStart + col, on);
  }
}

static void drawSnowflake(MilestoneCtx &ctx, int cx, int cy, bool wide) {
  setChristmasPixel(ctx, cx, cy);
  setChristmasPixel(ctx, cx - 1, cy);
  setChristmasPixel(ctx, cx + 1, cy);
  setChristmasPixel(ctx, cx, cy - 1);
  setChristmasPixel(ctx, cx, cy + 1);
  if (wide) {
    setChristmasPixel(ctx, cx - 1, cy - 1);
    setChristmasPixel(ctx, cx + 1, cy - 1);
    setChristmasPixel(ctx, cx - 1, cy + 1);
    setChristmasPixel(ctx, cx + 1, cy + 1);
  }
}

static void drawTree(MilestoneCtx &ctx, int centerX, uint8_t revealRows,
                     uint8_t twinkleFrame) {
  static const uint8_t TREE_WIDTH = 19;
  static const uint8_t TREE_HEIGHT = 8;
  static const uint32_t TREE_ROWS[] = {
      0b0000000001000000000, 0b0000000011100000000,
      0b0000000111110000000, 0b0000001111111000000,
      0b0000011111111100000, 0b0000111111111110000,
      0b0001111111111111000, 0b0000000011100000000,
  };

  int leftCol = centerX - TREE_WIDTH / 2;
  for (int row = 0; row < TREE_HEIGHT; row++) {
    if (row < TREE_HEIGHT - revealRows) {
      continue;
    }

    for (int col = 0; col < TREE_WIDTH; col++) {
      uint32_t mask = 1UL << (TREE_WIDTH - 1 - col);
      if ((TREE_ROWS[row] & mask) == 0) {
        continue;
      }

      bool ornament = row >= 2 && row <= 5 &&
                      ((row * 5 + col + twinkleFrame) % 9 == 0);
      if (!ornament || twinkleFrame % 2 == 0) {
        setChristmasPixel(ctx, leftCol + col, row);
      }
    }
  }
}

static void drawStar(MilestoneCtx &ctx, int cx, int cy, uint8_t frame) {
  setChristmasPixel(ctx, cx, cy - 1);
  setChristmasPixel(ctx, cx - 1, cy);
  setChristmasPixel(ctx, cx, cy);
  setChristmasPixel(ctx, cx + 1, cy);
  setChristmasPixel(ctx, cx, cy + 1);
  if (frame % 2 == 0) {
    setChristmasPixel(ctx, cx - 2, cy);
    setChristmasPixel(ctx, cx + 2, cy);
    setChristmasPixel(ctx, cx, cy + 2);
  }
}

static void drawPresent(MilestoneCtx &ctx, int leftCol, int topRow, int width,
                        int height, uint8_t frame) {
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      bool border = row == 0 || row == height - 1 || col == 0 ||
                    col == width - 1;
      bool ribbon = col == width / 2 || row == height / 2;
      if (border || ribbon || ((col + row + frame) % 7 == 0)) {
        setChristmasPixel(ctx, leftCol + col, topRow + row);
      }
    }
  }
  setChristmasPixel(ctx, leftCol + width / 2 - 1, topRow - 1);
  setChristmasPixel(ctx, leftCol + width / 2 + 1, topRow - 1);
}

static void drawPresents(MilestoneCtx &ctx, uint8_t frame) {
  drawPresent(ctx, ctx.cx - 15, 5, 7, 3, frame);
  drawPresent(ctx, ctx.cx + 8, 5, 7, 3, frame + 2);
  drawPresent(ctx, ctx.cx - 4, 6, 8, 2, frame + 4);
}

static void drawSanta(MilestoneCtx &ctx, int leftCol, uint8_t frame) {
  // Tiny Santa: hat, face/beard, body, and boots, readable at 8 px tall.
  setChristmasPixel(ctx, leftCol + 3, 0);
  setChristmasPixel(ctx, leftCol + 2, 1);
  setChristmasPixel(ctx, leftCol + 3, 1);
  setChristmasPixel(ctx, leftCol + 4, 1);
  setChristmasPixel(ctx, leftCol + 1, 2);
  setChristmasPixel(ctx, leftCol + 2, 2);
  setChristmasPixel(ctx, leftCol + 3, 2);
  setChristmasPixel(ctx, leftCol + 4, 2);
  setChristmasPixel(ctx, leftCol + 5, 2);
  setChristmasPixel(ctx, leftCol + 2, 3);
  setChristmasPixel(ctx, leftCol + 4, 3);
  setChristmasPixel(ctx, leftCol + 1, 4);
  setChristmasPixel(ctx, leftCol + 2, 4);
  setChristmasPixel(ctx, leftCol + 3, 4);
  setChristmasPixel(ctx, leftCol + 4, 4);
  setChristmasPixel(ctx, leftCol + 5, 4);
  setChristmasPixel(ctx, leftCol, 5);
  setChristmasPixel(ctx, leftCol + 1, 5);
  setChristmasPixel(ctx, leftCol + 2, 5);
  setChristmasPixel(ctx, leftCol + 3, 5);
  setChristmasPixel(ctx, leftCol + 4, 5);
  setChristmasPixel(ctx, leftCol + 5, 5);
  setChristmasPixel(ctx, leftCol + 6, 5);
  setChristmasPixel(ctx, leftCol + 1, 6);
  setChristmasPixel(ctx, leftCol + 5, 6);
  if (frame % 2 == 0) {
    setChristmasPixel(ctx, leftCol, 6);
    setChristmasPixel(ctx, leftCol + 6, 6);
  }

  for (int col = leftCol - 5; col < leftCol - 1; col++) {
    setChristmasPixel(ctx, col, 6);
    if ((col + frame) % 2 == 0) {
      setChristmasPixel(ctx, col, 5);
    }
  }
}

void runChristmasHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  struct Snowflake {
    int x;
    int y;
    bool wide;
  };

  Snowflake flakes[8];
  for (int i = 0; i < 8; i++) {
    flakes[i].x = (i * ctx.width) / 8 + (i % 2);
    flakes[i].y = -i;
    flakes[i].wide = i % 3 == 0;
  }

  for (int frame = 0; frame < 22; frame++) {
    milestoneClear(ctx);
    for (int i = 0; i < 8; i++) {
      flakes[i].y++;
      flakes[i].x += (frame + i) % 4 == 0 ? 1 : 0;
      if (flakes[i].y > ctx.height + 1) {
        flakes[i].y = -2;
        flakes[i].x = random(ctx.width);
      }
      drawSnowflake(ctx, flakes[i].x, flakes[i].y, flakes[i].wide);
    }
    milestoneFrameShow(ctx, 55, frame % 4);
  }

  for (int frame = 0; frame < ctx.width + 14; frame++) {
    milestoneClear(ctx);
    for (int flake = 0; flake < 6; flake++) {
      int x = (flake * 7 + frame / 2) % ctx.width;
      int y = (flake * 3 + frame) % ctx.height;
      drawSnowflake(ctx, x, y, flake % 3 == 0);
    }
    drawSanta(ctx, frame - 10, frame);
    milestoneFrameShow(ctx, 45, frame % 2 == 0 ? 4 : 0);
  }

  for (uint8_t reveal = 1; reveal <= 8; reveal++) {
    milestoneClear(ctx);
    drawTree(ctx, ctx.cx, reveal, reveal);
    if (reveal >= 2) {
      drawStar(ctx, ctx.cx, 0, reveal);
    }
    if (reveal >= 6) {
      drawPresents(ctx, reveal);
    }
    milestoneFrameShow(ctx, 120, min(8, (int)reveal));
  }

  for (int twinkle = 0; twinkle < 12; twinkle++) {
    milestoneClear(ctx);
    drawTree(ctx, ctx.cx, 8, twinkle);
    drawStar(ctx, ctx.cx, 0, twinkle);
    drawPresents(ctx, twinkle);
    if (twinkle % 3 == 0) {
      drawSnowflake(ctx, 2 + twinkle, 1 + (twinkle % 5), twinkle % 2 == 0);
      drawSnowflake(ctx, ctx.width - 3 - twinkle / 2, 6 - (twinkle % 4),
                    twinkle % 2 == 1);
    }
    milestoneFrameShow(ctx, 105, twinkle % 2 == 0 ? 6 : 0);
  }

  for (int glow = 0; glow < 8; glow++) {
    milestoneClear(ctx);
    drawTree(ctx, ctx.cx, 8, glow);
    drawStar(ctx, ctx.cx, 0, glow);
    drawPresents(ctx, glow);
    for (int row = 0; row < ctx.height; row++) {
      if ((row + glow) % 2 == 0) {
        setChristmasPixel(ctx, max(0, ctx.cx - 11 + row), row);
        setChristmasPixel(ctx, min(ctx.width - 1, ctx.cx + 11 - row), row);
      }
    }
    milestoneFrameShow(ctx, 55, min(7, (int)glow));
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Merry Christmas!");
}
