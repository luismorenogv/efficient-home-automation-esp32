/**
 * @file WebServer.h
 * @brief Declaration of WebServer class for hosting the web interface
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#pragma once

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "config.h"

// Class to manage the asynchronous web server
class WebServer {
public:
    WebServer();
    bool initialize();
    void start();
    AsyncWebServer& getServer();

private:
    AsyncWebServer server; // Asynchronous web server instance

    // Sets up HTTP routes for the web interface
    void setupRoutes();
};