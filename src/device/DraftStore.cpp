#include "device/DraftStore.h"

#ifdef ARDUINO

#include <algorithm>
#include <cstdio>

#include <inkkit/Storage.h>

#include "device/Config.h"

namespace inkwriter {

void DraftStore::begin() {
  inkkit::sd::ensureDir(kAppRoot);
  inkkit::sd::ensureDir(kDraftsDir);
}

std::vector<std::string> DraftStore::listDrafts() const {
  std::vector<std::string> names;
  inkkit::sd::listFiles(kDraftsDir, ".md", [&](const std::string& path) {
    const size_t slash = path.find_last_of('/');
    names.push_back(slash == std::string::npos ? path : path.substr(slash + 1));
  });
  std::sort(names.begin(), names.end(), std::greater<std::string>());
  return names;
}

bool DraftStore::load(const std::string& name, std::string& out) const {
  return inkkit::sd::readWholeFile((std::string(kDraftsDir) + "/" + name).c_str(), out);
}

bool DraftStore::save(const std::string& name, const std::string& text) const {
  return inkkit::sd::writeWholeFile((std::string(kDraftsDir) + "/" + name).c_str(), text);
}

std::string DraftStore::newDraftName() const {
  const auto existing = listDrafts();
  for (int n = 1; n < 1000; ++n) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "draft-%03d.md", n);
    if (std::find(existing.begin(), existing.end(), buf) == existing.end()) {
      return buf;
    }
  }
  return "draft-overflow.md";
}

}  // namespace inkwriter

#endif  // ARDUINO
