#include "google/protobuf/arenastring_impl.h"

#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

ArenaStringAccessor& ArenaStringAccessor::assign(const_pointer data,
                                                 size_type size) noexcept {
  auto* buffer = qualified_buffer(size);
  set_size(size);
  __builtin_memcpy(buffer, data, size);
  buffer[size] = '\0';
  return *this;
}

void ArenaStringAccessor::reserve(size_type required_capacity) noexcept {
  if (required_capacity > capacity()) {
    auto origin_size = size();
    recreate_buffer(required_capacity);
    set_size_and_terminator(origin_size);
  }
}

void ArenaStringAccessor::push_back(value_type c) noexcept {
  auto origin_size = size();
  auto* buffer = qualified_buffer(origin_size + 1, origin_size << 1);
  set_size(origin_size + 1);
  buffer[origin_size] = c;
  buffer[origin_size + 1] = '\0';
}

ArenaStringAccessor& ArenaStringAccessor::append(
    const_pointer append_data, size_type append_size) noexcept {
  auto origin_size = size();
  auto* buffer = qualified_buffer(origin_size + append_size);
  set_size(origin_size + append_size);
  __builtin_memcpy(buffer + origin_size, append_data, append_size);
  buffer[origin_size + append_size] = '\0';
  return *this;
}

void ArenaStringAccessor::swap(ArenaStringAccessor other) noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  auto* tmp = representation().data;
  *reinterpret_cast<pointer*>(_ptr) = other.representation().data;
  *reinterpret_cast<pointer*>(other._ptr) = tmp;
#else   // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  _ptr->swap(*other._ptr);
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
}

void ArenaStringAccessor::resize(size_type new_size, value_type c) noexcept {
  auto origin_size = size();
  auto buffer = qualified_buffer(new_size);
  set_size_and_terminator(new_size);
  if (new_size > origin_size) {
    __builtin_memset(buffer + origin_size, c, new_size - origin_size);
  }
}

internal::StdStringRep& ArenaStringAccessor::representation() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  return *(*reinterpret_cast<StdStringRep**>(_ptr) - 1);
#else   // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  return *reinterpret_cast<StdStringRep*>(_ptr);
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
}

ArenaStringAccessor::pointer ArenaStringAccessor::recreate_buffer(
    size_type capacity) noexcept {
#if __GLIBCXX__
  size_t buffer_size = capacity + 1;
  buffer_size = (buffer_size + 7) & static_cast<size_t>(-8);
  capacity = buffer_size - 1;
#if _GLIBCXX_USE_CXX11_ABI
  auto* buffer = reinterpret_cast<char*>(_arena->AllocateAligned(buffer_size));
  __builtin_memcpy(buffer, data(), size());
  auto& rep = representation();
  rep.data = buffer;
  rep.capacity = capacity;
  return buffer;
#else   // !_GLIBCXX_USE_CXX11_ABI
  auto* rep = reinterpret_cast<StdStringRep*>(
      _arena->AllocateAligned(sizeof(StdStringRep) + buffer_size));
  rep->capacity = capacity;
  rep->refcount = -1;
  __builtin_memcpy(rep->data, data(), size());
  *reinterpret_cast<pointer*>(_ptr) = rep->data;
  return rep->data;
#endif  // !_GLIBCXX_USE_CXX11_ABI
#else   // !__GLIBCXX__
  capacity = (capacity + 16) & static_cast<size_type>(-16);
  auto* buffer = reinterpret_cast<pointer>(_arena->AllocateAligned(capacity));
  __builtin_memcpy(buffer, data(), size());
  auto& rep = representation();
  rep.long_format.data = buffer;
  rep.long_format.capacity = capacity + 1;
  return rep.long_format.data;
#endif  // !__GLIBCXX__
}

ArenaStringAccessor::pointer ArenaStringAccessor::writable_buffer() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  return representation().refcount <= 0 ? representation().data : &(*_ptr)[0];
#else   // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  return &(*_ptr)[0];
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
}

void ArenaStringAccessor::set_size(size_type size) noexcept {
#if __GLIBCXX__
  auto& rep = representation();
  rep.size = size;
#else   // !__GLIBCXX__
  auto& rep = representation();
  if (rep.is_long()) {
    rep.long_format.size = size;
  } else {
    rep.shot_format.size = size << 1;
  }
#endif  // !__GLIBCXX__
}

void ArenaStringAccessor::set_size_and_terminator(size_type size) noexcept {
#if __GLIBCXX__
  auto& rep = representation();
  rep.size = size;
  rep.data[size] = '\0';
#else   // !__GLIBCXX__
  auto& rep = representation();
  if (rep.is_long()) {
    rep.long_format.size = size;
    rep.long_format.data[size] = '\0';
  } else {
    rep.shot_format.size = size << 1;
    rep.shot_format.data[size] = '\0';
  }
#endif  // !__GLIBCXX__
}

void MaybeArenaStringAccessor::clear() noexcept {
#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
  if (representation().refcount <= 0) {
    ArenaStringAccessor::clear();
    return;
  }
#endif  // !__GLIBCXX__ || _GLIBCXX_USE_CXX11_ABI
  underlying()->clear();
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
