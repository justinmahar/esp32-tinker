#include "milestone_animations.h"

#include "firmware_config.h"
#include "milestone_hours_animations.h"
#include "milestone_helpers.h"
#include "milestone_subs_animations.h"
#include "milestone_views_animations.h"

#include <MD_MAX72xx.h>
#include <stdio.h>

void milestoneShowCenteredText(MD_Parola &display, const char *text,
                               uint16_t holdMs) {
  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setTextAlignment(PA_CENTER);
  display.print(text);
  delay(holdMs);
}

void milestoneRevealLabels(MD_Parola &display, const char *statLabel,
                           const char *increaseLabel) {
  milestoneShowCenteredText(display, statLabel, 900);
  milestoneShowCenteredText(display, increaseLabel, 1200);
}

void milestoneCleanupDisplay(MD_Parola &display) {
  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setIntensity(displayBaselineIntensity());
}

void milestoneGetIncreaseLabel(MilestoneTier tier, char *buffer, size_t size) {
  switch (tier) {
  case MilestoneTier::Tier100:
    snprintf(buffer, size, "+100!");
    break;
  case MilestoneTier::Tier1K:
    snprintf(buffer, size, "+1K!");
    break;
  case MilestoneTier::Tier10K:
    snprintf(buffer, size, "+10K!");
    break;
  case MilestoneTier::Tier100K:
    snprintf(buffer, size, "+100K!");
    break;
  case MilestoneTier::Tier1M:
    snprintf(buffer, size, "+1M!");
    break;
  case MilestoneTier::Tier10M:
    snprintf(buffer, size, "+10M!");
    break;
  case MilestoneTier::Tier100M:
    snprintf(buffer, size, "+100M!");
    break;
  }
}

static MilestoneTier tierFromAnimation(MilestoneAnimation animation) {
  if (animation == MilestoneAnimation::Views100M) {
    return MilestoneTier::Tier100M;
  }
  return static_cast<MilestoneTier>(static_cast<uint8_t>(animation) % 6);
}

static bool hasMilestoneAnimation(MilestoneAnimation animation) {
  return animation != MilestoneAnimation::Views100 &&
         animation != MilestoneAnimation::Views1K;
}

static void runMilestoneIntroAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  const int cornerX[4] = {0, ctx.width - 1, 0, ctx.width - 1};
  const int cornerY[4] = {0, 0, ctx.height - 1, ctx.height - 1};
  const int convergeSteps = 12;
  for (int frame = 0; frame <= convergeSteps; frame++) {
    milestoneClear(ctx);
    for (int dot = 0; dot < 4; dot++) {
      for (int trail = 0; trail < 4; trail++) {
        int t = max(0, frame - trail);
        int x = cornerX[dot] + (ctx.cx - cornerX[dot]) * t / convergeSteps;
        int y = cornerY[dot] + (ctx.cy - cornerY[dot]) * t / convergeSteps;
        if (x >= 0 && x < ctx.width && y >= 0 && y < ctx.height) {
          ctx.matrix->setPoint(y, ctx.colStart + x, trail < 2);
        }
      }
    }
    if (frame > 5) {
      milestoneDrawDiamondRing(ctx, frame - 5);
    }
    milestoneFrameShow(ctx, 36, min(7, frame / 2));
  }

  for (int ring = 0; ring <= 16; ring++) {
    milestoneClear(ctx);
    milestoneDrawExplosionRing(ctx, ctx.cx, ctx.cy, ring);
    if (ring > 2) {
      milestoneDrawExplosionRing(ctx, ctx.cx, ctx.cy, ring - 2);
    }
    if (ring > 4) {
      int reach = min(ctx.width / 2, ring);
      for (int len = 0; len <= reach; len++) {
        if (ctx.cx - len >= 0) {
          ctx.matrix->setPoint(ctx.cy, ctx.colStart + ctx.cx - len, true);
        }
        if (ctx.cx + len < ctx.width) {
          ctx.matrix->setPoint(ctx.cy, ctx.colStart + ctx.cx + len, true);
        }
        if (ctx.cy - len >= 0) {
          ctx.matrix->setPoint(ctx.cy - len, ctx.colStart + ctx.cx, true);
        }
        if (ctx.cy + len < ctx.height) {
          ctx.matrix->setPoint(ctx.cy + len, ctx.colStart + ctx.cx, true);
        }
      }
    }
    milestoneFrameShow(ctx, ring < 8 ? 24 : 34, min(6, ring / 2));
  }

  for (int frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      bool chase = ((c + frame) % 6) < 3;
      ctx.matrix->setPoint(0, ctx.colStart + c, chase);
      ctx.matrix->setPoint(ctx.height - 1, ctx.colStart + c, !chase);
    }
    for (int row = 1; row < ctx.height - 1; row++) {
      ctx.matrix->setPoint(row, ctx.colStart, ((row + frame) % 4) < 2);
      ctx.matrix->setPoint(row, ctx.colStart + ctx.width - 1,
                           ((row + frame + 2) % 4) < 2);
    }
    milestoneDrawDiamondRing(ctx, 2 + (frame % 4));
    milestoneFrameShow(ctx, 42, frame % 2 == 0 ? 4 : 0);
  }

  for (int pulse = 0; pulse < 3; pulse++) {
    milestoneClear(ctx);
    milestoneFillAll(ctx);
    milestoneFrameShow(ctx, pulse == 2 ? 110 : 60, 0);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, 38, 0);
  }
  milestoneEffectEnd(ctx);

  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setIntensity(animationDisplayIntensity(0));
  display.setTextAlignment(PA_LEFT);
  display.displayScroll("Milestone hit!", PA_LEFT, PA_SCROLL_LEFT, 55);
  while (!display.displayAnimate()) {
    delay(10);
  }
  delay(250);
}

void runMilestoneAnimation(MD_Parola &display, MilestoneAnimation animation) {
  if (hasMilestoneAnimation(animation) && !DEBUG_DISABLE_ANIMATION_INTROS) {
    runMilestoneIntroAnimation(display);
  }

  MilestoneTier tier = tierFromAnimation(animation);

  switch (animation) {
  case MilestoneAnimation::Subs100:
  case MilestoneAnimation::Subs1K:
  case MilestoneAnimation::Subs10K:
  case MilestoneAnimation::Subs100K:
  case MilestoneAnimation::Subs1M:
  case MilestoneAnimation::Subs10M:
    runSubsMilestoneAnimation(display, tier);
    break;

  case MilestoneAnimation::Views100:
  case MilestoneAnimation::Views1K:
  case MilestoneAnimation::Views10K:
  case MilestoneAnimation::Views100K:
  case MilestoneAnimation::Views1M:
  case MilestoneAnimation::Views10M:
  case MilestoneAnimation::Views100M:
    runViewsMilestoneAnimation(display, tier);
    break;

  case MilestoneAnimation::Hours100:
  case MilestoneAnimation::Hours1K:
  case MilestoneAnimation::Hours10K:
  case MilestoneAnimation::Hours100K:
  case MilestoneAnimation::Hours1M:
  case MilestoneAnimation::Hours10M:
    runHoursMilestoneAnimation(display, tier);
    break;
  }
}
