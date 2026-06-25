#pragma once

#include <cstdio>
#include <cstdlib>

// LADA_ASSERT: debug-only invariant check. Disabled when NDEBUG is set, so it
// must not have side effects that the program relies on.
//
// LADA_ENSURE: always-on invariant check. On failure, prints file:line:message
// to stderr and aborts. Use for contracts that must hold in release builds.
//
// Both macros evaluate `cond` exactly once.

#define LADA_ENSURE(cond, msg)                                              \
  do {                                                                      \
    if (!(cond)) {                                                          \
      std::fprintf(stderr, "%s:%d: LADA_ENSURE failed: %s (cond: %s)\n",    \
                   __FILE__, __LINE__, (msg), #cond);                       \
      std::abort();                                                         \
    }                                                                       \
  } while (0)

#ifdef NDEBUG
#define LADA_ASSERT(cond, msg) ((void)0)
#else
#define LADA_ASSERT(cond, msg) LADA_ENSURE(cond, msg)
#endif
