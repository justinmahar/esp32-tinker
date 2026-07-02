#include "scroller.h"

static const char DEFAULT_SCROLL_MESSAGE[] = "ESP32 Tinker";

static const char *activeScrollMessage(const ProgramConfig &cfg) {
  return cfg.scrollMessage.length() > 0 ? cfg.scrollMessage.c_str()
                                        : DEFAULT_SCROLL_MESSAGE;
}

void scrollerStart(const ProgramConfig &cfg) {
  Display.displayClear();
  Display.setTextAlignment(PA_LEFT);
  Display.displayScroll(activeScrollMessage(cfg), PA_LEFT, PA_SCROLL_LEFT,
                        cfg.scrollSpeedMs);
}

void scrollerTick(const ProgramConfig &cfg) {
  if (Display.displayAnimate()) {
    Display.displayReset();
  }
}
