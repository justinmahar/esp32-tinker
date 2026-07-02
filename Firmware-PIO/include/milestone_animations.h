#ifndef MILESTONE_ANIMATIONS_H
#define MILESTONE_ANIMATIONS_H

#include <MD_Parola.h>
#include <stdint.h>

// Tier boundaries shared by subscribers, views, and watch hours.
enum class MilestoneTier : uint8_t {
  Tier100 = 0,
  Tier1K = 1,
  Tier10K = 2,
  Tier100K = 3,
  Tier1M = 4,
  Tier10M = 5,
  Tier100M = 6, // views-only apex tier (+100M, 200M, …)
};

enum class MilestoneAnimation : uint8_t {
  Subs100 = 0,
  Subs1K = 1,
  Subs10K = 2,
  Subs100K = 3,
  Subs1M = 4,
  Subs10M = 5,

  Views100 = 6,
  Views1K = 7,
  Views10K = 8,
  Views100K = 9,
  Views1M = 10,
  Views10M = 11,

  Hours100 = 12,
  Hours1K = 13,
  Hours10K = 14,
  Hours100K = 15,
  Hours1M = 16,
  Hours10M = 17,

  Views100M = 18, // views-only; not part of the 6-tier subs/hours grid
};

void runMilestoneAnimation(MD_Parola &display, MilestoneAnimation animation);

// Shared helpers used by per-stat animation modules.
void milestoneShowCenteredText(MD_Parola &display, const char *text,
                               uint16_t holdMs);
void milestoneRevealLabels(MD_Parola &display, const char *statLabel,
                           const char *increaseLabel);
void milestoneCleanupDisplay(MD_Parola &display);
void milestoneGetIncreaseLabel(MilestoneTier tier, char *buffer, size_t size);

#endif
