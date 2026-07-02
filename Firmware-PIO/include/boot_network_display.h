#ifndef BOOT_NETWORK_DISPLAY_H
#define BOOT_NETWORK_DISPLAY_H

#include <Arduino.h>
#include <IPAddress.h>
#include <MD_Parola.h>

void showBootIpAddress(MD_Parola &display, const IPAddress &ipAddress);

#endif
