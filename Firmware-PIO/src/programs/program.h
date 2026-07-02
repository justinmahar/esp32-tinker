#pragma once

#include <MD_Parola.h>
#include <WString.h>

extern MD_Parola Display;

enum class ProgramId : uint8_t { Scroller, Fireworks };

constexpr size_t MAX_SCROLL_MESSAGE_LENGTH = 64;

struct ProgramConfig {
  ProgramId program;
  String scrollMessage;
  unsigned int scrollSpeedMs;
  unsigned int fireworksMinLaunchDelayMs;
  unsigned int fireworksMaxLaunchDelayMs;
  unsigned int fireworksAnimSpeedMs;
  uint8_t brightness;
  uint8_t fireworksMaxBrightness;
};

ProgramId parseProgramId(const String &value);
const char *programIdToString(ProgramId id);
void programStart(const ProgramConfig &cfg);
void programTick(const ProgramConfig &cfg);
