// InkWriter screens and the two-mode state machine.
//
// WRITING MODE: BLE HID host + display + SD. Wi-Fi is never initialised.
// SYNC MODE:    Wi-Fi + display + SD. Entered only after BleHid.end() has
//               torn the NimBLE stack down and returned its RAM.
// The modes never overlap; see ARCHITECTURE.md for the RAM budget.
#pragma once

#ifdef ARDUINO

#include <string>
#include <vector>

#include <inkkit/Buttons.h>

#include "core/Editor.h"
#include "core/Session.h"
#include "device/DraftStore.h"
#include "device/SyncClient.h"
#include "device/TextRenderer.h"

namespace inkwriter {

class InkWriterApp {
 public:
  InkWriterApp(TextRenderer& tr, inkkit::Buttons& buttons)
      : tr_(tr), buttons_(buttons) {}

  void begin();
  void tick();
  void onBeforeSleep();

 private:
  enum class Screen : uint8_t { Editor, Menu, Pairing, Message };

  void loadSettings();
  void autosaveIfDue();
  void enterSyncMode();  // BLE down -> Wi-Fi up -> upload -> Wi-Fi down -> BLE up
  void handleKeys();

  void renderEditor(bool full);
  void renderMenu(bool full);
  void renderPairing(bool full);
  void renderMessage(const std::string& head, const std::string& detail);

  TextRenderer& tr_;
  inkkit::Buttons& buttons_;
  DraftStore store_;
  SyncClient sync_;

  Screen screen_ = Screen::Menu;
  Editor editor_;
  std::string draftName_;
  int sessionStartWords_ = 0;
  int goalWords_ = 0;
  Sprint sprint_;
  Streak streak_;
  uint32_t sprintMinutes_ = 0;

  std::vector<std::string> menuItems_;
  int menuSel_ = 0;
  int pairSel_ = 0;
  bool focusMode_ = false;
  uint32_t lastEditMs_ = 0;
  uint32_t lastSaveMs_ = 0;
  bool pendingRepaint_ = false;
  int topLine_ = 0;  // first visible editor line (page scroll)
};

}  // namespace inkwriter

#endif  // ARDUINO
