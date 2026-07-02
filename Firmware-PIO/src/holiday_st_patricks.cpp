#include "holiday_st_patricks.h"

#include "holiday_easter_eggs.h"
#include "milestone_helpers.h"

#include <MD_MAX72xx.h>
#include <math.h>

static const uint8_t SHAMROCK_WIDTH = 15;
static const uint8_t SHAMROCK_HEIGHT = 8;
static const uint16_t SHAMROCK_LEFT_LEAF_ROWS[] = {
    0b000000000000000, 0b000000000000000, 0b001100000000000,
    0b011110000000000, 0b111110000000000, 0b011100000000000,
    0b000000000000000, 0b000000000000000,
};
static const uint16_t SHAMROCK_TOP_LEAF_ROWS[] = {
    0b000001110000000, 0b000011111000000, 0b000001110000000,
    0b000000100000000, 0b000000000000000, 0b000000000000000,
    0b000000000000000, 0b000000000000000,
};
static const uint16_t SHAMROCK_RIGHT_LEAF_ROWS[] = {
    0b000000000000000, 0b000000000000000, 0b000000000011000,
    0b000000000111100, 0b000000000111110, 0b000000000011100,
    0b000000000000000, 0b000000000000000,
};
static const uint16_t SHAMROCK_CENTER_ROWS[] = {
    0b000000000000000, 0b000000000000000, 0b000000000000000,
    0b000000111000000, 0b000001111100000, 0b000000111000000,
    0b000000110000000, 0b000001100000000,
};

static bool shamrockPixelLit(int row, int col, uint8_t revealStage) {
  if (row < 0 || row >= SHAMROCK_HEIGHT || col < 0 || col >= SHAMROCK_WIDTH) {
    return false;
  }
  uint16_t mask = 1u << (SHAMROCK_WIDTH - 1 - col);

  switch (revealStage) {
  case 0:
    return (SHAMROCK_TOP_LEAF_ROWS[row] & mask) != 0;
  case 1:
    return (SHAMROCK_TOP_LEAF_ROWS[row] & mask) != 0 ||
           (SHAMROCK_LEFT_LEAF_ROWS[row] & mask) != 0;
  case 2:
    return (SHAMROCK_TOP_LEAF_ROWS[row] & mask) != 0 ||
           (SHAMROCK_LEFT_LEAF_ROWS[row] & mask) != 0 ||
           (SHAMROCK_RIGHT_LEAF_ROWS[row] & mask) != 0;
  default:
    return (SHAMROCK_TOP_LEAF_ROWS[row] & mask) != 0 ||
           (SHAMROCK_LEFT_LEAF_ROWS[row] & mask) != 0 ||
           (SHAMROCK_RIGHT_LEAF_ROWS[row] & mask) != 0 ||
           (SHAMROCK_CENTER_ROWS[row] & mask) != 0;
  }
}

static void drawShamrock(MilestoneCtx &ctx, int leftCol, int topRow,
                         uint8_t revealStage, bool dimStem) {
  for (int row = 0; row < SHAMROCK_HEIGHT; row++) {
    for (int col = 0; col < SHAMROCK_WIDTH; col++) {
      if (!shamrockPixelLit(row, col, revealStage)) {
        continue;
      }
      int displayRow = topRow + row;
      int displayCol = leftCol + col;
      if (displayRow < 0 || displayRow >= ctx.height || displayCol < 0 ||
          displayCol >= ctx.width) {
        continue;
      }
      bool stem = (SHAMROCK_CENTER_ROWS[row] &
                   (1u << (SHAMROCK_WIDTH - 1 - col))) != 0 &&
                  row >= 6;
      if (!(dimStem && stem)) {
        ctx.matrix->setPoint(displayRow, ctx.colStart + displayCol, true);
      }
    }
  }
}

static void drawMiniPot(MilestoneCtx &ctx, int leftCol, int topRow,
                        bool sparkle) {
  static const uint8_t POT_WIDTH = 9;
  static const uint8_t POT_HEIGHT = 5;
  static const uint16_t POT_ROWS[] = {
      0b011111110, 0b001111100, 0b011111110, 0b111111111, 0b011111110,
  };

  for (int row = 0; row < POT_HEIGHT; row++) {
    for (int col = 0; col < POT_WIDTH; col++) {
      if ((POT_ROWS[row] & (1u << (POT_WIDTH - 1 - col))) == 0) {
        continue;
      }
      int displayRow = topRow + row;
      int displayCol = leftCol + col;
      if (displayRow >= 0 && displayRow < ctx.height && displayCol >= 0 &&
          displayCol < ctx.width) {
        ctx.matrix->setPoint(displayRow, ctx.colStart + displayCol, true);
      }
    }
  }

  if (sparkle && topRow - 1 >= 0) {
    for (int col = 1; col < POT_WIDTH; col += 2) {
      int displayCol = leftCol + col;
      if (displayCol >= 0 && displayCol < ctx.width) {
        ctx.matrix->setPoint(topRow - 1, ctx.colStart + displayCol, true);
      }
    }
  }
}

static void drawBeerMug(MilestoneCtx &ctx, int leftCol, uint8_t fillRows,
                        uint8_t foamFrame) {
  const int mugTop = 0;
  const int mugBottom = 7;
  const int mugLeft = leftCol + 1;
  const int mugRight = leftCol + 8;
  const int handleLeft = leftCol + 9;
  const int handleRight = leftCol + 11;

  for (int col = mugLeft; col <= mugRight; col++) {
    if ((col + foamFrame) % 3 != 0) {
      ctx.matrix->setPoint(mugTop, ctx.colStart + col, true);
    }
  }
  for (int col = mugLeft + 1; col <= mugRight - 1; col += 2) {
    ctx.matrix->setPoint(mugTop + 1, ctx.colStart + col, true);
  }

  for (int row = 2; row <= mugBottom; row++) {
    ctx.matrix->setPoint(row, ctx.colStart + mugLeft, true);
    ctx.matrix->setPoint(row, ctx.colStart + mugRight, true);
  }
  for (int col = mugLeft; col <= mugRight; col++) {
    ctx.matrix->setPoint(mugBottom, ctx.colStart + col, true);
  }

  ctx.matrix->setPoint(3, ctx.colStart + handleLeft, true);
  ctx.matrix->setPoint(3, ctx.colStart + handleRight, true);
  ctx.matrix->setPoint(4, ctx.colStart + handleRight, true);
  ctx.matrix->setPoint(5, ctx.colStart + handleRight, true);
  ctx.matrix->setPoint(6, ctx.colStart + handleLeft, true);
  ctx.matrix->setPoint(6, ctx.colStart + handleRight, true);

  int firstFillRow = max(2, mugBottom - (int)fillRows);
  for (int row = firstFillRow; row < mugBottom; row++) {
    for (int col = mugLeft + 1; col < mugRight; col++) {
      bool bubble = ((row + col + foamFrame) % 7 == 0);
      if (!bubble) {
        ctx.matrix->setPoint(row, ctx.colStart + col, true);
      }
    }
  }
}

static void drawGoldCoin(MilestoneCtx &ctx, int centerX, int centerY, int size) {
  for (int row = -size; row <= size; row++) {
    for (int col = -size; col <= size; col++) {
      if (abs(col) + abs(row) <= size + 1) {
        int px = centerX + col;
        int py = centerY + row;
        if (px >= 0 && px < ctx.width && py >= 0 && py < ctx.height) {
          ctx.matrix->setPoint(py, ctx.colStart + px, true);
        }
      }
    }
  }
}

void runStPatricksHolidayAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  struct Coin {
    int x;
    int y;
    int size;
  };
  Coin coins[6];
  for (int i = 0; i < 6; i++) {
    coins[i].x = (i * ctx.width) / 6 + 1;
    coins[i].y = -2 - (i % 3);
    coins[i].size = 1;
  }

  for (int frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    for (int i = 0; i < 6; i++) {
      coins[i].y++;
      if (coins[i].y > ctx.height + 2) {
        coins[i].y = -2;
        coins[i].x = random(ctx.width);
      }
      drawGoldCoin(ctx, coins[i].x, coins[i].y, coins[i].size);
    }
    milestoneFrameShow(ctx, 42, 0);
  }

  for (uint8_t fill = 0; fill <= 5; fill++) {
    for (uint8_t shimmer = 0; shimmer < 2; shimmer++) {
      milestoneClear(ctx);
      drawBeerMug(ctx, ctx.cx - 6, fill, fill + shimmer);
      milestoneFrameShow(ctx, 85, fill);
    }
  }
  for (int cheers = 0; cheers < 3; cheers++) {
    milestoneClear(ctx);
    drawBeerMug(ctx, ctx.cx - 6, 5, cheers);
    ctx.matrix->setPoint(1, ctx.colStart + max(0, ctx.cx - 10), true);
    ctx.matrix->setPoint(2, ctx.colStart + min(ctx.width - 1, ctx.cx + 10),
                         true);
    milestoneFrameShow(ctx, 120, cheers % 2 == 0 ? 6 : 0);
  }

  for (int frame = 0; frame < 6; frame++) {
    milestoneClear(ctx);
    drawMiniPot(ctx, ctx.cx - 4, 3, frame % 2 == 0);
    milestoneFrameShow(ctx, 115, frame % 2 == 0 ? 6 : 0);
  }

  int shamLeft = ctx.cx - SHAMROCK_WIDTH / 2;
  int shamTop = 0;
  for (uint8_t stage = 0; stage < 4; stage++) {
    milestoneClear(ctx);
    drawShamrock(ctx, shamLeft, shamTop, stage, false);
    milestoneFrameShow(ctx, 220, 0);
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    drawShamrock(ctx, shamLeft, shamTop, 3, pulse % 2 == 1);
    if (pulse == 0) {
      ctx.matrix->setPoint(0, ctx.colStart + max(0, shamLeft - 2), true);
      ctx.matrix->setPoint(1, ctx.colStart + min(ctx.width - 1, shamLeft + 16),
                           true);
    }
    milestoneFrameShow(ctx, 120, pulse % 2 == 0 ? 6 : 0);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, 70, 0);
  }

  for (int burst = 0; burst < 10; burst++) {
    milestoneClear(ctx);
    drawShamrock(ctx, shamLeft, shamTop, 3, false);
    for (int spark = 0; spark < 8; spark++) {
      float angle = spark * 0.785398f + burst * 0.25f;
      int px = ctx.cx + (int)(burst * cosf(angle) * 0.9f);
      int py = 3 + (int)(burst * sinf(angle) * 0.7f);
      if (px >= 0 && px < ctx.width && py >= 0 && py < ctx.height) {
        ctx.matrix->setPoint(py, ctx.colStart + px, burst % 2 == 0);
      }
    }
    milestoneFrameShow(ctx, 45, min(7, (int)burst));
  }

  milestoneEffectEnd(ctx);
  holidayScrollMessage(display, "Happy St Patrick's Day!");
}
