#include "milestone_subs_animations.h"

#include "milestone_helpers.h"

#include <MD_MAX72xx.h>

static void runSubsTier(MD_Parola &display, void (*animation)(MD_Parola &),
                        MilestoneTier tier, uint16_t statHoldMs,
                        uint16_t labelHoldMs) {
  animation(display);
  char increaseLabel[12];
  milestoneGetIncreaseLabel(tier, increaseLabel, sizeof(increaseLabel));
  milestoneShowCenteredText(display, "SUBS", statHoldMs);
  milestoneShowCenteredText(display, increaseLabel, labelHoldMs);
  milestoneCleanupDisplay(display);
}

// +100 — upward bars, diamond ping, flash.
static void subsAnimTier100(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  uint8_t barHeight[32];
  for (int c = 0; c < ctx.width; c++) {
    barHeight[c] = 1 + (c % 4);
  }

  for (int frame = 0; frame < 12; frame++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      uint8_t target = 2 + (frame * 3 + c * 2) % (ctx.height - 1);
      if (barHeight[c] < target) {
        barHeight[c]++;
      } else if (barHeight[c] > 2) {
        barHeight[c]--;
      }
      for (int row = ctx.height - (int)barHeight[c]; row < ctx.height; row++) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
    milestoneFrameShow(ctx, 32, 0);
  }

  delay(60);

  for (int ring = 1; ring <= 5; ring++) {
    milestoneClear(ctx);
    milestoneDrawDiamondRing(ctx, ring);
    milestoneFrameShow(ctx, 45, min(6, ring * 2));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 80, 0);
  milestoneEffectEnd(ctx);
}

// +1K — ripple surge, double diamond, spark rain, fade.
static void subsAnimTier1K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx);

  milestoneAnimRippleBars(ctx, 20, 36);
  delay(80);
  milestoneAnimDoubleDiamondPulse(ctx, 48);
  milestoneAnimSparkRain(ctx, 16, 38);

  for (int frame = 0; frame < 6; frame++) {
    milestoneClear(ctx);
    if (frame < 2) {
      milestoneFillAll(ctx);
    }
    milestoneFrameShow(ctx, 55, max(0, 13 - frame * 3));
  }

  milestoneEffectEnd(ctx);
}

// +10K — slow build, tsunami, counter climb, falling comets, fireworks, pulses, collapse.
static void subsAnimTier10K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx);

  milestoneAnimSlowRingBuild(ctx, 10, 58);
  delay(100);
  milestoneAnimTripleTsunami(ctx, 22, 42, 90);
  milestoneAnimCounterClimb(ctx, 32);
  milestoneAnimCounterShimmer(ctx, 8, 35);
  milestoneAnimFallingComets(ctx, 2, 24);

  const int burstX[4] = {ctx.width / 4, 3 * ctx.width / 4, ctx.width / 4,
                         3 * ctx.width / 4};
  const int burstY[4] = {2, 2, 5, 5};
  milestoneAnimFireworkBarrage(ctx, burstX, burstY, 4, 24, 3, 8, 0.55f, 40);

  milestoneAnimVictoryPulses(ctx, 3, 140, 90);
  milestoneAnimInwardCollapse(ctx, 45);
  milestoneEffectEnd(ctx);
}

// +100K — rockets, megablast, barrage, shockwaves, chain blasts, finale, embers.
static void subsAnimTier100K(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx);

  milestoneAnimRockets(ctx, 20, 34);
  delay(60);
  milestoneAnimMegablast(ctx, 12, 28, 42, true);

  const int barrageX[8] = {ctx.width / 8,     ctx.width / 4,     3 * ctx.width / 8,
                           5 * ctx.width / 8, 3 * ctx.width / 4, 7 * ctx.width / 8,
                           ctx.width / 3,     2 * ctx.width / 3};
  const int barrageY[8] = {2, 4, 3, 5, 2, 4, 5, 3};
  milestoneAnimFireworkBarrage(ctx, barrageX, barrageY, 8, 32, 2, 10, 0.62f, 38);

  milestoneAnimShockwaveFlashes(ctx, 3, 55, 45);

  const int chainX[5] = {ctx.width / 10, ctx.width / 4, ctx.width / 2,
                         3 * ctx.width / 4, 9 * ctx.width / 10};
  milestoneAnimChainExplosions(ctx, chainX, 5, 7, 30, 40);

  const int finaleX[9] = {2,
                          ctx.width / 4,
                          ctx.width / 2,
                          3 * ctx.width / 4,
                          ctx.width - 3,
                          4,
                          ctx.width - 5,
                          ctx.width / 3,
                          2 * ctx.width / 3};
  const int finaleY[9] = {1, 2, 0, 2, 1, 6, 6, 7, 7};
  milestoneAnimFireworkBarrage(ctx, finaleX, finaleY, 9, 36, 1, 12, 0.72f, 40);

  milestoneAnimEmberFallout(ctx, 22, 34);
  milestoneAnimFinalFlashes(ctx, 3, 70, 45, 120);
  milestoneAnimInwardCollapse(ctx, 40);
  milestoneEffectEnd(ctx);
}

// +1M — million-scale: edge surge, trinity blast, firestorm barrages, chain
// reaction, shockwave storm, falling comets, supernova finale.
static void subsAnimTier1M(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx);

  milestoneAnimSlowRingBuild(ctx, 14, 54);
  delay(120);
  milestoneAnimDualEdgeClimb(ctx, 28);
  milestoneAnimCounterShimmer(ctx, 10, 32);
  delay(80);

  milestoneAnimRockets(ctx, 32, 30);
  delay(70);

  const int trinityX[3] = {ctx.width / 5, ctx.width / 2, 4 * ctx.width / 5};
  const int trinityY[3] = {ctx.cy, ctx.cy - 1, ctx.cy};
  milestoneAnimMultiSiteBlast(ctx, trinityX, trinityY, 3, 12, 22, 34, true);
  milestoneAnimScreenShake(ctx, 10, 20);

  const int wave1X[10] = {1, ctx.width / 7, ctx.width / 4, 3 * ctx.width / 7,
                            ctx.width / 2, 4 * ctx.width / 7, 3 * ctx.width / 4,
                            6 * ctx.width / 7, ctx.width - 2, ctx.width / 2};
  const int wave1Y[10] = {0, 3, 6, 2, 1, 2, 6, 3, 0, 7};
  milestoneAnimFireworkBarrage(ctx, wave1X, wave1Y, 10, 42, 2, 12, 0.72f, 34);

  const int chainForward[6] = {ctx.width / 12, ctx.width / 4, ctx.width / 2,
                               3 * ctx.width / 4, 11 * ctx.width / 12,
                               ctx.width / 2};
  milestoneAnimChainExplosions(ctx, chainForward, 6, 9, 26, 32);

  const int chainReverse[6] = {11 * ctx.width / 12, 3 * ctx.width / 4,
                               ctx.width / 2, ctx.width / 4, ctx.width / 12,
                               ctx.width / 2};
  milestoneAnimChainExplosions(ctx, chainReverse, 6, 9, 26, 32);

  milestoneAnimShockwaveFlashes(ctx, 7, 48, 38);
  milestoneAnimFallingComets(ctx, 3, 22);

  const int wave2X[13] = {2, ctx.width / 8, ctx.width / 5, 2 * ctx.width / 5,
                          ctx.width / 2, 3 * ctx.width / 5, 4 * ctx.width / 5,
                          7 * ctx.width / 8, ctx.width - 3, 4, ctx.width - 5,
                          ctx.width / 3, 2 * ctx.width / 3};
  const int wave2Y[13] = {0, 2, 5, 3, 0, 3, 5, 2, 0, 7, 6, 7, 1};
  milestoneAnimFireworkBarrage(ctx, wave2X, wave2Y, 13, 50, 1, 14, 0.82f, 30);

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 130, 0);
  milestoneAnimScreenShake(ctx, 8, 18);

  milestoneAnimEmberFallout(ctx, 32, 28);
  milestoneAnimVictoryPulses(ctx, 6, 110, 65);
  milestoneAnimFinalFlashes(ctx, 6, 58, 32, 150);
  milestoneAnimInwardCollapse(ctx, 34);
  milestoneEffectEnd(ctx);
}

// +10M — ultimate: full build, triple blasts, max fireworks, bidirectional chains.
static void subsAnimTier10M(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx);

  milestoneAnimSlowRingBuild(ctx, 12, 52);
  delay(120);
  milestoneAnimTripleTsunami(ctx, 24, 40, 80);
  milestoneAnimRockets(ctx, 28, 30);
  delay(80);

  const int blastSites[3] = {ctx.width / 4, ctx.width / 2, 3 * ctx.width / 4};
  for (int site = 0; site < 3; site++) {
    for (int ring = 0; ring <= 11; ring++) {
      milestoneClear(ctx);
      milestoneDrawExplosionRing(ctx, blastSites[site], ctx.cy, ring);
      if (ring > 2) {
        milestoneDrawExplosionRing(ctx, blastSites[site], ctx.cy, ring - 2);
      }
      milestoneFrameShow(ctx, ring < 6 ? 24 : 36, min(9, (int)ring));
    }
    if (site < 2) {
      delay(60);
    } else {
      milestoneClear(ctx);
      milestoneFillAll(ctx);
      milestoneFrameShow(ctx, 100, 0);
      milestoneClear(ctx);
      milestoneFrameShow(ctx, 50, 0);
    }
  }

  const int megaX[12] = {1, ctx.width / 8, ctx.width / 4, 3 * ctx.width / 8,
                         ctx.width / 2, 5 * ctx.width / 8, 3 * ctx.width / 4,
                         7 * ctx.width / 8, ctx.width - 2, 4, ctx.width - 5,
                         ctx.width / 2};
  const int megaY[12] = {0, 2, 4, 3, 1, 3, 5, 2, 0, 7, 6, 7};
  milestoneAnimFireworkBarrage(ctx, megaX, megaY, 12, 44, 2, 12, 0.78f, 34);

  milestoneAnimShockwaveFlashes(ctx, 6, 48, 38);

  const int chainForward[7] = {ctx.width / 14, ctx.width / 6, ctx.width / 3,
                               ctx.width / 2, 2 * ctx.width / 3, 5 * ctx.width / 6,
                               13 * ctx.width / 14};
  milestoneAnimChainExplosions(ctx, chainForward, 7, 8, 26, 30);

  const int chainReverse[7] = {13 * ctx.width / 14, 5 * ctx.width / 6,
                               2 * ctx.width / 3, ctx.width / 2,
                               ctx.width / 3, ctx.width / 6, ctx.width / 14};
  milestoneAnimChainExplosions(ctx, chainReverse, 7, 8, 26, 30);

  const int finaleX[13] = {1, 3, ctx.width / 5, 2 * ctx.width / 5, ctx.width / 2,
                           3 * ctx.width / 5, 4 * ctx.width / 5, ctx.width - 4,
                           ctx.width - 2, ctx.width / 4, 3 * ctx.width / 4,
                           ctx.width / 3, 2 * ctx.width / 3};
  const int finaleY[13] = {0, 7, 2, 4, 0, 4, 2, 7, 0, 6, 6, 7, 1};
  milestoneAnimFireworkBarrage(ctx, finaleX, finaleY, 13, 48, 1, 14, 0.8f, 32);

  milestoneAnimEmberFallout(ctx, 30, 30);
  milestoneAnimVictoryPulses(ctx, 5, 120, 70);
  milestoneAnimFinalFlashes(ctx, 5, 60, 35, 140);
  milestoneAnimInwardCollapse(ctx, 36);
  milestoneEffectEnd(ctx);
}

void runSubsMilestoneAnimation(MD_Parola &display, MilestoneTier tier) {
  switch (tier) {
  case MilestoneTier::Tier100:
    runSubsTier(display, subsAnimTier100, tier, 750, 900);
    break;
  case MilestoneTier::Tier1K:
    runSubsTier(display, subsAnimTier1K, tier, 850, 1000);
    break;
  case MilestoneTier::Tier10K:
    runSubsTier(display, subsAnimTier10K, tier, 1100, 1300);
    break;
  case MilestoneTier::Tier100K:
    runSubsTier(display, subsAnimTier100K, tier, 1200, 1500);
    break;
  case MilestoneTier::Tier1M:
    runSubsTier(display, subsAnimTier1M, tier, 1500, 1900);
    break;
  case MilestoneTier::Tier10M:
    runSubsTier(display, subsAnimTier10M, tier, 1400, 1800);
    break;
  default:
    break;
  }
}
