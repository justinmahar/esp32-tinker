#include "milestone_hours_animations.h"

#include "milestone_helpers.h"

static void runHoursTier(MD_Parola &display, void (*animation)(MD_Parola &),
                         MilestoneTier tier, uint16_t statHoldMs,
                         uint16_t labelHoldMs) {
  animation(display);
  char increaseLabel[12];
  milestoneGetIncreaseLabel(tier, increaseLabel, sizeof(increaseLabel));
  milestoneShowCenteredText(display, "HOURS", statHoldMs);
  milestoneShowCenteredText(display, increaseLabel, labelHoldMs);
  milestoneCleanupDisplay(display);
}

// Two triangles meeting at the pinch (row 3). Half-width per row, rows 0–7.
static const int8_t HOURS_GLASS_HW[] = {3, 2, 1, 0, 1, 2, 3, 3};

static const int8_t HOURS_TOP_SAND_R[] = {0, 0, 0, 0, 0, 1, 1, 1};
static const int8_t HOURS_TOP_SAND_DX[] = {-2, -1, 0, 1, 2, -1, 0, 1};
static const int8_t HOURS_BOT_SAND_R[] = {7, 7, 7, 7, 7, 6, 6, 6};
static const int8_t HOURS_BOT_SAND_DX[] = {-2, -1, 0, 1, 2, -1, 0, 1};
static const int HOURS_SAND_COUNT = 8;

static void hoursDrawHourglassFrameAt(MilestoneCtx &ctx, int ox, int revealThroughRow) {
  for (int row = 0; row < ctx.height && row <= revealThroughRow; row++) {
    int hw = HOURS_GLASS_HW[row];
    if (hw == 0) {
      if (ox >= 0 && ox < ctx.width) {
        ctx.matrix->setPoint(row, ctx.colStart + ox, true);
      }
      continue;
    }
    for (int c = ox - hw; c <= ox + hw; c++) {
      if (c < 0 || c >= ctx.width) {
        continue;
      }
      bool cap = (row == 0 || row == 7);
      bool edge = (c == ox - hw || c == ox + hw);
      if (cap || edge) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
  }
}

static void hoursDrawHourglassFrame(MilestoneCtx &ctx, int revealThroughRow) {
  hoursDrawHourglassFrameAt(ctx, ctx.cx, revealThroughRow);
}

static void hoursDrawClockTicks(MilestoneCtx &ctx) {
  static const int8_t TICK_DX[] = {-6, 6, -7, 7, -6, 6};
  static const int8_t TICK_ROW[] = {1, 1, 4, 4, 6, 6};
  for (int i = 0; i < 6; i++) {
    int col = ctx.cx + TICK_DX[i];
    int row = TICK_ROW[i];
    if (col >= 0 && col < ctx.width) {
      ctx.matrix->setPoint(row, ctx.colStart + col, true);
    }
  }
}

static void hoursDrawSandStream(MilestoneCtx &ctx, int headRow, int tailRow) {
  for (int row = tailRow; row <= headRow; row++) {
    if (row >= 0 && row < ctx.height && (row == headRow || row == tailRow)) {
      ctx.matrix->setPoint(row, ctx.colStart + ctx.cx, true);
    }
  }
}

static void hoursDrawTopSand(MilestoneCtx &ctx, int remaining) {
  int start = HOURS_SAND_COUNT - remaining;
  for (int i = start; i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_TOP_SAND_R[i];
    int col = ctx.cx + HOURS_TOP_SAND_DX[i];
    ctx.matrix->setPoint(row, ctx.colStart + col, true);
  }
}

static void hoursDrawBottomSand(MilestoneCtx &ctx, int count) {
  for (int i = 0; i < count && i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_BOT_SAND_R[i];
    int col = ctx.cx + HOURS_BOT_SAND_DX[i];
    ctx.matrix->setPoint(row, ctx.colStart + col, true);
  }
}

static void hoursDrawFallingGrainAt(MilestoneCtx &ctx, int ox, int row, int trailRow) {
  if (ox < 0 || ox >= ctx.width) {
    return;
  }
  ctx.matrix->setPoint(row, ctx.colStart + ox, true);
  if (trailRow >= 0 && trailRow < ctx.height && trailRow != row) {
    ctx.matrix->setPoint(trailRow, ctx.colStart + ox, true);
  }
}

static void hoursDrawFallingGrain(MilestoneCtx &ctx, int row, int trailRow) {
  hoursDrawFallingGrainAt(ctx, ctx.cx, row, trailRow);
}

static void hoursDrawFilledHourglass(MilestoneCtx &ctx, int topRemaining, int bottomCount) {
  hoursDrawClockTicks(ctx);
  hoursDrawTopSand(ctx, topRemaining);
  hoursDrawBottomSand(ctx, bottomCount);
  hoursDrawHourglassFrame(ctx, ctx.height - 1);
}

static void hoursDrawTopSandAt(MilestoneCtx &ctx, int ox, int remaining) {
  int start = HOURS_SAND_COUNT - remaining;
  for (int i = start; i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_TOP_SAND_R[i];
    int col = ox + HOURS_TOP_SAND_DX[i];
    if (col >= 0 && col < ctx.width) {
      ctx.matrix->setPoint(row, ctx.colStart + col, true);
    }
  }
}

static void hoursDrawBottomSandAt(MilestoneCtx &ctx, int ox, int count) {
  for (int i = 0; i < count && i < HOURS_SAND_COUNT; i++) {
    int row = HOURS_BOT_SAND_R[i];
    int col = ox + HOURS_BOT_SAND_DX[i];
    if (col >= 0 && col < ctx.width) {
      ctx.matrix->setPoint(row, ctx.colStart + col, true);
    }
  }
}

static void hoursDrawDualHourglasses(MilestoneCtx &ctx, int leftOx, int rightOx,
                                     int topRemaining, int bottomCount) {
  hoursDrawTopSandAt(ctx, leftOx, topRemaining);
  hoursDrawTopSandAt(ctx, rightOx, topRemaining);
  hoursDrawBottomSandAt(ctx, leftOx, bottomCount);
  hoursDrawBottomSandAt(ctx, rightOx, bottomCount);
  hoursDrawHourglassFrameAt(ctx, leftOx, ctx.height - 1);
  hoursDrawHourglassFrameAt(ctx, rightOx, ctx.height - 1);
}

static void hoursAnimDualBuild(MilestoneCtx &ctx, int leftOx, int rightOx) {
  for (int row = 0; row < ctx.height; row++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrameAt(ctx, leftOx, row);
    hoursDrawHourglassFrameAt(ctx, rightOx, row);
    milestoneFrameShow(ctx, 40, min(7, (int)row));
  }
}

static void hoursAnimDualFill(MilestoneCtx &ctx, int leftOx, int rightOx) {
  for (int top = 1; top <= HOURS_SAND_COUNT; top++) {
    milestoneClear(ctx);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, top, 0);
    milestoneFrameShow(ctx, top == HOURS_SAND_COUNT ? 100 : 38, min(7, (int)top));
  }
}

static void hoursAnimDualDrain(MilestoneCtx &ctx, int leftOx, int rightOx) {
  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  const int fallSteps = 6;
  for (int grain = 0; grain < HOURS_SAND_COUNT; grain++) {
    for (int step = 0; step < fallSteps; step++) {
      milestoneClear(ctx);
      hoursDrawTopSandAt(ctx, leftOx, HOURS_SAND_COUNT - grain - 1);
      hoursDrawTopSandAt(ctx, rightOx, HOURS_SAND_COUNT - grain - 1);
      hoursDrawBottomSandAt(ctx, leftOx, grain);
      hoursDrawBottomSandAt(ctx, rightOx, grain);
      hoursDrawFallingGrainAt(ctx, leftOx, fallRows[step],
                              step > 0 ? fallRows[step - 1] : -1);
      hoursDrawFallingGrainAt(ctx, rightOx, fallRows[step],
                              step > 0 ? fallRows[step - 1] : -1);
      hoursDrawHourglassFrameAt(ctx, leftOx, ctx.height - 1);
      hoursDrawHourglassFrameAt(ctx, rightOx, ctx.height - 1);
      milestoneFrameShow(ctx, step < 2 ? 36 : 28, min(5, (int)step));
    }
    milestoneClear(ctx);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, HOURS_SAND_COUNT - grain - 1,
                             grain + 1);
    milestoneFrameShow(ctx, 22, 0);
  }
}

static void hoursAnimDualFinale(MilestoneCtx &ctx, int leftOx, int rightOx) {
  milestoneClear(ctx);
  hoursDrawDualHourglasses(ctx, leftOx, rightOx, 0, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 120, 0);

  for (int ring = 1; ring <= 4; ring++) {
    milestoneClear(ctx);
    milestoneDrawExplosionRing(ctx, leftOx, ctx.cy, ring);
    milestoneDrawExplosionRing(ctx, rightOx, ctx.cy, ring);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, 0, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, 44, min(6, (int)ring));
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    hoursDrawDualHourglasses(ctx, leftOx, rightOx, 0, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, pulse == 2 ? 90 : 60, pulse % 2 == 0 ? 4 : 0);
    if (pulse < 2) {
      milestoneClear(ctx);
      milestoneFrameShow(ctx, 30, 0);
    }
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 70, 0);
}

static void hoursDrawTripleHourglasses(MilestoneCtx &ctx, int leftOx, int centerOx,
                                       int rightOx, int topRemaining, int bottomCount) {
  hoursDrawTopSandAt(ctx, leftOx, topRemaining);
  hoursDrawTopSandAt(ctx, centerOx, topRemaining);
  hoursDrawTopSandAt(ctx, rightOx, topRemaining);
  hoursDrawBottomSandAt(ctx, leftOx, bottomCount);
  hoursDrawBottomSandAt(ctx, centerOx, bottomCount);
  hoursDrawBottomSandAt(ctx, rightOx, bottomCount);
  hoursDrawHourglassFrameAt(ctx, leftOx, ctx.height - 1);
  hoursDrawHourglassFrameAt(ctx, centerOx, ctx.height - 1);
  hoursDrawHourglassFrameAt(ctx, rightOx, ctx.height - 1);
}

static void hoursDrawWingTicks(MilestoneCtx &ctx, int litThrough) {
  const int cols[6] = {1, 1, 2, -2, -2, -3};
  const int rows[6] = {1, 6, 4, 1, 6, 4};
  for (int i = 0; i < 6 && i <= litThrough; i++) {
    int col = cols[i] >= 0 ? cols[i] : ctx.width + cols[i];
    if (col >= 0 && col < ctx.width) {
      ctx.matrix->setPoint(rows[i], ctx.colStart + col, true);
    }
  }
}

static void hoursDrawSandStreamAt(MilestoneCtx &ctx, int ox, int headRow, int tailRow) {
  for (int row = tailRow; row <= headRow; row++) {
    if (row >= 0 && row < ctx.height && (row == headRow || row == tailRow)) {
      ctx.matrix->setPoint(row, ctx.colStart + ox, true);
    }
  }
}

static bool hoursGlassOccupies(MilestoneCtx &ctx, int ox, int col, int row) {
  if (row < 0 || row >= ctx.height) {
    return false;
  }
  int hw = HOURS_GLASS_HW[row];
  return col >= ox - hw && col <= ox + hw;
}

static bool hoursTripleBlocksPixel(MilestoneCtx &ctx, int leftOx, int centerOx,
                                   int rightOx, int col, int row) {
  return hoursGlassOccupies(ctx, leftOx, col, row) ||
         hoursGlassOccupies(ctx, centerOx, col, row) ||
         hoursGlassOccupies(ctx, rightOx, col, row);
}

static void hoursStampTripleFull(MilestoneCtx &ctx, int leftOx, int centerOx, int rightOx) {
  hoursDrawTripleHourglasses(ctx, leftOx, centerOx, rightOx, 0, HOURS_SAND_COUNT);
}

static void hoursAnimTripleBuildCascade(MilestoneCtx &ctx, int leftOx, int centerOx,
                                        int rightOx) {
  const int oxs[] = {leftOx, centerOx, rightOx};
  for (int glass = 0; glass < 3; glass++) {
    for (int row = 0; row < ctx.height; row++) {
      milestoneClear(ctx);
      for (int done = 0; done < glass; done++) {
        hoursDrawHourglassFrameAt(ctx, oxs[done], ctx.height - 1);
      }
      hoursDrawHourglassFrameAt(ctx, oxs[glass], row);
      milestoneFrameShow(ctx, 32, min(7, row + glass * 2));
    }
    for (int ring = 1; ring <= 4; ring++) {
      milestoneClear(ctx);
      for (int done = 0; done <= glass; done++) {
        hoursDrawHourglassFrameAt(ctx, oxs[done], ctx.height - 1);
      }
      milestoneDrawExplosionRing(ctx, oxs[glass], ctx.cy, ring);
      hoursDrawHourglassFrameAt(ctx, oxs[glass], ctx.height - 1);
      milestoneFrameShow(ctx, 26, min(6, ring + glass));
    }
    milestoneClear(ctx);
    milestoneFillAll(ctx);
    milestoneFrameShow(ctx, glass == 2 ? 55 : 35, 0);
    milestoneClear(ctx);
    for (int done = 0; done <= glass; done++) {
      hoursDrawHourglassFrameAt(ctx, oxs[done], ctx.height - 1);
    }
    milestoneFrameShow(ctx, 40, 0);
  }
}

static void hoursAnimTripleFill(MilestoneCtx &ctx, int leftOx, int centerOx, int rightOx) {
  for (int top = 2; top <= HOURS_SAND_COUNT; top += 2) {
    milestoneClear(ctx);
    hoursDrawTripleHourglasses(ctx, leftOx, centerOx, rightOx, top, 0);
    milestoneFrameShow(ctx, 28, min(7, (int)top));
  }
  milestoneClear(ctx);
  hoursDrawTripleHourglasses(ctx, leftOx, centerOx, rightOx, HOURS_SAND_COUNT, 0);
  milestoneFrameShow(ctx, 80, 0);
}

static void hoursAnimTripleSandRush(MilestoneCtx &ctx, int leftOx, int centerOx,
                                    int rightOx) {
  const int oxs[] = {leftOx, centerOx, rightOx};
  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  for (int batch = 0; batch < HOURS_SAND_COUNT / 2; batch++) {
    int grainsDone = batch * 2;
    for (int step = 0; step < 6; step++) {
      milestoneClear(ctx);
      for (int g = 0; g < 3; g++) {
        hoursDrawTopSandAt(ctx, oxs[g], HOURS_SAND_COUNT - grainsDone - 2);
        hoursDrawBottomSandAt(ctx, oxs[g], grainsDone);
        hoursDrawSandStreamAt(ctx, oxs[g], fallRows[step], max(1, fallRows[step] - 2));
        hoursDrawHourglassFrameAt(ctx, oxs[g], ctx.height - 1);
      }
      milestoneFrameShow(ctx, 18, min(4, (int)step));
    }
    grainsDone += 2;
    milestoneClear(ctx);
    hoursDrawTripleHourglasses(ctx, leftOx, centerOx, rightOx,
                               HOURS_SAND_COUNT - grainsDone, grainsDone);
    milestoneFrameShow(ctx, 16, 0);
  }
}

static void hoursAnimTripleDrain(MilestoneCtx &ctx, int leftOx, int centerOx, int rightOx) {
  const int oxs[] = {leftOx, centerOx, rightOx};
  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  const int fallSteps = 6;
  for (int grain = 0; grain < HOURS_SAND_COUNT; grain++) {
    for (int step = 0; step < fallSteps; step++) {
      milestoneClear(ctx);
      for (int g = 0; g < 3; g++) {
        hoursDrawTopSandAt(ctx, oxs[g], HOURS_SAND_COUNT - grain - 1);
        hoursDrawBottomSandAt(ctx, oxs[g], grain);
        hoursDrawFallingGrainAt(ctx, oxs[g], fallRows[step],
                                step > 0 ? fallRows[step - 1] : -1);
        hoursDrawHourglassFrameAt(ctx, oxs[g], ctx.height - 1);
      }
      milestoneFrameShow(ctx, step < 2 ? 28 : 20, min(5, (int)step));
    }
    milestoneClear(ctx);
    hoursDrawTripleHourglasses(ctx, leftOx, centerOx, rightOx, HOURS_SAND_COUNT - grain - 1,
                               grain + 1);
    milestoneFrameShow(ctx, 18, 0);
  }
}

static void hoursAnimTripleTickStorm(MilestoneCtx &ctx, int leftOx, int centerOx,
                                     int rightOx) {
  for (int pass = 0; pass < 2; pass++) {
    for (int tick = 0; tick < 6; tick++) {
      milestoneClear(ctx);
      hoursDrawWingTicks(ctx, tick);
      hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
      milestoneFrameShow(ctx, pass == 0 ? 32 : 24, min(6, (int)tick));
    }
  }
}

static void hoursAnimTripleChainBlasts(MilestoneCtx &ctx, int leftOx, int centerOx,
                                       int rightOx) {
  const int chainX[] = {leftOx, centerOx, rightOx, centerOx, leftOx, rightOx};
  for (int site = 0; site < 6; site++) {
    for (int ring = 0; ring <= 6; ring++) {
      milestoneClear(ctx);
      hoursDrawWingTicks(ctx, 5);
      for (int s = 0; s <= site; s++) {
        int drawRing = (s == site) ? ring : max(0, 6 - (site - s));
        milestoneDrawExplosionRing(ctx, chainX[s], ctx.cy, drawRing);
        if (drawRing > 1) {
          milestoneDrawExplosionRing(ctx, chainX[s], ctx.cy, drawRing - 1);
        }
      }
      hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
      milestoneFrameShow(ctx, 20, min(7, (int)ring));
    }
    delay(15);
  }
}

static void hoursAnimTripleFireworks(MilestoneCtx &ctx, int leftOx, int centerOx,
                                     int rightOx) {
  const int burstX[] = {leftOx, centerOx, rightOx, 2, ctx.width - 3, leftOx, rightOx};
  const int burstY[] = {1, 1, 1, 3, 3, 6, 6};
  for (int wave = 0; wave < 3; wave++) {
    for (int frame = 0; frame < 16; frame++) {
      milestoneClear(ctx);
      hoursDrawWingTicks(ctx, 5);
      for (int b = 0; b < 4 + wave; b++) {
        int idx = (b + wave) % 7;
        milestoneDrawFireworkBurst(ctx, burstX[idx], burstY[idx], frame + b, 10,
                                   0.58f + wave * 0.06f);
      }
      hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
      milestoneFrameShow(ctx, 26, min(6, frame / 3));
    }
    delay(35);
  }
}

static void hoursAnimTripleExplosionWave(MilestoneCtx &ctx, int leftOx, int centerOx,
                                         int rightOx) {
  for (int ring = 1; ring <= 7; ring++) {
    milestoneClear(ctx);
    milestoneDrawExplosionRing(ctx, leftOx, ctx.cy, ring);
    milestoneDrawExplosionRing(ctx, centerOx, ctx.cy, ring);
    milestoneDrawExplosionRing(ctx, rightOx, ctx.cy, ring);
    if (ring > 2) {
      milestoneDrawDiamondRing(ctx, ring + 1);
    }
    hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
    milestoneFrameShow(ctx, 34, min(6, (int)ring));
  }
}

static void hoursAnimTripleMegablast(MilestoneCtx &ctx, int leftOx, int centerOx,
                                      int rightOx) {
  for (int ring = 0; ring <= 11; ring++) {
    milestoneClear(ctx);
    milestoneDrawExplosionRing(ctx, ctx.cx, ctx.cy, ring);
    if (ring > 2) {
      milestoneDrawExplosionRing(ctx, ctx.cx, ctx.cy, ring - 2);
    }
    if (ring % 3 == 0) {
      milestoneDrawExplosionRing(ctx, leftOx, ctx.cy, ring / 2 + 1);
      milestoneDrawExplosionRing(ctx, rightOx, ctx.cy, ring / 2 + 1);
    }
    hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
    milestoneFrameShow(ctx, ring < 6 ? 22 : 30, min(9, ring / 2));
  }
  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 75, 0);
  milestoneClear(ctx);
  hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
  milestoneFrameShow(ctx, 50, 0);
}

static void hoursAnimTripleSparkStorm(MilestoneCtx &ctx, int leftOx, int centerOx,
                                      int rightOx) {
  for (int frame = 0; frame < 28; frame++) {
    milestoneClear(ctx);
    hoursDrawWingTicks(ctx, 5);
    for (int c = 0; c < ctx.width; c++) {
      int sparkRow = (frame * 2 + c * 5) % ctx.height;
      if (hoursTripleBlocksPixel(ctx, leftOx, centerOx, rightOx, c, sparkRow)) {
        continue;
      }
      if (frame % 2 == 0) {
        ctx.matrix->setPoint(sparkRow, ctx.colStart + c, true);
      }
      if (frame % 5 == c % 5) {
        int trailRow = sparkRow + 1;
        if (trailRow < ctx.height &&
            !hoursTripleBlocksPixel(ctx, leftOx, centerOx, rightOx, c, trailRow)) {
          ctx.matrix->setPoint(trailRow, ctx.colStart + c, true);
        }
      }
    }
    hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
    milestoneFrameShow(ctx, 24, min(8, frame / 4));
  }
}

static void hoursAnimTripleVictory(MilestoneCtx &ctx, int leftOx, int centerOx,
                                   int rightOx) {
  milestoneAnimShockwaveFlashes(ctx, 2, 45, 35);
  milestoneClear(ctx);
  hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
  milestoneFrameShow(ctx, 60, 0);

  for (int pulse = 0; pulse < 5; pulse++) {
    milestoneClear(ctx);
    hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
    milestoneFrameShow(ctx, pulse == 4 ? 100 : 55, pulse % 2 == 0 ? 3 : 0);
    if (pulse < 4) {
      milestoneClear(ctx);
      milestoneFrameShow(ctx, 22, 0);
    }
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 85, 0);
  milestoneAnimInwardCollapse(ctx, 32);
}

static void hoursAnimTripleBuildTogether(MilestoneCtx &ctx, int leftOx, int centerOx,
                                         int rightOx) {
  for (int row = 0; row < ctx.height; row++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrameAt(ctx, leftOx, row);
    hoursDrawHourglassFrameAt(ctx, centerOx, row);
    hoursDrawHourglassFrameAt(ctx, rightOx, row);
    milestoneFrameShow(ctx, 30, min(7, (int)row));
  }
  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 50, 0);
  milestoneClear(ctx);
  hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
  milestoneFrameShow(ctx, 55, 0);
}

static void hoursAnimTripleMultiBlast(MilestoneCtx &ctx, int leftOx, int centerOx,
                                      int rightOx) {
  const int originX[] = {leftOx, centerOx, rightOx};
  for (int ring = 0; ring <= 12; ring++) {
    milestoneClear(ctx);
    for (int s = 0; s < 3; s++) {
      milestoneDrawExplosionRing(ctx, originX[s], ctx.cy, ring);
      if (ring > 2) {
        milestoneDrawExplosionRing(ctx, originX[s], ctx.cy, ring - 2);
      }
    }
    hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
    milestoneFrameShow(ctx, ring < 7 ? 18 : 26, min(9, ring / 2));
  }
  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 80, 0);
  milestoneClear(ctx);
  hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
  milestoneFrameShow(ctx, 55, 0);
}

static void hoursRestoreTripleHold(MilestoneCtx &ctx, int leftOx, int centerOx, int rightOx,
                                   uint16_t holdMs) {
  milestoneClear(ctx);
  hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
  milestoneFrameShow(ctx, holdMs, 0);
}

// +1M hours — triple sand story, firework barrage, shockwave, victory pulses.
static void hoursAnimTier1M(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  const int leftOx = ctx.cx - 10;
  const int centerOx = ctx.cx;
  const int rightOx = ctx.cx + 10;

  milestoneAnimSlowRingBuild(ctx, 12, 40);
  delay(60);
  milestoneAnimDualEdgeClimb(ctx, 28);
  milestoneAnimCounterShimmer(ctx, 8, 30);
  delay(45);

  hoursAnimTripleBuildTogether(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleFill(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleDrain(ctx, leftOx, centerOx, rightOx);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 75);

  hoursAnimTripleExplosionWave(ctx, leftOx, centerOx, rightOx);
  milestoneAnimScreenShake(ctx, 6, 18);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 55);

  const int fwX[10] = {1, ctx.width / 6, ctx.width / 4, 2 * ctx.width / 5,
                       ctx.cx, 3 * ctx.width / 5, 3 * ctx.width / 4,
                       5 * ctx.width / 6, ctx.width - 2, rightOx};
  const int fwY[10] = {0, 3, 6, 2, 1, 2, 6, 3, 0, 2};
  milestoneAnimFireworkBarrage(ctx, fwX, fwY, 10, 38, 2, 12, 0.74f, 30);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 65);

  const int chainX[5] = {2, leftOx, centerOx, rightOx, ctx.width - 3};
  milestoneAnimChainExplosions(ctx, chainX, 5, 7, 26, 28);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 55);

  milestoneAnimShockwaveFlashes(ctx, 4, 40, 32);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 50);

  for (int pulse = 0; pulse < 4; pulse++) {
    milestoneClear(ctx);
    hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
    milestoneFrameShow(ctx, pulse == 3 ? 95 : 55, pulse % 2 == 0 ? 4 : 0);
    if (pulse < 3) {
      milestoneClear(ctx);
      milestoneFrameShow(ctx, 22, 0);
    }
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 95, 0);
  milestoneAnimFinalFlashes(ctx, 3, 60, 35, 125);
  milestoneAnimInwardCollapse(ctx, 30);
  milestoneEffectEnd(ctx);
}

// +10M hours — ultimate fullscreen spectacle, twin firework barrages, ember finale.
static void hoursAnimTier10M(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  const int leftOx = ctx.cx - 10;
  const int centerOx = ctx.cx;
  const int rightOx = ctx.cx + 10;

  milestoneAnimSlowRingBuild(ctx, 14, 46);
  delay(90);
  milestoneAnimDualEdgeClimb(ctx, 24);
  milestoneAnimCounterShimmer(ctx, 10, 28);
  delay(60);
  milestoneAnimRockets(ctx, 26, 26);
  delay(50);

  hoursAnimTripleBuildTogether(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleFill(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleSandRush(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleDrain(ctx, leftOx, centerOx, rightOx);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 90);

  hoursAnimTripleMultiBlast(ctx, leftOx, centerOx, rightOx);
  milestoneAnimScreenShake(ctx, 10, 18);

  const int fw1X[10] = {1, ctx.width / 7, ctx.width / 4, 3 * ctx.width / 7, ctx.cx,
                        4 * ctx.width / 7, 3 * ctx.width / 4, 6 * ctx.width / 7,
                        ctx.width - 2, leftOx};
  const int fw1Y[10] = {0, 3, 6, 2, 1, 2, 6, 3, 0, 2};
  milestoneAnimFireworkBarrage(ctx, fw1X, fw1Y, 10, 38, 2, 12, 0.72f, 30);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 75);

  const int chainForward[6] = {2, leftOx, centerOx, rightOx, ctx.width - 3, ctx.cx};
  milestoneAnimChainExplosions(ctx, chainForward, 6, 8, 24, 28);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 60);

  const int chainReverse[6] = {ctx.width - 3, rightOx, centerOx, leftOx, 2, ctx.cx};
  milestoneAnimChainExplosions(ctx, chainReverse, 6, 8, 24, 28);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 60);

  milestoneAnimShockwaveFlashes(ctx, 5, 42, 34);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 50);

  milestoneAnimTripleTsunami(ctx, 18, 34, 65);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 55);

  hoursAnimTripleMegablast(ctx, leftOx, centerOx, rightOx);
  milestoneAnimFallingComets(ctx, 3, 20);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 65);

  const int fw2X[12] = {1, ctx.width / 8, ctx.width / 4, 3 * ctx.width / 8, ctx.cx,
                        5 * ctx.width / 8, 3 * ctx.width / 4, 7 * ctx.width / 8,
                        ctx.width - 2, 3, ctx.width - 4, rightOx};
  const int fw2Y[12] = {0, 2, 5, 3, 0, 3, 5, 2, 0, 7, 6, 2};
  milestoneAnimFireworkBarrage(ctx, fw2X, fw2Y, 12, 44, 1, 14, 0.82f, 28);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 70);

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 110, 0);
  milestoneAnimScreenShake(ctx, 8, 16);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 50);

  milestoneAnimEmberFallout(ctx, 26, 26);
  hoursRestoreTripleHold(ctx, leftOx, centerOx, rightOx, 45);

  for (int pulse = 0; pulse < 6; pulse++) {
    milestoneClear(ctx);
    hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
    milestoneFrameShow(ctx, pulse == 5 ? 105 : 58, pulse % 2 == 0 ? 4 : 0);
    if (pulse < 5) {
      milestoneClear(ctx);
      milestoneFrameShow(ctx, 20, 0);
    }
  }

  milestoneAnimFinalFlashes(ctx, 5, 55, 30, 130);
  milestoneAnimInwardCollapse(ctx, 30);
  milestoneEffectEnd(ctx);
}

static void hoursAnimTickSweep(MilestoneCtx &ctx) {
  static const int8_t TICK_DX[] = {-6, 6, -7, 7, -6, 6};
  static const int8_t TICK_ROW[] = {1, 1, 4, 4, 6, 6};
  for (int i = 0; i < 6; i++) {
    milestoneClear(ctx);
    for (int t = 0; t < 6; t++) {
      int col = ctx.cx + TICK_DX[t];
      int row = TICK_ROW[t];
      if (col >= 0 && col < ctx.width && t <= i) {
        ctx.matrix->setPoint(row, ctx.colStart + col, true);
      }
    }
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 46, min(6, (int)i));
  }
}

static void hoursAnimTimeRipple(MilestoneCtx &ctx, int bands) {
  for (int band = 0; band < bands; band++) {
    milestoneClear(ctx);
    hoursDrawTopSand(ctx, 0);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    hoursDrawClockTicks(ctx);
    for (int c = 0; c < ctx.width; c++) {
      if (abs(c - ctx.cx) > 6 + band) {
        continue;
      }
      for (int row = ctx.cy - band; row <= ctx.cy + band; row++) {
        if (row < 0 || row >= ctx.height) {
          continue;
        }
        bool edge = (row == ctx.cy - band || row == ctx.cy + band);
        if (edge || band == 0) {
          ctx.matrix->setPoint(row, ctx.colStart + c, true);
        }
      }
    }
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 50, min(5, band * 2));
  }
}

static void hoursAnimBottomSettle(MilestoneCtx &ctx) {
  for (int g = 0; g < HOURS_SAND_COUNT; g++) {
    milestoneClear(ctx);
    hoursDrawClockTicks(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    int row = HOURS_BOT_SAND_R[g];
    int col = ctx.cx + HOURS_BOT_SAND_DX[g];
    ctx.matrix->setPoint(row, ctx.colStart + col, true);
    ctx.matrix->setPoint(g % 2 == 0 ? 3 : 4, ctx.colStart + ctx.cx, true);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 42, min(6, g / 2));
  }
}

// +100 hours — hourglass builds, full top drains into bottom, ping, flash.
static void hoursAnimTier100(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (int row = 0; row < ctx.height; row++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, row);
    milestoneFrameShow(ctx, 45, min(7, (int)row));
  }

  milestoneClear(ctx);
  hoursDrawHourglassFrame(ctx, ctx.height - 1);
  hoursDrawTopSand(ctx, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 120, 0);

  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  const int fallSteps = 6;
  for (int grain = 0; grain < HOURS_SAND_COUNT; grain++) {
    for (int step = 0; step < fallSteps; step++) {
      milestoneClear(ctx);
      hoursDrawTopSand(ctx, HOURS_SAND_COUNT - grain - 1);
      hoursDrawBottomSand(ctx, grain);
      hoursDrawFallingGrain(ctx, fallRows[step], step > 0 ? fallRows[step - 1] : -1);
      hoursDrawHourglassFrame(ctx, ctx.height - 1);
      milestoneFrameShow(ctx, step < 2 ? 42 : 34, min(5, (int)step));
    }
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    hoursDrawTopSand(ctx, HOURS_SAND_COUNT - grain - 1);
    hoursDrawBottomSand(ctx, grain + 1);
    milestoneFrameShow(ctx, 26, 0);
  }

  milestoneClear(ctx);
  hoursDrawHourglassFrame(ctx, ctx.height - 1);
  hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 140, 0);

  for (int ring = 1; ring <= 3; ring++) {
    milestoneClear(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    milestoneDrawDiamondRing(ctx, ring + 4);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 48, min(6, ring * 2));
  }

  for (int pulse = 0; pulse < 2; pulse++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, pulse == 0 ? 90 : 55, pulse == 0 ? 0 : 1);
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 65, 0);
  milestoneEffectEnd(ctx);
}

// +1K hours — clock ticks, sand stream pour, ripple, spark rain, finale.
static void hoursAnimTier1K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    if (pulse > 0) {
      hoursDrawClockTicks(ctx);
    }
    milestoneFrameShow(ctx, pulse == 2 ? 70 : 50, min(5, pulse * 2));
  }

  hoursAnimTickSweep(ctx);

  for (int top = 2; top <= HOURS_SAND_COUNT; top += 2) {
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, top, 0);
    milestoneFrameShow(ctx, 36, min(6, top / 2));
  }

  milestoneClear(ctx);
  hoursDrawFilledHourglass(ctx, HOURS_SAND_COUNT, 0);
  milestoneFrameShow(ctx, 100, 0);

  for (int tick = 0; tick < 4; tick++) {
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, HOURS_SAND_COUNT, 0);
    hoursDrawClockTicks(ctx);
    milestoneFrameShow(ctx, tick == 3 ? 80 : 55, tick % 2 == 0 ? 3 : 0);
  }

  const int8_t fallRows[] = {1, 2, 3, 4, 5, 6};
  const int fallSteps = 6;
  for (int batch = 0; batch < HOURS_SAND_COUNT / 2; batch++) {
    int grainsDone = batch * 2;
    for (int step = 0; step < fallSteps; step++) {
      milestoneClear(ctx);
      hoursDrawClockTicks(ctx);
      hoursDrawTopSand(ctx, HOURS_SAND_COUNT - grainsDone - 2);
      hoursDrawBottomSand(ctx, grainsDone);
      hoursDrawSandStream(ctx, fallRows[step], max(1, fallRows[step] - 2));
      if (step == 2) {
        ctx.matrix->setPoint(3, ctx.colStart + ctx.cx - 1, true);
        ctx.matrix->setPoint(4, ctx.colStart + ctx.cx, true);
      }
      if (step == 3) {
        ctx.matrix->setPoint(3, ctx.colStart + ctx.cx + 1, true);
        ctx.matrix->setPoint(4, ctx.colStart + ctx.cx, true);
      }
      hoursDrawHourglassFrame(ctx, ctx.height - 1);
      milestoneFrameShow(ctx, 30, min(4, (int)step));
    }
    grainsDone += 2;
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, HOURS_SAND_COUNT - grainsDone, grainsDone);
    milestoneFrameShow(ctx, 26, 0);
  }

  milestoneClear(ctx);
  hoursDrawFilledHourglass(ctx, 0, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 110, 0);

  hoursAnimTimeRipple(ctx, 4);
  hoursAnimBottomSettle(ctx);

  milestoneClear(ctx);
  hoursDrawFilledHourglass(ctx, 0, HOURS_SAND_COUNT);
  milestoneFrameShow(ctx, 90, 0);

  for (int ring = 1; ring <= 5; ring++) {
    milestoneClear(ctx);
    hoursDrawClockTicks(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    milestoneDrawDiamondRing(ctx, ring + 3);
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 46, min(6, (int)ring));
  }

  for (int frame = 0; frame < 22; frame++) {
    milestoneClear(ctx);
    hoursDrawClockTicks(ctx);
    hoursDrawBottomSand(ctx, HOURS_SAND_COUNT);
    for (int c = 0; c < ctx.width; c++) {
      if (abs(c - ctx.cx) <= 4) {
        continue;
      }
      int sparkRow = (frame * 2 + c * 5) % ctx.height;
      if (frame % 2 == 0) {
        ctx.matrix->setPoint(sparkRow, ctx.colStart + c, true);
      }
      if (frame % 7 == 0 && c == 1) {
        for (int trail = 0; trail < 3; trail++) {
          int row = (sparkRow + trail) % ctx.height;
          if (trail < 2) {
            ctx.matrix->setPoint(row, ctx.colStart + c, true);
          }
        }
      }
    }
    hoursDrawHourglassFrame(ctx, ctx.height - 1);
    milestoneFrameShow(ctx, 32, min(8, frame / 4));
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    hoursDrawFilledHourglass(ctx, 0, HOURS_SAND_COUNT);
    milestoneFrameShow(ctx, pulse == 2 ? 90 : 65, pulse % 2 == 0 ? 3 : 0);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, 35, 0);
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 75, 0);
  milestoneEffectEnd(ctx);
}

// +10K hours — twin hourglasses build, fill, drain in sync, celebration.
static void hoursAnimTier10K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  const int leftOx = ctx.cx - 9;
  const int rightOx = ctx.cx + 9;

  hoursAnimDualBuild(ctx, leftOx, rightOx);
  hoursAnimDualFill(ctx, leftOx, rightOx);
  hoursAnimDualDrain(ctx, leftOx, rightOx);
  hoursAnimDualFinale(ctx, leftOx, rightOx);
  milestoneEffectEnd(ctx);
}

// +100K hours — triple hourglasses, sand rush, chain blasts, fireworks, megablast.
static void hoursAnimTier100K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  const int leftOx = ctx.cx - 10;
  const int centerOx = ctx.cx;
  const int rightOx = ctx.cx + 10;

  milestoneAnimSlowRingBuild(ctx, 10, 40);
  delay(70);

  hoursAnimTripleBuildCascade(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleFill(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleSandRush(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleDrain(ctx, leftOx, centerOx, rightOx);

  milestoneClear(ctx);
  hoursStampTripleFull(ctx, leftOx, centerOx, rightOx);
  milestoneFrameShow(ctx, 100, 0);

  hoursAnimTripleTickStorm(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleChainBlasts(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleFireworks(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleExplosionWave(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleMegablast(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleSparkStorm(ctx, leftOx, centerOx, rightOx);
  hoursAnimTripleVictory(ctx, leftOx, centerOx, rightOx);
  milestoneEffectEnd(ctx);
}

void runHoursMilestoneAnimation(MD_Parola &display, MilestoneTier tier) {
  switch (tier) {
  case MilestoneTier::Tier100:
    runHoursTier(display, hoursAnimTier100, tier, 750, 900);
    break;
  case MilestoneTier::Tier1K:
    runHoursTier(display, hoursAnimTier1K, tier, 850, 1000);
    break;
  case MilestoneTier::Tier10K:
    runHoursTier(display, hoursAnimTier10K, tier, 1100, 1300);
    break;
  case MilestoneTier::Tier100K:
    runHoursTier(display, hoursAnimTier100K, tier, 1200, 1500);
    break;
  case MilestoneTier::Tier1M:
    runHoursTier(display, hoursAnimTier1M, tier, 1300, 1600);
    break;
  case MilestoneTier::Tier10M:
    runHoursTier(display, hoursAnimTier10M, tier, 1500, 2000);
    break;
  default:
    break;
  }
}
