#include "boot_network_display.h"

namespace {
const uint16_t BOOT_IP_SCROLL_SPEED_MS = 80;
const uint16_t BOOT_IP_HOLD_MS = 2000;
char bootIpBuffer[16];
} // namespace

void showBootIpAddress(MD_Parola &display, const IPAddress &ipAddress) {
  ipAddress.toString().toCharArray(bootIpBuffer, sizeof(bootIpBuffer));

  display.displayClear();
  display.displayScroll(bootIpBuffer, PA_LEFT, PA_SCROLL_LEFT,
                        BOOT_IP_SCROLL_SPEED_MS);
  while (!display.displayAnimate()) {
    delay(10);
  }
  delay(BOOT_IP_HOLD_MS);
}
