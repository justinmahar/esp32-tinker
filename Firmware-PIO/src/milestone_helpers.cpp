#include "milestone_helpers.h"

#include <math.h>
#include <stdlib.h>

static bool animationBrightnessBoostEnabled = false;
static uint8_t animationBrightnessBoostAmount = 0;
static uint8_t displayBaselineBrightness = 0;

void animationBrightnessConfigure(uint8_t baselineIntensity, bool boostEnabled,
                                  uint8_t boostAmount) {
  displayBaselineBrightness = min((uint8_t)15, baselineIntensity);
  animationBrightnessBoostEnabled = boostEnabled;
  animationBrightnessBoostAmount = min((uint8_t)15, boostAmount);
}

uint8_t displayBaselineIntensity() { return displayBaselineBrightness; }

uint8_t animationDisplayIntensity(uint8_t normalizedIntensity) {
  uint8_t intensity = displayBaselineBrightness +
                      min((uint8_t)15, normalizedIntensity);
  if (animationBrightnessBoostEnabled) {
    intensity += animationBrightnessBoostAmount;
  }
  return intensity > 15 ? 15 : intensity;
}

void milestoneCtxInit(MD_Parola &display, MilestoneCtx &ctx) {
  ctx.display = &display;
  ctx.matrix = display.getGraphicObject();
  display.getDisplayExtent(ctx.colStart, ctx.colEnd);
  ctx.width = ctx.colEnd - ctx.colStart + 1;
  ctx.height = 8;
  ctx.cx = ctx.width / 2;
  ctx.cy = ctx.height / 2;
}

void milestoneEffectBegin(MilestoneCtx &ctx, uint8_t intensity) {
  ctx.matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  ctx.display->setIntensity(animationDisplayIntensity(intensity));
}

void milestoneEffectEnd(MilestoneCtx &ctx) {
  ctx.matrix->clear();
  ctx.matrix->update();
  ctx.matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void milestoneFrameShow(MilestoneCtx &ctx, uint16_t delayMs, uint8_t intensity) {
  ctx.display->setIntensity(animationDisplayIntensity(intensity));
  ctx.matrix->update();
  delay(delayMs);
}

void milestoneClear(MilestoneCtx &ctx) { ctx.matrix->clear(); }

void milestoneFillAll(MilestoneCtx &ctx) {
  for (int c = 0; c < ctx.width; c++) {
    for (int row = 0; row < ctx.height; row++) {
      ctx.matrix->setPoint(row, ctx.colStart + c, true);
    }
  }
}

void milestoneDrawDiamondRing(MilestoneCtx &ctx, int ring) {
  for (int c = 0; c < ctx.width; c++) {
    for (int row = 0; row < ctx.height; row++) {
      if (abs(c - ctx.cx) + abs(row - ctx.cy) == ring) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
  }
}

void milestoneDrawExplosionRing(MilestoneCtx &ctx, int originX, int originY,
                                int ring) {
  for (int c = 0; c < ctx.width; c++) {
    for (int row = 0; row < ctx.height; row++) {
      if (abs(c - originX) + abs(row - originY) == ring) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
  }
}

void milestoneDrawFireworkBurst(MilestoneCtx &ctx, int originX, int originY,
                                int burstFrame, int particleCount,
                                float speed) {
  for (int particle = 0; particle < particleCount; particle++) {
    float angle = particle * (6.283185f / particleCount) + burstFrame * 0.1f;
    int px = originX + (int)(burstFrame * cosf(angle) * speed);
    int py = originY + (int)(burstFrame * sinf(angle) * speed);
    if (px >= 0 && px < ctx.width && py >= 0 && py < ctx.height) {
      ctx.matrix->setPoint(py, ctx.colStart + px,
                           burstFrame % 2 == 0 || particle % 2 == 0);
    }
  }
}

void milestoneAnimInwardCollapse(MilestoneCtx &ctx, uint16_t frameDelayMs) {
  const int maxShrink = max(ctx.width / 2, ctx.height / 2) + 1;
  for (int shrink = 0; shrink <= maxShrink; shrink++) {
    milestoneClear(ctx);
    int left = shrink;
    int right = ctx.width - 1 - shrink;
    int top = shrink;
    int bottom = ctx.height - 1 - shrink;
    if (left <= right && top <= bottom) {
      for (int c = left; c <= right; c++) {
        ctx.matrix->setPoint(top, ctx.colStart + c, true);
        ctx.matrix->setPoint(bottom, ctx.colStart + c, true);
      }
      for (int row = top; row <= bottom; row++) {
        ctx.matrix->setPoint(row, ctx.colStart + left, true);
        ctx.matrix->setPoint(row, ctx.colStart + right, true);
      }
    }
    milestoneFrameShow(ctx, frameDelayMs, max(0, 14 - shrink * 2));
  }
}

void milestoneAnimVictoryPulses(MilestoneCtx &ctx, int pulseCount,
                                uint16_t onMs, uint16_t offMs) {
  for (int pulse = 0; pulse < pulseCount; pulse++) {
    milestoneClear(ctx);
    milestoneFillAll(ctx);
    milestoneFrameShow(ctx, onMs, 0);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, offMs, 0);
  }
}

void milestoneAnimSparkRain(MilestoneCtx &ctx, int frames,
                            uint16_t frameDelayMs) {
  uint8_t dropHead[32];
  uint8_t dropSpeed[32];
  for (int c = 0; c < ctx.width; c++) {
    dropHead[c] = random(ctx.height + 2);
    dropSpeed[c] = 1 + (c % 3);
  }

  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
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
    milestoneFrameShow(ctx, frameDelayMs, min(9, frame / 2));
  }
}

void milestoneAnimFallingComets(MilestoneCtx &ctx, int passes, uint16_t frameDelayMs) {
  const int maxComets = 12;
  int cometCol[maxComets];
  int cometRow[maxComets];
  int cometDir[maxComets];
  bool cometLive[maxComets];

  for (int pass = 0; pass < passes; pass++) {
    int cometCount = min(maxComets, 6 + pass * 2);
    int spawnSpan = ctx.width + ctx.height * 2;
    for (int i = 0; i < cometCount; i++) {
      cometDir[i] = (i + pass) % 2 == 0 ? 1 : -1;
      int spawnOffset = (i * spawnSpan / cometCount + pass * 5) % spawnSpan;
      cometCol[i] = cometDir[i] > 0 ? spawnOffset - ctx.height
                                     : ctx.width + ctx.height - spawnOffset;
      cometRow[i] = -(i % 5) - 1;
      cometLive[i] = true;
    }

    for (int frame = 0; frame < ctx.width + ctx.height + 14; frame++) {
      milestoneClear(ctx);
      for (int i = 0; i < cometCount; i++) {
        if (!cometLive[i]) {
          if (frame % (4 + i) == pass % 2) {
            cometDir[i] = random(2) == 0 ? 1 : -1;
            cometCol[i] = cometDir[i] > 0 ? -random(ctx.height + 4)
                                          : ctx.width - 1 + random(ctx.height + 4);
            cometRow[i] = -random(4) - 1;
            cometLive[i] = true;
          }
          continue;
        }
        cometCol[i] += cometDir[i];
        cometRow[i]++;
        bool offLeft = cometDir[i] < 0 && cometCol[i] + 5 < 0;
        bool offRight = cometDir[i] > 0 && cometCol[i] - 5 >= ctx.width;
        if (cometRow[i] - 5 > ctx.height || offLeft || offRight) {
          cometLive[i] = false;
          continue;
        }
        for (int t = 0; t < 6; t++) {
          int c = cometCol[i] - cometDir[i] * t;
          int r = cometRow[i] - t;
          if (c >= 0 && c < ctx.width && r >= 0 && r < ctx.height) {
            ctx.matrix->setPoint(r, ctx.colStart + c, true);
          }
          if (t <= 3) {
            int sideC = c - cometDir[i];
            if (sideC >= 0 && sideC < ctx.width && r >= 0 && r < ctx.height) {
              ctx.matrix->setPoint(r, ctx.colStart + sideC, t <= 1 || frame % 2 == 0);
            }
          }
          if (t == 0) {
            int headRow = r + 1;
            if (c >= 0 && c < ctx.width && headRow >= 0 && headRow < ctx.height) {
              ctx.matrix->setPoint(headRow, ctx.colStart + c, true);
            }
          }
        }
      }
      milestoneFrameShow(ctx, frameDelayMs, min(7, pass * 2 + frame / 8));
    }

    if (pass + 1 < passes) {
      milestoneClear(ctx);
      for (int spark = 0; spark < 14; spark++) {
        ctx.matrix->setPoint(random(ctx.height), ctx.colStart + random(ctx.width), true);
      }
      milestoneFrameShow(ctx, 38, 0);
    }
  }
}

void milestoneAnimRockets(MilestoneCtx &ctx, int frames, uint16_t frameDelayMs) {
  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      int launch = (c * 5 + 2) % 9;
      if (frame < launch) {
        continue;
      }
      int rocketTop = min(ctx.height - 1, frame - launch);
      for (int trail = 0; trail < 4; trail++) {
        int row = ctx.height - 1 - rocketTop + trail;
        if (row >= 0 && row < ctx.height) {
          ctx.matrix->setPoint(row, ctx.colStart + c, trail < 2);
        }
      }
    }
    milestoneFrameShow(ctx, frameDelayMs, min(10, frame / 3));
  }
}

void milestoneAnimMegablast(MilestoneCtx &ctx, int maxRing, uint16_t fastDelayMs,
                            uint16_t slowDelayMs, bool flashAfter) {
  for (int ring = 0; ring <= maxRing; ring++) {
    milestoneClear(ctx);
    milestoneDrawExplosionRing(ctx, ctx.cx, ctx.cy, ring);
    if (ring > 2) {
      milestoneDrawExplosionRing(ctx, ctx.cx, ctx.cy, ring - 2);
    }
    milestoneFrameShow(ctx, ring < maxRing / 2 ? fastDelayMs : slowDelayMs,
                       min(9, (int)ring));
  }

  if (flashAfter) {
    milestoneClear(ctx);
    milestoneFillAll(ctx);
    milestoneFrameShow(ctx, 90, 0);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, 50, 0);
  }
}

void milestoneAnimFireworkBarrage(MilestoneCtx &ctx, const int *originX,
                                  const int *originY, int burstCount,
                                  int frames, int staggerFrames,
                                  int particleCount, float speed,
                                  uint16_t frameDelayMs) {
  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
    for (int b = 0; b < burstCount; b++) {
      if (frame < b * staggerFrames) {
        continue;
      }
      milestoneDrawFireworkBurst(ctx, originX[b], originY[b],
                                 frame - b * staggerFrames, particleCount,
                                 speed);
    }
    milestoneFrameShow(ctx, frameDelayMs, min(7, frame / 4));
  }
}

void milestoneAnimChainExplosions(MilestoneCtx &ctx, const int *originX,
                                  int siteCount, int maxRing,
                                  uint16_t frameDelayMs, uint16_t sitePauseMs) {
  for (int site = 0; site < siteCount; site++) {
    for (int ring = 0; ring <= maxRing; ring++) {
      milestoneClear(ctx);
      for (int s = 0; s <= site; s++) {
        int drawRing = (s == site) ? ring : maxRing;
        milestoneDrawExplosionRing(ctx, originX[s], ctx.cy, drawRing);
        if (drawRing > 1) {
          milestoneDrawExplosionRing(ctx, originX[s], ctx.cy, drawRing - 1);
        }
      }
      milestoneFrameShow(ctx, frameDelayMs, min(6, (int)ring));
    }
    delay(sitePauseMs);
  }
}

static void milestoneDrawExpandingRipple(MilestoneCtx &ctx, int radius) {
  for (int c = 0; c < ctx.width; c++) {
    for (int row = 0; row < ctx.height; row++) {
      if (abs(c - ctx.cx) + abs(row - ctx.cy) == radius) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
  }
}

void milestoneAnimRippleBars(MilestoneCtx &ctx, int frames,
                             uint16_t frameDelayMs) {
  for (int ring = 0; ring < frames; ring++) {
    milestoneClear(ctx);
    milestoneDrawExpandingRipple(ctx, ring);
    milestoneFrameShow(ctx, frameDelayMs, min(9, ring / 3));
  }
}

void milestoneAnimDoubleDiamondPulse(MilestoneCtx &ctx, uint16_t frameDelayMs) {
  for (int pass = 0; pass < 2; pass++) {
    const int startRing = pass == 0 ? 1 : 3;
    const int endRing = pass == 0 ? 6 : 8;
    for (int ring = startRing; ring <= endRing; ring++) {
      milestoneClear(ctx);
      milestoneDrawDiamondRing(ctx, ring);
      milestoneFrameShow(ctx, frameDelayMs, min(7, ring * 2));
    }
    if (pass == 0) {
      delay(70);
    }
  }
}

void milestoneAnimTripleTsunami(MilestoneCtx &ctx, int framesPerWave,
                                uint16_t frameDelayMs, uint16_t wavePauseMs) {
  for (int wave = 0; wave < 3; wave++) {
    for (int ring = 0; ring < framesPerWave; ring++) {
      milestoneClear(ctx);
      milestoneDrawExpandingRipple(ctx, ring);
      milestoneFrameShow(ctx, frameDelayMs, min(7, wave * 2 + ring / 4));
    }
    if (wave < 2) {
      delay(wavePauseMs);
    }
  }
}

void milestoneAnimCounterClimb(MilestoneCtx &ctx, uint16_t frameDelayMs) {
  uint8_t colFill[32] = {0};
  for (int frame = 0; frame < ctx.width + ctx.height; frame++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      if (frame >= c + 2) {
        colFill[c] = min((uint8_t)ctx.height, (uint8_t)(colFill[c] + 1));
      }
      for (int row = ctx.height - (int)colFill[c]; row < ctx.height; row++) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
    milestoneFrameShow(ctx, frameDelayMs, min(9, frame / 4));
  }
}

void milestoneAnimCounterShimmer(MilestoneCtx &ctx, int frames,
                                 uint16_t frameDelayMs) {
  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
    int barH = ctx.height - (frame % 2);
    for (int c = 0; c < ctx.width; c++) {
      for (int row = ctx.height - barH; row < ctx.height; row++) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
    milestoneFrameShow(ctx, frameDelayMs, 0);
  }
}

void milestoneAnimSlowRingBuild(MilestoneCtx &ctx, int maxRing,
                                uint16_t frameDelayMs) {
  for (int ring = 1; ring <= maxRing; ring++) {
    milestoneClear(ctx);
    milestoneDrawDiamondRing(ctx, ring);
    milestoneFrameShow(ctx, frameDelayMs, min(11, (int)ring));
  }
}

void milestoneAnimShockwaveFlashes(MilestoneCtx &ctx, int flashCount,
                                   uint16_t flashMs, uint16_t emberMs) {
  for (int wave = 0; wave < flashCount; wave++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      for (int row = 0; row < ctx.height; row++) {
        ctx.matrix->setPoint(row, ctx.colStart + c, (c + row + wave) % 3 != 0);
      }
    }
    milestoneFrameShow(ctx, flashMs, 0);

    milestoneClear(ctx);
    for (int i = 0; i < 18; i++) {
      ctx.matrix->setPoint(random(ctx.height), ctx.colStart + random(ctx.width),
                           true);
    }
    milestoneFrameShow(ctx, emberMs, 0);
  }
}

void milestoneAnimEmberFallout(MilestoneCtx &ctx, int frames,
                               uint16_t frameDelayMs) {
  uint8_t emberCol[32];
  uint8_t emberRow[32];
  for (int i = 0; i < ctx.width; i++) {
    emberCol[i] = random(ctx.width);
    emberRow[i] = random(ctx.height / 2);
  }

  for (int frame = 0; frame < frames; frame++) {
    milestoneClear(ctx);
    for (int i = 0; i < ctx.width; i++) {
      emberRow[i] = (emberRow[i] + 1) % (ctx.height + 3);
      int row = (int)emberRow[i];
      int c = emberCol[i];
      if (row >= 0 && row < ctx.height) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
      if (frame % 5 == i % 5) {
        emberCol[i] = random(ctx.width);
      }
    }
    milestoneFrameShow(ctx, frameDelayMs, max(0, 10 - frame / 3));
  }
}

void milestoneAnimFinalFlashes(MilestoneCtx &ctx, int flashCount,
                               uint16_t flashMs, uint16_t gapMs,
                               uint16_t lastFlashMs) {
  for (int flash = 0; flash < flashCount; flash++) {
    milestoneClear(ctx);
    milestoneFillAll(ctx);
    milestoneFrameShow(ctx, flash == flashCount - 1 ? lastFlashMs : flashMs, 0);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, gapMs, 0);
  }
}

void milestoneAnimMultiSiteBlast(MilestoneCtx &ctx, const int *originX,
                                 const int *originY, int siteCount, int maxRing,
                                 uint16_t fastDelayMs, uint16_t slowDelayMs,
                                 bool flashAfter) {
  for (int ring = 0; ring <= maxRing; ring++) {
    milestoneClear(ctx);
    for (int s = 0; s < siteCount; s++) {
      milestoneDrawExplosionRing(ctx, originX[s], originY[s], ring);
      if (ring > 2) {
        milestoneDrawExplosionRing(ctx, originX[s], originY[s], ring - 2);
      }
    }
    milestoneFrameShow(ctx, ring < maxRing / 2 ? fastDelayMs : slowDelayMs,
                       min(9, (int)ring));
  }

  if (flashAfter) {
    milestoneClear(ctx);
    milestoneFillAll(ctx);
    milestoneFrameShow(ctx, 110, 0);
    milestoneClear(ctx);
    milestoneFrameShow(ctx, 55, 0);
  }
}

void milestoneAnimDualEdgeClimb(MilestoneCtx &ctx, uint16_t frameDelayMs) {
  for (int frame = 0; frame < ctx.width / 2 + ctx.height + 2; frame++) {
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      int distFromEdge = min(c, ctx.width - 1 - c);
      int barH = 0;
      if (frame >= distFromEdge) {
        barH = min(ctx.height, frame - distFromEdge + 1);
      }
      for (int row = ctx.height - barH; row < ctx.height; row++) {
        ctx.matrix->setPoint(row, ctx.colStart + c, true);
      }
    }
    milestoneFrameShow(ctx, frameDelayMs, min(10, frame / 4));
  }
}

void milestoneAnimScreenShake(MilestoneCtx &ctx, int frames,
                              uint16_t frameDelayMs) {
  for (int frame = 0; frame < frames; frame++) {
    int offset = (frame % 3) - 1;
    milestoneClear(ctx);
    for (int c = 0; c < ctx.width; c++) {
      for (int row = 0; row < ctx.height; row++) {
        if ((c + row + frame) % 2 == 0) {
          int col = c + offset;
          if (col >= 0 && col < ctx.width) {
            ctx.matrix->setPoint(row, ctx.colStart + col, true);
          }
        }
      }
    }
    milestoneFrameShow(ctx, frameDelayMs, 0);
  }
}
