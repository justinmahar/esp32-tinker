#ifndef HOLIDAY_EASTER_EGGS_H
#define HOLIDAY_EASTER_EGGS_H

#include <MD_Parola.h>
#include <stdint.h>

enum class HolidayId : uint8_t {
  None = 0,
  StPatricksDay = 1,
  Halloween = 2,
  Christmas = 3,
  ValentinesDay = 4,
  JulyFourth = 5,
  GroundhogDay = 6,
  NewYearsEve = 7,
  PiDay = 8,
  AprilFools = 9,
  CincoDeMayo = 10,
  JustinBirthday = 11,
  DadBirthday = 12,
  PirateDay = 13,
  VeteransDay = 14,
  Thanksgiving = 15,
  EarthDay = 16,
  MemorialDay = 17,
  CreatorCatDay = 18,
  LaborDay = 19,
  LeapDay = 20,
  Easter = 21,
  FathersDay = 22,
  PresidentsDay = 23,
  CoffeeDay = 24,
};

// Set true to cycle every holiday animation on boot (dev preview).
const bool PREVIEW_HOLIDAYS = false;

// Set true to preview a single holiday on boot (when PREVIEW_HOLIDAYS is false).
const bool RUN_HOLIDAY_PREVIEW_ON_BOOT = false;
const HolidayId HOLIDAY_PREVIEW_BOOT = HolidayId::CoffeeDay;

// Set true to make today's holiday a random holiday for testing reminders.
const bool DEBUG_RANDOM_HOLIDAY_TODAY = false;

// Configure America/Eastern holiday time conversion.
bool holidayEasterEggsInit();

// Seed the holiday clock from the stats API's serverTimeUnix field.
bool holidayEasterEggsSetServerTime(double serverTimeUnix);

// Returns today's holiday in EST, or None.
HolidayId getCurrentHoliday();

// Play animation + scrolling greeting for the given holiday.
void runHolidayEasterEgg(MD_Parola &display, HolidayId holiday);

// Play every holiday animation in calendar order (PREVIEW_HOLIDAYS boot mode).
void runHolidayPreviewCycle(MD_Parola &display);

// Shared ending for holiday animations.
void holidayScrollMessage(MD_Parola &display, const char *message);

// On holidays, runs the easter egg at the configured reminder interval.
// Set the interval to 0 to disable holidays.
// Returns true if one played.
bool checkAndRunHolidayEasterEgg(MD_Parola &display,
                                 unsigned int reminderIntervalMinutes);

#endif
