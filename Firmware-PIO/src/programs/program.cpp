#include "program.h"

#include "fireworks.h"
#include "scroller.h"

ProgramId parseProgramId(const String &value) {
  if (value == "fireworks") {
    return ProgramId::Fireworks;
  }
  return ProgramId::Scroller;
}

const char *programIdToString(ProgramId id) {
  switch (id) {
  case ProgramId::Fireworks:
    return "fireworks";
  case ProgramId::Scroller:
  default:
    return "scroller";
  }
}

void programStart(const ProgramConfig &cfg) {
  switch (cfg.program) {
  case ProgramId::Fireworks:
    fireworksStart(cfg);
    break;
  case ProgramId::Scroller:
  default:
    scrollerStart(cfg);
    break;
  }
}

void programTick(const ProgramConfig &cfg) {
  switch (cfg.program) {
  case ProgramId::Fireworks:
    fireworksTick(cfg);
    break;
  case ProgramId::Scroller:
  default:
    scrollerTick(cfg);
    break;
  }
}
