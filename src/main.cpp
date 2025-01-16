#include "stopwatch.h"

Stopwatch& stopwatch = Stopwatch::getInstance();

void setup() {
    stopwatch.setup();
}

void loop() {
    stopwatch.loop();
}