#include "holiday_easter_eggs.h"

#include "firmware_config.h"
#include "holiday_april_fools.h"
#include "holiday_birthdays.h"
#include "holiday_christmas.h"
#include "holiday_cinco_de_mayo.h"
#include "holiday_coffee_day.h"
#include "holiday_creator_cat_day.h"
#include "holiday_easter.h"
#include "holiday_earth_day.h"
#include "holiday_fathers_day.h"
#include "holiday_groundhog.h"
#include "holiday_halloween.h"
#include "holiday_july_fourth.h"
#include "holiday_labor_day.h"
#include "holiday_leap_day.h"
#include "holiday_memorial_day.h"
#include "holiday_new_years_eve.h"
#include "holiday_pi_day.h"
#include "holiday_presidents_day.h"
#include "holiday_pirate_day.h"
#include "holiday_st_patricks.h"
#include "holiday_thanksgiving.h"
#include "holiday_valentines.h"
#include "holiday_veterans_day.h"
#include "milestone_helpers.h"

#include <Arduino.h>
#include <MD_MAX72xx.h>
#include <math.h>
#include <time.h>

static const char *TZ_EST = "EST5EDT,M3.2.0/2,M11.1.0/2";

static bool timeSynced = false;
static time_t holidayServerTimeUnix = 0;
static unsigned long holidayServerTimeMillis = 0;
static unsigned long lastHolidayEggMs = 0;

static const HolidayId HOLIDAY_TEST_CHOICES[] = {
    HolidayId::StPatricksDay, HolidayId::Halloween,       HolidayId::Christmas,
    HolidayId::ValentinesDay, HolidayId::JulyFourth,      HolidayId::GroundhogDay,
    HolidayId::NewYearsEve,   HolidayId::PiDay,           HolidayId::AprilFools,
    HolidayId::CincoDeMayo,   HolidayId::JustinBirthday,  HolidayId::DadBirthday,
    HolidayId::PirateDay,     HolidayId::VeteransDay,     HolidayId::Thanksgiving,
    HolidayId::EarthDay,      HolidayId::MemorialDay,     HolidayId::CreatorCatDay,
    HolidayId::LaborDay,      HolidayId::LeapDay,         HolidayId::Easter,
    HolidayId::FathersDay,    HolidayId::PresidentsDay,   HolidayId::CoffeeDay,
};

static HolidayId getRandomDebugHoliday() {
  static HolidayId selectedHoliday = HolidayId::None;
  if (selectedHoliday == HolidayId::None) {
    size_t holidayCount = sizeof(HOLIDAY_TEST_CHOICES) / sizeof(HolidayId);
    selectedHoliday = HOLIDAY_TEST_CHOICES[random(holidayCount)];
    Serial.print("Debug random holiday selected: ");
    Serial.println(static_cast<uint8_t>(selectedHoliday));
  }
  return selectedHoliday;
}

static void drawHolidayZoomStar(MilestoneCtx &ctx, float outerRadiusX,
                                float outerRadiusY, int zoomFrame) {
  const int vertexCount = 10;
  float vertexX[vertexCount];
  float vertexY[vertexCount];
  bool starPixels[8][32] = {{false}};
  const float innerScale = 0.45f;

  for (int i = 0; i < vertexCount; i++) {
    float angle = -1.5707963f + i * 0.6283185f;
    float scale = (i % 2 == 0) ? 1.0f : innerScale;
    vertexX[i] = ctx.cx + cosf(angle) * outerRadiusX * scale;
    vertexY[i] = ctx.cy + sinf(angle) * outerRadiusY * scale;
  }

  for (int row = 0; row < ctx.height; row++) {
    const float sampleY[3] = {row + 0.25f, row + 0.5f, row + 0.75f};
    for (int sample = 0; sample < 3; sample++) {
      float intersections[vertexCount];
      int intersectionCount = 0;

      for (int i = 0, j = vertexCount - 1; i < vertexCount; j = i++) {
        bool crosses =
            (vertexY[i] > sampleY[sample]) != (vertexY[j] > sampleY[sample]);
        if (!crosses) {
          continue;
        }

        intersections[intersectionCount++] =
            (vertexX[j] - vertexX[i]) * (sampleY[sample] - vertexY[i]) /
                (vertexY[j] - vertexY[i]) +
            vertexX[i];
      }

      for (int i = 1; i < intersectionCount; i++) {
        float key = intersections[i];
        int j = i - 1;
        while (j >= 0 && intersections[j] > key) {
          intersections[j + 1] = intersections[j];
          j--;
        }
        intersections[j + 1] = key;
      }

      for (int i = 0; i + 1 < intersectionCount; i += 2) {
        float left = intersections[i];
        float right = intersections[i + 1];
        for (int col = 0; col < ctx.width; col++) {
          float sampleX = col + 0.5f;
          if (sampleX >= left && sampleX <= right) {
            starPixels[row][col] = true;
          }
        }
      }
    }
  }

  for (int row = 0; row < ctx.height; row++) {
    for (int col = 0; col < ctx.width; col++) {
      if (starPixels[row][col]) {
        ctx.matrix->setPoint(row, ctx.colStart + col, true);
        continue;
      }

      bool left = col > 0 && starPixels[row][col - 1];
      bool right = col + 1 < ctx.width && starPixels[row][col + 1];
      bool up = row > 0 && starPixels[row - 1][col];
      bool down = row + 1 < ctx.height && starPixels[row + 1][col];
      bool upLeft = row > 0 && col > 0 && starPixels[row - 1][col - 1];
      bool upRight =
          row > 0 && col + 1 < ctx.width && starPixels[row - 1][col + 1];
      bool downLeft =
          row + 1 < ctx.height && col > 0 && starPixels[row + 1][col - 1];
      bool downRight = row + 1 < ctx.height && col + 1 < ctx.width &&
                       starPixels[row + 1][col + 1];

      if ((left && right) || (up && down) || (upLeft && downRight) ||
          (upRight && downLeft)) {
        ctx.matrix->setPoint(row, ctx.colStart + col, true);
      }
    }
  }

  if (zoomFrame >= 14) {
    const int patchWidth = 8;
    const int patchHeight = 6;
    int left = ctx.cx - patchWidth / 2;
    int top = ctx.cy - patchHeight / 2;
    for (int row = top; row < top + patchHeight; row++) {
      if (row < 0 || row >= ctx.height) {
        continue;
      }
      for (int col = left; col < left + patchWidth; col++) {
        if (col >= 0 && col < ctx.width) {
          ctx.matrix->setPoint(row, ctx.colStart + col, true);
        }
      }
    }
  }
}

static void drawHolidayBurstOverlay(MilestoneCtx &ctx, int originX, int originY,
                                    int burstFrame, int particleCount,
                                    float speed) {
  for (int particle = 0; particle < particleCount; particle++) {
    float angle = particle * (6.283185f / particleCount) + burstFrame * 0.1f;
    int px = originX + (int)(burstFrame * cosf(angle) * speed);
    int py = originY + (int)(burstFrame * sinf(angle) * speed);
    if (px >= 0 && px < ctx.width && py >= 0 && py < ctx.height &&
        (burstFrame % 2 == 0 || particle % 2 == 0)) {
      ctx.matrix->setPoint(py, ctx.colStart + px, true);
    }
  }
}

static void runHolidayIntroAnimation(MD_Parola &display) {
  MilestoneCtx ctx;
  milestoneCtxInit(display, ctx);
  milestoneEffectBegin(ctx, 0);

  const int fountainX[5] = {2, ctx.width / 4, ctx.cx, 3 * ctx.width / 4,
                            ctx.width - 3};
  for (int frame = 0; frame < 22; frame++) {
    milestoneClear(ctx);
    for (int i = 0; i < 5; i++) {
      int launch = (i * 3) % 7;
      int age = frame - launch;
      if (age < 0) {
        continue;
      }

      int height = min(ctx.height - 1, age);
      for (int trail = 0; trail < 4; trail++) {
        int row = ctx.height - 1 - height + trail;
        if (row >= 0 && row < ctx.height) {
          ctx.matrix->setPoint(row, ctx.colStart + fountainX[i], trail < 2);
        }
      }

      if (age > 4) {
        int burstFrame = age - 4;
        milestoneDrawFireworkBurst(ctx, fountainX[i], 1 + (i % 2), burstFrame,
                                   8, 0.42f);
      }
    }

    for (int c = 0; c < ctx.width; c++) {
      if ((c * 3 + frame) % 11 == 0) {
        ctx.matrix->setPoint((c + frame) % ctx.height, ctx.colStart + c, true);
      }
      if ((c + frame) % 7 == 0) {
        ctx.matrix->setPoint((ctx.height - 1 + frame - c) % ctx.height,
                             ctx.colStart + c, true);
      }
    }
    milestoneFrameShow(ctx, 34, min(7, frame / 3));
  }

  const int burstX[6] = {3, ctx.width / 5, 2 * ctx.width / 5,
                         3 * ctx.width / 5, 4 * ctx.width / 5,
                         ctx.width - 4};
  const int burstY[6] = {1, 5, 2, 6, 1, 5};
  for (int frame = 0; frame < 18; frame++) {
    milestoneClear(ctx);
    for (int b = 0; b < 6; b++) {
      int staggeredFrame = frame - b / 2;
      if (staggeredFrame >= 0) {
        milestoneDrawFireworkBurst(ctx, burstX[b], burstY[b], staggeredFrame,
                                   10, 0.55f);
      }
    }
    for (int c = 0; c < ctx.width; c += 2) {
      bool topChase = ((c + frame) % 6) < 3;
      ctx.matrix->setPoint(0, ctx.colStart + c, topChase);
      ctx.matrix->setPoint(ctx.height - 1, ctx.colStart + c, !topChase);
    }
    milestoneFrameShow(ctx, 38, frame % 2 == 0 ? 3 : 0);
  }

  for (int frame = 0; frame < 22; frame++) {
    milestoneClear(ctx);
    float progress = (frame + 1) / 22.0f;
    float easedProgress = progress * progress * progress;
    float outerRadiusX = 0.8f + easedProgress * (ctx.width * 1.35f);
    float outerRadiusY = 0.6f + easedProgress * (ctx.height * 1.9f);
    drawHolidayZoomStar(ctx, outerRadiusX, outerRadiusY, frame);

    const int confettiBurstX[4] = {2, ctx.width - 3, ctx.width / 4,
                                   3 * ctx.width / 4};
    const int confettiBurstY[4] = {1, 6, 5, 2};
    for (int burst = 0; burst < 4; burst++) {
      drawHolidayBurstOverlay(ctx, confettiBurstX[burst],
                              confettiBurstY[burst], frame + burst * 2, 8,
                              0.48f);
    }

    for (int c = 0; c < ctx.width; c += 2) {
      int row = (c * 3 + frame * 2) % ctx.height;
      ctx.matrix->setPoint(row, ctx.colStart + c, true);
      if (c % 4 == 0) {
        int secondRow =
            (ctx.height * 4 + ctx.height - 1 + frame - c / 2) % ctx.height;
        ctx.matrix->setPoint(secondRow, ctx.colStart + c, true);
      }
    }
    milestoneFrameShow(ctx, frame == 21 ? 180 : 32,
                       min(6, frame / 2));
  }

  milestoneClear(ctx);
  milestoneFillAll(ctx);
  milestoneFrameShow(ctx, 160, 0);

  for (int frame = 0; frame < 10; frame++) {
    milestoneClear(ctx);
    for (int row = 0; row < ctx.height; row++) {
      for (int col = 0; col < ctx.width; col++) {
        int dissolveOrder = (col * 7 + row * 11) % 10;
        if (dissolveOrder >= frame) {
          ctx.matrix->setPoint(row, ctx.colStart + col, true);
        }
      }
    }
    milestoneFrameShow(ctx, frame == 9 ? 90 : 38, max(0, 13 - frame));
  }

  milestoneEffectEnd(ctx);

  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setIntensity(animationDisplayIntensity(0));
  display.setTextAlignment(PA_LEFT);
  display.displayScroll("Holiday!", PA_LEFT, PA_SCROLL_LEFT, 60);
  while (!display.displayAnimate()) {
    delay(10);
  }
  delay(200);
}

static bool isThanksgiving(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 10 && timeinfo.tm_wday == 4 &&
         timeinfo.tm_mday >= 22 && timeinfo.tm_mday <= 28;
}

static bool isMemorialDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 4 && timeinfo.tm_wday == 1 &&
         timeinfo.tm_mday >= 25 && timeinfo.tm_mday <= 31;
}

static bool isLaborDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 8 && timeinfo.tm_wday == 1 &&
         timeinfo.tm_mday >= 1 && timeinfo.tm_mday <= 7;
}

static bool isFathersDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 5 && timeinfo.tm_wday == 0 &&
         timeinfo.tm_mday >= 15 && timeinfo.tm_mday <= 21;
}

static bool isPresidentsDay(const struct tm &timeinfo) {
  return timeinfo.tm_mon == 1 && timeinfo.tm_wday == 1 &&
         timeinfo.tm_mday >= 15 && timeinfo.tm_mday <= 21;
}

static bool isEaster(const struct tm &timeinfo) {
  int year = timeinfo.tm_year + 1900;
  int goldenYear = year % 19;
  int century = year / 100;
  int yearOfCentury = year % 100;
  int leapCentury = century / 4;
  int centuryRemainder = century % 4;
  int correction = (century + 8) / 25;
  int moonCorrection = (century - correction + 1) / 3;
  int epact = (19 * goldenYear + century - leapCentury - moonCorrection + 15) % 30;
  int yearLeap = yearOfCentury / 4;
  int yearRemainder = yearOfCentury % 4;
  int weekdayCorrection =
      (32 + 2 * centuryRemainder + 2 * yearLeap - epact - yearRemainder) % 7;
  int monthOffset =
      (goldenYear + 11 * epact + 22 * weekdayCorrection) / 451;
  int easterMonth =
      (epact + weekdayCorrection - 7 * monthOffset + 114) / 31;
  int easterDay =
      ((epact + weekdayCorrection - 7 * monthOffset + 114) % 31) + 1;

  return timeinfo.tm_mon == easterMonth - 1 && timeinfo.tm_mday == easterDay;
}

void holidayScrollMessage(MD_Parola &display, const char *message) {
  MD_MAX72XX *matrix = display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setTextAlignment(PA_LEFT);
  display.displayScroll(message, PA_LEFT, PA_SCROLL_LEFT, 80);
  while (!display.displayAnimate()) {
    delay(10);
  }
  delay(400);
  matrix->clear();
  matrix->update();
  display.displayClear();
  display.setIntensity(displayBaselineIntensity());
}

bool holidayEasterEggsInit() {
  setenv("TZ", TZ_EST, 1);
  tzset();
  Serial.println("Holiday timezone configured (EST).");
  return true;
}

bool holidayEasterEggsSetServerTime(double serverTimeUnix) {
  if (!isfinite(serverTimeUnix) || serverTimeUnix <= 0) {
    Serial.println("Holiday clock missing valid server time.");
    return false;
  }

  unsigned long now = millis();
  holidayServerTimeUnix = (time_t)serverTimeUnix;
  holidayServerTimeMillis = now;
  timeSynced = true;
  Serial.println("Holiday clock synced from stats API.");
  return true;
}

HolidayId getCurrentHoliday() {
  if (DEBUG_RANDOM_HOLIDAY_TODAY) {
    return getRandomDebugHoliday();
  }

  if (!timeSynced) {
    return HolidayId::None;
  }

  time_t currentUnixTime =
      holidayServerTimeUnix + ((millis() - holidayServerTimeMillis) / 1000);
  struct tm timeinfo;
  if (localtime_r(&currentUnixTime, &timeinfo) == nullptr) {
    return HolidayId::None;
  }

  if (isEaster(timeinfo)) {
    return HolidayId::Easter;
  }
  if (timeinfo.tm_mon == 2 && timeinfo.tm_mday == 17) {
    return HolidayId::StPatricksDay;
  }
  if (timeinfo.tm_mon == 2 && timeinfo.tm_mday == 14) {
    return HolidayId::PiDay;
  }
  if (timeinfo.tm_mon == 3 && timeinfo.tm_mday == 1) {
    return HolidayId::AprilFools;
  }
  if (timeinfo.tm_mon == 3 && timeinfo.tm_mday == 22) {
    return HolidayId::EarthDay;
  }
  if (timeinfo.tm_mon == 4 && timeinfo.tm_mday == 5) {
    return HolidayId::CincoDeMayo;
  }
  if (isMemorialDay(timeinfo)) {
    return HolidayId::MemorialDay;
  }
  if (timeinfo.tm_mon == 1 && timeinfo.tm_mday == 2) {
    return HolidayId::GroundhogDay;
  }
  if (timeinfo.tm_mon == 1 && timeinfo.tm_mday == 14) {
    return HolidayId::ValentinesDay;
  }
  if (isPresidentsDay(timeinfo)) {
    return HolidayId::PresidentsDay;
  }
  if (timeinfo.tm_mon == 1 && timeinfo.tm_mday == 29) {
    return HolidayId::LeapDay;
  }
  if (isFathersDay(timeinfo)) {
    return HolidayId::FathersDay;
  }
  if (timeinfo.tm_mon == 6 && timeinfo.tm_mday == 4) {
    return HolidayId::JulyFourth;
  }
  if (timeinfo.tm_mon == 7 && timeinfo.tm_mday == 8) {
    return HolidayId::CreatorCatDay;
  }
  if (timeinfo.tm_mon == 7 && timeinfo.tm_mday == 12) {
    return HolidayId::JustinBirthday;
  }
  if (timeinfo.tm_mon == 8 && timeinfo.tm_mday == 12) {
    return HolidayId::DadBirthday;
  }
  if (isLaborDay(timeinfo)) {
    return HolidayId::LaborDay;
  }
  if (timeinfo.tm_mon == 8 && timeinfo.tm_mday == 19) {
    return HolidayId::PirateDay;
  }
  if (timeinfo.tm_mon == 8 && timeinfo.tm_mday == 29) {
    return HolidayId::CoffeeDay;
  }
  if (timeinfo.tm_mon == 9 && timeinfo.tm_mday == 31) {
    return HolidayId::Halloween;
  }
  if (timeinfo.tm_mon == 10 && timeinfo.tm_mday == 11) {
    return HolidayId::VeteransDay;
  }
  if (isThanksgiving(timeinfo)) {
    return HolidayId::Thanksgiving;
  }
  if (timeinfo.tm_mon == 11 && timeinfo.tm_mday == 25) {
    return HolidayId::Christmas;
  }
  if ((timeinfo.tm_mon == 11 && timeinfo.tm_mday == 31) ||
      (timeinfo.tm_mon == 0 && timeinfo.tm_mday == 1)) {
    return HolidayId::NewYearsEve;
  }

  return HolidayId::None;
}

void runHolidayPreviewCycle(MD_Parola &display) {
  static const HolidayId previewOrder[] = {
      HolidayId::NewYearsEve,      HolidayId::GroundhogDay,
      HolidayId::ValentinesDay,    HolidayId::PresidentsDay,
      HolidayId::LeapDay,          HolidayId::PiDay,
      HolidayId::StPatricksDay,    HolidayId::Easter,
      HolidayId::AprilFools,       HolidayId::EarthDay,
      HolidayId::CincoDeMayo,      HolidayId::MemorialDay,
      HolidayId::FathersDay,       HolidayId::JulyFourth,
      HolidayId::CreatorCatDay,    HolidayId::JustinBirthday,
      HolidayId::DadBirthday,      HolidayId::LaborDay,
      HolidayId::PirateDay,
      HolidayId::CoffeeDay,
      HolidayId::Halloween,        HolidayId::VeteransDay,
      HolidayId::Thanksgiving,     HolidayId::Christmas,
  };

  for (HolidayId holiday : previewOrder) {
    Serial.print("Holiday preview: ");
    Serial.println(static_cast<uint8_t>(holiday));
    runHolidayEasterEgg(display, holiday);
  }
}

void runHolidayEasterEgg(MD_Parola &display, HolidayId holiday) {
  if (holiday != HolidayId::None && !DEBUG_DISABLE_ANIMATION_INTROS) {
    runHolidayIntroAnimation(display);
  }

  switch (holiday) {
  case HolidayId::StPatricksDay:
    runStPatricksHolidayAnimation(display);
    break;
  case HolidayId::Halloween:
    runHalloweenHolidayAnimation(display);
    break;
  case HolidayId::Christmas:
    runChristmasHolidayAnimation(display);
    break;
  case HolidayId::ValentinesDay:
    runValentinesHolidayAnimation(display);
    break;
  case HolidayId::JulyFourth:
    runJulyFourthHolidayAnimation(display);
    break;
  case HolidayId::GroundhogDay:
    runGroundhogHolidayAnimation(display);
    break;
  case HolidayId::NewYearsEve:
    runNewYearsEveHolidayAnimation(display);
    break;
  case HolidayId::PiDay:
    runPiDayHolidayAnimation(display);
    break;
  case HolidayId::AprilFools:
    runAprilFoolsHolidayAnimation(display);
    break;
  case HolidayId::CincoDeMayo:
    runCincoDeMayoHolidayAnimation(display);
    break;
  case HolidayId::JustinBirthday:
    runBirthdayHolidayAnimation(display, "Justin's birthday!");
    break;
  case HolidayId::DadBirthday:
    runBirthdayHolidayAnimation(display, "Happy birthday Dad!");
    break;
  case HolidayId::PirateDay:
    runPirateDayHolidayAnimation(display);
    break;
  case HolidayId::VeteransDay:
    runVeteransDayHolidayAnimation(display);
    break;
  case HolidayId::Thanksgiving:
    runThanksgivingHolidayAnimation(display);
    break;
  case HolidayId::EarthDay:
    runEarthDayHolidayAnimation(display);
    break;
  case HolidayId::MemorialDay:
    runMemorialDayHolidayAnimation(display);
    break;
  case HolidayId::CreatorCatDay:
    runCreatorCatDayHolidayAnimation(display);
    break;
  case HolidayId::LaborDay:
    runLaborDayHolidayAnimation(display);
    break;
  case HolidayId::LeapDay:
    runLeapDayHolidayAnimation(display);
    break;
  case HolidayId::Easter:
    runEasterHolidayAnimation(display);
    break;
  case HolidayId::FathersDay:
    runFathersDayHolidayAnimation(display);
    break;
  case HolidayId::PresidentsDay:
    runPresidentsDayHolidayAnimation(display);
    break;
  case HolidayId::CoffeeDay:
    runCoffeeDayHolidayAnimation(display);
    break;
  case HolidayId::None:
    break;
  }
}

bool checkAndRunHolidayEasterEgg(MD_Parola &display,
                                 unsigned int reminderIntervalMinutes) {
  if (PREVIEW_HOLIDAYS || reminderIntervalMinutes == 0) {
    return false;
  }

  HolidayId holiday = getCurrentHoliday();
  if (holiday == HolidayId::None) {
    return false;
  }

  unsigned long now = millis();
  unsigned long intervalMs =
      (unsigned long)reminderIntervalMinutes * 60UL * 1000UL;
  if (lastHolidayEggMs != 0 &&
      now - lastHolidayEggMs < intervalMs) {
    return false;
  }

  Serial.print("Holiday easter egg: ");
  Serial.println(static_cast<uint8_t>(holiday));

  runHolidayEasterEgg(display, holiday);
  lastHolidayEggMs = now;
  return true;
}
