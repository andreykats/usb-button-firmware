#ifndef _WS_WIFI_H_
#define _WS_WIFI_H_

#include <WiFi.h>
#include <WebServer.h> 
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "stdio.h"
#include "WS_Flow.h"

// The name and password of the WiFi access point
#define APSSID       "ESP32-S3-Matrix"
#define APPSK        "waveshare"

extern uint8_t Flow_Flag;       // Relay current status flag
extern char Text[100];

void handleRoot();
void handleGetData();
void handleSwitch(uint8_t ledNumber);

void handleSwitch1();
void handleSwitch2();
void handleSwitch3();
void handleSwitch4();
void handleSwitch5();
void handleSwitch6();
void handleSwitch7();
void handleSwitch8();
void WIFI_Init();
void WIFI_Loop();

#endif