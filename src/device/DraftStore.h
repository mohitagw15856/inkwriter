// Draft persistence: plain Markdown files under /inkwriter/drafts.
#pragma once

#ifdef ARDUINO

#include <functional>
#include <string>
#include <vector>

namespace inkwriter {

class DraftStore {
 public:
  void begin();

  std::vector<std::string> listDrafts() const;  // filenames, newest name first
  bool load(const std::string& name, std::string& out) const;
  bool save(const std::string& name, const std::string& text) const;

  // A fresh draft name: draft-NNN.md with the first unused number.
  std::string newDraftName() const;
};

}  // namespace inkwriter

#endif  // ARDUINO
