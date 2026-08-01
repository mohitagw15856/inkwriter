// Host unit tests for the InkWriter editor and session core.
#include <cstdio>
#include <string>

#include "core/Editor.h"
#include "core/Session.h"

using namespace inkwriter;

static int g_checks = 0;
#define CHECK(cond)                                                        \
  do {                                                                     \
    ++g_checks;                                                            \
    if (!(cond)) {                                                         \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      return 1;                                                            \
    }                                                                      \
  } while (0)

static int testEditorTyping() {
  Editor e;
  for (char c : std::string("hello world")) e.insertChar(c);
  CHECK(e.text() == "hello world\n");
  CHECK(e.wordCount() == 2);
  e.insertNewline();
  for (char c : std::string("second line")) e.insertChar(c);
  CHECK(e.lines().size() == 2);
  CHECK(e.cursorLine() == 1);
  CHECK(e.wordCount() == 4);
  return 0;
}

static int testBackspaceJoinsLines() {
  Editor e;
  e.setText("one\ntwo\n");
  CHECK(e.lines().size() == 2);
  // Cursor lands at end of doc; move to start of "two" then backspace.
  e.moveHome();
  e.backspace();
  CHECK(e.lines().size() == 1);
  CHECK(e.text() == "onetwo\n");
  CHECK(e.cursorCol() == 3);
  return 0;
}

static int testCursorClamping() {
  Editor e;
  e.setText("long line here\nx\n");
  e.moveUp();
  e.moveEnd();
  CHECK(e.cursorCol() == 14);
  e.moveDown();
  CHECK(e.cursorCol() == 1);  // clamped to "x"
  e.moveLeft();
  e.moveLeft();              // wraps to end of previous line
  CHECK(e.cursorLine() == 0);
  return 0;
}

static int testRoundTripAndParagraph() {
  Editor e;
  e.setText("para one line a\npara one line b\n\npara two\n");
  CHECK(e.text() == "para one line a\npara one line b\n\npara two\n");
  size_t f, l;
  e.currentParagraph(f, l);  // cursor at end -> "para two"
  CHECK(f == 3 && l == 3);
  return 0;
}

static int testWordsAndGoal() {
  CHECK(countWords("") == 0);
  CHECK(countWords("  a  b ") == 2);
  CHECK(goalEighths(0, 500) == 0);
  CHECK(goalEighths(250, 500) == 4);
  CHECK(goalEighths(500, 500) == 8);
  CHECK(goalEighths(9000, 500) == 8);
  CHECK(goalEighths(100, 0) == 0);
  return 0;
}

static int testSprint() {
  Sprint s;
  CHECK(s.remainingSeconds(0) == 0);
  s.start(1000, 60000);
  CHECK(s.running());
  CHECK(s.remainingSeconds(1000) == 60);
  CHECK(s.remainingSeconds(31000) == 30);
  CHECK(s.remainingSeconds(61001) == 0);
  CHECK(!s.running());
  return 0;
}

static int testStreak() {
  Streak s;
  CHECK(s.recordWriting(100) == 1);
  CHECK(s.recordWriting(100) == 1);   // same day, unchanged
  CHECK(s.recordWriting(101) == 2);   // consecutive day
  CHECK(s.recordWriting(105) == 1);   // gap resets
  Streak r = Streak::parse(s.serialise());
  CHECK(r.lastDay == 105 && r.length == 1);
  return 0;
}

int main() {
  if (testEditorTyping()) return 1;
  if (testBackspaceJoinsLines()) return 1;
  if (testCursorClamping()) return 1;
  if (testRoundTripAndParagraph()) return 1;
  if (testWordsAndGoal()) return 1;
  if (testSprint()) return 1;
  if (testStreak()) return 1;
  std::printf("%d checks, 0 failures\n", g_checks);
  return 0;
}
