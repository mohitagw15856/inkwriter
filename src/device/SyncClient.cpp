#include "device/SyncClient.h"

#ifdef ARDUINO

#include <HTTPClient.h>
#include <WiFi.h>

#include <ctime>

#include <inkkit/Storage.h>

#include "device/Config.h"
#include "device/DraftStore.h"

namespace inkwriter {

const char* syncResultLabel(SyncResult r) {
  switch (r) {
    case SyncResult::Ok: return "Drafts synced";
    case SyncResult::NoConfig: return "No sync config (config.txt)";
    case SyncResult::NoWifi: return "No Wi-Fi connection";
    case SyncResult::HttpError: return "Server error";
    case SyncResult::NothingToSync: return "No drafts to sync";
  }
  return "Unknown";
}

bool SyncClient::loadConfig() {
  std::string text;
  if (!inkkit::sd::readWholeFile(kConfigPath, text)) return false;
  ssid_.clear(); pass_.clear(); url_.clear(); user_.clear(); password_.clear();
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find('\n', pos);
    if (end == std::string::npos) end = text.size();
    std::string line = text.substr(pos, end - pos);
    pos = end + 1;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const size_t sp = line.find(' ');
    if (sp == std::string::npos) continue;
    const std::string key = line.substr(0, sp);
    const std::string value = line.substr(sp + 1);
    if (key == "ssid") ssid_ = value;
    else if (key == "pass") pass_ = value;
    else if (key == "url") url_ = value;
    else if (key == "user") user_ = value;
    else if (key == "password") password_ = value;
  }
  return !ssid_.empty() && !url_.empty();
}

SyncResult SyncClient::syncAll(int& uploaded) {
  uploaded = 0;
  if (ssid_.empty() || url_.empty()) return SyncResult::NoConfig;

  DraftStore store;
  const auto drafts = store.listDrafts();
  if (drafts.empty()) return SyncResult::NothingToSync;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_.c_str(), pass_.c_str());
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 15000) {
      WiFi.mode(WIFI_OFF);
      return SyncResult::NoWifi;
    }
    delay(100);
  }

  // Conflict-safe snapshot name suffix: one UTC stamp for this sync run.
  char stamp[20];
  const time_t now = time(nullptr);
  struct tm parts;
  gmtime_r(&now, &parts);
  snprintf(stamp, sizeof(stamp), "%04d%02d%02dT%02d%02d%02d", parts.tm_year + 1900,
           parts.tm_mon + 1, parts.tm_mday, parts.tm_hour, parts.tm_min, parts.tm_sec);

  SyncResult result = SyncResult::Ok;
  for (const auto& name : drafts) {
    std::string body;
    if (!store.load(name, body)) continue;

    const std::string base = name.size() > 3 ? name.substr(0, name.size() - 3) : name;
    const std::string target = url_ + "/" + base + "-" + stamp + ".md";

    HTTPClient http;
    http.setTimeout(10000);
    if (!http.begin(target.c_str())) {
      result = SyncResult::HttpError;
      break;
    }
    if (!user_.empty()) http.setAuthorization(user_.c_str(), password_.c_str());
    http.addHeader("Content-Type", "text/markdown");
    const int code = http.PUT(reinterpret_cast<uint8_t*>(body.data()), body.size());
    http.end();
    if (code >= 200 && code < 300) {
      ++uploaded;
    } else {
      result = SyncResult::HttpError;
      break;
    }
  }

  WiFi.mode(WIFI_OFF);
  return uploaded == 0 && result == SyncResult::Ok ? SyncResult::NothingToSync : result;
}

}  // namespace inkwriter

#endif  // ARDUINO
