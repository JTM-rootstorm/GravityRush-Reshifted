// libSceFios2's scheduler kicks, made to count a kick even when the
// scheduler's queue is empty. Replaces 0x2A150, 0x2A1D0 and 0x2A240
// (manifest/libSceFios2.manifest); FINDINGS.md, "the stop after TITLE:
// first".
//
// A kick tells a scheduler that something its chunks wait for has changed
// (an I/O completed, a psarc buffer was released, the library resumed):
// under the scheduler's mutex it bumps the kick counter (+0x40F0), clears
// the busy hints (busySkip +0x40E8, the busy count +0x40F8) and wakes the
// scheduler's thread. The originals do all of that only when the queue
// (count +0x40D8) is not empty.
//
// But the queue is also empty while the only chunk is being executed:
// ExecuteChunk (0x2A3E0) takes it off the queue, notes the kick counter
// and drops the mutex while the chunk runs. If the chunk answers busy
// (its data not read yet, no free psarc buffer), ExecuteChunk queues it
// again and, when the kick counter has not moved, marks it skipped; a busy
// psarc buffer also sets the busy count, under which the scheduler skips
// the archive's chunks without asking again. A kick that came during the
// run, with the queue empty, changed nothing, so the chunk stays skipped
// and no later kick comes for what already happened: every chunk queued
// behind it on that scheduler waits for good. The same holds for a chunk
// AddToSchedule (0x29BA0) runs at once on the caller's thread.
//
// The window is in the library's own code. Here host file reads complete
// almost at once, and about one start in 50 (four GPU-free runs at once)
// had an I/O completion or a buffer release land in it. Counting every
// kick closes it; the thread is still only woken for a non-empty queue
// (with an empty one it is either running the chunk or has nothing to
// do).

#include <cstdint>

#define RECOMP_LIFT_X64 1
#include "runtime/lifted.h"

namespace recomp::gen {

void f_80002E290(Ctx& c);  // the library's condition signal
void f_80002E610(Ctx& c);  // mutex lock
void f_80002E6B0(Ctx& c);  // mutex unlock

namespace {

using namespace ::recomp::lift;

constexpr uint64_t kSchedulers = 0x800064808;  // list head, next at +8
constexpr uint64_t kDefault = 0x80004CEE0;     // the default scheduler
// Scheduler fields.
constexpr uint64_t kMutex = 0x10, kCond = 0x28, kCount = 0x40D8, kBusySkip = 0x40E8,
                   kKicks = 0x40F0, kBusy = 0x40F8, kShutdown = 0x4115;

// A call as lifted code makes it, returning to the original's call site.
void Call(Ctx& c, void (*f)(Ctx&), uint64_t arg, uint64_t ret) {
  c.rdi = arg;
  c.rsp -= 8;
  Wr<uint64_t>(g_guest_base, c.rsp, ret);
  f(c);
}

// The original's body for one scheduler, with its mutex held; the call
// sites' return addresses are the original's.
void Kick(Ctx& c, uint64_t s, uint64_t signal_ret) {
  uint8_t* const B = g_guest_base;
  Wr<uint32_t>(B, s + kKicks, Rd<uint32_t>(B, s + kKicks) + 1);
  Wr<uint64_t>(B, s + kBusySkip, 0);
  Wr<uint64_t>(B, s + kBusy, 0);
  if (Rd<uint64_t>(B, s + kCount) != 0) Call(c, f_80002E290, s + kCond, signal_ret);
}

// Every scheduler except `except` and, if skip_default, the default one.
void KickAll(Ctx& c, uint64_t except, bool skip_default, uint64_t lock_ret,
             uint64_t signal_ret, uint64_t unlock_ret) {
  uint8_t* const B = g_guest_base;
  for (uint64_t s = Rd<uint64_t>(B, kSchedulers); s; s = Rd<uint64_t>(B, s + 8)) {
    if (s == except || (skip_default && s == kDefault) || Rd<uint8_t>(B, s + kShutdown)) {
      continue;
    }
    Call(c, f_80002E610, s + kMutex, lock_ret);
    Kick(c, s, signal_ret);
    Call(c, f_80002E6B0, s + kMutex, unlock_ret);
  }
  c.rsp += 8;  // ret
}

}  // namespace

// Kicks every scheduler (sceFiosResume and others).
void f_80002A150(Ctx& c) { KickAll(c, 0, false, 0x80002A18E, 0x80002A1BD, 0x80002A1C5); }

// Kicks the scheduler in rdi (I/O completions).
void f_80002A1D0(Ctx& c) {
  uint8_t* const B = g_guest_base;
  const uint64_t s = c.rdi;
  if (Rd<uint8_t>(B, s + kShutdown)) {
    c.rsp += 8;  // ret
    return;
  }
  Call(c, f_80002E610, s + kMutex, 0x80002A1F4);
  Kick(c, s, 0x80002A226);
  c.rdi = s + kMutex;
  LIFT_MUSTTAIL return f_80002E6B0(c);  // the original's tail jump
}

// Kicks every scheduler but the one in rdi and the default one (psarc
// buffer releases).
void f_80002A240(Ctx& c) {
  KickAll(c, c.rdi, true, 0x80002A28F, 0x80002A2BE, 0x80002A2C6);
}

}  // namespace recomp::gen
