#ifndef DEBUG_HPP
#define DEBUG_HPP

namespace dbg {
static const char *RESET = "\033[0m";
static const char *BOLD = "\033[1m";

static const char *RED = "\033[31m";
static const char *GREEN = "\033[32m";
static const char *YELLOW = "\033[33m";
static const char *BLUE = "\033[34m";
//static const char *MAGENTA = "\033[35m";
static const char *CYAN = "\033[36m";
//static const char *GRAY = "\033[90m";

static const int KEY_WIDTH = 20;
} // namespace dbg

#endif // !DEBUG_HPP
