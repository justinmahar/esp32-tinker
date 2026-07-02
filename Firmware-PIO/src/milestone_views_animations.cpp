#include "milestone_views_animations.h"

#include "milestone_helpers.h"

static void runViewsTier(MD_Parola &display, void (*animation)(MD_Parola &),
                         MilestoneTier tier, uint16_t statHoldMs,
                         uint16_t labelHoldMs) {
  animation(display);
  char increaseLabel[12];
  milestoneGetIncreaseLabel(tier, increaseLabel, sizeof(increaseLabel));
  milestoneShowCenteredText(display, "VIEWS", statHoldMs);
  milestoneShowCenteredText(display, increaseLabel, labelHoldMs);
  milestoneCleanupDisplay(display);
}


// --- +10M exclusive effects ---

static void viewsDrawReticle(MilestoneCtx &ctx, int tx, int ty, int arm) {
  for (int d = 1; d <= arm; d++) {
    if (tx - d >= 0) {
      ctx.matrix->setPoint(ty, ctx.colStart + tx - d, d == arm);
    }
    if (tx + d < ctx.width) {
      ctx.matrix->setPoint(ty, ctx.colStart + tx + d, d == arm);
    }
    if (ty - d >= 0) {
      ctx.matrix->setPoint(ty - d, ctx.colStart + tx, d == arm);
    }
    if (ty + d < ctx.height) {
      ctx.matrix->setPoint(ty + d, ctx.colStart + tx, d == arm);
    }
  }
  ctx.matrix->setPoint(ty, ctx.colStart + tx, true);
}

static void viewsAnimReticleLock(MilestoneCtx &ctx, int tx, int ty) {
  for (int arm = 1; arm <= 6; arm++) {
    milestoneClear(ctx);
    viewsDrawReticle(ctx, tx, ty, arm);
    if (arm > 3) {
      viewsDrawReticle(ctx, tx - 8, ty, arm - 3);
      viewsDrawReticle(ctx, tx + 8, ty, arm - 3);
    }
    milestoneFrameShow(ctx, 38, min(7, (int)arm));
  }
}

static void viewsDrawNova(MilestoneCtx &ctx, int ox, int oy, int radius) {
  for (int c = 0; c < ctx.width; c++) {
    for (int row = 0; row < ctx.height; row++) {
      int d = abs(c - ox) + abs(row - oy);
      if (d <= radius) {
        bool shell = (d == radius || d == radius - 1);
        bool core = d <= max(1, radius / 3);
        ctx.matrix->setPoint(row, ctx.colStart + c, shell || core);
      }
    }
  }
}

static void viewsAnimNovaDetonation(MilestoneCtx &ctx, int ox, int oy, int maxR) {
  for (int r = 0; r <= maxR; r++) {
    milestoneClear(ctx);
    viewsDrawNova(ctx, ox, oy, r);
    if (r > 2) {
      viewsDrawNova(ctx, ox, oy, r - 2);
    }
    milestoneDrawFireworkBurst(ctx, ox, oy, r, 10, 0.35f);
    milestoneFrameShow(ctx, r < maxR / 2 ? 22 : 32, min(8, (int)r));
  }
  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 45, 0);
  milestoneClear(ctx);
  milestoneFrameShow(ctx, 30, 0);
}

static void viewsDrawShrapnel(MilestoneCtx &ctx, int ox, int oy, int reach) {
  const int dirs[8][2] = {{1, 0},  {-1, 0}, {0, 1},  {0, -1},
                          {1, 1},  {1, -1}, {-1, 1}, {-1, -1}};
  for (int d = 0; d < 8; d++) {
    for (int len = 1; len <= reach; len++) {
      int c = ox + dirs[d][0] * len;
      int row = oy + dirs[d][1] * len;
      if (c >= 0 && c < ctx.width && row >= 0 && row < ctx.height) {
        bool tip = len >= reach - 1;
        ctx.matrix->setPoint(row, ctx.colStart + c, tip || len % 2 == 0);
      }
    }
  }
}

static void viewsAnimShrapnelBurst(MilestoneCtx &ctx, int ox, int oy) {
  for (int reach = 2; reach <= 14; reach++) {
    milestoneClear(ctx);
    viewsDrawShrapnel(ctx, ox, oy, reach);
    viewsDrawShrapnel(ctx, ox - 8, oy, max(2, reach - 2));
    viewsDrawShrapnel(ctx, ox + 8, oy, max(2, reach - 2));
    milestoneFrameShow(ctx, reach < 8 ? 24 : 30, min(9, reach / 2));
  }
}

static void viewsAnimGridDetonation(MilestoneCtx &ctx) {
  const int cellW = 4;
  const int cellH = 2;
  int cellsX = ctx.width / cellW;
  int cellsY = ctx.height / cellH;
  int total = cellsX * cellsY;
  int order[32];
  int count = 0;
  for (int gy = 0; gy < cellsY; gy++) {
    for (int gx = 0; gx < cellsX; gx++) {
      order[count++] = gy * cellsX + gx;
    }
  }
  for (int i = count - 1; i > 0; i--) {
    int j = random(i + 1);
    int tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
  }

  for (int step = 0; step < total; step++) {
    milestoneClear(ctx);
    for (int s = 0; s <= step; s++) {
      int idx = order[s];
      int gx = idx % cellsX;
      int gy = idx / cellsX;
      int cx = gx * cellW + cellW / 2;
      int cy = gy * cellH + cellH / 2;
      int blast = min(4, s / 2 + 2);
      viewsDrawNova(ctx, cx, cy, blast);
      if (s == step) {
        viewsDrawShrapnel(ctx, cx, cy, blast + 2);
      }
    }
    milestoneFrameShow(ctx, 32, min(6, (int)step));
  }
}

static void viewsAnimGravityWell(MilestoneCtx &ctx) {
  const int count = 22;
  int px[22];
  int py[22];
  for (int i = 0; i < count; i++) {
    switch (i % 4) {
    case 0:
      px[i] = random(ctx.width);
      py[i] = 0;
      break;
    case 1:
      px[i] = ctx.width - 1;
      py[i] = random(ctx.height);
      break;
    case 2:
      px[i] = random(ctx.width);
      py[i] = ctx.height - 1;
      break;
    default:
      px[i] = 0;
      py[i] = random(ctx.height);
      break;
    }
  }

  const int steps = 15;
  for (int frame = 0; frame <= steps; frame++) {
    milestoneClear(ctx);
    for (int i = 0; i < count; i++) {
      int nx = px[i] + (ctx.cx - px[i]) * frame / steps;
      int ny = py[i] + (ctx.cy - py[i]) * frame / steps;
      int tx = px[i] + (ctx.cx - px[i]) * max(0, frame - 1) / steps;
      int ty = py[i] + (ctx.cy - py[i]) * max(0, frame - 1) / steps;
      ctx.matrix->setPoint(ny, ctx.colStart + nx, true);
      if (frame > 0 && (nx != tx || ny != ty)) {
        ctx.matrix->setPoint(ty, ctx.colStart + tx, frame % 2 == 0);
      }
    }
    if (frame >= steps - 3) {
      milestoneDrawDiamondRing(ctx, steps - frame + 1);
    }
    milestoneFrameShow(ctx, 26, min(7, (int)frame));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 35, 0);
  milestoneClear(ctx);
  milestoneFrameShow(ctx, 25, 0);
}

// Almond-shaped eye outline with optional pupil.
static const int8_t VIEWS_EYE_HALF_WIDTH[] = {2, 4, 5, 4, 2};
static const int VIEWS_EYE_SPACING = 8;

static void viewsDrawSingleEye(MilestoneCtx &ctx, int eyeCx, int eyeCy,
                               int openThroughDy, bool showPupil,
                               int pupilOffsetX, bool blinkClosed,
                               int eyeScale, int pupilSize) {
  if (blinkClosed) {
    for (int col = eyeCx - 5 - eyeScale; col <= eyeCx + 5 + eyeScale; col++) {
      if (col < 0 || col >= ctx.width) {
        continue;
      }
      ctx.matrix->setPoint(eyeCy, ctx.colStart + col, true);
    }
    return;
  }

  for (int dy = -2; dy <= 2; dy++) {
    if (dy > openThroughDy) {
      continue;
    }
    int row = eyeCy + dy;
    if (row < 0 || row >= ctx.height) {
      continue;
    }
    int hw = VIEWS_EYE_HALF_WIDTH[dy + 2] + eyeScale;
    for (int col = eyeCx - hw; col <= eyeCx + hw; col++) {
      if (col < 0 || col >= ctx.width) {
        continue;
      }
      bool edge = (col == eyeCx - hw || col == eyeCx + hw || dy == -2 || dy == 2);
      ctx.matrix->setPoint(row, ctx.colStart + col, edge);
    }
  }

  if (showPupil) {
    for (int dy = -pupilSize; dy <= pupilSize - 1; dy++) {
      for (int dx = -pupilSize; dx <= pupilSize; dx++) {
        int row = eyeCy + dy;
        int col = eyeCx + pupilOffsetX + dx;
        if (row >= 0 && row < ctx.height && col >= 0 && col < ctx.width) {
          ctx.matrix->setPoint(row, ctx.colStart + col, true);
        }
      }
    }
  }
}

static void viewsDrawEyes(MilestoneCtx &ctx, int openThroughDy, bool showPupil,
                          int pupilOffsetX, bool blinkClosed, int eyeScale,
                          int pupilSize) {
  viewsDrawSingleEye(ctx, ctx.cx - VIEWS_EYE_SPACING, ctx.cy, openThroughDy,
                     showPupil, pupilOffsetX, blinkClosed, eyeScale, pupilSize);
  viewsDrawSingleEye(ctx, ctx.cx + VIEWS_EYE_SPACING, ctx.cy, openThroughDy,
                     showPupil, pupilOffsetX, blinkClosed, eyeScale, pupilSize);
}

static void viewsDrawEyesFrame(MilestoneCtx &ctx, int openThroughDy,
                               bool showPupil, int pupilOffsetX,
                               bool blinkClosed) {
  viewsDrawEyes(ctx, openThroughDy, showPupil, pupilOffsetX, blinkClosed, 0, 1);
}

// Tiny almond for crowd / peripheral eyes.
static void viewsDrawMiniEye(MilestoneCtx &ctx, int eyeCx, int eyeCy, bool open) {
  if (!open) {
    for (int col = eyeCx - 2; col <= eyeCx + 2; col++) {
      if (col >= 0 && col < ctx.width) {
        ctx.matrix->setPoint(eyeCy, ctx.colStart + col, true);
      }
    }
    return;
  }
  for (int dy = -1; dy <= 1; dy++) {
    int row = eyeCy + dy;
    if (row < 0 || row >= ctx.height) {
      continue;
    }
    int hw = (dy == 0) ? 2 : 1;
    for (int col = eyeCx - hw; col <= eyeCx + hw; col++) {
      if (col >= 0 && col < ctx.width) {
        bool edge = (col == eyeCx - hw || col == eyeCx + hw || dy == -1 || dy == 1);
        ctx.matrix->setPoint(row, ctx.colStart + col, edge || (dy == 0 && abs(col - eyeCx) <= 1));
      }
    }
  }
}

static void viewsDrawCrowdEyes(MilestoneCtx &ctx, int revealThrough,
                                bool blinkClosed) {
  const int cols[] = {3, 9, 15, 21, 27};
  const int rows[] = {1, 4, 6};
  int idx = 0;
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 5; c++) {
      if (idx > revealThrough) {
        return;
      }
      viewsDrawMiniEye(ctx, cols[c], rows[r], !blinkClosed);
      idx++;
    }
  }
}

static void viewsAnimEyeShockwave(MilestoneCtx &ctx) {
  const int cols[] = {3, 9, 15, 21, 27};
  const int rows[] = {1, 4, 6};
  const int maxWave = 18;

  for (int wave = 0; wave <= maxWave; wave++) {
    milestoneClear(ctx);
    viewsDrawNova(ctx, ctx.cx, ctx.cy, min(8, wave / 2));
    if (wave > 4) {
      milestoneDrawExplosionRing(ctx, ctx.cx, ctx.cy, wave - 4);
    }
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 5; c++) {
        int dist = abs(cols[c] - ctx.cx) + abs(rows[r] - ctx.cy);
        if (dist <= wave) {
          viewsDrawMiniEye(ctx, cols[c], rows[r], true);
        }
      }
    }
    if (wave > 10 && wave % 2 == 0) {
      milestoneDrawFireworkBurst(ctx, ctx.cx, ctx.cy, wave, 8, 0.32f);
    }
    milestoneFrameShow(ctx, wave < maxWave / 2 ? 30 : 36, min(8, wave / 2));
  }
}

static void viewsAnimTwinGravityWell(MilestoneCtx &ctx) {
  const int count = 28;
  int px[28];
  int py[28];
  for (int i = 0; i < count; i++) {
    switch (i % 4) {
    case 0:
      px[i] = random(ctx.width);
      py[i] = 0;
      break;
    case 1:
      px[i] = ctx.width - 1;
      py[i] = random(ctx.height);
      break;
    case 2:
      px[i] = random(ctx.width);
      py[i] = ctx.height - 1;
      break;
    default:
      px[i] = 0;
      py[i] = random(ctx.height);
      break;
    }
  }

  const int leftCx = ctx.cx - 6;
  const int rightCx = ctx.cx + 6;
  const int steps = 12;
  for (int phase = 0; phase < 2; phase++) {
    for (int frame = 0; frame <= steps; frame++) {
      milestoneClear(ctx);
      for (int i = 0; i < count; i++) {
        int fx = phase == 0 ? (i % 2 == 0 ? leftCx : rightCx) : ctx.cx;
        int nx = px[i] + (fx - px[i]) * frame / steps;
        int ny = py[i] + (ctx.cy - py[i]) * frame / steps;
        ctx.matrix->setPoint(ny, ctx.colStart + nx, true);
        if (frame > 0) {
          int tx = px[i] + (fx - px[i]) * (frame - 1) / steps;
          int ty = py[i] + (ctx.cy - py[i]) * (frame - 1) / steps;
          ctx.matrix->setPoint(ty, ctx.colStart + tx, frame % 2 == 0);
        }
      }
      if (frame >= steps - 2) {
        milestoneDrawDiamondRing(ctx, steps - frame + 1);
      }
      milestoneFrameShow(ctx, 22, min(6, (int)frame));
    }
    if (phase == 0) {
      milestoneClear(ctx);
      viewsDrawNova(ctx, leftCx, ctx.cy, 4);
      viewsDrawNova(ctx, rightCx, ctx.cy, 4);
      milestoneFrameShow(ctx, 60, 0);
      for (int i = 0; i < count; i++) {
        px[i] = i % 2 == 0 ? leftCx : rightCx;
        py[i] = ctx.cy;
      }
    }
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 40, 0);
  milestoneClear(ctx);
  milestoneFrameShow(ctx, 30, 0);
}

static void viewsAnimOmniscience(MilestoneCtx &ctx) {
  for (int wave = 0; wave <= 14; wave++) {
    milestoneClear(ctx);
    for (int c = 2; c < ctx.width - 1; c += 3) {
      for (int row = 1; row < ctx.height - 1; row += 2) {
        if (abs(c - ctx.cx) + abs(row - ctx.cy) <= wave) {
          viewsDrawMiniEye(ctx, c, row, true);
        }
      }
    }
    milestoneFrameShow(ctx, 28, min(8, wave / 2));
  }
  milestoneClear(ctx);
  for (int c = 2; c < ctx.width - 1; c += 3) {
    for (int row = 1; row < ctx.height - 1; row += 2) {
      viewsDrawMiniEye(ctx, c, row, true);
    }
  }
  milestoneFrameShow(ctx, 180, 0);
}

static void viewsDrawColossalEyes(MilestoneCtx &ctx, int eyeScale, int pupilSize) {
  int spacing = max(1, 3 - eyeScale / 2);
  viewsDrawSingleEye(ctx, ctx.cx - spacing, ctx.cy, 2, true, 0, false, eyeScale,
                     pupilSize);
  viewsDrawSingleEye(ctx, ctx.cx + spacing, ctx.cy, 2, true, 0, false, eyeScale,
                     pupilSize);
}

static void viewsAnimEyeSingularity(MilestoneCtx &ctx) {
  for (int step = 14; step >= 0; step--) {
    milestoneClear(ctx);
    for (int c = 2; c < ctx.width - 1; c += 3) {
      for (int row = 1; row < ctx.height - 1; row += 2) {
        int dist = abs(c - ctx.cx) + abs(row - ctx.cy);
        if (dist >= step * 2 && dist <= step * 2 + 3) {
          viewsDrawMiniEye(ctx, c, row, step % 3 != 0);
        }
      }
    }
    int scale = 3 + (14 - step) / 2;
    viewsDrawColossalEyes(ctx, min(6, (int)scale), min(4, scale - 1));
    milestoneFrameShow(ctx, 34, min(7, (14 - step) / 2));
  }
  milestoneClear(ctx);
  viewsDrawColossalEyes(ctx, 6, 4);
  milestoneFrameShow(ctx, 220, 0);
}

static void viewsAnimCornerSupernova(MilestoneCtx &ctx) {
  const int sites[5][2] = {{1, 0},
                           {ctx.width - 2, 0},
                           {1, ctx.height - 1},
                           {ctx.width - 2, ctx.height - 1},
                           {ctx.cx, ctx.cy}};
  for (int r = 0; r <= 16; r++) {
    milestoneClear(ctx);
    for (int s = 0; s < 5; s++) {
      viewsDrawNova(ctx, sites[s][0], sites[s][1], r);
      if (r > 3) {
        milestoneDrawExplosionRing(ctx, sites[s][0], sites[s][1], r - 3);
      }
      if (r > 8) {
        milestoneDrawFireworkBurst(ctx, sites[s][0], sites[s][1], r, 8, 0.38f);
      }
    }
    milestoneFrameShow(ctx, r < 8 ? 24 : 32, min(8, r / 2));
  }
}

static void viewsAnimMeteorShower(MilestoneCtx &ctx, int frames) {
  const int meteorCount = 10;
  int meteorCol[10];
  int meteorRow[10];
  for (int i = 0; i < meteorCount; i++) {
    meteorCol[i] = 2 + random(max(1, ctx.width - 4));
    meteorRow[i] = -random(8);
  }

  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
    viewsDrawColossalEyes(ctx, 4, 3);
    for (int i = 0; i < meteorCount; i++) {
      meteorRow[i] += 1 + (i % 3);
      if (meteorRow[i] > ctx.height + 3) {
        meteorRow[i] = -random(6);
        meteorCol[i] = 1 + random(max(1, ctx.width - 2));
      }
      for (int trail = 0; trail < 5; trail++) {
        int row = meteorRow[i] - trail;
        if (row >= 0 && row < ctx.height) {
          ctx.matrix->setPoint(row, ctx.colStart + meteorCol[i], trail < 2);
        }
      }
      if (meteorRow[i] >= ctx.height - 2 && meteorRow[i] <= ctx.height) {
        viewsDrawNova(ctx, meteorCol[i], ctx.height - 1, 3);
        viewsDrawShrapnel(ctx, meteorCol[i], ctx.height - 1, 4);
      }
    }
    milestoneFrameShow(ctx, 26, min(6, frame / 4));
  }
}

static void viewsAnimHypernovaDetonation(MilestoneCtx &ctx, int ox, int oy) {
  for (int r = 0; r <= 12; r++) {
    milestoneClear(ctx);
    viewsDrawNova(ctx, ox, oy, r);
    if (r > 2) {
      viewsDrawNova(ctx, ox, oy, r - 2);
    }
    if (r > 5) {
      milestoneDrawExplosionRing(ctx, ox, oy, r - 5);
    }
    milestoneDrawFireworkBurst(ctx, ox, oy, r, 14, 0.42f);
    viewsDrawShrapnel(ctx, ox, oy, min(12, (int)r));
    milestoneFrameShow(ctx, r < 6 ? 20 : 28, min(7, r / 2));
  }
  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 55, 0);
  milestoneClear(ctx);
  milestoneFrameShow(ctx, 35, 0);
}

static void viewsDrawGiantEyes(MilestoneCtx &ctx, int openThroughDy,
                               int pupilOffsetX, bool blinkClosed, int eyeScale,
                               int pupilSize) {
  int spacing = max(2, VIEWS_EYE_SPACING - eyeScale);
  viewsDrawSingleEye(ctx, ctx.cx - spacing, ctx.cy, openThroughDy, true,
                     pupilOffsetX, blinkClosed, eyeScale, pupilSize);
  viewsDrawSingleEye(ctx, ctx.cx + spacing, ctx.cy, openThroughDy, true,
                     pupilOffsetX, blinkClosed, eyeScale, pupilSize);
}

static void viewsAnimViewDeluge(MilestoneCtx &ctx, int frames) {
  uint8_t dropHead[32];
  uint8_t dropSpeed[32];
  for (int c = 0; c < ctx.width; c++) {
    dropHead[c] = random(ctx.height + 4);
    dropSpeed[c] = 1 + (c % 2);
  }

  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, 2, 2);
    for (int c = 0; c < ctx.width; c++) {
      if (frame % dropSpeed[c] == 0) {
        dropHead[c] = (dropHead[c] + 1) % (ctx.height + 6);
      }
      for (int trail = 0; trail < 6; trail++) {
        int row = (int)dropHead[c] - trail;
        if (row >= 0 && row < ctx.height) {
          ctx.matrix->setPoint(row, ctx.colStart + c, trail < 3);
        }
      }
    }
    milestoneFrameShow(ctx, 28, min(7, frame / 4));
  }
}

static void viewsAnimRadialPulse(MilestoneCtx &ctx, int rings) {
  for (int ring = 0; ring < rings; ring++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      for (int row = 0; row < ctx.height; row++) {
        int d = abs(c - ctx.cx) + abs(row - ctx.cy);
        if (d == ring || d == ring + 1) {
          ctx.matrix->setPoint(row, ctx.colStart + c, true);
        }
      }
    }
    if (ring % 3 == 0) {
      viewsDrawGiantEyes(ctx, 2, 0, false, 2, 2);
    }
    milestoneFrameShow(ctx, 28, min(7, ring / 2));
  }
}

static void viewsAnimPlasmaStorm(MilestoneCtx &ctx, int frames) {
  const int sitesX[] = {3, 10, 16, 22, 28, ctx.width / 2};
  const int sitesY[] = {1, 5, 2, 6, 3, ctx.cy};
  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, 2, 2);
    for (int s = 0; s < 6; s++) {
      if ((frame + s) % 3 == 0) {
        int r = (frame + s) % 6 + 1;
        viewsDrawNova(ctx, sitesX[s], sitesY[s], r);
        milestoneDrawFireworkBurst(ctx, sitesX[s], sitesY[s], frame + s, 6,
                                   0.4f);
      }
    }
    milestoneFrameShow(ctx, 30, min(7, frame / 3));
  }
}

// +10K views — two eyes open, glance, blink, ripple, flash.
static void viewsAnimTier10K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (int dy = -2; dy <= 2; dy++) {
    milestoneClear(ctx);
    viewsDrawEyesFrame(ctx, dy, false, 0, false);
    milestoneFrameShow(ctx, 48, min(7, (dy + 2) * 2));
  }

  milestoneClear(ctx);
  viewsDrawEyesFrame(ctx, 2, true, 0, false);
  milestoneFrameShow(ctx, 180, 0);

  const int glanceOffsets[] = {-2, 0, 2, 0};
  for (int i = 0; i < 4; i++) {
    milestoneClear(ctx);
    viewsDrawEyesFrame(ctx, 2, true, glanceOffsets[i], false);
    milestoneFrameShow(ctx, 62, 0);
  }

  milestoneClear(ctx);
  viewsDrawEyesFrame(ctx, 2, true, 0, true);
  milestoneFrameShow(ctx, 95, 0);

  milestoneClear(ctx);
  viewsDrawEyesFrame(ctx, 2, true, 0, false);
  milestoneFrameShow(ctx, 130, 0);

  for (int frame = 0; frame < 10; frame++) {
    milestoneClear(ctx);
    milestoneDrawFireworkBurst(ctx, ctx.cx - VIEWS_EYE_SPACING, ctx.cy, frame, 8,
                               0.45f);
    milestoneDrawFireworkBurst(ctx, ctx.cx + VIEWS_EYE_SPACING, ctx.cy, frame, 8,
                               0.45f);
    milestoneFrameShow(ctx, 38, min(6, frame / 2));
  }

  for (int band = 0; band < 4; band++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      for (int row = ctx.cy - band; row <= ctx.cy + band; row++) {
        if (row < 0 || row >= ctx.height) {
          continue;
        }
        bool edge = (row == ctx.cy - band || row == ctx.cy + band);
        ctx.matrix->setPoint(row, ctx.colStart + c, edge || band == 0);
      }
    }
    milestoneFrameShow(ctx, 52, min(6, band * 2));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 90, 0);
  milestoneEffectEnd(ctx);
}

// +100K views — dilated eyes, wide look-around, double blink, view rain, burst.
static void viewsAnimTier100K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  for (int dy = -2; dy <= 2; dy++) {
    milestoneClear(ctx);
    viewsDrawEyesFrame(ctx, dy, false, 0, false);
    milestoneFrameShow(ctx, 44, min(7, (dy + 2) * 2));
  }

  milestoneClear(ctx);
  viewsDrawEyes(ctx, 2, true, 0, false, 0, 1);
  milestoneFrameShow(ctx, 160, 0);

  for (int pupilSize = 1; pupilSize <= 2; pupilSize++) {
    milestoneClear(ctx);
    viewsDrawEyes(ctx, 2, true, 0, false, 0, pupilSize);
    milestoneFrameShow(ctx, 70, 0);
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    viewsDrawEyes(ctx, 2, true, 0, false, pulse % 2, 2);
    milestoneFrameShow(ctx, 58, pulse % 2 == 0 ? 3 : 0);
  }

  const int lookOffsets[] = {-3, -2, -1, 0, 1, 2, 3, 2, 1, 0};
  for (int i = 0; i < 10; i++) {
    milestoneClear(ctx);
    viewsDrawEyes(ctx, 2, true, lookOffsets[i], false, 0, 2);
    milestoneFrameShow(ctx, 55, 0);
  }

  for (int blink = 0; blink < 2; blink++) {
    milestoneClear(ctx);
    viewsDrawEyes(ctx, 2, true, 0, true, 0, 2);
    milestoneFrameShow(ctx, 85, 0);
    milestoneClear(ctx);
    viewsDrawEyes(ctx, 2, true, 0, false, 0, 2);
    milestoneFrameShow(ctx, 110, 0);
  }

  uint8_t dropHead[32];
  uint8_t dropSpeed[32];
  for (int c = 0; c < ctx.width; c++) {
    dropHead[c] = random(ctx.height + 2);
    dropSpeed[c] = 1 + (c % 3);
  }

  for (int frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    viewsDrawEyes(ctx, 2, true, 0, false, 0, 2);
    for (int c = 0; c < ctx.width; c++) {
      if (frame % dropSpeed[c] == c % dropSpeed[c]) {
        dropHead[c] = (dropHead[c] + 1) % (ctx.height + 4);
      }
      for (int trail = 0; trail < 4; trail++) {
        int row = (int)dropHead[c] - trail;
        if (row >= 0 && row < ctx.height) {
          ctx.matrix->setPoint(row, ctx.colStart + c, trail < 2);
        }
      }
    }
    milestoneFrameShow(ctx, 36, min(8, frame / 3));
  }

  for (int frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    milestoneDrawFireworkBurst(ctx, ctx.cx - VIEWS_EYE_SPACING, ctx.cy, frame, 10,
                               0.58f);
    milestoneDrawFireworkBurst(ctx, ctx.cx + VIEWS_EYE_SPACING, ctx.cy, frame, 10,
                               0.58f);
    milestoneFrameShow(ctx, 40, min(6, frame / 2));
  }

  for (int band = 0; band < 5; band++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      for (int row = ctx.cy - band; row <= ctx.cy + band; row++) {
        if (row < 0 || row >= ctx.height) {
          continue;
        }
        bool edge = (row == ctx.cy - band || row == ctx.cy + band);
        ctx.matrix->setPoint(row, ctx.colStart + c, edge || band == 0);
      }
    }
    milestoneFrameShow(ctx, 48, min(6, band * 2));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 85, 0);
  milestoneEffectEnd(ctx);
}

// +1M views — crowd eyes converge, giant stare, view deluge, blasts, finale.
static void viewsAnimTier1M(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  milestoneAnimSlowRingBuild(ctx, 12, 50);
  delay(90);

  for (int reveal = 0; reveal < 15; reveal++) {
    milestoneClear(ctx);
    viewsDrawCrowdEyes(ctx, reveal, false);
    milestoneFrameShow(ctx, 38, min(9, reveal / 2));
  }

  milestoneClear(ctx);
  viewsDrawCrowdEyes(ctx, 14, false);
  milestoneFrameShow(ctx, 220, 0);

  for (int blink = 0; blink < 2; blink++) {
    milestoneClear(ctx);
    viewsDrawCrowdEyes(ctx, 14, true);
    milestoneFrameShow(ctx, 75, 0);
    milestoneClear(ctx);
    viewsDrawCrowdEyes(ctx, 14, false);
    milestoneFrameShow(ctx, 95, 0);
  }

  for (int fade = 14; fade >= 0; fade--) {
    milestoneClear(ctx);
    viewsDrawCrowdEyes(ctx, fade, false);
    if (fade <= 4) {
      viewsDrawGiantEyes(ctx, min(2, 4 - fade), 0, false, 4 - fade, 1);
    }
    milestoneFrameShow(ctx, 42, min(7, (14 - fade) / 2));
  }

  for (int scale = 1; scale <= 3; scale++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, scale, min(3, scale + 1));
    milestoneFrameShow(ctx, 72, min(5, (int)scale));
  }

  milestoneClear(ctx);
  viewsDrawGiantEyes(ctx, 2, 0, false, 3, 3);
  milestoneFrameShow(ctx, 280, 0);

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, 3, 3);
    milestoneFrameShow(ctx, pulse == 2 ? 120 : 90, 0);
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, true, 3, 3);
    milestoneFrameShow(ctx, 70, 0);
  }

  milestoneClear(ctx);
  viewsDrawGiantEyes(ctx, 2, 0, false, 3, 3);
  milestoneFrameShow(ctx, 160, 0);

  for (int pulse = 0; pulse < 4; pulse++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, 3 - pulse % 2, 3);
    milestoneDrawExplosionRing(ctx, ctx.cx - 2, ctx.cy, pulse + 2);
    milestoneDrawExplosionRing(ctx, ctx.cx + 2, ctx.cy, pulse + 2);
    milestoneFrameShow(ctx, 55, pulse % 2 == 0 ? 4 : 0);
  }

  viewsAnimViewDeluge(ctx, 20);

  for (int ring = 0; ring < 8; ring++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, 2, 2);
    milestoneDrawExplosionRing(ctx, ctx.cx - 2, ctx.cy, ring);
    milestoneDrawExplosionRing(ctx, ctx.cx + 2, ctx.cy, ring);
    if (ring > 2) {
      milestoneDrawExplosionRing(ctx, ctx.cx - 2, ctx.cy, ring - 3);
      milestoneDrawExplosionRing(ctx, ctx.cx + 2, ctx.cy, ring - 3);
    }
    milestoneFrameShow(ctx, ring < 5 ? 26 : 38, min(8, (int)ring));
  }

  const int barrageX[8] = {2, ctx.width / 5, 2 * ctx.width / 5, ctx.width / 2,
                           3 * ctx.width / 5, 4 * ctx.width / 5, ctx.width - 3,
                           ctx.width / 2};
  const int barrageY[8] = {0, 2, 5, 1, 5, 2, 0, 7};
  milestoneAnimFireworkBarrage(ctx, barrageX, barrageY, 8, 44, 2, 14, 0.78f, 32);

  const int chainX[7] = {1, ctx.width / 6, ctx.width / 3, ctx.width / 2,
                         2 * ctx.width / 3, 5 * ctx.width / 6, ctx.width - 2};
  milestoneAnimChainExplosions(ctx, chainX, 7, 10, 24, 28);

  milestoneAnimShockwaveFlashes(ctx, 5, 52, 40);

  for (int frame = 0; frame < 14; frame++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, 2, 2);
    milestoneDrawFireworkBurst(ctx, ctx.cx - 2, ctx.cy, frame, 12, 0.68f);
    milestoneDrawFireworkBurst(ctx, ctx.cx + 2, ctx.cy, frame, 12, 0.68f);
    milestoneFrameShow(ctx, 36, min(5, frame / 3));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 120, 0);

  milestoneAnimEmberFallout(ctx, 24, 26);
  milestoneAnimVictoryPulses(ctx, 5, 100, 60);
  milestoneAnimFinalFlashes(ctx, 5, 55, 30, 140);
  milestoneAnimInwardCollapse(ctx, 32);
  milestoneEffectEnd(ctx);
}

// +10M views — singularity open, nova blasts, grid detonation, plasma storm, finale.
static void viewsAnimTier10M(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  viewsAnimGravityWell(ctx);
  viewsAnimEyeShockwave(ctx);
  delay(50);

  viewsAnimReticleLock(ctx, ctx.cx, ctx.cy);

  for (int flash = 0; flash < 4; flash++) {
    milestoneClear(ctx);
    if (flash % 2 == 0) {
      viewsDrawCrowdEyes(ctx, 14, false);
    }
    viewsDrawReticle(ctx, ctx.cx, ctx.cy, 4);
    milestoneFrameShow(ctx, flash < 4 ? 45 : 70, flash % 2 == 0 ? 0 : 2);
  }

  milestoneClear(ctx);
  viewsDrawGiantEyes(ctx, 2, 0, false, 3, 3);
  viewsDrawReticle(ctx, ctx.cx - 2, ctx.cy, 3);
  viewsDrawReticle(ctx, ctx.cx + 2, ctx.cy, 3);
  milestoneFrameShow(ctx, 200, 0);

  viewsAnimNovaDetonation(ctx, ctx.cx - 2, ctx.cy, 9);
  viewsAnimNovaDetonation(ctx, ctx.cx + 2, ctx.cy, 9);

  milestoneClear(ctx);
  viewsDrawGiantEyes(ctx, 2, 0, false, 3, 3);
  milestoneFrameShow(ctx, 80, 0);

  viewsAnimShrapnelBurst(ctx, ctx.cx, ctx.cy);
  delay(50);

  viewsAnimGridDetonation(ctx);
  delay(40);

  viewsAnimViewDeluge(ctx, 18);

  viewsAnimPlasmaStorm(ctx, 16);

  const int megaX[10] = {1, ctx.width / 6, ctx.width / 3, ctx.width / 2,
                         2 * ctx.width / 3, 5 * ctx.width / 6, ctx.width - 2,
                         4, ctx.width - 5, ctx.width / 2};
  const int megaY[10] = {0, 2, 5, 1, 5, 2, 0, 7, 6, 7};
  milestoneAnimFireworkBarrage(ctx, megaX, megaY, 10, 46, 2, 14, 0.82f, 30);

  const int blastSites[5] = {2, ctx.width / 4, ctx.width / 2, 3 * ctx.width / 4,
                             ctx.width - 3};
  for (int site = 0; site < 5; site++) {
    for (int ring = 0; ring <= 10; ring++) {
      milestoneClear(ctx);
      viewsDrawGiantEyes(ctx, 2, 0, false, 2, 2);
      milestoneDrawExplosionRing(ctx, blastSites[site], ctx.cy, ring);
      if (ring > 3) {
        milestoneDrawExplosionRing(ctx, blastSites[site], ctx.cy, ring - 3);
        viewsDrawNova(ctx, blastSites[site], ctx.cy, ring - 5);
      }
      milestoneFrameShow(ctx, ring < 5 ? 22 : 30, min(8, (int)ring));
    }
    delay(35);
  }

  viewsAnimRadialPulse(ctx, 12);

  for (int frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    viewsDrawGiantEyes(ctx, 2, 0, false, 2, 2);
    milestoneDrawFireworkBurst(ctx, ctx.cx - 2, ctx.cy, frame, 14, 0.75f);
    milestoneDrawFireworkBurst(ctx, ctx.cx + 2, ctx.cy, frame, 14, 0.75f);
    viewsDrawShrapnel(ctx, ctx.cx, ctx.cy, min(10, frame / 2 + 2));
    milestoneFrameShow(ctx, 34, min(5, frame / 3));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 140, 0);

  milestoneAnimShockwaveFlashes(ctx, 7, 50, 38);
  milestoneAnimEmberFallout(ctx, 26, 26);
  milestoneAnimVictoryPulses(ctx, 6, 110, 65);
  milestoneAnimFinalFlashes(ctx, 6, 58, 32, 150);
  milestoneAnimInwardCollapse(ctx, 30);
  milestoneEffectEnd(ctx);
}

// +100M views — twin singularity, omniscience, corner supernovas, meteor rain, hypernova.
static void viewsAnimTier100M(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  viewsAnimTwinGravityWell(ctx);
  viewsAnimEyeShockwave(ctx);
  delay(40);

  viewsAnimOmniscience(ctx);
  viewsAnimEyeSingularity(ctx);

  for (int blink = 0; blink < 2; blink++) {
    milestoneClear(ctx);
    viewsDrawColossalEyes(ctx, 6, 4);
    milestoneFrameShow(ctx, 100, 0);
    milestoneClear(ctx);
    viewsDrawColossalEyes(ctx, 6, 4);
    for (int col = ctx.cx - 10; col <= ctx.cx + 10; col++) {
      if (col >= 0 && col < ctx.width) {
        ctx.matrix->setPoint(ctx.cy, ctx.colStart + col, true);
      }
    }
    milestoneFrameShow(ctx, 65, 0);
  }

  milestoneClear(ctx);
  viewsDrawColossalEyes(ctx, 6, 4);
  milestoneFrameShow(ctx, 160, 0);

  viewsAnimCornerSupernova(ctx);
  delay(50);

  viewsAnimMeteorShower(ctx, 22);
  viewsAnimShrapnelBurst(ctx, ctx.cx, ctx.cy);

  viewsAnimHypernovaDetonation(ctx, ctx.cx - 1, ctx.cy);
  viewsAnimHypernovaDetonation(ctx, ctx.cx + 1, ctx.cy);

  milestoneClear(ctx);
  viewsDrawColossalEyes(ctx, 5, 4);
  milestoneFrameShow(ctx, 90, 0);

  viewsAnimGridDetonation(ctx);
  viewsAnimViewDeluge(ctx, 22);
  viewsAnimPlasmaStorm(ctx, 20);

  const int apexX[12] = {0, 2, ctx.width / 7, ctx.width / 4, ctx.width / 3,
                         ctx.width / 2, 2 * ctx.width / 3, 3 * ctx.width / 4,
                         6 * ctx.width / 7, ctx.width - 3, ctx.width - 1,
                         ctx.width / 2};
  const int apexY[12] = {0, 7, 2, 5, 3, 0, 3, 5, 2, 7, 0, 7};
  milestoneAnimFireworkBarrage(ctx, apexX, apexY, 12, 50, 1, 16, 0.85f, 28);

  const int trinityX[3] = {ctx.width / 6, ctx.width / 2, 5 * ctx.width / 6};
  const int trinityY[3] = {ctx.cy, 1, ctx.cy};
  milestoneAnimMultiSiteBlast(ctx, trinityX, trinityY, 3, 13, 20, 32, true);

  const int blastSites[7] = {1, ctx.width / 5, 2 * ctx.width / 5, ctx.width / 2,
                             3 * ctx.width / 5, 4 * ctx.width / 5, ctx.width - 2};
  for (int ring = 0; ring <= 12; ring++) {
    milestoneClear(ctx);
    viewsDrawColossalEyes(ctx, 4, 3);
    for (int s = 0; s < 7; s++) {
      milestoneDrawExplosionRing(ctx, blastSites[s], ctx.cy, ring);
      if (ring > 4) {
        viewsDrawNova(ctx, blastSites[s], ctx.cy, ring - 5);
      }
    }
    milestoneFrameShow(ctx, ring < 6 ? 20 : 28, min(8, ring / 2));
  }

  milestoneAnimMegablast(ctx, 14, 24, 36, true);
  viewsAnimRadialPulse(ctx, 14);

  for (int frame = 0; frame < 20; frame++) {
    milestoneClear(ctx);
    viewsDrawColossalEyes(ctx, 4, 3);
    milestoneDrawFireworkBurst(ctx, ctx.cx - 1, ctx.cy, frame, 16, 0.8f);
    milestoneDrawFireworkBurst(ctx, ctx.cx + 1, ctx.cy, frame, 16, 0.8f);
    viewsDrawShrapnel(ctx, ctx.cx, ctx.cy, min(12, frame / 2 + 3));
    milestoneFrameShow(ctx, 32, min(5, frame / 3));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 160, 0);

  milestoneAnimShockwaveFlashes(ctx, 8, 48, 36);
  milestoneAnimEmberFallout(ctx, 28, 24);
  milestoneAnimVictoryPulses(ctx, 7, 105, 60);
  milestoneAnimFinalFlashes(ctx, 7, 55, 28, 160);
  milestoneAnimInwardCollapse(ctx, 28);
  milestoneEffectEnd(ctx);
}

void runViewsMilestoneAnimation(MD_Parola &display, MilestoneTier tier) {
  switch (tier) {
  case MilestoneTier::Tier100:
  case MilestoneTier::Tier1K:
    // Views milestones start at +10K; lower tiers use the same boundaries as subs/hours
    // but do not play an animation.
    return;
  case MilestoneTier::Tier10K:
    runViewsTier(display, viewsAnimTier10K, tier, 850, 1000);
    break;
  case MilestoneTier::Tier100K:
    runViewsTier(display, viewsAnimTier100K, tier, 950, 1100);
    break;
  case MilestoneTier::Tier1M:
    runViewsTier(display, viewsAnimTier1M, tier, 1500, 1900);
    break;
  case MilestoneTier::Tier10M:
    runViewsTier(display, viewsAnimTier10M, tier, 1600, 2000);
    break;
  case MilestoneTier::Tier100M:
    runViewsTier(display, viewsAnimTier100M, tier, 1800, 2200);
    break;
  default:
    break;
  }
}
