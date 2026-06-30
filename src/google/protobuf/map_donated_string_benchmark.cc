// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// ============================================================================
// ARENASTRING PATCH v2.1 — Map DonatedString micro-benchmark (§7.2 / §7.3).
//
// A self-contained benchmark (no external benchmark library) that quantifies
// the benefit of arena-backed Donated std::string keys/values in protobuf map
// fields. Heap allocations are counted by overriding the global operator
// new/delete and toggling a counter only around the measured region (single
// threaded on purpose). Latency uses steady_clock; arena footprint uses
// Arena::SpaceUsed.
//
// Scenarios (§7.2):
//   [Parse ms]   map<string,string>  — both key & value Donated
//   [Parse mi]   map<int32,string>   — value Donated
//   [Parse mk]   map<string,int32>   — key Donated
//   [Parse dup]  map<string,string> with duplicate keys on the wire
//   [Insert]     try_emplace into no-arena vs arena Map
//   [Write]      first write via operator[] (promote) vs mutable_*_accessor
//   [Swap]       same-arena (O(1)) vs cross-arena (deep copy)
//
// Targets (§7.3): parse malloc down >=80%, parse latency down >=30%,
//                 accessor vs operator[] >= 2x on first write.
//
// Build & run:
//   bazel run -c opt //src/google/protobuf:map_donated_string_benchmark
//   bazel run -c opt //src/google/protobuf:map_donated_string_benchmark -- \
//       --entries=5000 --value_len=128 --iters=100
// ============================================================================

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/map.h"
#include "google/protobuf/unittest_proto3_arenastring_mutable.pb.h"

// ---------------------------------------------------------------------------
// Global allocation counter. Only counts while g_count_enabled is true.
// ---------------------------------------------------------------------------
namespace {
bool g_count_enabled = false;
std::atomic<std::size_t> g_alloc_calls{0};
std::atomic<std::size_t> g_alloc_bytes{0};

inline void CountAlloc(std::size_t n) {
  if (g_count_enabled) {
    g_alloc_calls.fetch_add(1, std::memory_order_relaxed);
    g_alloc_bytes.fetch_add(n, std::memory_order_relaxed);
  }
}

void ResetCounters() {
  g_alloc_calls.store(0, std::memory_order_relaxed);
  g_alloc_bytes.store(0, std::memory_order_relaxed);
}
}  // namespace

void* operator new(std::size_t n) {
  CountAlloc(n);
  void* p = std::malloc(n == 0 ? 1 : n);
  if (p == nullptr) throw std::bad_alloc();
  return p;
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

#if defined(__cpp_aligned_new) && __cpp_aligned_new >= 201606L
void* operator new(std::size_t n, std::align_val_t a) {
  CountAlloc(n);
  void* p = nullptr;
  std::size_t align = std::max(sizeof(void*), static_cast<std::size_t>(a));
  if (posix_memalign(&p, align, n == 0 ? align : n) != 0) throw std::bad_alloc();
  return p;
}
void* operator new[](std::size_t n, std::align_val_t a) {
  return ::operator new(n, a);
}
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept {
  std::free(p);
}
#endif  // __cpp_aligned_new

namespace {

using ::google::protobuf::Arena;
using ::proto3_arenastring_unittest::ArenaProto3;
using Clock = std::chrono::steady_clock;
using StrMap = ::google::protobuf::Map<std::string, std::string>;

double NanosSince(Clock::time_point t0) {
  return std::chrono::duration<double, std::nano>(Clock::now() - t0).count();
}

struct Config {
  int entries = 2000;
  int value_len = 80;  // payload length (bytes), kept > SSO to force a buffer
  int iters = 50;
};

// Pre-built, reusable inputs (constructed OUTSIDE every measured region).
struct Inputs {
  std::vector<std::string> keys;
  std::string value;
  std::string ser_ms;   // map<string,string> wire bytes
  std::string ser_mi;   // map<int32,string> wire bytes
  std::string ser_mk;   // map<string,int32> wire bytes
  std::string ser_dup;  // map<string,string> with each key duplicated on wire
};

Inputs BuildInputs(const Config& cfg) {
  Inputs in;
  in.keys.reserve(cfg.entries);
  for (int i = 0; i < cfg.entries; ++i) {
    in.keys.push_back(absl::StrCat("key_", i));
  }
  in.value.assign(static_cast<std::size_t>(cfg.value_len), 'x');

  {
    ArenaProto3 src;
    auto* m = src.mutable_ms();
    for (int i = 0; i < cfg.entries; ++i) (*m)[in.keys[i]] = in.value;
    src.SerializeToString(&in.ser_ms);
  }
  // Duplicate every key on the wire (forces InsertOrReplaceNode on parse).
  in.ser_dup = in.ser_ms + in.ser_ms;
  {
    ArenaProto3 src;
    auto* m = src.mutable_mi();
    for (int i = 0; i < cfg.entries; ++i) (*m)[i] = in.value;
    src.SerializeToString(&in.ser_mi);
  }
  {
    ArenaProto3 src;
    auto* m = src.mutable_mk();
    for (int i = 0; i < cfg.entries; ++i) (*m)[in.keys[i]] = i;
    src.SerializeToString(&in.ser_mk);
  }
  return in;
}

struct Metric {
  std::size_t calls = 0;
  std::size_t bytes = 0;
  double ns_per_op = 0.0;
};

std::size_t Sink(const ArenaProto3& m) {
  return m.ms().size() + m.mi().size() + m.mk().size();
}

// --------------------------- [Parse] --------------------------------------
Metric BenchParseHeap(const std::string& data, int iters) {
  ResetCounters();
  std::size_t sink = 0;
  g_count_enabled = true;
  auto t0 = Clock::now();
  for (int it = 0; it < iters; ++it) {
    ArenaProto3 m;  // heap-backed: each string key/value gets its own malloc
    m.ParseFromString(data);
    sink += Sink(m);
  }
  double ns = NanosSince(t0);
  g_count_enabled = false;
  Metric r;
  r.calls = g_alloc_calls.load();
  r.bytes = g_alloc_bytes.load();
  r.ns_per_op = ns / iters;
  if (sink == 0xdeadbeef) std::printf(" ");  // defeat dead-code elimination
  return r;
}

Metric BenchParseArena(const std::string& data, int iters,
                       std::size_t* arena_used) {
  ResetCounters();
  std::size_t sink = 0;
  g_count_enabled = true;
  auto t0 = Clock::now();
  for (int it = 0; it < iters; ++it) {
    Arena arena;
    auto* m = Arena::CreateMessage<ArenaProto3>(&arena);
    m->ParseFromString(data);
    sink += Sink(*m);
  }
  double ns = NanosSince(t0);
  g_count_enabled = false;
  Metric r;
  r.calls = g_alloc_calls.load();
  r.bytes = g_alloc_bytes.load();
  r.ns_per_op = ns / iters;
  if (sink == 0xdeadbeef) std::printf(" ");
  if (arena_used != nullptr) {
    Arena arena;
    auto* m = Arena::CreateMessage<ArenaProto3>(&arena);
    m->ParseFromString(data);
    *arena_used = arena.SpaceUsed();
  }
  return r;
}

// --------------------------- [Insert] -------------------------------------
Metric BenchInsertHeap(const Inputs& in, int iters) {
  ResetCounters();
  g_count_enabled = true;
  auto t0 = Clock::now();
  for (int it = 0; it < iters; ++it) {
    StrMap m;  // no arena
    for (const auto& k : in.keys) m.try_emplace(k, in.value);
  }
  double ns = NanosSince(t0);
  g_count_enabled = false;
  Metric r{g_alloc_calls.load(), g_alloc_bytes.load(), ns / iters};
  return r;
}

Metric BenchInsertArena(const Inputs& in, int iters) {
  ResetCounters();
  g_count_enabled = true;
  auto t0 = Clock::now();
  for (int it = 0; it < iters; ++it) {
    Arena arena;
    auto* m = Arena::Create<StrMap>(&arena);
    for (const auto& k : in.keys) m->try_emplace(k, in.value);
  }
  double ns = NanosSince(t0);
  g_count_enabled = false;
  Metric r{g_alloc_calls.load(), g_alloc_bytes.load(), ns / iters};
  return r;
}

// --------------------------- [Write] --------------------------------------
// First mutable write to every (Donated) value of a freshly parsed arena map.
// Parse happens OUTSIDE the timed/counted region to isolate the write cost.
Metric BenchFirstWriteBracket(const Inputs& in, int iters) {
  ResetCounters();
  double total_ns = 0.0;
  for (int it = 0; it < iters; ++it) {
    Arena arena;
    auto* m = Arena::CreateMessage<ArenaProto3>(&arena);
    m->ParseFromString(in.ser_ms);  // values are Donated here
    auto* mm = m->mutable_ms();
    g_count_enabled = true;
    auto t0 = Clock::now();
    for (const auto& k : in.keys) (*mm)[k] = in.value;  // promote + assign
    total_ns += NanosSince(t0);
    g_count_enabled = false;
  }
  Metric r{g_alloc_calls.load(), g_alloc_bytes.load(), total_ns / iters};
  return r;
}

Metric BenchFirstWriteAccessor(const Inputs& in, int iters) {
  ResetCounters();
  double total_ns = 0.0;
  for (int it = 0; it < iters; ++it) {
    Arena arena;
    auto* m = Arena::CreateMessage<ArenaProto3>(&arena);
    m->ParseFromString(in.ser_ms);  // values are Donated here
    g_count_enabled = true;
    auto t0 = Clock::now();
    for (const auto& k : in.keys) {
      m->mutable_ms_accessor(k).assign(in.value.data(), in.value.size());
    }
    total_ns += NanosSince(t0);
    g_count_enabled = false;
  }
  Metric r{g_alloc_calls.load(), g_alloc_bytes.load(), total_ns / iters};
  return r;
}

// --------------------------- [Swap] ---------------------------------------
// Same-arena swap is an O(1) representation swap; cross-arena swap forces a
// deep copy. We give same-arena far more iterations since each op is tiny.
double BenchSwapSameArena(const Inputs& in, int iters) {
  Arena arena;
  auto* a = Arena::Create<StrMap>(&arena);
  auto* b = Arena::Create<StrMap>(&arena);
  for (const auto& k : in.keys) a->try_emplace(k, in.value);
  auto t0 = Clock::now();
  for (int i = 0; i < iters; ++i) a->swap(*b);  // content ping-pongs a<->b
  return NanosSince(t0) / iters;
}

double BenchSwapCrossArena(const Inputs& in, int iters) {
  double total = 0.0;
  for (int i = 0; i < iters; ++i) {
    Arena a1, a2;  // fresh per iteration so deep copies don't accumulate
    auto* a = Arena::Create<StrMap>(&a1);
    auto* b = Arena::Create<StrMap>(&a2);
    for (const auto& k : in.keys) a->try_emplace(k, in.value);
    auto t0 = Clock::now();
    a->swap(*b);  // different arenas -> deep copy
    total += NanosSince(t0);
  }
  return total / iters;
}

double Reduction(double base, double improved) {
  if (base <= 0.0) return 0.0;
  return (base - improved) / base * 100.0;
}

void PrintMetric(const char* label, const Metric& m) {
  std::printf("  %-18s mallocs=%-10zu bytes=%-12zu %.1f ns/op\n", label,
              m.calls, m.bytes, m.ns_per_op);
}

void ReportParse(const char* title, const std::string& data, int iters,
                 bool show_arena_used) {
  std::size_t arena_used = 0;
  Metric heap = BenchParseHeap(data, iters);
  Metric arena =
      BenchParseArena(data, iters, show_arena_used ? &arena_used : nullptr);
  std::printf("[%s]  (wire %zu B)\n", title, data.size());
  PrintMetric("heap", heap);
  PrintMetric("arena+donated", arena);
  if (show_arena_used) {
    std::printf("  arena SpaceUsed (1 msg) = %zu bytes\n", arena_used);
  }
  std::printf("  malloc reduction = %.1f%%   latency reduction = %.1f%%\n\n",
              Reduction(heap.calls, arena.calls),
              Reduction(heap.ns_per_op, arena.ns_per_op));
}

}  // namespace

int main(int argc, char** argv) {
  Config cfg;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto eq = a.find('=');
    if (eq == std::string::npos) continue;
    const std::string key = a.substr(0, eq);
    const int val = std::atoi(a.c_str() + eq + 1);
    if (key == "--entries" && val > 0) cfg.entries = val;
    else if (key == "--value_len" && val > 0) cfg.value_len = val;
    else if (key == "--iters" && val > 0) cfg.iters = val;
  }

  const Inputs in = BuildInputs(cfg);

  std::printf("=== protobuf map DonatedString benchmark ===\n");
  std::printf("entries=%d  value_len=%d  iters=%d\n\n", cfg.entries,
              cfg.value_len, cfg.iters);

  // [Parse] across key/value type combinations.
  ReportParse("Parse map<string,string>", in.ser_ms, cfg.iters, true);
  ReportParse("Parse map<int32,string> (value donated)", in.ser_mi, cfg.iters,
              false);
  ReportParse("Parse map<string,int32> (key donated)", in.ser_mk, cfg.iters,
              false);
  ReportParse("Parse map<string,string> duplicate-keys", in.ser_dup, cfg.iters,
              false);

  // [Insert]
  {
    Metric heap = BenchInsertHeap(in, cfg.iters);
    Metric arena = BenchInsertArena(in, cfg.iters);
    std::printf("[Insert try_emplace map<string,string>]\n");
    PrintMetric("no-arena", heap);
    PrintMetric("arena+donated", arena);
    std::printf("  malloc reduction = %.1f%%   latency reduction = %.1f%%\n\n",
                Reduction(heap.calls, arena.calls),
                Reduction(heap.ns_per_op, arena.ns_per_op));
  }

  // [Write]
  {
    Metric br = BenchFirstWriteBracket(in, cfg.iters);
    Metric ac = BenchFirstWriteAccessor(in, cfg.iters);
    std::printf("[First write to Donated values]\n");
    PrintMetric("operator[]", br);
    PrintMetric("mutable_*_accessor", ac);
    const double speedup =
        ac.ns_per_op > 0.0 ? br.ns_per_op / ac.ns_per_op : 0.0;
    std::printf("  malloc reduction = %.1f%%   accessor speedup = %.2fx\n\n",
                Reduction(br.calls, ac.calls), speedup);
  }

  // [Swap]
  {
    const int same_iters = cfg.iters * 200;  // O(1) op, needs more samples
    double same = BenchSwapSameArena(in, same_iters);
    double cross = BenchSwapCrossArena(in, cfg.iters);
    std::printf("[Swap map<string,string>]\n");
    std::printf("  same-arena  (O(1))      %.1f ns/op\n", same);
    std::printf("  cross-arena (deep copy) %.1f ns/op\n", cross);
    std::printf("  cross/same ratio = %.1fx\n\n",
                same > 0.0 ? cross / same : 0.0);
  }

  std::printf("Done.\n");
  return 0;
}
