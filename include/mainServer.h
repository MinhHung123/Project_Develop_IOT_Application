#ifndef MAIN_SERVER_H
#define MAIN_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "global.h"

void setupSTAMode();
void TaskWebServer(void *pvParameters);

#endif // MAIN_SERVER_H