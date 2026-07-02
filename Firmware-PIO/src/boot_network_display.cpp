#include "boot_network_display.h"

namespace {
const uint16_t BOOT_IP_SCROLL_SPEED_MS = 80;
const uint16_t BOOT_IP_HOLD_MS = 2000;
} // namespace

void showBootIpAddress(MD_Parola &display, const IPAddress &ipAddress) {
  String ipText = ipAddress.toString();

  display.displayClear();
  display.displayScroll(ipText.c_str(), PA_LEFT, PA_SCROLL_LEFT,
                        BOOT_IP_SCROLL_SPEED_MS);
  while (!display.displayAnimate()) {
    delay(10);
  }
  delay(BOOT_IP_HOLD_MS);
}
