/**
 * @file WebServer.cpp
 * @brief Implementation of WebServer class for hosting the web interface
 *
 * @author Luis Moreno
 * @date Dec 8, 2024
 */

#include "MasterDevice/WebServer.h"

WebServer::WebServer() : server(80) {
}

void WebServer::initialize() {
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        LOG_ERROR("Failed to mount LittleFS");
        return;
    }
    setupRoutes();
}

void WebServer::setupRoutes() {
    // Serve index.html at root
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    // Serve style.css
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/style.css", "text/css");
    });

    // Serve script.js
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/script.js", "application/javascript");
    });

    // Serve favicon.png
    server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(LittleFS, "/favicon.png", "image/png");
    });
}

void WebServer::start() {
    server.begin();
    LOG_INFO("Web server started");
}

AsyncWebServer& WebServer::getServer() {
    return server;
}
