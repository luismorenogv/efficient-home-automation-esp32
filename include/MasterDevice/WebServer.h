/**
 * @file WebServer.h
 * @brief Declaration of WebServer class for hosting the web interface
 *
 * @author Luis Moreno
 * @date Nov 21, 2024
 */

#pragma once

#include <ESPAsyncWebServer.h>

class WebServer {
public:
    WebServer();
    void initialize();
    void start();
    AsyncWebServer& getServer();

private:
    AsyncWebServer server;
    void setupRoutes();
};
