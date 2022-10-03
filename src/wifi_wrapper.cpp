#include "wifi_wrapper.h"

#include <WiFi.h>

#include "API_KEYS.h"

bool connect_to_wifi() {
    if (WL_CONNECT_FAILED == WiFi.begin(WIFI_SSID, WIFI_PASSWORD)) {
        return false;
    } else {
        if (WL_CONNECTED ==
            WiFi.waitForConnectResult()) {  // attempt to connect for 10s
            return true;
        } else {  // connection failed, time out

            disconnect_from_wifi();
            return false;
        }
    }
}

void disconnect_from_wifi() {
    // turn off radios
    WiFi.mode(WIFI_OFF);
    btStop();
}
