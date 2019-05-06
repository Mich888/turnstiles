#include "turnstile.h"
#include <cassert>
#include <mutex>
#include <queue>
#include <thread>

class Turnstile {
public:
  std::mutex cm;
  std::condition_variable cv;
  bool canIPass;
  int howManySleeps;

  Turnstile() : canIPass(false), howManySleeps(0){};
};

namespace {
  std::mutex Mutexes[256];
  std::mutex guard;
  char dummy = 'd';

  class TurnstilesPool {
  private:
    std::queue<Turnstile *> freeTurnstiles;
    int givenTurnstiles;

  public:
    TurnstilesPool() {
      freeTurnstiles = std::queue<Turnstile *>();
      for (int i = 0; i < 10000; i++) {
        freeTurnstiles.push(new Turnstile);
      }
      givenTurnstiles = 10000;
    }

    void extendQueue() {
      for (int i = 0; i < 100; i++) {
        freeTurnstiles.push(new Turnstile);
      }
      givenTurnstiles += 100;
    }

    void decreaseQueueSize() {
      size_t size = freeTurnstiles.size();
      for (int i = 0; i < size / 2; i++) {
        Turnstile *del = freeTurnstiles.front();
        freeTurnstiles.pop();
        delete (del);
      }
    }

    void giveBackTurnstile(Turnstile *turnstile) {
      guard.lock();
      freeTurnstiles.push(turnstile);
      if (freeTurnstiles.size() > givenTurnstiles * 3 / 4) {
        //decreaseQueueSize();
      }
      guard.unlock();
    }

    Turnstile *getTurnstile() {
      guard.lock();
      Turnstile *ans = freeTurnstiles.front();
      freeTurnstiles.pop();
      if (freeTurnstiles.empty()) {
        extendQueue();
      }
      guard.unlock();
      return ans;
    }

    ~TurnstilesPool() {
      size_t size = freeTurnstiles.size();
      for (int i = 0; i < size; i++) {
        Turnstile *del = freeTurnstiles.front();
        freeTurnstiles.pop();
        delete (del);
      }
    }
  };

  TurnstilesPool &freeTurnstilesPool() {
    static TurnstilesPool ans = TurnstilesPool();
    return ans;
  }
}  // namespace

Mutex::Mutex() { turnstile = nullptr; }

void Mutex::lock() {
  uintptr_t id = reinterpret_cast<uintptr_t>(this) % 256;
  Mutexes[id].lock();
  auto *dummyTurnstile = reinterpret_cast<Turnstile *>(&dummy);

  if (turnstile == nullptr) {  // We are first thread that came.
    turnstile = dummyTurnstile;
    Mutexes[id].unlock();
  } else {
    if (turnstile == dummyTurnstile) {
      turnstile = freeTurnstilesPool().getTurnstile();
    }

    std::unique_lock<std::mutex> lkc(turnstile->cm);
    (turnstile->howManySleeps)++;
    Mutexes[id].unlock();
    turnstile->cv.wait(lkc, [this] { return turnstile->canIPass; });
  }
}

void Mutex::unlock() {
  assert(turnstile != nullptr);
  auto id = reinterpret_cast<uintptr_t>(this) % 256;
  Mutexes[id].lock();

  // There aren't any waiting threads, so we don't have to wake any.
  if (turnstile == reinterpret_cast<Turnstile *>(&dummy)) {
    turnstile = nullptr;
  } else {
    if ((turnstile->howManySleeps) == 0) {  // Give back turnstile to pool.
      freeTurnstilesPool().giveBackTurnstile(turnstile);
      turnstile = nullptr;
    } else {  //  We are waking one thread that is waiting on condition.
      turnstile->canIPass = true;
      (turnstile->howManySleeps)--;
      turnstile->cv.notify_one();
    }
  }
  Mutexes[id].unlock();
}