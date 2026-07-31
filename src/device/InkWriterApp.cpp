#include "device/InkWriterApp.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <BleKeyboardHost.h>

#include <ctime>

#include <inkkit/Storage.h>

#include "device/Config.h"

namespace inkwriter {

namespace {

uint32_t dayKey() { return static_cast<uint32_t>(time(nullptr) / 86400); }

}  // namespace

void InkWriterApp::begin() {
  store_.begin();
  loadSettings();

  std::string streakText;
  if (inkkit::sd::readWholeFile(kStreakPath, streakText)) {
    streak_ = Streak::parse(streakText);
  }

  // Writing mode is the boot default: BLE up, Wi-Fi untouched.
  BleHid.begin("InkWriter");

  menuItems_ = {"New draft", "Open draft", "Sync drafts (Wi-Fi)", "Pair keyboard",
                "Toggle focus mode", "Start sprint"};
  renderMenu(true);
}

void InkWriterApp::loadSettings() {
  goalWords_ = kDefaultGoalWords;
  sprintMinutes_ = kDefaultSprintMinutes;
  std::string text;
  if (!inkkit::sd::readWholeFile(kConfigPath, text)) return;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    const std::string line = text.substr(pos, end - pos);
    pos = end + 1;
    const size_t sp = line.find(' ');
    if (sp == std::string::npos) continue;
    const std::string key = line.substr(0, sp);
    const std::string value = line.substr(sp + 1);
    if (key == "goal") goalWords_ = atoi(value.c_str());
    if (key == "sprint_minutes") sprintMinutes_ = strtoul(value.c_str(), nullptr, 10);
  }
}

void InkWriterApp::autosaveIfDue() {
  if (!editor_.dirty() || draftName_.empty()) return;
  if (millis() - lastSaveMs_ < kAutosaveMs) return;
  if (store_.save(draftName_, editor_.text())) {
    editor_.clearDirty();
    lastSaveMs_ = millis();
    const uint32_t len = streak_.recordWriting(dayKey());
    (void)len;
    inkkit::sd::writeWholeFile(kStreakPath, streak_.serialise());
  }
}

void InkWriterApp::enterSyncMode() {
  // The whole point of the mode split: BLE RAM comes back before Wi-Fi asks
  // for its own. TODO(hardware-test): measure free heap across this seam.
  renderMessage("Sync mode", "Stopping Bluetooth...");
  BleHid.end();

  renderMessage("Sync mode", "Connecting to Wi-Fi...");
  int uploaded = 0;
  SyncResult r = SyncResult::NoConfig;
  if (sync_.loadConfig()) {
    r = sync_.syncAll(uploaded);
  }
  std::string detail = syncResultLabel(r);
  if (r == SyncResult::Ok) {
    detail += " (" + std::to_string(uploaded) + " files)";
  }

  // Back to writing mode: Wi-Fi is already off inside syncAll.
  BleHid.begin("InkWriter");
  renderMessage("Sync finished", detail);
}

void InkWriterApp::handleKeys() {
  freeink::KeyEvent ev;
  bool edited = false;
  while (BleHid.popKey(ev)) {
    if (screen_ != Screen::Editor) continue;
    using SK = freeink::SpecialKey;
    switch (ev.special) {
      case SK::None:
        if (ev.ch) {
          editor_.insertChar(ev.ch);
          edited = true;
        }
        break;
      case SK::Enter: editor_.insertNewline(); edited = true; break;
      case SK::Backspace: editor_.backspace(); edited = true; break;
      case SK::Left: editor_.moveLeft(); edited = true; break;
      case SK::Right: editor_.moveRight(); edited = true; break;
      case SK::Up: editor_.moveUp(); edited = true; break;
      case SK::Down: editor_.moveDown(); edited = true; break;
      case SK::Home: editor_.moveHome(); edited = true; break;
      case SK::End: editor_.moveEnd(); edited = true; break;
      case SK::Tab: focusMode_ = !focusMode_; edited = true; break;
      case SK::Escape:
        screen_ = Screen::Menu;
        renderMenu(true);
        return;
      default: break;
    }
  }
  if (edited) {
    lastEditMs_ = millis();
    pendingRepaint_ = true;
  }
}

void InkWriterApp::tick() {
  buttons_.update();
  BleHid.poll();
  handleKeys();
  autosaveIfDue();

  // E-ink pacing: repaint with a fast refresh once keys go quiet for 150 ms
  // rather than per keystroke; page scrolls repaint immediately and fully.
  if (pendingRepaint_ && millis() - lastEditMs_ > 150 && screen_ == Screen::Editor) {
    pendingRepaint_ = false;
    renderEditor(false);
  }

  switch (screen_) {
    case Screen::Editor:
      break;  // keyboard-driven
    case Screen::Menu: {
      if (buttons_.wasPressed(kBtnDown)) {
        menuSel_ = (menuSel_ + 1) % static_cast<int>(menuItems_.size());
        renderMenu(false);
      } else if (buttons_.wasPressed(kBtnUp)) {
        menuSel_ = (menuSel_ + static_cast<int>(menuItems_.size()) - 1) %
                   static_cast<int>(menuItems_.size());
        renderMenu(false);
      } else if (buttons_.wasPressed(kBtnSelect)) {
        switch (menuSel_) {
          case 0:  // New draft
            draftName_ = store_.newDraftName();
            editor_.setText("");
            sessionStartWords_ = 0;
            screen_ = Screen::Editor;
            renderEditor(true);
            break;
          case 1: {  // Open draft (newest; cycle with repeated selects later)
            const auto drafts = store_.listDrafts();
            if (!drafts.empty()) {
              std::string text;
              if (store_.load(drafts.front(), text)) {
                draftName_ = drafts.front();
                editor_.setText(text);
                sessionStartWords_ = editor_.wordCount();
                screen_ = Screen::Editor;
                renderEditor(true);
              }
            }
            break;
          }
          case 2: enterSyncMode(); break;
          case 3:
            screen_ = Screen::Pairing;
            BleHid.startScan(5000);
            renderPairing(true);
            break;
          case 4:
            focusMode_ = !focusMode_;
            renderMenu(false);
            break;
          case 5:
            sprint_.start(millis(), sprintMinutes_ * 60000u);
            screen_ = Screen::Editor;
            renderEditor(true);
            break;
        }
      } else if (buttons_.wasPressed(kBtnBack) && !draftName_.empty()) {
        screen_ = Screen::Editor;
        renderEditor(true);
      }
      break;
    }
    case Screen::Pairing: {
      if (buttons_.wasPressed(kBtnDown) && BleHid.deviceCount() > 0) {
        pairSel_ = (pairSel_ + 1) % BleHid.deviceCount();
        renderPairing(false);
      } else if (buttons_.wasPressed(kBtnUp) && BleHid.deviceCount() > 0) {
        pairSel_ = (pairSel_ + BleHid.deviceCount() - 1) % BleHid.deviceCount();
        renderPairing(false);
      } else if (buttons_.wasPressed(kBtnSelect) && BleHid.deviceCount() > 0) {
        BleHid.stopScan();
        BleHid.connect(BleHid.device(static_cast<uint8_t>(pairSel_)).addr);
        renderMessage("Pairing", "Connecting to keyboard...");
      } else if (buttons_.wasPressed(kBtnBack)) {
        BleHid.stopScan();
        screen_ = Screen::Menu;
        renderMenu(true);
      } else if (!BleHid.isScanning()) {
        renderPairing(false);  // refresh results as the scan settles
      }
      break;
    }
    case Screen::Message: {
      if (BleHid.isConnected()) {
        BleHid.releaseScanResults();
        screen_ = draftName_.empty() ? Screen::Menu : Screen::Editor;
        if (screen_ == Screen::Menu) renderMenu(true); else renderEditor(true);
      } else if (buttons_.wasPressed(kBtnBack) || buttons_.wasPressed(kBtnSelect)) {
        screen_ = Screen::Menu;
        renderMenu(true);
      }
      break;
    }
  }
}

void InkWriterApp::onBeforeSleep() {
  if (editor_.dirty() && !draftName_.empty()) {
    store_.save(draftName_, editor_.text());
  }
  BleHid.end();
}

void InkWriterApp::renderEditor(bool full) {
  tr_.clear();
  const int lineH = tr_.lineHeight();
  const int cols = tr_.columnsFor(tr_.width() - 2 * kMarginX);
  const int pageLines = (tr_.height() - 2 * lineH - 12) / lineH;

  size_t firstVisible = 0;
  size_t lastVisible = editor_.lines().size();
  if (focusMode_) {
    size_t f, l;
    editor_.currentParagraph(f, l);
    firstVisible = f;
    lastVisible = l + 1;
  }

  // Keep the cursor on screen; page scrolls get a full refresh.
  const int cursor = static_cast<int>(editor_.cursorLine());
  if (cursor < topLine_ || cursor >= topLine_ + pageLines) {
    topLine_ = cursor - pageLines / 2;
    if (topLine_ < 0) topLine_ = 0;
    full = true;
  }

  int y = kMarginTop;
  const auto& lines = editor_.lines();
  for (size_t i = static_cast<size_t>(topLine_); i < lines.size(); ++i) {
    if (focusMode_ && (i < firstVisible || i >= lastVisible)) continue;
    if (y + lineH > tr_.height() - 2 * lineH) break;
    std::string shown = lines[i];
    if (static_cast<int>(shown.size()) > cols) shown.resize(static_cast<size_t>(cols));
    if (i == editor_.cursorLine()) {
      // Cursor as an underscore overlay at the column position.
      std::string withCursor = shown;
      const size_t col = editor_.cursorCol() > withCursor.size() ? withCursor.size()
                                                                 : editor_.cursorCol();
      withCursor.insert(col, "_");
      if (static_cast<int>(withCursor.size()) > cols) withCursor.resize(static_cast<size_t>(cols));
      tr_.text(kMarginX, y, withCursor);
    } else {
      tr_.text(kMarginX, y, shown);
    }
    y += lineH;
  }

  // Status bar: words, goal eighths, sprint, streak, connection.
  const int words = editor_.wordCount();
  const int sessionWords = words - sessionStartWords_;
  const int eighths = goalEighths(sessionWords, goalWords_);
  std::string bar = std::to_string(words) + "w ";
  bar += "[";
  for (int i = 0; i < 8; ++i) bar += (i < eighths) ? '#' : '.';
  bar += "]";
  if (sprint_.running()) {
    const uint32_t rem = sprint_.remainingSeconds(millis());
    char t[16];
    snprintf(t, sizeof(t), " %lu:%02lu", static_cast<unsigned long>(rem / 60),
             static_cast<unsigned long>(rem % 60));
    bar += t;
  }
  bar += " streak " + std::to_string(streak_.length) + "d";
  bar += BleHid.isConnected() ? " [kbd]" : " [no kbd]";
  if (focusMode_) bar += " [focus]";
  tr_.textInverted(kMarginX, tr_.height() - tr_.lineHeight() - 6, bar,
                   tr_.lineHeight() + 6);
  tr_.flush(full);
}

void InkWriterApp::renderMenu(bool full) {
  tr_.clear();
  tr_.textInverted(kMarginX, kMarginTop, "InkWriter  (SELECT opens, BACK returns)",
                   tr_.lineHeight() + 6);
  int y = 2 * tr_.lineHeight() + 10;
  for (size_t i = 0; i < menuItems_.size(); ++i) {
    const std::string marker = (static_cast<int>(i) == menuSel_) ? "> " : "  ";
    tr_.text(kMarginX, y, marker + menuItems_[i]);
    y += tr_.lineHeight();
  }
  y += tr_.lineHeight();
  tr_.text(kMarginX, y,
           BleHid.isConnected() ? std::string("Keyboard: ") + BleHid.connectedName()
                                : std::string("Keyboard: not connected"));
  tr_.flush(full);
}

void InkWriterApp::renderPairing(bool full) {
  tr_.clear();
  tr_.textInverted(kMarginX, kMarginTop, "Pair keyboard  (scanning...)",
                   tr_.lineHeight() + 6);
  int y = 2 * tr_.lineHeight() + 10;
  if (BleHid.deviceCount() == 0) {
    tr_.text(kMarginX, y, BleHid.isScanning() ? "Scanning..." : "No devices found. BACK retries.");
  }
  for (uint8_t i = 0; i < BleHid.deviceCount(); ++i) {
    const auto& d = BleHid.device(i);
    const std::string marker = (i == pairSel_) ? "> " : "  ";
    std::string label = d.name;
    if (d.hid) label += " [HID]";
    tr_.text(kMarginX, y, marker + label);
    y += tr_.lineHeight();
  }
  tr_.flush(full);
}

void InkWriterApp::renderMessage(const std::string& head, const std::string& detail) {
  screen_ = Screen::Message;
  tr_.clear();
  tr_.textInverted(kMarginX, kMarginTop, head, tr_.lineHeight() + 6);
  tr_.text(kMarginX, tr_.height() / 2, detail);
  tr_.flush(true);
}

}  // namespace inkwriter

#endif  // ARDUINO
