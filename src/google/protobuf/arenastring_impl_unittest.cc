#include "google/protobuf/arenastring_impl.h"

#include "google/protobuf/arena.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/reflection.h"
#include "google/protobuf/text_format.h"
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

// ============================================================================
// v2.1 Map DonatedString unit tests.
//
// These tests directly drive ::google::protobuf::Map<K, V> with K/V being
// std::string, exercising:
//   - INV-1: any non-const std::string& exposed to user code must NOT be in
//            Donated state (operator[], at(), iterator deref must promote).
//   - INV-2: a Donated string's data() must be inside the arena.
//   - INV-3: the donated_flags tag must agree with reality after promote.
//   - INV-4: arena == nullptr means strict fallback to standard std::string.
//   - INV-5: erase/clear must not invoke ~basic_string() on a Donated string
//            (otherwise the arena buffer would be free()'d).
// ============================================================================

// Assert that the character buffer of `s` lives inside the test arena buffer.
static void AssertDonatedDataInArena(ArenaStringTest& t,
                                     const std::string& s) {
  if (s.capacity() > 0) {
    t.assert_address_on_arena(s.data(), true);
  }
}
// Assert that the character buffer of `s` does NOT live inside the arena.
static void AssertDataNotInArena(ArenaStringTest& t, const std::string& s) {
  if (s.capacity() > 0) {
    t.assert_address_on_arena(s.data(), false);
  }
}

// --- INV-2: try_emplace puts value (and string-key) in arena ---
TEST_F(ArenaStringTest, MapValueDonated_Insert_DataInArena) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  auto r = m->try_emplace("key1", long_string);
  ASSERT_TRUE(r.second);
  // Use const_iterator to read without triggering promote.
  const auto& cm = *m;
  auto cit = cm.find("key1");
  ASSERT_NE(cit, cm.end());
  ASSERT_EQ(long_string, cit->second);
  AssertDonatedDataInArena(*this, cit->second);
  AssertDonatedDataInArena(*this, cit->first);
}

// --- INV-4: arena == nullptr falls back to standard std::string ---
TEST_F(ArenaStringTest, MapValueDonated_NoArena_FallbackPath) {
  ::google::protobuf::Map<std::string, std::string> m;
  auto r = m.try_emplace("key1", long_string);
  ASSERT_TRUE(r.second);
  auto it = m.find("key1");
  ASSERT_NE(it, m.end());
  ASSERT_EQ(long_string, it->second);
  AssertDataNotInArena(*this, it->second);
  AssertDataNotInArena(*this, it->first);
  // Mutating accesses are fully safe in the no-arena path.
  it->second.append("!!");
  ASSERT_EQ(long_string + "!!", it->second);
}

// --- INV-1: const-iterator does NOT promote Donated values ---
TEST_F(ArenaStringTest, MapValueDonated_ConstIterator_KeepsDonated) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace("k", long_string);
  m->try_emplace("k2", short_string);
  const auto& cm = *m;
  for (const auto& kv : cm) {
    AssertDonatedDataInArena(*this, kv.second);
    AssertDonatedDataInArena(*this, kv.first);
  }
}

// --- INV-1: non-const iterator deref promotes ---
TEST_F(ArenaStringTest, MapValueDonated_Iterator_TriggersPromote) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace("k", long_string);
  {
    const auto& cm = *m;
    AssertDonatedDataInArena(*this, cm.find("k")->second);
  }
  auto it = m->find("k");
  ASSERT_EQ(long_string, it->second);
  AssertDataNotInArena(*this, it->second);
  // INV-1: any standard std::string mutating op is now safe.
  it->second.append(" extra");
  ASSERT_EQ(long_string + " extra", it->second);
  // Promoting again is a no-op: tag was cleared.
  AssertDataNotInArena(*this, m->find("k")->second);
}

// --- INV-1: range-for mutable promotes every entry on demand ---
TEST_F(ArenaStringTest, MapValueDonated_RangeForMutable_TriggersPromote) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  for (int i = 0; i < 8; ++i) {
    m->try_emplace("k" + std::to_string(i), long_string);
  }
  for (auto& kv : *m) {
    kv.second.append("!");  // would crash without lazy promote
  }
  for (auto& kv : *m) {
    AssertDataNotInArena(*this, kv.second);
    ASSERT_EQ(long_string.size() + 1, kv.second.size());
  }
}

// --- INV-1: at() non-const overload promotes ---
TEST_F(ArenaStringTest, MapValueDonated_AtNonConst_TriggersPromote) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace("k", long_string);
  std::string& s = m->at("k");
  AssertDataNotInArena(*this, s);
  s.reserve(s.capacity() * 4);  // would crash if s.data() were in arena
  ASSERT_EQ(long_string, s);
}

// --- INV-1: operator[] non-const overload promotes ---
TEST_F(ArenaStringTest, MapValueDonated_OperatorBracket_TriggersPromote) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace("k", long_string);
  std::string& s = (*m)["k"];
  AssertDataNotInArena(*this, s);
  s.append(" appended");
  ASSERT_EQ(long_string + " appended", s);
}

// --- After promote, shrink_to_fit / reserve(0) must be safe (INV-1 cont.) ---
TEST_F(ArenaStringTest, MapValueDonated_ShrinkToFitAfterPromote_NoCrash) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace("k", long_string);
  std::string& s = m->at("k");  // promote
  s.shrink_to_fit();
  s.reserve(0);
  ASSERT_EQ(long_string, s);
}

// --- INV-5: erase a Donated entry must not free arena buffer ---
TEST_F(ArenaStringTest, MapValueDonated_Erase_NoDoubleFree) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace("k", long_string);
  ASSERT_EQ(1u, m->erase("k"));
  ASSERT_EQ(0u, m->size());
}

// --- INV-3: erase a promoted entry; heap buffer cleaned up on arena reset ---
TEST_F(ArenaStringTest, MapValueDonated_Erase_AfterPromote_NormalDestruct) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace("k", long_string);
  (void)m->at("k");  // promote -> tag cleared, destructor registered
  ASSERT_EQ(1u, m->erase("k"));
  ASSERT_EQ(0u, m->size());
}

// --- INV-5: Clear a Donated map ---
TEST_F(ArenaStringTest, MapValueDonated_Clear_NoCrash) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  for (int i = 0; i < 16; ++i) {
    m->try_emplace("k" + std::to_string(i), long_string);
  }
  m->clear();
  ASSERT_EQ(0u, m->size());
}

// --- Rehash: many inserts trigger Resize; existing entries stay valid ---
TEST_F(ArenaStringTest, MapValueDonated_Rehash_StateCorrect) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  constexpr int kN = 256;
  for (int i = 0; i < kN; ++i) {
    m->try_emplace("k_" + std::to_string(i), long_string);
  }
  ASSERT_EQ(static_cast<size_t>(kN), m->size());
  const auto& cm = *m;
  for (int i = 0; i < kN; ++i) {
    auto it = cm.find("k_" + std::to_string(i));
    ASSERT_NE(it, cm.end());
    ASSERT_EQ(long_string, it->second);
    AssertDonatedDataInArena(*this, it->second);
  }
}

// --- Same-arena swap: representation swap; data pointers don't move ---
TEST_F(ArenaStringTest, MapValueDonated_Swap_SameArena_NoCopy) {
  auto* a = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  auto* b = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  a->try_emplace("ak", long_string);
  b->try_emplace("bk", short_string);
  const char* a_value_data = a->find("ak")->second.data();
  const char* b_value_data = b->find("bk")->second.data();
  a->swap(*b);
  ASSERT_EQ(long_string, b->find("ak")->second);
  ASSERT_EQ(short_string, a->find("bk")->second);
  ASSERT_EQ(a_value_data, b->find("ak")->second.data());
  ASSERT_EQ(b_value_data, a->find("bk")->second.data());
}

// --- Cross-arena swap forces deep copy through the copy ctor path ---
TEST_F(ArenaStringTest, MapValueDonated_Swap_CrossArena_DeepCopy) {
  ::google::protobuf::ArenaOptions opts;
  static char buffer2[1L << 20];
  opts.initial_block = buffer2;
  opts.initial_block_size = sizeof(buffer2);
  ::google::protobuf::Arena arena2(opts);
  auto* a = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  auto* b = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(&arena2);
  a->try_emplace("ak", long_string);
  b->try_emplace("bk", short_string);
  a->swap(*b);
  ASSERT_EQ(short_string, a->find("bk")->second);
  ASSERT_EQ(long_string, b->find("ak")->second);
}

// --- Initializer-list insert in no-arena Map ---
TEST_F(ArenaStringTest, MapValueDonated_InitializerList) {
  ::google::protobuf::Map<std::string, std::string> m;
  m.insert({{"a", long_string}, {"b", short_string}});
  ASSERT_EQ(long_string, m.find("a")->second);
  ASSERT_EQ(short_string, m.find("b")->second);
}

// --- Direct API: init_donated_in_place / promote_donated_to_heap /
//     relocate_donated round-trip ---
TEST_F(ArenaStringTest, DonatedApi_InitPromoteRelocate) {
  void* raw = arena->AllocateAligned(sizeof(std::string));
  std::string* s = reinterpret_cast<std::string*>(raw);
  auto acc = ::google::protobuf::internal::init_donated_in_place(arena, s);
  acc.assign(long_string.data(), long_string.size());
  ASSERT_EQ(long_string, *s);
  assert_address_on_arena(s->data(), true);

  ::google::protobuf::internal::promote_donated_to_heap(s);
  ASSERT_EQ(long_string, *s);
  assert_address_on_arena(s->data(), false);
  s->reserve(s->capacity() * 4);
  s->append(" tail");
  ASSERT_EQ(long_string + " tail", *s);
  s->~basic_string();  // we did not register; manually clean up.

  ::google::protobuf::ArenaOptions opts2;
  static char buffer3[1L << 18];
  opts2.initial_block = buffer3;
  opts2.initial_block_size = sizeof(buffer3);
  ::google::protobuf::Arena arena2(opts2);
  void* dst_raw = arena2.AllocateAligned(sizeof(std::string));
  void* src_raw = arena->AllocateAligned(sizeof(std::string));
  auto* dst = reinterpret_cast<std::string*>(dst_raw);
  auto* src = reinterpret_cast<std::string*>(src_raw);
  ::google::protobuf::internal::init_donated_in_place(arena, src)
      .assign(short_string.data(), short_string.size());
  bool dst_is_donated = false;
  ::google::protobuf::internal::relocate_donated(
      dst, src, &arena2, arena, /*src_is_donated=*/true, &dst_is_donated);
  ASSERT_TRUE(dst_is_donated);
  ASSERT_EQ(short_string, *dst);
}

// ============================================================================
// v2.1 Map DonatedString unit tests — phase-2 additions.
//
// These close the gaps that the first batch left as TODOs once the TC parser
// map handler and the donated map-key path landed. They cover:
//   - §4.6 #1   MapValueDonated_Parse_DataInArena
//   - §4.6 #21  MapValueDonated_DuplicateKey_Parse
//   - §4.6 #18  MapValueDonated_NoArena_SwapMatrix (the remaining 3 rows)
//   - §5.3 #1   MapKeyDonated_Parse_KeyInArena
//   - §5.3 #2/3/4 MapKeyDonated_Lookup_{StdString,StringView,CString}
//   - §5.3 #5   MapKeyDonated_Rehash_KeyStable
//
// Note on the TC parser donated path: per InitializeMapNodeEntry /
// ParseOneMapEntry in generated_message_tctable_lite.cc, the parser routes
// map key/value bytes into arena memory whenever the map lives on an arena
// (arena != nullptr) — it does NOT depend on cc_mutable_donated_string. So we
// exercise both Proto3 (option off) and ArenaProto3 (option on); both must
// behave identically for the map donated path.
// ============================================================================

// --- §4.6 #1: parsing a map<string,string> on an arena routes value bytes
//     into arena memory (Donated). const access keeps them Donated (INV-1). ---
template <typename M>
static void MapValueParseDataInArena(ArenaStringTest& t, Arena* tarena) {
  M fm;  // source message lives on the heap (no arena)
  (*fm.mutable_ms())["k_long"] = long_string;
  (*fm.mutable_ms())["k_short"] = short_string;
  std::string data;
  ASSERT_TRUE(fm.SerializeToString(&data));

  auto* tm = Arena::CreateMessage<M>(tarena);
  ASSERT_TRUE(tm->ParseFromString(data));
  // const access via Message::ms() must NOT promote the Donated values.
  const auto& ms = tm->ms();
  ASSERT_EQ(2u, ms.size());
  auto it_long = ms.find("k_long");
  ASSERT_NE(it_long, ms.end());
  ASSERT_EQ(long_string, it_long->second);
  auto it_short = ms.find("k_short");
  ASSERT_NE(it_short, ms.end());
  ASSERT_EQ(short_string, it_short->second);
  if (tarena != nullptr) {
    // Donated: value char buffers live inside the arena.
    t.assert_address_on_arena(it_long->second.data(), true);
    t.assert_address_on_arena(it_short->second.data(), true);
  } else {
    // INV-4: no arena means a plain heap-backed std::string.
    t.assert_address_on_arena(it_long->second.data(), false);
    t.assert_address_on_arena(it_short->second.data(), false);
  }
  if (tarena == nullptr) delete tm;
}
TEST_F(ArenaStringTest, MapValueDonated_Parse_DataInArena) {
  MapValueParseDataInArena<Proto3>(*this, arena);
  MapValueParseDataInArena<Proto3>(*this, nullptr);
  MapValueParseDataInArena<ArenaProto3>(*this, arena);
  MapValueParseDataInArena<ArenaProto3>(*this, nullptr);
}

// --- §4.6 #21: a duplicate map key on the wire collapses to a single entry
//     with the last value winning; the surviving value stays Donated and no
//     double-free occurs when the superseded node is reclaimed by the arena. ---
template <typename M>
static void MapValueParseDuplicateKey(ArenaStringTest& t, Arena* tarena) {
  M fm1;
  (*fm1.mutable_ms())["dup"] = short_string;
  M fm2;
  (*fm2.mutable_ms())["dup"] = long_string;
  std::string d1, d2;
  ASSERT_TRUE(fm1.SerializeToString(&d1));
  ASSERT_TRUE(fm2.SerializeToString(&d2));
  // The same map key "dup" now appears twice in the serialized stream.
  std::string data = d1 + d2;

  auto* tm = Arena::CreateMessage<M>(tarena);
  ASSERT_TRUE(tm->ParseFromString(data));
  const auto& ms = tm->ms();
  ASSERT_EQ(1u, ms.size());  // duplicate collapses to one entry
  auto it = ms.find("dup");
  ASSERT_NE(it, ms.end());
  ASSERT_EQ(long_string, it->second);  // last value on the wire wins
  if (tarena != nullptr) {
    t.assert_address_on_arena(it->second.data(), true);
  } else {
    t.assert_address_on_arena(it->second.data(), false);
  }
  if (tarena == nullptr) delete tm;
}
TEST_F(ArenaStringTest, MapValueDonated_DuplicateKey_Parse) {
  MapValueParseDuplicateKey<Proto3>(*this, arena);
  MapValueParseDuplicateKey<Proto3>(*this, nullptr);
  MapValueParseDuplicateKey<ArenaProto3>(*this, arena);
  MapValueParseDuplicateKey<ArenaProto3>(*this, nullptr);
}

// --- §4.6 #18: complete the no-arena rows of the Swap matrix. The same-arena
//     and cross-arena rows are already covered by Swap_SameArena_NoCopy and
//     Swap_CrossArena_DeepCopy; here we add the three rows that involve at
//     least one arena-less map:
//       row 1: (null, null)   -> same arena -> InternalSwap (representation)
//       row 2: (arena, null)  -> different  -> deep copy
//       row 3: (null, arena)  -> different  -> deep copy
TEST_F(ArenaStringTest, MapValueDonated_NoArena_SwapMatrix) {
  using MapT = ::google::protobuf::Map<std::string, std::string>;

  // row 1: both maps have no arena -> representation swap, no copies.
  {
    MapT a, b;
    a.try_emplace("ak", long_string);
    b.try_emplace("bk", short_string);
    a.swap(b);
    ASSERT_EQ(long_string, b.find("ak")->second);
    ASSERT_EQ(short_string, a.find("bk")->second);
    AssertDataNotInArena(*this, b.find("ak")->second);
    AssertDataNotInArena(*this, a.find("bk")->second);
  }

  // row 2: lhs on arena, rhs arena-less -> different arenas -> deep copy.
  {
    auto* a = ::google::protobuf::Arena::Create<MapT>(arena);
    MapT b;
    a->try_emplace("ak", long_string);
    b.try_emplace("bk", short_string);
    a->swap(b);
    ASSERT_EQ(long_string, b.find("ak")->second);
    ASSERT_EQ(short_string, a->find("bk")->second);
    // `b` is now fully heap-backed.
    AssertDataNotInArena(*this, b.find("ak")->second);
  }

  // row 3: lhs arena-less, rhs on arena -> different arenas -> deep copy.
  {
    MapT a;
    auto* b = ::google::protobuf::Arena::Create<MapT>(arena);
    a.try_emplace("ak", long_string);
    b->try_emplace("bk", short_string);
    a.swap(*b);
    ASSERT_EQ(short_string, a.find("bk")->second);
    ASSERT_EQ(long_string, b->find("ak")->second);
    AssertDataNotInArena(*this, a.find("bk")->second);
  }
}

// --- §5.3 #1: parsing a map<string,string> with a long (non-SSO) key on an
//     arena routes the key bytes into arena memory (Donated). ---
template <typename M>
static void MapKeyParseKeyInArena(ArenaStringTest& t, Arena* tarena) {
  const std::string long_key = long_string;  // guaranteed non-SSO
  M fm;
  (*fm.mutable_ms())[long_key] = short_string;
  std::string data;
  ASSERT_TRUE(fm.SerializeToString(&data));

  auto* tm = Arena::CreateMessage<M>(tarena);
  ASSERT_TRUE(tm->ParseFromString(data));
  const auto& ms = tm->ms();
  auto it = ms.find(long_key);
  ASSERT_NE(it, ms.end());
  ASSERT_EQ(long_key, it->first);
  ASSERT_EQ(short_string, it->second);
  if (tarena != nullptr) {
    // Donated key: the key char buffer lives inside the arena.
    t.assert_address_on_arena(it->first.data(), true);
  } else {
    t.assert_address_on_arena(it->first.data(), false);
  }
  if (tarena == nullptr) delete tm;
}
TEST_F(ArenaStringTest, MapKeyDonated_Parse_KeyInArena) {
  MapKeyParseKeyInArena<Proto3>(*this, arena);
  MapKeyParseKeyInArena<Proto3>(*this, nullptr);
  MapKeyParseKeyInArena<ArenaProto3>(*this, arena);
  MapKeyParseKeyInArena<ArenaProto3>(*this, nullptr);
}

// --- §5.3 #2: heterogeneous lookup of a Donated key by std::string. The key
//     buffer must remain Donated (lookup never promotes the key). ---
TEST_F(ArenaStringTest, MapKeyDonated_Lookup_StdString) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace(long_string, short_string);  // long key -> Donated
  const auto& cm = *m;
  std::string probe = long_string;
  auto it = cm.find(probe);
  ASSERT_NE(it, cm.end());
  ASSERT_EQ(short_string, it->second);
  AssertDonatedDataInArena(*this, it->first);
}

// --- §5.3 #3: heterogeneous lookup of a Donated key by absl::string_view. ---
TEST_F(ArenaStringTest, MapKeyDonated_Lookup_StringView) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace(long_string, short_string);
  const auto& cm = *m;
  ::absl::string_view probe(long_string);
  auto it = cm.find(probe);
  ASSERT_NE(it, cm.end());
  ASSERT_EQ(short_string, it->second);
  AssertDonatedDataInArena(*this, it->first);
}

// --- §5.3 #4: heterogeneous lookup of a Donated key by C string. ---
TEST_F(ArenaStringTest, MapKeyDonated_Lookup_CString) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  m->try_emplace(long_string, short_string);
  const auto& cm = *m;
  auto it = cm.find(long_string.c_str());
  ASSERT_NE(it, cm.end());
  ASSERT_EQ(short_string, it->second);
  AssertDonatedDataInArena(*this, it->first);
}

// --- §5.3 #5: a rehash-heavy workload keeps Donated keys stable and in arena.
//     Rehashing relinks nodes but never moves their physical address, so the
//     in-node key control block (and its arena buffer) stays put. ---
TEST_F(ArenaStringTest, MapKeyDonated_Rehash_KeyStable) {
  auto* m = ::google::protobuf::Arena::Create<
      ::google::protobuf::Map<std::string, std::string>>(arena);
  constexpr int kN = 256;
  const std::string prefix(64, 'K');  // ensure every key is non-SSO
  for (int i = 0; i < kN; ++i) {
    m->try_emplace(prefix + std::to_string(i), long_string);
  }
  ASSERT_EQ(static_cast<size_t>(kN), m->size());
  const auto& cm = *m;
  for (int i = 0; i < kN; ++i) {
    auto it = cm.find(prefix + std::to_string(i));
    ASSERT_NE(it, cm.end());
    ASSERT_EQ(long_string, it->second);
    AssertDonatedDataInArena(*this, it->first);
  }
}

// --- §5.3 #6 (semantics corrected vs. the original "OldKeyKept" name): a
//     duplicate map key on the wire replaces the WHOLE node via
//     InsertOrReplaceNode (not just the value). The superseded node is unlinked
//     and — on an arena — left for the arena to reclaim (no eager destruction,
//     no double-free of the donated buffer). The user-visible result is
//     size()==1 with the last value winning, and the surviving node has freshly
//     donated key/value. ---
template <typename M>
static void MapKeyDuplicateNodeReplaced(ArenaStringTest& t, Arena* tarena) {
  const std::string long_key = long_string;  // non-SSO key
  M fm1;
  (*fm1.mutable_ms())[long_key] = short_string;
  M fm2;
  (*fm2.mutable_ms())[long_key] = long_string;
  std::string d1, d2;
  ASSERT_TRUE(fm1.SerializeToString(&d1));
  ASSERT_TRUE(fm2.SerializeToString(&d2));
  std::string data = d1 + d2;  // the long key appears twice on the wire

  auto* tm = Arena::CreateMessage<M>(tarena);
  ASSERT_TRUE(tm->ParseFromString(data));
  const auto& ms = tm->ms();
  ASSERT_EQ(1u, ms.size());
  auto it = ms.find(long_key);
  ASSERT_NE(it, ms.end());
  ASSERT_EQ(long_key, it->first);
  ASSERT_EQ(long_string, it->second);  // last value on the wire wins
  if (tarena != nullptr) {
    // The surviving node is freshly donated for both key and value.
    t.assert_address_on_arena(it->first.data(), true);
    t.assert_address_on_arena(it->second.data(), true);
  } else {
    t.assert_address_on_arena(it->first.data(), false);
    t.assert_address_on_arena(it->second.data(), false);
  }
  if (tarena == nullptr) delete tm;
}
TEST_F(ArenaStringTest, MapKeyDonated_DuplicateKey_NodeReplaced) {
  MapKeyDuplicateNodeReplaced<Proto3>(*this, arena);
  MapKeyDuplicateNodeReplaced<Proto3>(*this, nullptr);
  MapKeyDuplicateNodeReplaced<ArenaProto3>(*this, arena);
  MapKeyDuplicateNodeReplaced<ArenaProto3>(*this, nullptr);
}

// --- §5.3 #9: mixed donated/promoted state across a duplicate key. The first
//     parse puts a Donated entry on the arena; a mutable access promotes its
//     value to a heap-owned std::string (and registers an arena destructor).
//     Merging a duplicate key then drives InsertOrReplaceNode: the promoted
//     node is unlinked (left for the arena to reclaim) and replaced by a fresh
//     Donated node. The result must be a single, correct, re-donated entry with
//     no double-free. ---
template <typename M>
static void MapKeyDuplicateMixedState(ArenaStringTest& t, Arena* tarena) {
  const std::string long_key = long_string;
  M fm1;
  (*fm1.mutable_ms())[long_key] = short_string;
  M fm2;
  (*fm2.mutable_ms())[long_key] = long_string;
  std::string d1, d2;
  ASSERT_TRUE(fm1.SerializeToString(&d1));
  ASSERT_TRUE(fm2.SerializeToString(&d2));

  auto* tm = Arena::CreateMessage<M>(tarena);
  ASSERT_TRUE(tm->ParseFromString(d1));  // entry is Donated
  // Promote the value to a heap-owned std::string via a mutable access.
  (*tm->mutable_ms())[long_key].append("!");
  ASSERT_EQ(short_string + "!", tm->ms().find(long_key)->second);
  // Merge a duplicate key: replaces the promoted node with a fresh Donated one.
  ASSERT_TRUE(tm->MergeFromString(d2));
  const auto& ms = tm->ms();
  ASSERT_EQ(1u, ms.size());
  auto it = ms.find(long_key);
  ASSERT_NE(it, ms.end());
  ASSERT_EQ(long_string, it->second);  // merged value wins
  if (tarena != nullptr) {
    t.assert_address_on_arena(it->first.data(), true);
    t.assert_address_on_arena(it->second.data(), true);
  }
  if (tarena == nullptr) delete tm;
}
TEST_F(ArenaStringTest, MapKeyDonated_DuplicateKey_MixedState) {
  MapKeyDuplicateMixedState<Proto3>(*this, arena);
  MapKeyDuplicateMixedState<Proto3>(*this, nullptr);
  MapKeyDuplicateMixedState<ArenaProto3>(*this, arena);
  MapKeyDuplicateMixedState<ArenaProto3>(*this, nullptr);
}

// --- §4.6 #9 / §4.3.3: the protoc-generated hot-path accessor
//     `mutable_xxx_accessor(key)` lets user code write a large string straight
//     into arena memory WITHOUT promoting the Donated value (INV-2). It is only
//     generated when the file sets cc_mutable_donated_string=true, so this test
//     drives ArenaProto2 / ArenaProto3 (which opt in) rather than Proto2/Proto3.
//
//     The accessor never exposes a raw std::string&, so the value stays Donated
//     and its data() keeps living in the arena across repeated writes. With no
//     arena it falls back to standard std::string semantics (INV-4).
template <typename M>
static void MapMutableAccessorNoPromote(ArenaStringTest& t, Arena* arena) {
  auto* m = Arena::CreateMessage<M>(arena);
  // Hot-path write: assign a large (non-SSO) value through the accessor.
  m->mutable_ms_accessor("k").assign(long_string.data(), long_string.size());
  {
    const auto& ms = m->ms();  // const access: no promote
    auto it = ms.find("k");
    ASSERT_NE(it, ms.end());
    ASSERT_EQ(long_string, it->second);
    if (arena != nullptr) {
      t.assert_address_on_arena(it->second.data(), true);  // still Donated
    } else {
      t.assert_address_on_arena(it->second.data(), false);  // INV-4 fallback
    }
  }
  // A second write through the accessor must also stay on the donated path.
  m->mutable_ms_accessor("k").append("!!!");
  {
    const auto& ms = m->ms();
    auto it = ms.find("k");
    ASSERT_NE(it, ms.end());
    ASSERT_EQ(long_string + "!!!", it->second);
    if (arena != nullptr) {
      t.assert_address_on_arena(it->second.data(), true);
    }
  }
  if (arena == nullptr) delete m;
}
TEST_F(ArenaStringTest, MapValueDonated_MutableAccessor_NoPromote) {
  MapMutableAccessorNoPromote<ArenaProto3>(*this, arena);
  MapMutableAccessorNoPromote<ArenaProto3>(*this, nullptr);
  MapMutableAccessorNoPromote<ArenaProto2>(*this, arena);
  MapMutableAccessorNoPromote<ArenaProto2>(*this, nullptr);
}

// --- §4.6 #20 / §4.5: reflection interoperates safely with Donated map values.
//
//     There is no PUBLIC single-value mutable map reflection API:
//     Reflection::InsertOrLookupMapValue / MutableMapData are private (friend
//     only). Public users reach a map field through its repeated<MapEntry>
//     view (FieldSize / GetRepeatedMessage / MutableRepeatedMessage), which
//     goes through SyncRepeated<->Map and COPIES value content between the
//     Donated Map node and a MapEntry message — it never hands out a mutable
//     std::string& aliasing the donated buffer, so INV-1 holds with no
//     reflection-side promote.
//
//     (The private single-value path DOES promote: InsertOrLookupMapValueNoSyncImpl
//     in map_field_inl.h funnels through the non-const Map::iterator whose
//     operator* runs MaybePromoteDonatedValue. That promote is already pinned
//     by the *_TriggersPromote tests above.)
//
//     This test exercises the full public read+write round-trip over a Donated
//     map: read the donated value via reflection, mutate it via the repeated
//     view, and confirm it syncs back correctly with no crash / corruption.
template <typename M>
static void MapReflectionDonatedRoundTrip(ArenaStringTest& t, Arena* arena) {
  M fm;
  (*fm.mutable_ms())["k"] = long_string;
  std::string data;
  ASSERT_TRUE(fm.SerializeToString(&data));
  auto* m = ::google::protobuf::Arena::CreateMessage<M>(arena);
  ASSERT_TRUE(m->ParseFromString(data));
  if (arena != nullptr) {
    // Pre-condition: the parsed value is Donated.
    const auto& ms = m->ms();
    auto it = ms.find("k");
    ASSERT_NE(it, ms.end());
    t.assert_address_on_arena(it->second.data(), true);
  }

  const ::google::protobuf::Reflection* r = m->GetReflection();
  const ::google::protobuf::Descriptor* d = m->GetDescriptor();
  const ::google::protobuf::FieldDescriptor* f = d->FindFieldByName("ms");
  ASSERT_NE(f, nullptr);
  ASSERT_TRUE(f->is_map());

  // Read the Donated value through the public repeated<MapEntry> view.
  ASSERT_EQ(1, r->FieldSize(*m, f));
  const auto& entry = r->GetRepeatedMessage(*m, f, 0);
  const auto* ed = entry.GetDescriptor();
  const auto* er = entry.GetReflection();
  ASSERT_EQ("k", er->GetString(entry, ed->FindFieldByName("key")));
  ASSERT_EQ(long_string, er->GetString(entry, ed->FindFieldByName("value")));

  // Mutate through the public repeated-message view, then force a
  // repeated -> map sync (via re-serialization) and confirm the round-trip.
  auto* mentry = r->MutableRepeatedMessage(m, f, 0);
  mentry->GetReflection()->SetString(
      mentry, mentry->GetDescriptor()->FindFieldByName("value"), short_string);
  std::string out;
  ASSERT_TRUE(m->SerializeToString(&out));
  M check;
  ASSERT_TRUE(check.ParseFromString(out));
  ASSERT_EQ(1u, check.ms().size());
  ASSERT_EQ(short_string, check.ms().find("k")->second);

  if (arena == nullptr) delete m;
}
TEST_F(ArenaStringTest, MapValueDonated_Reflection_MutableMapData_Promote) {
  MapReflectionDonatedRoundTrip<Proto3>(*this, arena);
  MapReflectionDonatedRoundTrip<Proto3>(*this, nullptr);
  MapReflectionDonatedRoundTrip<ArenaProto3>(*this, arena);
  MapReflectionDonatedRoundTrip<ArenaProto3>(*this, nullptr);
}

// ============================================================================
// v2.1 Map DonatedString unit tests — P1 §5.3 corner cases.
//   - §5.3 #8  MapKeyDonated_ParseError_TempBufferCleanup
//   - §5.3 #12 MapKeyDonated_InvalidUtf8_NoTagCorruption
//   - §5.3 #7/#10/#11 MapKeyDonated_DuplicateKey_ArenaGrowsBounded
// ============================================================================

// --- §5.3 #8: a parse failure midway through a Donated map entry must not
//     crash or leak. Donated temp buffers already written into the arena are
//     reclaimed by the arena; the message must stay safe to read/clear. ---
template <typename M>
static void MapParseErrorCleanup(ArenaStringTest& t, Arena* arena) {
  M fm;
  (*fm.mutable_ms())["k1"] = long_string;
  (*fm.mutable_ms())["k2"] = short_string;
  std::string good;
  ASSERT_TRUE(fm.SerializeToString(&good));
  // Truncate inside the last length-delimited payload to force a parse error
  // after some donated bytes may already have been written.
  ASSERT_GT(good.size(), 3u);
  std::string bad = good.substr(0, good.size() - 3);

  auto* m = ::google::protobuf::Arena::CreateMessage<M>(arena);
  // Parse is expected to fail on truncated input; the contract under test is
  // "no crash / no leak / map stays usable", not the boolean result itself.
  EXPECT_FALSE(m->ParseFromString(bad));
  // Safe const access after a failed parse.
  for (const auto& kv : m->ms()) {
    (void)kv.first.size();
    (void)kv.second.size();
  }
  // Safe mutable access (would crash if a half-written donated value kept a
  // dangling tag) and clear.
  for (auto& kv : *m->mutable_ms()) kv.second.append("!");
  m->Clear();
  ASSERT_EQ(0u, m->ms().size());
  if (arena == nullptr) delete m;
}
TEST_F(ArenaStringTest, MapKeyDonated_ParseError_TempBufferCleanup) {
  MapParseErrorCleanup<Proto3>(*this, arena);
  MapParseErrorCleanup<Proto3>(*this, nullptr);
  MapParseErrorCleanup<ArenaProto3>(*this, arena);
  MapParseErrorCleanup<ArenaProto3>(*this, nullptr);
}

// --- §5.3 #12: parsing an invalid-UTF-8 string value into a proto3 map must
//     not corrupt the donated tag state. proto3 string maps reject invalid
//     UTF-8 (parse fails); regardless, the map must remain safe to read AND
//     mutate (mutable access promotes — a stale/dangling tag would crash). ---
template <typename M>
static void MapInvalidUtf8NoCorruption(ArenaStringTest& t, Arena* arena) {
  // Hand-craft a single `ms` (field 13) entry whose value is invalid UTF-8.
  //   outer:  tag=(13<<3)|2 = 0x6A, len
  //   entry:  key   field1 LEN: 0x0A, len=1, 'k'
  //           value field2 LEN: 0x12, len=2, 0xFF 0xFF  (invalid UTF-8)
  std::string entry;
  entry.push_back(static_cast<char>(0x0A));
  entry.push_back(static_cast<char>(0x01));
  entry.push_back('k');
  entry.push_back(static_cast<char>(0x12));
  entry.push_back(static_cast<char>(0x02));
  entry.push_back(static_cast<char>(0xFF));
  entry.push_back(static_cast<char>(0xFF));
  std::string wire;
  wire.push_back(static_cast<char>(0x6A));
  wire.push_back(static_cast<char>(entry.size()));
  wire += entry;

  auto* m = ::google::protobuf::Arena::CreateMessage<M>(arena);
  // Result is implementation-defined w.r.t. UTF-8 strictness; the invariant is
  // "no crash / no tag corruption".
  (void)m->ParseFromString(wire);
  for (const auto& kv : m->ms()) {
    (void)kv.first.size();
    (void)kv.second.size();
  }
  for (auto& kv : *m->mutable_ms()) kv.second.append("!");  // promotes safely
  m->Clear();
  if (arena == nullptr) delete m;
}
TEST_F(ArenaStringTest, MapKeyDonated_InvalidUtf8_NoTagCorruption) {
  MapInvalidUtf8NoCorruption<Proto3>(*this, arena);
  MapInvalidUtf8NoCorruption<Proto3>(*this, nullptr);
  MapInvalidUtf8NoCorruption<ArenaProto3>(*this, arena);
  MapInvalidUtf8NoCorruption<ArenaProto3>(*this, nullptr);
}

// --- §5.3 #7/#10/#11: duplicate keys on the wire leave the superseded Donated
//     nodes in the arena (InsertOrReplaceNode does not free them). This wastes
//     arena memory, but the waste is BOUNDED by the number of wire occurrences
//     — never unbounded — and the final map is correct. We quantify it by
//     comparing Arena::SpaceUsed for a single-occurrence vs a 3x-duplicated
//     wire (using long values to make the per-duplicate waste measurable). ---
template <typename M>
static void MapDuplicateKeyArenaWasteBounded(ArenaStringTest& t) {
  constexpr int kN = 64;
  M fm;
  for (int i = 0; i < kN; ++i) {
    (*fm.mutable_ms())["k_" + std::to_string(i)] = long_string;
  }
  std::string single;
  ASSERT_TRUE(fm.SerializeToString(&single));
  const std::string dup = single + single + single;  // each key appears 3x

  // Default arenas (grow on demand) so SpaceUsed reflects real allocation.
  ::google::protobuf::Arena a1;
  auto* m1 = ::google::protobuf::Arena::CreateMessage<M>(&a1);
  ASSERT_TRUE(m1->ParseFromString(single));
  const size_t base = a1.SpaceUsed();

  ::google::protobuf::Arena a2;
  auto* m2 = ::google::protobuf::Arena::CreateMessage<M>(&a2);
  ASSERT_TRUE(m2->ParseFromString(dup));
  const size_t waste = a2.SpaceUsed();

  // Final maps collapse duplicates to the same correct size.
  ASSERT_EQ(static_cast<size_t>(kN), m1->ms().size());
  ASSERT_EQ(static_cast<size_t>(kN), m2->ms().size());
  for (int i = 0; i < kN; ++i) {
    auto it = m2->ms().find("k_" + std::to_string(i));
    ASSERT_NE(it, m2->ms().end());
    ASSERT_EQ(long_string, it->second);
  }
  // Waste exists (>= baseline) but is bounded (not unbounded): a 3x wire must
  // not blow past a small constant factor of the single-occurrence footprint.
  EXPECT_GE(waste, base);
  EXPECT_LE(waste, base * 5);
}
TEST_F(ArenaStringTest, MapKeyDonated_DuplicateKey_ArenaGrowsBounded) {
  MapDuplicateKeyArenaWasteBounded<Proto3>(*this);
  MapDuplicateKeyArenaWasteBounded<ArenaProto3>(*this);
}

// --- §6: TextFormat parser correctly handles map<string,string> on an arena.
//     TextFormat (and JSON) parse maps via the repeated<MapEntry> reflection
//     view + reflection SetString — NOT through the Map<K,V> node directly — so
//     they never expose a donated bare reference (INV-1 holds; §6 is a pure
//     perf optimization, not a correctness requirement). This pins end-to-end
//     correctness: value content lands via the single-field ArenaStringPtr
//     donated path on the MapEntry message, and a subsequent mutable Map access
//     (which would promote a donated node value) stays safe. ---
template <typename M>
static void MapTextFormatParse(ArenaStringTest& t, Arena* arena) {
  auto* m = ::google::protobuf::Arena::CreateMessage<M>(arena);
  const std::string text =
      absl::StrCat("ms { key: \"k\" value: \"", long_string, "\" }");
  ASSERT_TRUE(::google::protobuf::TextFormat::ParseFromString(text, m));
  const auto& ms = m->ms();
  auto it = ms.find("k");
  ASSERT_NE(it, ms.end());
  ASSERT_EQ(long_string, it->second);
  // INV-1: a mutable Map access (promotes a donated value if present) is safe.
  (*m->mutable_ms())["k"].append("!");
  ASSERT_EQ(long_string + "!", m->ms().find("k")->second);
  if (arena == nullptr) delete m;
}
TEST_F(ArenaStringTest, MapValueDonated_TextFormat_ParseMap_Correct) {
  MapTextFormatParse<Proto3>(*this, arena);
  MapTextFormatParse<Proto3>(*this, nullptr);
  MapTextFormatParse<ArenaProto3>(*this, arena);
  MapTextFormatParse<ArenaProto3>(*this, nullptr);
}
