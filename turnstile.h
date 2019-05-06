#ifndef SRC_TURNSTILE_H_
#define SRC_TURNSTILE_H_

#include <type_traits>

class Turnstile;

class Mutex {
private:
  Turnstile *turnstile;

public:
  Mutex();
  Mutex(const Mutex &) = delete;

  void lock();    // NOLINT
  void unlock();  // NOLINT
};

#endif  // SRC_TURNSTILE_H_
