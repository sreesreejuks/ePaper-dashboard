#include "screen_manager.h"
#include "dashboard_chrome.h"

#include <Arduino.h>

ScreenManager::ScreenManager(Screen **screens, int count, uint32_t rotationIntervalMs)
    : screens_(screens), count_(count), rotationIntervalMs_(rotationIntervalMs) {}

void ScreenManager::drawCurrentFull(EpdDisplay &display) {
    display.beginFrame();
    DashboardChrome::draw(display, screens_[current_]->name(), current_, count_);
    screens_[current_]->drawContent(display);
    display.endFrame();
}

void ScreenManager::begin(EpdDisplay &display) {
    nextRotation_ = millis() + rotationIntervalMs_;
    drawCurrentFull(display);
}

void ScreenManager::update(EpdDisplay &display) {
    if (millis() >= nextRotation_) {
        current_ = (current_ + 1) % count_;
        nextRotation_ = millis() + rotationIntervalMs_;
        drawCurrentFull(display);
        return;
    }
    screens_[current_]->tick(display);
}

void ScreenManager::forceFullRefresh(EpdDisplay &display) {
    display.fullClear(); // aggressive flash-clear, resets accumulated partial-refresh ghosting
    drawCurrentFull(display);
}
