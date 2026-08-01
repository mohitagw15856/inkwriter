// Sync-mode upload client: pushes drafts to a WebDAV or plain HTTP endpoint
// with conflict-safe snapshot naming (never merges, never overwrites a
// server-side edit: every upload is `<draft>-<UTC timestamp>.md`).
//
// Runs ONLY in sync mode: BLE is fully torn down (BleHid.end()) before Wi-Fi
// starts, per the RAM budget in ARCHITECTURE.md.
#pragma once

#ifdef ARDUINO

#include <string>

namespace inkwriter {

enum class SyncResult : uint8_t {
  Ok,
  NoConfig,
  NoWifi,
  HttpError,
  NothingToSync,
};

const char* syncResultLabel(SyncResult r);

class SyncClient {
 public:
  // Reads ssid/pass/url (and optional user/password for WebDAV basic auth)
  // from /inkwriter/config.txt.
  bool loadConfig();

  // Connects, uploads every draft as a timestamped snapshot, disconnects.
  // `uploaded` reports the number of files pushed.
  // TODO(hardware-test): association, WebDAV servers, TLS memory headroom.
  SyncResult syncAll(int& uploaded);

 private:
  std::string ssid_;
  std::string pass_;
  std::string url_;
  std::string user_;
  std::string password_;
};

}  // namespace inkwriter

#endif  // ARDUINO
