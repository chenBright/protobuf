#pragma once

// Feature check macro
#define GOOGLE_PROTOBUF_HAS_DONATED_STRING 1

#include "google/protobuf/arena.h"
#if defined(__has_include) && __has_include("google/protobuf/config.h")
#include "google/protobuf/config.h"
#endif

#include "absl/strings/internal/resize_uninitialized.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

namespace internal {

#if __GLIBCXX__
#if _GLIBCXX_USE_CXX11_ABI
struct StdStringRep {
  char* data;
  uint64_t size;
  union {
    uint64_t capacity;
    char local[16];
  };
};
#else                  // !_GLIBCXX_USE_CXX11_ABI
struct StdStringRep {
  uint64_t size;
  uint64_t capacity;
  int32_t refcount;
  uint32_t gap;
  char data[0];
};
#endif                 // !_GLIBCXX_USE_CXX11_ABI
#elif _LIBCPP_VERSION  // && !__GLIBCXX__
#if _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT
static_assert(false, "don not support _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT yet");
#endif  // _LIBCPP_ABI_ALTERNATE_STRING_LAYOUT
#if _LIBCPP_BIG_ENDIAN
static_assert(false, "don not support _LIBCPP_BIG_ENDIAN yet");
#endif  // _LIBCPP_BIG_ENDIAN
union StdStringRep {
  struct {
    typename ::std::string::size_type capacity;
    typename ::std::string::size_type size;
    typename ::std::string::pointer data;
  } long_format;
  struct {
    uint8_t size;
    typename ::std::string::value_type data[0];
  } shot_format;

  inline bool is_long() const noexcept { return shot_format.size & 0x01; }

  inline ::std::string::size_type long_capacity() const noexcept {
    return long_format.capacity & ~static_cast<::std::string::size_type>(0x01);
  }
};
#endif  // _LIBCPP_VERSION && !__GLIBCXX__
}  // namespace internal

// Wrap a full arenastring pointer and arena it belongs to.
// provide function make it look like a string*
//
// Full arenastring itself and it's dynamic content both placed on arena
class ArenaStringAccessor {
 public:
  using value_type = ::std::string::value_type;
  using traits_type = ::std::string::traits_type;
  using allocator_type = ::std::string::allocator_type;
  using size_type = ::std::string::size_type;
  using difference_type = ::std::string::difference_type;
  using reference = ::std::string::reference;
  using const_reference = ::std::string::const_reference;
  using pointer = ::std::string::pointer;
  using const_pointer = ::std::string::const_pointer;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = ::std::reverse_iterator<iterator>;
  using const_reverse_iterator = ::std::reverse_iterator<const_iterator>;

  using StdStringRep = internal::StdStringRep;

  // Disable default constructor and copy constructor
  ArenaStringAccessor() = delete;
  inline ArenaStringAccessor(ArenaStringAccessor&&) noexcept = default;
  inline ArenaStringAccessor(const ArenaStringAccessor&) noexcept = default;
  ArenaStringAccessor& operator=(ArenaStringAccessor&&) = delete;
  ArenaStringAccessor& operator=(const ArenaStringAccessor&) = delete;
  ~ArenaStringAccessor() noexcept = default;

  // Assign
  inline ArenaStringAccessor& operator=(::absl::string_view other) noexcept {
    return assign(other.data(), other.size());
  }
  inline ArenaStringAccessor& assign(::absl::string_view other) noexcept {
    return assign(other.data(), other.size());
  }
  ArenaStringAccessor& assign(const_pointer data, size_type size) noexcept;

  // Element access
  inline reference operator[](size_type position) noexcept {
    return writable_buffer()[position];
  }
  inline const_reference operator[](size_type position) const noexcept {
    return data()[position];
  }
  inline const_pointer data() const noexcept { return c_str(); }
  inline const_pointer c_str() const noexcept {
    return static_cast<const ::std::string*>(_ptr)->c_str();
  }
  inline operator ::absl::string_view() const noexcept {
    return ::absl::string_view(data(), size());
  }
  inline operator const ::std::string&() const noexcept { return *_ptr; }

  // Iterators
  inline iterator begin() noexcept { return iterator(writable_buffer()); }
  inline const_iterator cbegin() const noexcept {
    return const_iterator(data());
  }
  inline const_iterator end() noexcept {
    return iterator(writable_buffer() + size());
  }
  inline const_iterator cend() const noexcept {
    return const_iterator(data() + size());
  }

  // Capacity
  inline bool empty() const noexcept { return _ptr->empty(); }
  inline size_type size() const noexcept { return _ptr->size(); }
  void reserve(size_type required_capacity) noexcept;
  inline size_type capacity() const noexcept { return _ptr->capacity(); }

  // Modifiers
  inline void clear() noexcept { set_size_and_terminator(0); }
  void push_back(value_type c) noexcept;
  inline ArenaStringAccessor& append(::absl::string_view sv) noexcept {
    return append(sv.data(), sv.size());
  }
  ArenaStringAccessor& append(const_pointer append_data,
                              size_type append_size) noexcept;
  inline ArenaStringAccessor& operator+=(char ch) noexcept {
    push_back(ch);
    return *this;
  }
  inline ArenaStringAccessor& operator+=(::absl::string_view sv) noexcept {
    return append(sv.data(), sv.size());
  }
  void resize(size_type new_size) noexcept { resize(new_size, '\0'); }
  void resize(size_type new_size, value_type c) noexcept;
  void swap(ArenaStringAccessor other) noexcept;

  // Operations
  inline int compare(::absl::string_view other) const noexcept {
    return static_cast<::absl::string_view>(*this).compare(other);
  }

  ////////////////////////////////////////////////////////////////////////////
  // Special function
  inline static ArenaStringAccessor create(Arena* arena) noexcept {
    auto* ptr = reinterpret_cast<::std::string*>(
        arena->AllocateAligned(sizeof(::std::string)));
    new (ptr)::std::string();
    return ArenaStringAccessor(arena, ptr);
  }

  template <typename T>
  inline static ArenaStringAccessor create(Arena* arena, T&& value) noexcept {
    return create(arena) = ::std::forward<T>(value);
  }

  // Clear function dont need arena
  inline static void clear(::std::string* ptr) noexcept {
    ArenaStringAccessor(nullptr, ptr).clear();
  }

  // Wrap function don't need arena
  // but left and right must both on same arena
  inline static void swap(::std::string* left, ::std::string* right) noexcept {
    ArenaStringAccessor(nullptr, left)
        .swap(ArenaStringAccessor(nullptr, right));
  }

  // Wrapper construct
  inline ArenaStringAccessor(Arena* arena, ::std::string* ptr) noexcept
      : _arena(arena), _ptr(ptr) {}
  inline Arena* arena() const noexcept { return _arena; }
  inline ::std::string* underlying() const noexcept { return _ptr; }

  // Support absl::strings_internal::STLStringResizeUninitialized
  inline char* __resize_default_init(size_type new_size) noexcept {
    auto buffer = qualified_buffer(new_size);
    set_size_and_terminator(new_size);
    return buffer;
  }

  // Also support absl::Format(ArenaStringAccessor, ...)
  inline operator ::absl::FormatRawSink() noexcept {
    return ::absl::FormatRawSink(this);
  }
  ////////////////////////////////////////////////////////////////////////////

 protected:
  StdStringRep& representation() noexcept;

  pointer recreate_buffer(size_type capacity) noexcept;

  pointer writable_buffer() noexcept;

  inline pointer qualified_buffer(size_type required_capacity,
                                  size_type predict_capacity) noexcept {
    return required_capacity <= capacity() ? writable_buffer()
                                           : recreate_buffer(predict_capacity);
  }

  inline pointer qualified_buffer(size_type required_capacity) noexcept {
    return qualified_buffer(required_capacity, required_capacity);
  }

  void set_size(size_type size) noexcept;

  void set_size_and_terminator(size_type size) noexcept;

 private:
  // Support absl::Format(ArenaStringAccessor*, ...)
  friend inline void AbslFormatFlush(ArenaStringAccessor* accessor,
                                     ::absl::string_view sv) noexcept {
    accessor->append(sv.data(), sv.size());
  }

  Arena* _arena;
  ::std::string* _ptr;
};

inline bool operator==(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return *left.underlying() == *right.underlying();
}

inline bool operator==(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return left == *right.underlying();
}

inline bool operator==(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return *left.underlying() == right;
}

inline bool operator!=(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return !(left == right);
}

inline bool operator!=(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return !(left == right);
}

inline bool operator!=(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return !(left == right);
}

inline bool operator<(const ArenaStringAccessor& left,
                      const ArenaStringAccessor& right) noexcept {
  return *left.underlying() < *right.underlying();
}

inline bool operator<(::absl::string_view left,
                      const ArenaStringAccessor& right) noexcept {
  return left < *right.underlying();
}

inline bool operator<(const ArenaStringAccessor& left,
                      ::absl::string_view right) noexcept {
  return *left.underlying() < right;
}

inline bool operator<=(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return *left.underlying() <= *right.underlying();
}

inline bool operator<=(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return left <= *right.underlying();
}

inline bool operator<=(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return *left.underlying() <= right;
}

inline bool operator>(const ArenaStringAccessor& left,
                      const ArenaStringAccessor& right) noexcept {
  return *left.underlying() > *right.underlying();
}

inline bool operator>(::absl::string_view left,
                      const ArenaStringAccessor& right) noexcept {
  return left > *right.underlying();
}

inline bool operator>(const ArenaStringAccessor& left,
                      ::absl::string_view right) noexcept {
  return *left.underlying() > right;
}

inline bool operator>=(const ArenaStringAccessor& left,
                       const ArenaStringAccessor& right) noexcept {
  return *left.underlying() >= *right.underlying();
}

inline bool operator>=(::absl::string_view left,
                       const ArenaStringAccessor& right) noexcept {
  return left >= *right.underlying();
}

inline bool operator>=(const ArenaStringAccessor& left,
                       ::absl::string_view right) noexcept {
  return *left.underlying() >= right;
}

class MaybeArenaStringAccessor : public ArenaStringAccessor {
 public:
  using ArenaStringAccessor::ArenaStringAccessor;

  MaybeArenaStringAccessor(const ArenaStringAccessor& other) noexcept
      : ArenaStringAccessor(other) {}

  // Assign
  template <typename T>
  inline MaybeArenaStringAccessor& operator=(T&& other) {
    return assign(::std::forward<T>(other));
  }
  template <typename T>
  inline MaybeArenaStringAccessor& assign(T&& other) {
    ::absl::string_view sv(::std::forward<T>(other));
    return assign(sv.data(), sv.size());
  }
  inline MaybeArenaStringAccessor& assign(const_pointer data, size_type size) {
    if (arena() != nullptr) {
      ArenaStringAccessor::assign(data, size);
    } else {
      underlying()->assign(data, size);
    }
    return *this;
  }
  // Deal with assign string specially. Try to keep copy on write state when
  // using old abi
  inline MaybeArenaStringAccessor& operator=(const ::std::string& other) {
    return assign(other);
  }
  inline MaybeArenaStringAccessor& assign(const ::std::string& other) {
    if (arena() != nullptr) {
      ArenaStringAccessor::assign(other);
    } else {
      underlying()->assign(other);
    }
    return *this;
  }
  inline MaybeArenaStringAccessor& operator=(::std::string& other) {
    return assign(static_cast<const ::std::string&>(other));
  }
  inline MaybeArenaStringAccessor& assign(::std::string& other) {
    return assign(static_cast<const ::std::string&>(other));
  }
  inline MaybeArenaStringAccessor& operator=(::std::string&& other) {
    return assign(::std::move(other));
  }
  inline MaybeArenaStringAccessor& assign(::std::string&& other) {
    if (arena() != nullptr) {
      ArenaStringAccessor::assign(other);
    } else {
      underlying()->assign(::std::move(other));
    }
    return *this;
  }
  template <typename T>
  inline MaybeArenaStringAccessor& operator=(
      ::std::reference_wrapper<T> other) {
    return assign(other);
  }
  template <typename T>
  inline MaybeArenaStringAccessor& assign(::std::reference_wrapper<T> other) {
    return assign(other.get());
  }

  inline void reserve(size_type required_capacity) {
    if (arena() != nullptr) {
      ArenaStringAccessor::reserve(required_capacity);
    } else if (required_capacity > capacity()) {
      underlying()->reserve(required_capacity);
    }
  }

  void clear() noexcept;

  inline void push_back(value_type c) {
    if (arena() != nullptr) {
      ArenaStringAccessor::push_back(c);
    } else {
      underlying()->push_back(c);
    }
  }

  inline MaybeArenaStringAccessor& append(::absl::string_view sv) {
    return append(sv.data(), sv.size());
  }

  inline MaybeArenaStringAccessor& append(const_pointer data, size_type size) {
    if (arena() != nullptr) {
      ArenaStringAccessor::append(data, size);
    } else {
      underlying()->append(data, size);
    }
    return *this;
  }

  inline MaybeArenaStringAccessor& operator+=(char ch) noexcept {
    push_back(ch);
    return *this;
  }

  inline MaybeArenaStringAccessor& operator+=(::absl::string_view sv) noexcept {
    return append(sv.data(), sv.size());
  }

  inline void resize(size_type size) {
    if (arena() != nullptr) {
      ArenaStringAccessor::resize(size);
    } else {
      underlying()->resize(size);
    }
  }
  inline void resize(size_type size, value_type c) {
    if (arena() != nullptr) {
      ArenaStringAccessor::resize(size, c);
    } else {
      underlying()->resize(size, c);
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  // Special function
  inline static MaybeArenaStringAccessor create(Arena* arena) {
    if (arena != nullptr) {
      return ArenaStringAccessor::create(arena);
    } else {
      return MaybeArenaStringAccessor(new ::std::string);
    }
  }

  template <typename T>
  inline static MaybeArenaStringAccessor create(Arena* arena, T&& value) {
    return create(arena) = ::std::forward<T>(value);
  }

  inline static void clear(::std::string* ptr) noexcept {
    MaybeArenaStringAccessor(ptr).clear();
  }

  // Add wrapper constructor for normal string
  inline MaybeArenaStringAccessor(::std::string* string) noexcept
      : ArenaStringAccessor(nullptr, string) {}
  using ArenaStringAccessor::arena;
  using ArenaStringAccessor::underlying;

  // Support absl::strings_internal::STLStringResizeUninitialized
  inline void __resize_default_init(size_type new_size) noexcept {
    if (arena() != nullptr) {
      ArenaStringAccessor::__resize_default_init(new_size);
    } else {
      ::absl::strings_internal::STLStringResizeUninitialized(underlying(),
                                                             new_size);
    }
  }

  // Make operator* and operator-> both to self to imitate a string*
  inline MaybeArenaStringAccessor* operator->() { return this; }
  inline const MaybeArenaStringAccessor* operator->() const { return this; }
  inline MaybeArenaStringAccessor& operator*() { return *this; }
  inline const MaybeArenaStringAccessor& operator*() const { return *this; }

  inline void destroy() noexcept {
    if (arena() == nullptr) {
      delete underlying();
    }
  }

  // Also support absl::Format(MaybeArenaStringAccessor, ...)
  inline operator ::absl::FormatRawSink() noexcept {
    return ::absl::FormatRawSink(this);
  }
  ////////////////////////////////////////////////////////////////////////////

 private:
  // Support absl::Format
  friend inline void AbslFormatFlush(MaybeArenaStringAccessor* accessor,
                                     ::absl::string_view sv) noexcept {
    accessor->append(sv.data(), sv.size());
  }
};

#if GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
using MutableStringType = MaybeArenaStringAccessor;
using MutableStringReferenceType = MaybeArenaStringAccessor;
#else   // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
using MutableStringType = ::std::string*;
using MutableStringReferenceType = ::std::string&;
#endif  // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING

namespace internal {

// ============================================================================
// Map DonatedString support APIs (v2.1 plan).
//
// These APIs are designed to be used by Map<K,V> where K and/or V is
// std::string, in order to construct/destruct donated std::strings whose
// character buffer is owned by an Arena.
//
// The key invariants enforced by these APIs (per arenastring_map_plan v2.1):
//   INV-1: Any mutable std::string& exposed to user code must NOT be in
//          Donated state. Promote it first.
//   INV-2: A Donated std::string's data() must reside in arena memory.
//   INV-3: The donated tag stored alongside a node must agree with the
//          actual state of the std::string.
//   INV-4: When arena == nullptr, all code paths must fall back to standard
//          std::string semantics.
//   INV-5: A Donated std::string's character buffer must NOT be freed by
//          std::string's destructor. The arena reclaims it.
// ============================================================================

// Construct a std::string in-place at `placed_ptr` (which the caller already
// owns the storage for, e.g. a Map node body) and immediately make it a
// Donated string whose character buffer is owned by `arena`.
//
// Pre-conditions:
//   - `arena != nullptr`
//   - `placed_ptr` points to sizeof(std::string) bytes of uninitialized memory
//     suitably aligned for std::string.
//
// Post-conditions:
//   - `*placed_ptr` is a valid empty std::string in Donated state
//   - The returned ArenaStringAccessor wraps `arena` and `placed_ptr`
//   - The caller is responsible for setting the donated tag and for NOT
//     invoking std::string::~basic_string() on `placed_ptr` until the string
//     has been promoted via `promote_donated_to_heap`.
PROTOBUF_ALWAYS_INLINE inline ArenaStringAccessor init_donated_in_place(
    Arena* arena, ::std::string* placed_ptr) noexcept {
  // Use placement new to construct an empty std::string. The buffer (if any
  // is later allocated) will be redirected to arena memory via the
  // ArenaStringAccessor write path; the small-string optimization (SSO)
  // buffer for an empty string lives inside the std::string object itself
  // and is harmless (the destructor of an SSO std::string is a no-op for
  // the buffer).
  new (placed_ptr)::std::string();
  return ArenaStringAccessor(arena, placed_ptr);
}

// Wrap an already-Donated std::string with a MaybeArenaStringAccessor so it
// can be used by parser / generated code through the accessor API.
//
// Pre-conditions:
//   - `ptr` was previously initialized via `init_donated_in_place(arena, ptr)`
//     (or is a regular std::string when `arena == nullptr`).
PROTOBUF_ALWAYS_INLINE inline MaybeArenaStringAccessor wrap_existing_donated(
    Arena* arena, ::std::string* ptr) noexcept {
  return MaybeArenaStringAccessor(arena, ptr);
}

// Promote a Donated std::string in-place into a heap-owned std::string so
// that all subsequent standard std::string mutating operations (reserve,
// resize, append, shrink_to_fit, destructor) are safe.
//
// Strategy: read the current (donated) data pointer and size first, then
// placement-new a brand-new std::string over the same storage with
// ::std::string(data, size). The new ctor copies the bytes from the
// still-live arena buffer into a fresh self-owned (heap) buffer; the
// old representation is silently dropped without ~basic_string() ever
// being invoked on the donated state.
//
// Per C++17 the arguments `ptr->data()` and `ptr->size()` are fully
// evaluated before placement-new constructs the new object, so reading
// from the old representation is well-defined.
//
// Post-conditions (per v2.1):
//   1. `ptr->data()` is owned by std::string / system heap.
//   2. The old arena buffer is merely abandoned (the arena will reclaim it).
//   3. The caller MUST clear the corresponding donated tag immediately.
//   4. All standard std::string operations are safe afterwards.
PROTOBUF_ALWAYS_INLINE inline void promote_donated_to_heap(
    ::std::string* ptr) noexcept {
  new (ptr)::std::string(ptr->data(), ptr->size());
}

// Relocate the contents of a (possibly Donated) std::string from `src` (an
// existing, live std::string) into `dst` (uninitialized storage), choosing
// the cheapest legal strategy based on arena placement.
//
// Strategy matrix:
//   * dst_arena == nullptr && src_arena == nullptr:
//       Standard move-construct. dst takes ownership of src's heap buffer,
//       src becomes valid-but-empty.
//   * dst_arena == src_arena (both non-null):
//       Same-arena fast path: the buffer is still valid in the same arena,
//       so we can shallow-copy std::string representation (placement-new an
//       empty std::string into dst, then memcpy the std::string state from
//       src). src is left in a detached state safe to abandon (tag transfer
//       must follow).
//       NOTE: We choose the safer copy-and-init strategy (init_donated +
//       copy from src's view) to avoid having to know the exact ABI byte
//       layout from this header. This costs one buffer allocation in dst's
//       arena but keeps the implementation portable and INV-5 safe.
//   * dst_arena != src_arena (one null, both non-null, or both non-null):
//       Cross-arena deep copy. Construct dst donated in dst_arena and copy
//       content over. src is left untouched (caller decides whether/when to
//       drop it).
//
// `dst_is_donated_out` receives true when the resulting `dst` is in Donated
// state (i.e. the caller must record the tag bit), false when dst became a
// regular heap-owned std::string.
PROTOBUF_ALWAYS_INLINE inline void relocate_donated(
    ::std::string* dst, ::std::string* src, Arena* dst_arena,
    Arena* src_arena, bool src_is_donated, bool* dst_is_donated_out) {
  if (dst_arena == nullptr) {
    // dst lives on system heap.
    new (dst)::std::string(::std::move(*src));
    *dst_is_donated_out = false;
    return;
  }
  // dst lives on dst_arena. Construct as donated and copy content.
  init_donated_in_place(dst_arena, dst);
  ArenaStringAccessor(dst_arena, dst).assign(src->data(), src->size());
  *dst_is_donated_out = true;
  // For the same-arena case, the source's arena buffer is simply abandoned
  // (still owned by src_arena until it is reset). For cross-arena, src is
  // left intact so the caller can release it as appropriate.
  (void)src_arena;
  (void)src_is_donated;
}

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
