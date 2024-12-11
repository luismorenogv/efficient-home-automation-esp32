/**
 * @file WebServer.cpp
 * @brief Implementation of WebServer class for hosting the web interface
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "MasterDevice/WebServer.h"
#include <SPIFFS.h>

WebServer::WebServer() : server(80) {
}

void WebServer::initialize() {
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }
    setupRoutes();
}

void WebServer::setupRoutes() {
    // Serve index.html at root
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/index.html", "text/html");
    });

    // Serve style.css
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/style.css", "text/css");
    });

    // Serve script.js
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/script.js", "application/javascript");
    });

    // Serve favicon.png
    server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(SPIFFS, "/favicon.png", "image/png");
    });
}

void WebServer::start() {
    server.begin();
    Serial.println("Web server started");
}

AsyncWebServer& WebServer::getServer() {
    return server;
}
