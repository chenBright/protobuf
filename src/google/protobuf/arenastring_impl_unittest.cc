#include "google/protobuf/arenastring_impl.h"

#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/reflection.h"
#include "google/protobuf/unittest_arenastring.pb.h"
#include "google/protobuf/unittest_arenastring_mutable.pb.h"
#include "google/protobuf/unittest_proto3_arenastring.pb.h"
#include "google/protobuf/unittest_proto3_arenastring_mutable.pb.h"
#include "gtest/gtest.h"

using ::google::protobuf::Arena;
using ::google::protobuf::ArenaOptions;
using ::google::protobuf::MaybeArenaStringAccessor;
using ::google::protobuf::RepeatedPtrField;

using ::proto2_arenastring_unittest::ArenaProto2;
using ::proto2_arenastring_unittest::ArenaProto2Extension;
using ::proto2_arenastring_unittest::Proto2;
using ::proto2_arenastring_unittest::Proto2Extension;
using ::proto3_arenastring_unittest::ArenaProto3;
using ::proto3_arenastring_unittest::Proto3;

class ArenaStringTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    ArenaOptions options;
    options.initial_block = buffer;
    options.initial_block_size = sizeof(buffer);
    arena = new Arena(options);
  }

  virtual void TearDown() { delete arena; }

  void assert_address_on_arena(const void* address, bool on) {
    if (on) {
      ASSERT_GE(address, buffer);
      ASSERT_LT(address, buffer + sizeof(buffer));
    } else {
      ASSERT_TRUE(address < buffer || address >= buffer + sizeof(buffer));
    }
  }

  void assert_on_arena(const ::std::string& string, bool on, bool content_on) {
    assert_address_on_arena(&string, on);
    if (string.capacity() > 0) {
      assert_address_on_arena(string.c_str(), content_on);
      assert_address_on_arena(string.c_str() + string.capacity() - 1,
                              content_on);
    }
  }

  void assert_on_arena(const ::std::string& string, bool on) {
    assert_on_arena(string, on, on);
  }

  char buffer[1L << 20];
  Arena* arena;
};

::std::string tiny_string = ::std::string(::std::string().capacity(), 'x');
::std::string short_string = ::std::string(::std::string().capacity() + 1, 'y');
::std::string long_string = ::std::string(::std::string().capacity() + 64, 'z');

template <typename T>
static void create_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  {
    auto accessor = MaybeArenaStringAccessor::create(arena);
    ASSERT_TRUE(accessor.empty());
    assert_on_arena(accessor);
    accessor.destroy();
  }
  {
    auto accessor = MaybeArenaStringAccessor::create(arena, tiny_string);
    ASSERT_EQ(tiny_string, accessor);
    ASSERT_EQ(tiny_string, accessor.c_str());
    assert_on_arena(accessor);
    accessor.destroy();
  }
  {
    auto accessor = MaybeArenaStringAccessor::create(arena, short_string);
    ASSERT_EQ(short_string, accessor);
    ASSERT_EQ(short_string, accessor.c_str());
    assert_on_arena(accessor);
    accessor.destroy();
  }
  {
    auto accessor = MaybeArenaStringAccessor::create(arena, long_string);
    ASSERT_EQ(long_string, accessor);
    ASSERT_EQ(long_string, accessor.c_str());
    assert_on_arena(accessor);
    accessor.destroy();
  }
}
TEST_F(ArenaStringTest, create_on_arena) {
  create_on_arena(*this, arena);
  create_on_arena(*this, nullptr);
}

template <typename T>
static void correct_data_size_and_capacity(T& t, Arena* arena) {
  for (size_t i = 0; i < long_string.size(); ++i) {
    auto accessor = MaybeArenaStringAccessor::create(arena);
    accessor.assign(long_string.c_str(), i);
    ASSERT_EQ(i, ::strlen(accessor.c_str()));
    ASSERT_EQ(0, ::memcmp(long_string.c_str(), accessor.c_str(), i));
    ASSERT_EQ(i, accessor.size());
    ASSERT_LE(i, accessor.capacity());
    accessor.destroy();
  }
}
TEST_F(ArenaStringTest, correct_data_size_and_capacity) {
  correct_data_size_and_capacity(*this, arena);
  correct_data_size_and_capacity(*this, nullptr);
}

template <typename T>
static void assign_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto accessor = MaybeArenaStringAccessor::create(arena);
  accessor = tiny_string;
  ASSERT_EQ(tiny_string, accessor);
  ASSERT_STREQ(tiny_string.c_str(), accessor.c_str());
  assert_on_arena(accessor);
  accessor = short_string;
  ASSERT_EQ(short_string, accessor);
  ASSERT_STREQ(short_string.c_str(), accessor.c_str());
  assert_on_arena(accessor);
  accessor = long_string;
  ASSERT_EQ(long_string, accessor);
  ASSERT_STREQ(long_string.c_str(), accessor.c_str());
  assert_on_arena(accessor);
  accessor = short_string;
  ASSERT_EQ(short_string, accessor);
  ASSERT_STREQ(short_string.c_str(), accessor.c_str());
  assert_on_arena(accessor);
  accessor.destroy();
}
TEST_F(ArenaStringTest, assign_on_arena) {
  assign_on_arena(*this, arena);
  assign_on_arena(*this, nullptr);
}

template <typename T>
static void reserve_keep_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto accessor = MaybeArenaStringAccessor::create(arena);
  accessor = tiny_string;
  accessor.reserve(short_string.size());
  ASSERT_EQ(tiny_string, accessor);
  ASSERT_STREQ(tiny_string.c_str(), accessor.c_str());
  ASSERT_LE(short_string.size(), accessor.capacity());
  assert_on_arena(accessor);
  accessor.reserve(long_string.size());
  ASSERT_EQ(tiny_string, accessor);
  ASSERT_STREQ(tiny_string.c_str(), accessor.c_str());
  ASSERT_LE(long_string.size(), accessor.capacity());
  assert_on_arena(accessor);
  accessor.destroy();
}
TEST_F(ArenaStringTest, reserve_keep_on_arena) {
  reserve_keep_on_arena(*this, arena);
  reserve_keep_on_arena(*this, nullptr);
}

#if __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI
TEST_F(ArenaStringTest, do_not_copy_on_write) {
  auto std_string = new ::std::string();
  auto half_arena_string = Arena::Create<::std::string>(arena);
  auto accessor = MaybeArenaStringAccessor::create(arena);
  auto* arena_string = &(const ::std::string&)accessor;
  ASSERT_EQ(::std::string(*std_string).c_str(), std_string->c_str());
  ASSERT_EQ(::std::string(*half_arena_string).c_str(),
            half_arena_string->c_str());
  ASSERT_EQ(::std::string(*arena_string).c_str(), arena_string->c_str());
  std_string->assign(long_string);
  half_arena_string->assign(long_string);
  accessor->assign(long_string);
  ASSERT_EQ(::std::string(*std_string).c_str(), std_string->c_str());
  ASSERT_EQ(::std::string(*half_arena_string).c_str(),
            half_arena_string->c_str());
  ASSERT_NE(::std::string(*arena_string).c_str(), arena_string->c_str());
  std_string->clear();
  half_arena_string->clear();
  accessor->clear();
  ASSERT_EQ(::std::string(*std_string).c_str(), std_string->c_str());
  ASSERT_EQ(::std::string(*half_arena_string).c_str(),
            half_arena_string->c_str());
  ASSERT_NE(::std::string(*arena_string).c_str(), arena_string->c_str());
  std_string->assign(short_string);
  half_arena_string->assign(short_string);
  accessor->assign(short_string);
  ASSERT_EQ(::std::string(*std_string).c_str(), std_string->c_str());
  ASSERT_EQ(::std::string(*half_arena_string).c_str(),
            half_arena_string->c_str());
  ASSERT_NE(::std::string(*arena_string).c_str(), arena_string->c_str());
  std_string->append(long_string);
  half_arena_string->append(long_string);
  accessor->append(long_string);
  ASSERT_EQ(::std::string(*std_string).c_str(), std_string->c_str());
  ASSERT_EQ(::std::string(*half_arena_string).c_str(),
            half_arena_string->c_str());
  ASSERT_NE(::std::string(*arena_string).c_str(), arena_string->c_str());
  delete std_string;
}
#endif  // __GLIBCXX__ && !_GLIBCXX_USE_CXX11_ABI

TEST_F(ArenaStringTest, support_resize) {
  auto accessor = MaybeArenaStringAccessor::create(arena, "10086");
  accessor.resize(4);
  auto data = &accessor[0];
  ASSERT_EQ(4, accessor.size());
  ASSERT_EQ("1008", accessor);
  ASSERT_EQ(data, accessor.data());
  accessor.resize(2);
  data = &accessor[0];
  ASSERT_EQ(2, accessor.size());
  ASSERT_EQ("10", accessor);
  ASSERT_EQ(data, accessor.data());
  accessor.resize(4);
  data = &accessor[0];
  ASSERT_EQ(4, accessor.size());
  ASSERT_EQ(::absl::string_view("10\0\0", 4), accessor);
  ASSERT_EQ(data, accessor.data());
  accessor.destroy();
}

TEST_F(ArenaStringTest, support_resize_uninitialized) {
  auto accessor = MaybeArenaStringAccessor::create(arena, "10086");
  ::absl::strings_internal::STLStringResizeUninitialized(&accessor, 4);
  auto data = &accessor[0];
  ASSERT_EQ(4, accessor.size());
  ASSERT_EQ("1008", accessor);
  ASSERT_EQ(data, accessor.data());
  ::absl::strings_internal::STLStringResizeUninitialized(&accessor, 2);
  data = &accessor[0];
  ASSERT_EQ(2, accessor.size());
  ASSERT_EQ("10", accessor);
  ASSERT_EQ(data, accessor.data());
  ::absl::strings_internal::STLStringResizeUninitialized(&accessor, 4);
  data = &accessor[0];
  ASSERT_EQ(4, accessor.size());
  ASSERT_EQ(::absl::string_view("10\08", 4), accessor);
  ASSERT_EQ(data, accessor.data());
  accessor.destroy();
}

TEST_F(ArenaStringTest, support_absl_format) {
  auto accessor = MaybeArenaStringAccessor::create(arena, "hello world");
  ::absl::Format(accessor, " +%d", 10086);
  ASSERT_EQ("hello world +10086", accessor);
  accessor.destroy();
}

template <typename T>
static void alter_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto accessor = MaybeArenaStringAccessor::create(arena);
  accessor.push_back('x');
  assert_on_arena(accessor);
  accessor.clear();
  assert_on_arena(accessor);
  accessor.append(tiny_string);
  assert_on_arena(accessor);
  accessor.append(short_string);
  assert_on_arena(accessor);
  ::std::string tmp_string(long_string.c_str());
  auto tmp_ptr = tmp_string.c_str();
  accessor = ::std::move(tmp_string);
  ASSERT_EQ(long_string, accessor);
  if (arena != nullptr)
    ASSERT_NE(tmp_ptr, accessor.c_str());
  else {
    ASSERT_EQ(tmp_ptr, accessor.c_str());
  }
  accessor.destroy();
}
TEST_F(ArenaStringTest, alter_on_arena) {
  alter_on_arena(*this, arena);
  alter_on_arena(*this, nullptr);
}

template <typename M, typename T>
static void direct_set_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_s(short_string);
  ASSERT_EQ(short_string, m->s());
  assert_on_arena(m->s());
  m->set_b(long_string.c_str(), long_string.size());
  ASSERT_EQ(long_string, m->b());
  assert_on_arena(m->b());
  m->set_os(long_string);
  ASSERT_EQ(long_string, m->os());
  assert_on_arena(m->os());
  m->set_ob(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->ob());
  assert_on_arena(m->ob());
  m->add_rs(short_string);
  ASSERT_EQ(short_string, m->rs(0));
  assert_on_arena(m->rs(0));
  m->add_rs(long_string);
  ASSERT_EQ(long_string, m->rs(1));
  assert_on_arena(m->rs(1));
  m->add_rb(long_string.c_str(), long_string.size());
  ASSERT_EQ(long_string, m->rb(0));
  assert_on_arena(m->rb(0));
  m->add_rb(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->rb(1));
  assert_on_arena(m->rb(1));
  m->set_ons(short_string);
  ASSERT_EQ(short_string, m->ons());
  assert_on_arena(m->ons());
  m->set_onb(long_string.c_str(), long_string.size());
  ASSERT_FALSE(m->has_ons());
  ASSERT_TRUE(m->ons().empty());
  ASSERT_EQ(long_string, m->onb());
  assert_on_arena(m->onb());
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, direct_set_on_arena) {
  direct_set_on_arena<Proto3>(*this, arena);
  direct_set_on_arena<Proto3>(*this, nullptr);
  direct_set_on_arena<ArenaProto3>(*this, arena);
  direct_set_on_arena<ArenaProto3>(*this, nullptr);
}

template <typename M, typename T>
static void direct_set_on_arena_pb2(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_s(short_string);
  ASSERT_EQ(short_string, m->s());
  assert_on_arena(m->s());
  m->set_b(long_string.c_str(), long_string.size());
  ASSERT_EQ(long_string, m->b());
  assert_on_arena(m->b());
  m->set_qs(long_string);
  ASSERT_EQ(long_string, m->qs());
  assert_on_arena(m->qs());
  m->set_qb(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->qb());
  assert_on_arena(m->qb());
  m->set_ds(short_string);
  ASSERT_EQ(short_string, m->ds());
  assert_on_arena(m->ds());
  m->set_db(long_string.c_str(), long_string.size());
  ASSERT_EQ(long_string, m->db());
  assert_on_arena(m->db());
  m->add_rs(long_string);
  ASSERT_EQ(long_string, m->rs(0));
  assert_on_arena(m->rs(0));
  m->add_rs(short_string);
  ASSERT_EQ(short_string, m->rs(1));
  assert_on_arena(m->rs(1));
  m->add_rb(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->rb(0));
  assert_on_arena(m->rb(0));
  m->add_rb(long_string.c_str(), long_string.size());
  ASSERT_EQ(long_string, m->rb(1));
  assert_on_arena(m->rb(1));
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, direct_set_on_arena_pb2) {
  direct_set_on_arena_pb2<Proto2>(*this, arena);
  direct_set_on_arena_pb2<Proto2>(*this, nullptr);
  direct_set_on_arena_pb2<ArenaProto2>(*this, arena);
  direct_set_on_arena_pb2<ArenaProto2>(*this, nullptr);
}

template <typename M, typename T>
static void set_again_keep_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_s(short_string);
  m->set_s(long_string);
  ASSERT_EQ(long_string, m->s());
  assert_on_arena(m->s());
  m->set_b(long_string.c_str(), long_string.size());
  m->set_b(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->b());
  assert_on_arena(m->b());
  m->set_os(short_string);
  m->set_os(long_string);
  ASSERT_EQ(long_string, m->os());
  assert_on_arena(m->os());
  m->set_ob(long_string.c_str(), long_string.size());
  m->set_ob(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->ob());
  assert_on_arena(m->ob());
  m->set_ons(short_string);
  m->set_ons(long_string);
  ASSERT_EQ(long_string, m->ons());
  assert_on_arena(m->ons());
  m->set_onb(long_string.c_str(), long_string.size());
  m->set_onb(short_string.c_str(), short_string.size());
  ASSERT_FALSE(m->has_ons());
  ASSERT_TRUE(m->ons().empty());
  ASSERT_EQ(short_string, m->onb());
  assert_on_arena(m->onb());
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, set_again_keep_on_arena) {
  set_again_keep_on_arena<Proto3>(*this, arena);
  set_again_keep_on_arena<Proto3>(*this, nullptr);
  set_again_keep_on_arena<ArenaProto3>(*this, arena);
  set_again_keep_on_arena<ArenaProto3>(*this, nullptr);
}

template <typename M, typename T>
static void set_again_keep_on_arena_pb2(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_s(short_string);
  m->set_s(long_string);
  ASSERT_EQ(long_string, m->s());
  assert_on_arena(m->s());
  m->set_b(long_string.c_str(), long_string.size());
  m->set_b(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->b());
  assert_on_arena(m->b());
  m->set_qs(short_string);
  m->set_qs(long_string);
  ASSERT_EQ(long_string, m->qs());
  assert_on_arena(m->qs());
  m->set_qb(long_string.c_str(), long_string.size());
  m->set_qb(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->qb());
  assert_on_arena(m->qb());
  m->set_ds(short_string);
  m->set_ds(long_string);
  ASSERT_EQ(long_string, m->ds());
  assert_on_arena(m->ds());
  m->set_db(long_string.c_str(), long_string.size());
  m->set_db(short_string.c_str(), short_string.size());
  ASSERT_EQ(short_string, m->db());
  assert_on_arena(m->db());
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, set_again_keep_on_arena_pb2) {
  set_again_keep_on_arena_pb2<Proto2>(*this, arena);
  set_again_keep_on_arena_pb2<Proto2>(*this, nullptr);
  set_again_keep_on_arena_pb2<ArenaProto2>(*this, arena);
  set_again_keep_on_arena_pb2<ArenaProto2>(*this, nullptr);
}

template <typename M, typename T>
static void clear_keep_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_s(short_string);
  auto* ps = &m->s();
  m->set_b(long_string);
  auto* pb = &m->b();
  m->add_rs(short_string);
  auto* prs = &m->rs(0);
  m->add_rb(long_string);
  auto* prb = &m->rb(0);
  m->set_ons(short_string);
  m->set_onb(long_string);
  m->Clear();
  m->set_s(long_string);
  assert_on_arena(m->s());
  ASSERT_EQ(ps, &m->s());
  m->set_b(short_string);
  assert_on_arena(m->b());
  ASSERT_EQ(pb, &m->b());
  m->add_rs(long_string);
  assert_on_arena(m->rs(0));
  ASSERT_EQ(prs, &m->rs(0));
  m->add_rb(short_string);
  assert_on_arena(m->rb(0));
  ASSERT_EQ(prb, &m->rb(0));
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, clear_keep_on_arena) {
  clear_keep_on_arena<Proto2>(*this, arena);
  clear_keep_on_arena<Proto2>(*this, nullptr);
  clear_keep_on_arena<ArenaProto2>(*this, arena);
  clear_keep_on_arena<ArenaProto2>(*this, nullptr);
  clear_keep_on_arena<Proto3>(*this, arena);
  clear_keep_on_arena<Proto3>(*this, nullptr);
  clear_keep_on_arena<ArenaProto3>(*this, arena);
  clear_keep_on_arena<ArenaProto3>(*this, nullptr);
}

template <typename M, typename T>
static void clear_keep_on_arena_pb2(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_ds(short_string);
  auto* pds = &m->ds();
  m->set_db(long_string);
  auto* pdb = &m->db();
  m->set_qs(short_string);
  auto* pqs = &m->qs();
  m->set_qb(long_string);
  auto* pqb = &m->qb();
  m->Clear();
  m->set_ds(long_string);
  assert_on_arena(m->ds());
  ASSERT_EQ(pds, &m->ds());
  m->set_db(short_string);
  assert_on_arena(m->db());
  ASSERT_EQ(pdb, &m->db());
  m->set_qs(long_string);
  assert_on_arena(m->qs());
  ASSERT_EQ(pqs, &m->qs());
  m->set_qb(short_string);
  assert_on_arena(m->qb());
  ASSERT_EQ(pqb, &m->qb());
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, clear_keep_on_arena_pb2) {
  clear_keep_on_arena_pb2<Proto2>(*this, arena);
  clear_keep_on_arena_pb2<Proto2>(*this, nullptr);
  clear_keep_on_arena_pb2<ArenaProto2>(*this, arena);
  clear_keep_on_arena_pb2<ArenaProto2>(*this, nullptr);
}

template <typename M, typename T>
static void parse_and_merge_on_arena(T& t, Arena* farena, Arena* tarena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, tarena, tarena);
  };
  auto assert_mutable_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(
        s, tarena,
        tarena &&
            M().GetDescriptor()->file()->options().cc_mutable_donated_string());
  };
  ::std::string string;
  auto* fm = Arena::CreateMessage<M>(farena);
  auto* tm = Arena::CreateMessage<M>(tarena);
  fm->set_s(short_string);
  fm->set_b(long_string);
  fm->set_os(short_string);
  fm->set_ob(long_string);
  fm->set_ons(short_string);
  fm->set_onb(long_string);
  fm->add_rs(short_string);
  fm->add_rs(long_string);
  fm->add_rb(long_string);
  fm->add_rb(short_string);
  fm->SerializeToString(&string);
  ASSERT_TRUE(fm->SerializeToString(&string));
  ASSERT_TRUE(tm->ParseFromString(string));
  ASSERT_EQ(short_string, tm->s());
  assert_on_arena(tm->s());
  ASSERT_EQ(long_string, tm->b());
  assert_on_arena(tm->b());
  ASSERT_EQ(short_string, tm->os());
  assert_on_arena(tm->os());
  ASSERT_EQ(long_string, tm->ob());
  assert_on_arena(tm->ob());
  ASSERT_EQ(long_string, tm->onb());
  assert_on_arena(tm->onb());
  ASSERT_EQ(short_string, tm->rs(0));
  assert_on_arena(tm->rs(0));
  ASSERT_EQ(long_string, tm->rs(1));
  assert_on_arena(tm->rs(1));
  ASSERT_EQ(long_string, tm->rb(0));
  assert_on_arena(tm->rb(0));
  ASSERT_EQ(short_string, tm->rb(1));
  assert_on_arena(tm->rb(1));
  tm->mutable_s()->assign(short_string);
  assert_mutable_on_arena(tm->s());
  tm->mutable_rs(0)->assign(long_string);
  tm->mutable_rb(1)->assign(long_string);
  ASSERT_TRUE(tm->ParseFromString(string));
  ASSERT_EQ(short_string, tm->s());
  assert_mutable_on_arena(tm->s());
  ASSERT_EQ(long_string, tm->b());
  assert_on_arena(tm->b());
  ASSERT_EQ(long_string, tm->onb());
  assert_on_arena(tm->onb());
  ASSERT_EQ(short_string, tm->rs(0));
  assert_mutable_on_arena(tm->rs(0));
  ASSERT_EQ(long_string, tm->rs(1));
  assert_on_arena(tm->rs(1));
  ASSERT_EQ(long_string, tm->rb(0));
  assert_on_arena(tm->rb(0));
  ASSERT_EQ(short_string, tm->rb(1));
  assert_mutable_on_arena(tm->rb(1));
  tm->CopyFrom(*fm);
  ASSERT_EQ(short_string, tm->s());
  assert_mutable_on_arena(tm->s());
  ASSERT_EQ(long_string, tm->b());
  assert_on_arena(tm->b());
  ASSERT_EQ(long_string, tm->onb());
  assert_on_arena(tm->onb());
  ASSERT_EQ(short_string, tm->rs(0));
  assert_mutable_on_arena(tm->rs(0));
  ASSERT_EQ(long_string, tm->rs(1));
  assert_on_arena(tm->rs(1));
  ASSERT_EQ(long_string, tm->rb(0));
  assert_on_arena(tm->rb(0));
  ASSERT_EQ(short_string, tm->rb(1));
  assert_mutable_on_arena(tm->rb(1));
  if (!tarena) {
    delete tm;
  }
  if (!farena) {
    delete fm;
  }
}
TEST_F(ArenaStringTest, parse_and_merge_on_arena) {
  parse_and_merge_on_arena<Proto3>(*this, arena, arena);
  parse_and_merge_on_arena<Proto3>(*this, arena, nullptr);
  parse_and_merge_on_arena<Proto3>(*this, nullptr, arena);
  parse_and_merge_on_arena<Proto3>(*this, nullptr, nullptr);
  parse_and_merge_on_arena<ArenaProto3>(*this, arena, arena);
  parse_and_merge_on_arena<ArenaProto3>(*this, arena, nullptr);
  parse_and_merge_on_arena<ArenaProto3>(*this, nullptr, arena);
  parse_and_merge_on_arena<ArenaProto3>(*this, nullptr, nullptr);
}

template <typename M, typename T>
static void swap_on_arena(T& t, Arena* farena, Arena* tarena) {
  auto assert_nn_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, tarena, tarena);
  };
  auto assert_nm_on_arena = [&](const ::std::string& s) {
    if (tarena != farena) {
      t.assert_on_arena(s, tarena, tarena);
    } else {
      t.assert_on_arena(s, tarena,
                        tarena && M().GetDescriptor()
                                      ->file()
                                      ->options()
                                      .cc_mutable_donated_string());
    }
  };
  auto assert_mn_on_arena = [&](const ::std::string& s) {
    if (tarena != farena) {
      t.assert_on_arena(s, tarena, tarena);
    } else {
      t.assert_on_arena(s, tarena, tarena);
    }
  };
  auto assert_mm_on_arena = [&](const ::std::string& s) {
    if (tarena != farena) {
      t.assert_on_arena(s, tarena, tarena);
    } else {
      t.assert_on_arena(s, tarena,
                        tarena && M().GetDescriptor()
                                      ->file()
                                      ->options()
                                      .cc_mutable_donated_string());
    }
  };
  auto* fm = Arena::CreateMessage<M>(farena);
  auto* tm = Arena::CreateMessage<M>(tarena);
  fm->set_s(short_string);
  fm->mutable_b()->assign(long_string);
  fm->mutable_ons()->assign(short_string);
  fm->add_rs(short_string);
  fm->add_rs()->assign(long_string);
  fm->add_rb(long_string);
  fm->add_rb()->assign(short_string);

  tm->set_s(long_string);
  tm->set_b(short_string);
  tm->set_onb(long_string);
  tm->add_rs(long_string);
  tm->add_rs(short_string);
  tm->add_rb()->assign(short_string);
  tm->add_rb()->assign(long_string);

  tm->Swap(fm);
  ASSERT_EQ(short_string, tm->s());
  assert_nn_on_arena(tm->s());
  ASSERT_EQ(long_string, tm->b());
  assert_nm_on_arena(tm->b());
  ASSERT_EQ(short_string, tm->ons());
  assert_nm_on_arena(tm->ons());
  ASSERT_EQ(short_string, tm->rs(0));
  assert_nn_on_arena(tm->rs(0));
  ASSERT_EQ(long_string, tm->rs(1));
  assert_nm_on_arena(tm->rs(1));
  ASSERT_EQ(long_string, tm->rb(0));
  assert_mn_on_arena(tm->rb(0));
  ASSERT_EQ(short_string, tm->rb(1));
  assert_mm_on_arena(tm->rb(1));
  if (!tarena) {
    delete tm;
  }
  if (!farena) {
    delete fm;
  }
  fm = Arena::CreateMessage<M>(farena);
  tm = Arena::CreateMessage<M>(tarena);
  fm->mutable_s()->assign(short_string);
  fm->set_b(long_string);
  tm->mutable_s()->assign(long_string);
  tm->mutable_b()->assign(short_string);

  tm->Swap(fm);
  ASSERT_EQ(short_string, tm->s());
  assert_mm_on_arena(tm->s());
  ASSERT_EQ(long_string, tm->b());
  assert_mn_on_arena(tm->b());
  if (!tarena) {
    delete tm;
  }
  if (!farena) {
    delete fm;
  }
}
TEST_F(ArenaStringTest, swap_on_arena) {
  swap_on_arena<Proto3>(*this, arena, arena);
  swap_on_arena<Proto3>(*this, arena, nullptr);
  swap_on_arena<Proto3>(*this, nullptr, arena);
  swap_on_arena<Proto3>(*this, nullptr, nullptr);
  swap_on_arena<ArenaProto3>(*this, arena, arena);
  swap_on_arena<ArenaProto3>(*this, arena, nullptr);
  swap_on_arena<ArenaProto3>(*this, nullptr, arena);
  swap_on_arena<ArenaProto3>(*this, nullptr, nullptr);
  swap_on_arena<Proto2>(*this, arena, arena);
  swap_on_arena<Proto2>(*this, arena, nullptr);
  swap_on_arena<Proto2>(*this, nullptr, arena);
  swap_on_arena<Proto2>(*this, nullptr, nullptr);
  swap_on_arena<ArenaProto2>(*this, arena, arena);
  swap_on_arena<ArenaProto2>(*this, arena, nullptr);
  swap_on_arena<ArenaProto2>(*this, nullptr, arena);
  swap_on_arena<ArenaProto2>(*this, nullptr, nullptr);
}

template <typename M, typename T>
static void set_allocated_on_arena(T& t, Arena* arena) {
  auto cc_mutable_donated_string =
      M().GetDescriptor()->file()->options().cc_mutable_donated_string();
  auto m = Arena::CreateMessage<M>(arena);
  {
    auto s = new ::std::string{short_string};
    auto c = s->c_str();
    m->set_allocated_s(s);
    ASSERT_NE(s, &m->s());
    ASSERT_EQ(c, m->s().c_str());
    t.assert_on_arena(m->s(), arena && cc_mutable_donated_string, false);
    auto cs = &m->s();
    c = cs->c_str();
    m->mutable_s()->assign(long_string);
    ASSERT_EQ(cs, &m->s());
    ASSERT_NE(c, m->s().c_str());
    t.assert_on_arena(m->s(), arena && cc_mutable_donated_string, false);
  }
  {
    auto s = new ::std::string{short_string};
    auto c = s->c_str();
    m->set_b(long_string);
    m->set_allocated_b(s);
    ASSERT_NE(s, &m->b());
    ASSERT_EQ(c, m->b().c_str());
    t.assert_on_arena(m->b(), arena && cc_mutable_donated_string, false);
  }
  {
    auto s = new ::std::string{long_string};
    auto c = s->c_str();
    m->mutable_ons()->assign(short_string);
    m->set_allocated_ons(s);
    ASSERT_EQ(s, &m->ons());
    ASSERT_EQ(c, m->ons().c_str());
    t.assert_on_arena(m->ons(), false, false);
  }
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, set_allocated_on_arena) {
  set_allocated_on_arena<Proto3>(*this, arena);
  set_allocated_on_arena<Proto3>(*this, nullptr);
  set_allocated_on_arena<ArenaProto3>(*this, arena);
  set_allocated_on_arena<ArenaProto3>(*this, nullptr);
  set_allocated_on_arena<Proto2>(*this, arena);
  set_allocated_on_arena<Proto2>(*this, nullptr);
  set_allocated_on_arena<ArenaProto2>(*this, arena);
  set_allocated_on_arena<ArenaProto2>(*this, nullptr);
}

template <typename M, typename T>
static void release_on_arena(T& t, Arena* arena) {
  auto cc_mutable_donated_string =
      M().GetDescriptor()->file()->options().cc_mutable_donated_string();
  auto m = Arena::CreateMessage<M>(arena);
  {
    m->set_s(long_string);
    auto s = &m->s();
    auto c = s->c_str();
    auto r = m->release_s();
    if (arena || cc_mutable_donated_string) {
      ASSERT_NE(s, r);
    } else {
      ASSERT_EQ(s, r);
    }
    if (arena) {
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(c, r->c_str());
    }
    ASSERT_EQ(long_string, *r);
    delete r;
  }
  {
    m->set_os(short_string);
    auto s = &m->os();
    auto c = s->c_str();
    auto r = m->release_os();
    if (arena || cc_mutable_donated_string) {
      ASSERT_NE(s, r);
    } else {
      ASSERT_EQ(s, r);
    }
    if (arena) {
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(c, r->c_str());
    }
    ASSERT_EQ(short_string, *r);
    delete r;
  }
  {
    m->set_ons(long_string);
    auto s = &m->ons();
    auto c = s->c_str();
    auto r = m->release_ons();
    if (arena) {
      ASSERT_NE(s, r);
    } else {
      ASSERT_EQ(s, r);
    }
    if (arena) {
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(c, r->c_str());
    }
    ASSERT_EQ(long_string, *r);
    delete r;
  }
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, release_on_arena) {
  release_on_arena<Proto3>(*this, arena);
  release_on_arena<Proto3>(*this, nullptr);
  release_on_arena<ArenaProto3>(*this, arena);
  release_on_arena<ArenaProto3>(*this, nullptr);
}

template <typename M, typename T>
static void release_on_arena_pb2(T& t, Arena* arena) {
  auto cc_mutable_donated_string =
      M().GetDescriptor()->file()->options().cc_mutable_donated_string();
  auto m = Arena::CreateMessage<M>(arena);
  {
    m->set_s(long_string);
    auto s = &m->s();
    auto c = s->c_str();
    auto r = m->release_s();
    if (arena || cc_mutable_donated_string) {
      ASSERT_NE(s, r);
    } else {
      ASSERT_EQ(s, r);
    }
    if (arena) {
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(c, r->c_str());
    }
    ASSERT_EQ(long_string, *r);
    delete r;
  }
  {
    m->set_qs(short_string);
    auto s = &m->qs();
    auto c = s->c_str();
    auto r = m->release_qs();
    if (arena || cc_mutable_donated_string) {
      ASSERT_NE(s, r);
    } else {
      ASSERT_EQ(s, r);
    }
    if (arena) {
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(c, r->c_str());
    }
    ASSERT_EQ(short_string, *r);
    delete r;
  }
  {
    m->set_ds(long_string);
    auto s = &m->ds();
    auto c = s->c_str();
    auto r = m->release_ds();
    if (arena) {
      ASSERT_NE(s, r);
    } else {
      ASSERT_EQ(s, r);
    }
    if (arena) {
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(c, r->c_str());
    }
    ASSERT_EQ(long_string, *r);
    delete r;
  }
  {
    m->set_ons(short_string);
    auto s = &m->ons();
    auto c = s->c_str();
    auto r = m->release_ons();
    if (arena) {
      ASSERT_NE(s, r);
    } else {
      ASSERT_EQ(s, r);
    }
    if (arena) {
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(c, r->c_str());
    }
    ASSERT_EQ(short_string, *r);
    delete r;
  }
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, release_on_arena_pb2) {
  release_on_arena_pb2<Proto2>(*this, arena);
  release_on_arena_pb2<Proto2>(*this, nullptr);
  release_on_arena_pb2<ArenaProto2>(*this, arena);
  release_on_arena_pb2<ArenaProto2>(*this, nullptr);
}

template <typename M, typename T>
static void reflect_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  auto* r = m->GetReflection();
  auto* d = m->GetDescriptor();
  r->SetString(m, d->FindFieldByName("s"), long_string);
  assert_on_arena(m->s());
  assert_on_arena(r->GetStringReference(*m, d->FindFieldByName("s"), nullptr));
  r->SetString(m, d->FindFieldByName("os"), long_string);
  assert_on_arena(m->os());
  assert_on_arena(r->GetStringReference(*m, d->FindFieldByName("os"), nullptr));
  r->AddString(m, d->FindFieldByName("rs"), long_string);
  assert_on_arena(m->rs(0));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 0, nullptr));
  r->SetRepeatedString(m, d->FindFieldByName("rs"), 0,
                       long_string + long_string);
  assert_on_arena(m->rs(0));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 0, nullptr));
  r->template GetMutableRepeatedFieldRef<::std::string>(
       m, d->FindFieldByName("rs"))
      .Add(long_string);
  assert_on_arena(m->rs(1));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 1, nullptr));
  r->template GetMutableRepeatedFieldRef<::std::string>(
       m, d->FindFieldByName("rs"))
      .Set(1, long_string + long_string);
  assert_on_arena(m->rs(1));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 1, nullptr));
  r->SetString(m, d->FindFieldByName("ons"), long_string);
  assert_on_arena(m->ons());
  assert_on_arena(
      r->GetStringReference(*m, d->FindFieldByName("ons"), nullptr));
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, reflect_on_arena) {
  reflect_on_arena<Proto3>(*this, arena);
  reflect_on_arena<Proto3>(*this, nullptr);
  reflect_on_arena<ArenaProto3>(*this, arena);
  reflect_on_arena<ArenaProto3>(*this, nullptr);
}

template <typename M, typename T>
static void reflect_on_arena_pb2(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena, arena);
  };
  auto* m = Arena::CreateMessage<M>(arena);
  auto* r = m->GetReflection();
  auto* d = m->GetDescriptor();
  r->SetString(m, d->FindFieldByName("s"), long_string);
  assert_on_arena(m->s());
  assert_on_arena(r->GetStringReference(*m, d->FindFieldByName("s"), nullptr));
  r->SetString(m, d->FindFieldByName("qs"), long_string);
  assert_on_arena(m->qs());
  assert_on_arena(r->GetStringReference(*m, d->FindFieldByName("qs"), nullptr));
  r->SetString(m, d->FindFieldByName("ds"), long_string);
  assert_on_arena(m->ds());
  assert_on_arena(r->GetStringReference(*m, d->FindFieldByName("ds"), nullptr));
  r->AddString(m, d->FindFieldByName("rs"), long_string);
  assert_on_arena(m->rs(0));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 0, nullptr));
  r->SetRepeatedString(m, d->FindFieldByName("rs"), 0,
                       long_string + long_string);
  assert_on_arena(m->rs(0));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 0, nullptr));
  r->template GetMutableRepeatedFieldRef<::std::string>(
       m, d->FindFieldByName("rs"))
      .Add(long_string);
  assert_on_arena(m->rs(1));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 1, nullptr));
  r->template GetMutableRepeatedFieldRef<::std::string>(
       m, d->FindFieldByName("rs"))
      .Set(1, long_string + long_string);
  assert_on_arena(m->rs(1));
  assert_on_arena(
      r->GetRepeatedStringReference(*m, d->FindFieldByName("rs"), 1, nullptr));
  r->SetString(m, d->FindFieldByName("ons"), long_string);
  assert_on_arena(m->ons());
  assert_on_arena(
      r->GetStringReference(*m, d->FindFieldByName("ons"), nullptr));
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, reflect_on_arena_pb2) {
  reflect_on_arena_pb2<Proto2>(*this, arena);
  reflect_on_arena_pb2<Proto2>(*this, nullptr);
  reflect_on_arena_pb2<ArenaProto2>(*this, arena);
  reflect_on_arena_pb2<ArenaProto2>(*this, nullptr);
}

template <typename M, typename T>
static void repeated_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena, arena);
  };
  auto assert_mutable_on_arena = [&](const ::std::string& s) {
#if GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
    t.assert_on_arena(s, arena, arena);
#else   // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
    t.assert_on_arena(s, arena, false);
#endif  // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
  };
  auto m = Arena::CreateMessage<M>(arena);
  auto rs = m->mutable_rs();
  rs->Add(::std::string(short_string));
  ASSERT_EQ(short_string, rs->Get(0));
  assert_on_arena(rs->Get(0));
  rs->RemoveLast();
  rs->Add(::std::string(long_string));
  ASSERT_EQ(long_string, rs->Get(0));
  assert_on_arena(rs->Get(0));
  for (auto iter = m->rs().begin(); iter != m->rs().end(); ++iter) {
    ASSERT_EQ(long_string, *iter);
    assert_on_arena(*iter);
  }
  for (auto iter = rs->begin(); iter != rs->end(); ++iter) {
    ASSERT_EQ(long_string, *iter);
    assert_mutable_on_arena(*iter);
  }
  rs->Add(::std::string(short_string));
  ASSERT_EQ(short_string, rs->Get(1));
  assert_on_arena(rs->Get(1));
  rs->Mutable(1)->assign(long_string);
  ASSERT_EQ(long_string, rs->Get(1));
  assert_mutable_on_arena(rs->Get(1));
  rs->Add()->assign(long_string);
  ASSERT_EQ(long_string, rs->Get(2));
  assert_mutable_on_arena(rs->Get(2));
  rs->Add(::std::string(long_string));
  ASSERT_EQ(long_string, m->rs()[3]);
  assert_on_arena(m->rs()[3]);
  (*rs)[3].assign(long_string + long_string);
  ASSERT_EQ(long_string + long_string, rs->Get(3));
  assert_mutable_on_arena(rs->Get(3));
  rs->Add(::std::string(long_string));
  ASSERT_EQ(long_string, m->rs().at(4));
  assert_on_arena(m->rs().at(4));
  rs->at(4).assign(long_string + long_string);
  ASSERT_EQ(long_string + long_string, rs->Get(4));
  assert_mutable_on_arena(rs->Get(4));
  rs->Add(::std::string(short_string));
  ASSERT_EQ(short_string, rs->Get(5));
  assert_on_arena(rs->Get(5));
  rs->DeleteSubrange(1, 4);
  ASSERT_EQ(2, rs->size());
  if (!arena) {
    delete m;
  }
  m = Arena::CreateMessage<M>(arena);
  rs = m->mutable_rs();
  M fm;
  fm.add_rs(short_string);
  fm.add_rs(long_string);
  m->MergeFrom(fm);
  ASSERT_EQ(short_string, rs->Get(0));
  assert_on_arena(rs->Get(0));
  ASSERT_EQ(long_string, rs->Get(1));
  assert_on_arena(rs->Get(1));
  {
    ::std::string strs[2] = {short_string, long_string};
    rs->Add(strs, strs + 2);
    ASSERT_EQ(short_string, rs->Get(2));
    assert_on_arena(rs->Get(2));
    ASSERT_EQ(long_string, rs->Get(3));
    assert_on_arena(rs->Get(3));
  }
  {
    ::std::string strs[2] = {long_string, short_string};
    rs->Assign(strs, strs + 2);
    ASSERT_EQ(long_string, rs->Get(0));
    assert_on_arena(rs->Get(0));
    ASSERT_EQ(short_string, rs->Get(1));
    assert_on_arena(rs->Get(1));
  }
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, repeated_on_arena) {
  repeated_on_arena<Proto2>(*this, arena);
  repeated_on_arena<Proto2>(*this, nullptr);
  repeated_on_arena<ArenaProto2>(*this, arena);
  repeated_on_arena<ArenaProto2>(*this, nullptr);
  repeated_on_arena<Proto3>(*this, arena);
  repeated_on_arena<Proto3>(*this, nullptr);
  repeated_on_arena<ArenaProto3>(*this, arena);
  repeated_on_arena<ArenaProto3>(*this, nullptr);
}

template <typename M, typename T>
static void repeated_add_allocated_and_release_on_arena(T& t, Arena* arena) {
  auto m = Arena::CreateMessage<M>(arena);
  auto rs = m->mutable_rs();
  {
    auto s = new ::std::string(long_string);
    auto c = s->c_str();
    rs->AddAllocated(s);
    ASSERT_EQ(s, &rs->Get(0));
    ASSERT_EQ(c, rs->Get(0).c_str());
    t.assert_on_arena(rs->Get(0), false, false);
    s = new ::std::string(long_string);
    c = s->c_str();
    rs->AddAllocated(s);
    ASSERT_EQ(s, &rs->Get(1));
    ASSERT_EQ(c, rs->Get(1).c_str());
    t.assert_on_arena(rs->Get(1), false, false);
    rs->MutableAccessor(1)->assign(long_string + long_string);
    ASSERT_EQ(s, &rs->Get(1));
    ASSERT_NE(c, rs->Get(1).c_str());
    t.assert_on_arena(rs->Get(1), false, false);
    m->add_rs(long_string);
  }
  {
    auto s = &m->rs(m->rs_size() - 1);
    auto c = s->c_str();
    auto r = rs->ReleaseLast();
    ASSERT_EQ(long_string, *r);
    if (arena) {
      ASSERT_NE(s, r);
      ASSERT_NE(c, r->c_str());
    } else {
      ASSERT_EQ(s, r);
      ASSERT_EQ(c, r->c_str());
    }
    t.assert_on_arena(*r, false, false);
    delete r;
    s = &m->rs(m->rs_size() - 1);
    c = s->c_str();
    r = rs->ReleaseLast();
    ASSERT_EQ(long_string + long_string, *r);
    if (arena) {
      ASSERT_NE(s, r);
      ASSERT_EQ(c, r->c_str());
    } else {
      ASSERT_EQ(s, r);
      ASSERT_EQ(c, r->c_str());
    }
    t.assert_on_arena(*r, false, false);
    delete r;
    s = &m->rs(m->rs_size() - 1);
    c = s->c_str();
    r = rs->ReleaseLast();
    ASSERT_EQ(long_string, *r);
    if (arena) {
      ASSERT_NE(s, r);
      ASSERT_EQ(c, r->c_str());
    } else {
      ASSERT_EQ(s, r);
      ASSERT_EQ(c, r->c_str());
    }
    t.assert_on_arena(*r, false, false);
    delete r;
  }
  {
    auto mm = Arena::CreateMessage<M>(arena);
    mm->mutable_rs()->AddString()->assign(long_string);
    mm->mutable_rs()->AddAccessor()->assign(long_string);
    auto s = &mm->rs(mm->rs_size() - 1);
    auto c = s->c_str();
    rs->UnsafeArenaAddAllocated(mm->mutable_rs()->UnsafeArenaReleaseLast());
    auto ss = &m->rs(m->rs_size() - 1);
    auto cc = ss->c_str();
    ASSERT_EQ(*s, *ss);
    ASSERT_EQ(s, ss);
    ASSERT_EQ(c, cc);
    t.assert_on_arena(*s, arena, arena);
    s = &mm->rs(mm->rs_size() - 1);
    c = s->c_str();
    rs->UnsafeArenaAddAllocated(mm->mutable_rs()->UnsafeArenaReleaseLast());
    ss = &m->rs(m->rs_size() - 1);
    cc = ss->c_str();
    ASSERT_EQ(*s, *ss);
    ASSERT_EQ(s, ss);
    ASSERT_EQ(c, cc);
    t.assert_on_arena(*s, arena, false);
    if (!arena) {
      delete mm;
    }
  }
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, repeated_add_allocated_and_release_on_arena) {
  repeated_add_allocated_and_release_on_arena<Proto2>(*this, arena);
  repeated_add_allocated_and_release_on_arena<Proto2>(*this, nullptr);
  repeated_add_allocated_and_release_on_arena<ArenaProto2>(*this, arena);
  repeated_add_allocated_and_release_on_arena<ArenaProto2>(*this, nullptr);
  repeated_add_allocated_and_release_on_arena<Proto3>(*this, arena);
  repeated_add_allocated_and_release_on_arena<Proto3>(*this, nullptr);
  repeated_add_allocated_and_release_on_arena<ArenaProto3>(*this, arena);
  repeated_add_allocated_and_release_on_arena<ArenaProto3>(*this, nullptr);
}

template <typename M, typename T>
static void mutable_string_on_arena(T& t, Arena* arena) {
  auto assert_mutable_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(
        s, arena,
        arena &&
            M().GetDescriptor()->file()->options().cc_mutable_donated_string());
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_s(short_string);
  m->mutable_s()->assign(long_string);
  ASSERT_EQ(long_string, m->s());
  assert_mutable_on_arena(m->s());
  m->mutable_s()->assign(short_string);
  ASSERT_EQ(short_string, m->s());
  assert_mutable_on_arena(m->s());
  m->mutable_s()->clear();
  m->mutable_s()->push_back('x');
  m->mutable_s()->append("10086");
  ASSERT_EQ("x10086", m->s());
  assert_mutable_on_arena(m->s());
  m->set_ons(short_string);
  m->mutable_ons()->assign(long_string);
  ASSERT_EQ(long_string, m->ons());
  assert_mutable_on_arena(m->ons());
  m->mutable_ons()->assign(short_string);
  ASSERT_EQ(short_string, m->ons());
  assert_mutable_on_arena(m->ons());
  m->mutable_ons()->clear();
  m->mutable_ons()->push_back('x');
  m->mutable_ons()->append("10086");
  ASSERT_EQ("x10086", m->ons());
  assert_mutable_on_arena(m->ons());
  m->add_rs(short_string);
  m->mutable_rs(0)->assign(long_string);
  ASSERT_EQ(long_string, m->rs(0));
  assert_mutable_on_arena(m->rs(0));
  m->mutable_rs(0)->assign(short_string);
  ASSERT_EQ(short_string, m->rs(0));
  assert_mutable_on_arena(m->rs(0));
  m->mutable_rs(0)->clear();
  m->mutable_rs(0)->push_back('x');
  m->mutable_rs(0)->append("10086");
  ASSERT_EQ("x10086", m->rs(0));
  assert_mutable_on_arena(m->rs(0));
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, mutable_string_on_arena) {
  mutable_string_on_arena<Proto3>(*this, arena);
  mutable_string_on_arena<Proto3>(*this, nullptr);
  mutable_string_on_arena<ArenaProto3>(*this, arena);
  mutable_string_on_arena<ArenaProto3>(*this, nullptr);
  mutable_string_on_arena<Proto2>(*this, arena);
  mutable_string_on_arena<Proto2>(*this, nullptr);
  mutable_string_on_arena<ArenaProto2>(*this, arena);
  mutable_string_on_arena<ArenaProto2>(*this, nullptr);
}

template <typename M, typename T>
static void mutable_string_on_arena_pb2(T& t, Arena* arena) {
  auto assert_mutable_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(
        s, arena,
        arena &&
            M().GetDescriptor()->file()->options().cc_mutable_donated_string());
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->set_ds(short_string);
  m->mutable_ds()->assign(long_string);
  ASSERT_EQ(long_string, m->ds());
  assert_mutable_on_arena(m->ds());
  m->mutable_ds()->assign(short_string);
  ASSERT_EQ(short_string, m->ds());
  assert_mutable_on_arena(m->ds());
  m->mutable_ds()->clear();
  m->mutable_ds()->push_back('x');
  m->mutable_ds()->append("10086");
  ASSERT_EQ("x10086", m->ds());
  assert_mutable_on_arena(m->ds());
  m->set_qs(short_string);
  m->mutable_qs()->assign(long_string);
  ASSERT_EQ(long_string, m->qs());
  assert_mutable_on_arena(m->qs());
  m->mutable_qs()->assign(short_string);
  ASSERT_EQ(short_string, m->qs());
  assert_mutable_on_arena(m->qs());
  m->mutable_qs()->clear();
  m->mutable_qs()->push_back('x');
  m->mutable_qs()->append("10086");
  ASSERT_EQ("x10086", m->qs());
  assert_mutable_on_arena(m->qs());
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, mutable_string_on_arena_pb2) {
  mutable_string_on_arena_pb2<Proto2>(*this, arena);
  mutable_string_on_arena_pb2<Proto2>(*this, nullptr);
  mutable_string_on_arena_pb2<ArenaProto2>(*this, arena);
  mutable_string_on_arena_pb2<ArenaProto2>(*this, nullptr);
}

template <typename M, typename E, typename T>
static void extension_on_arena(T& t, Arena* arena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, arena, arena);
  };
  auto assert_mutable_on_arena = [&](const ::std::string& s) {
#if GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
    t.assert_on_arena(s, arena, arena);
#else   // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
    t.assert_on_arena(s, arena, false);
#endif  // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
  };
  auto* m = Arena::CreateMessage<M>(arena);
  m->SetExtension(E::es, short_string);
  ASSERT_EQ(short_string, m->GetExtension(E::es));
  assert_on_arena(m->GetExtension(E::es));
  m->ClearExtension(E::es);
  m->SetExtension(E::es, long_string);
  ASSERT_EQ(long_string, m->GetExtension(E::es));
  assert_on_arena(m->GetExtension(E::es));
  m->MutableExtension(E::es)->append(long_string);
  ASSERT_EQ(long_string + long_string, m->GetExtension(E::es));
  assert_mutable_on_arena(m->GetExtension(E::es));
  m->ClearExtension(E::es);
  m->MutableExtension(E::es)->assign(long_string.c_str());
  ASSERT_EQ(long_string, m->GetExtension(E::es));
  ASSERT_LE(long_string.size() * 2, m->GetExtension(E::es).capacity());
  assert_mutable_on_arena(m->GetExtension(E::es));
  m->AddExtension(E::ers, long_string);
  ASSERT_EQ(long_string, m->GetExtension(E::ers, 0));
  assert_on_arena(m->GetExtension(E::ers, 0));
  m->MutableExtension(E::ers, 0)->append(long_string);
  ASSERT_EQ(long_string + long_string, m->GetExtension(E::ers, 0));
  assert_mutable_on_arena(m->GetExtension(E::ers, 0));
  if (!arena) {
    delete m;
  }
}
TEST_F(ArenaStringTest, extension_on_arena) {
  extension_on_arena<Proto2, Proto2Extension>(*this, arena);
  extension_on_arena<Proto2, Proto2Extension>(*this, nullptr);
  extension_on_arena<ArenaProto2, ArenaProto2Extension>(*this, arena);
  extension_on_arena<ArenaProto2, ArenaProto2Extension>(*this, nullptr);
}

template <typename M, typename E, typename T>
static void extension_parse_and_merge_on_arena(T& t, Arena* farena,
                                               Arena* tarena) {
  auto assert_on_arena = [&](const ::std::string& s) {
    t.assert_on_arena(s, tarena, tarena);
  };
  auto assert_mutable_on_arena = [&](const ::std::string& s) {
#if GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
    t.assert_on_arena(s, tarena, tarena);
#else   // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
    t.assert_on_arena(s, tarena, false);
#endif  // !GOOGLE_PROTOBUF_MUTABLE_DONATED_STRING
  };
  ::std::string string;
  auto* fm = Arena::CreateMessage<M>(farena);
  auto* tm = Arena::CreateMessage<M>(tarena);
  tm->AddExtension(E::ers, short_string);
  fm->SetExtension(E::es, short_string);
  fm->set_qs(long_string);
  fm->set_qb(long_string);
  fm->set_qc(long_string);
  fm->AddExtension(E::ers, long_string);
  fm->AddExtension(E::ers, long_string);
  ASSERT_TRUE(fm->SerializeToString(&string));
  ASSERT_TRUE(tm->ParseFromString(string));
  ASSERT_EQ(short_string, tm->GetExtension(E::es));
  assert_on_arena(tm->GetExtension(E::es));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 0));
  assert_on_arena(tm->GetExtension(E::ers, 0));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 1));
  assert_on_arena(tm->GetExtension(E::ers, 1));
  tm->MutableExtension(E::es)->assign(long_string.c_str());
  tm->MutableExtension(E::ers, 0)->append(long_string);
  ASSERT_TRUE(tm->ParseFromString(string));
  ASSERT_EQ(short_string, tm->GetExtension(E::es));
  assert_mutable_on_arena(tm->GetExtension(E::es));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 0));
  assert_mutable_on_arena(tm->GetExtension(E::ers, 0));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 1));
  assert_on_arena(tm->GetExtension(E::ers, 1));
  if (!tarena) {
    delete tm;
  }
  tm = Arena::CreateMessage<M>(tarena);
  tm->MergeFrom(*fm);
  ASSERT_EQ(short_string, tm->GetExtension(E::es));
  assert_on_arena(tm->GetExtension(E::es));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 0));
  assert_on_arena(tm->GetExtension(E::ers, 0));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 1));
  assert_on_arena(tm->GetExtension(E::ers, 1));
  tm->MutableExtension(E::es)->assign(long_string.c_str());
  tm->MutableExtension(E::ers, 0)->append(long_string);
  tm->CopyFrom(*fm);
  ASSERT_EQ(short_string, tm->GetExtension(E::es));
  assert_mutable_on_arena(tm->GetExtension(E::es));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 0));
  assert_mutable_on_arena(tm->GetExtension(E::ers, 0));
  ASSERT_EQ(long_string, tm->GetExtension(E::ers, 1));
  assert_on_arena(tm->GetExtension(E::ers, 1));
  if (!farena) {
    delete fm;
  }
  if (!tarena) {
    delete tm;
  }
}
TEST_F(ArenaStringTest, extension_parse_and_merge_on_arena) {
  extension_parse_and_merge_on_arena<Proto2, Proto2Extension>(*this, arena,
                                                              arena);
  extension_parse_and_merge_on_arena<Proto2, Proto2Extension>(*this, arena,
                                                              nullptr);
  extension_parse_and_merge_on_arena<Proto2, Proto2Extension>(*this, nullptr,
                                                              arena);
  extension_parse_and_merge_on_arena<Proto2, Proto2Extension>(*this, nullptr,
                                                              nullptr);
  extension_parse_and_merge_on_arena<ArenaProto2, ArenaProto2Extension>(
      *this, arena, arena);
  extension_parse_and_merge_on_arena<ArenaProto2, ArenaProto2Extension>(
      *this, arena, nullptr);
  extension_parse_and_merge_on_arena<ArenaProto2, ArenaProto2Extension>(
      *this, nullptr, arena);
  extension_parse_and_merge_on_arena<ArenaProto2, ArenaProto2Extension>(
      *this, nullptr, nullptr);
}
