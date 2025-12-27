#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <animations/animation_types.h>
#include <animations/animations.h>
#include <settings.h>

static AsyncWebServer server(80);

struct AnimationInfo {
	uint8_t id;
	const char* name;
};

void setupWebServer();