#include "WebPage.h"
#include "Web.h"

extern AsyncWebServer  server;

// Externs for controlling state in main.cpp
extern uint8_t currentAnimationIndex;
extern int8_t brightness;
extern bool autoAdvanceEnabled;
extern bool animationEnabled;
extern int animation_change_interval;
extern bool displayClock;
extern uint8_t clockMode;
extern const uint8_t TOTAL_ANIMATIONS;
extern AnimationInfo animationInfoArray[];
extern unsigned long lastAnimationChangeTime;

void setupWebServer() {

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    // Endpoint to list all animations (with names)
    server.on("/animations", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "[";
        for (size_t i = 0; i < TOTAL_ANIMATIONS; i++) {
            if (i > 0) json += ",";
            json += "{\"id\":" + String(animationInfoArray[i].id) + ",\"name\":\"" + String(animationInfoArray[i].name) + "\"}";
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    // Endpoint to set parameters
    server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
        String msg = "";
        if (request->hasParam("animation")) {
            String anim = request->getParam("animation")->value();
            int idx = anim.toInt();
            if (idx >= 0 && idx < TOTAL_ANIMATIONS) {
                currentAnimationIndex = idx;
                lastAnimationChangeTime = millis(); // Reset timer on manual change
                msg += "Animation set. ";
            } else {
                msg += "Invalid animation index. ";
            }
        }
        if (request->hasParam("brightness")) {
            int b = request->getParam("brightness")->value().toInt();
            if (b >= 0 && b <= 255) {
                brightness = b;
                msg += "Brightness set. ";
            } else {
                msg += "Invalid brightness. ";
            }
        }
        if (request->hasParam("autoSwitch")) {
            String val = request->getParam("autoSwitch")->value();
            autoAdvanceEnabled = (val == "1" || val == "true");
            msg += "Auto-switch set. ";
        }
        if (request->hasParam("duration")) {
            int dur = request->getParam("duration")->value().toInt();
            if (dur >= 1 && dur <= 60) {
                animation_change_interval = dur;
                msg += "Duration set. ";
            } else {
                msg += "Invalid duration. ";
            }
        }
        if (request->hasParam("mode")) {
            String mode = request->getParam("mode")->value();
            if (mode == "clock") {
                displayClock = true;
                msg += "Clock mode enabled. ";
            } else if (mode == "animation") {
                displayClock = false;
                msg += "Animation mode enabled. ";
            }
        }
        if (request->hasParam("clockMode")) {
            int cm = request->getParam("clockMode")->value().toInt();
            if (cm >= 0 && cm <= 3) {
                clockMode = cm;
                msg += "Clock mode set. ";
            }
        }
        if (request->hasParam("animationEnabled")) {
            String val = request->getParam("animationEnabled")->value();
            animationEnabled = (val == "1" || val == "true");
            msg += "Animation enabled set. ";
        }
        request->send(200, "text/plain", msg.length() ? msg : "No valid parameters set.");
    });

    // Endpoint to get current state
    server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"animation\":" + String(currentAnimationIndex) + ",";
        json += "\"brightness\":" + String(brightness) + ",";
        json += "\"mode\":\"" + String(displayClock ? "clock" : "animation") + "\",";
        json += "\"clockMode\":" + String(clockMode) + ",";
        json += "\"maxBrightness\":" + String(MAX_BRIGHTNESS) + ",";
        json += "\"autoSwitch\":" + String(autoAdvanceEnabled ? 1 : 0) + ",";
        json += "\"duration\":" + String(animation_change_interval);
        json += "}";
        request->send(200, "application/json", json);
    });

    server.begin();
}
