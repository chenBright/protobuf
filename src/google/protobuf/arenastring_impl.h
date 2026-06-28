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

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
