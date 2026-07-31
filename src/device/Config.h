// Device-side configuration: SD layout, sync settings, renderer metrics.
#pragma once

#include <cstdint>

namespace inkwriter {

constexpr const char* kAppRoot = "/inkwriter";
constexpr const char* kDraftsDir = "/inkwriter/drafts";
constexpr const char* kConfigPath = "/inkwriter/config.txt";
constexpr const char* kStreakPath = "/inkwriter/streak.txt";
constexpr const char* kTag = "IW";

// Session defaults; the config file can override goal and sprint minutes.
constexpr int kDefaultGoalWords = 500;
constexpr uint32_t kDefaultSprintMinutes = 15;
constexpr uint32_t kAutosaveMs = 5000;

// Physical buttons (ecosystem convention; TODO(hardware-test): confirm).
constexpr uint8_t kBtnSelect = 1;
constexpr uint8_t kBtnBack = 0;
constexpr uint8_t kBtnUp = 4;
constexpr uint8_t kBtnDown = 5;

// Text renderer metrics (shared with the ecosystem 5x7 font renderer).
constexpr int kMarginX = 6;
constexpr int kMarginTop = 6;
constexpr int kLineGap = 2;   // extra pixels between text lines
constexpr int kGlyphGap = 1;  // extra pixels between glyphs

}  // namespace inkwriter
