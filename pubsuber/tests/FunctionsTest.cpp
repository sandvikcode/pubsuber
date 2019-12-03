#include <gtest/gtest.h>
#include <random>

#include "Algorithms.h"

using namespace pubsuber;
using namespace std::chrono;

std::string RandomString(std::string::size_type length) {
  static auto &chrs =
      "0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  static std::mt19937 rg{std::random_device{}()};
  static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

  std::string s;
  s.reserve(length);

  while (length--) {
    s += chrs[pick(rg)];
  }
  return s;
}

TEST(AlgorithmTest, erase_keys) {
  WatchDescrContainer from;
  AckIDSet what;

  for (auto i = 0; i < 100; ++i) {
    auto s = RandomString(64);
    from.insert_or_assign(s, AckWatchDescriptior());

    if (i % 2 == 0) {
      what.insert(s);
    }
  }

  const auto wasSize = from.size();
  erase_keys(from, what);

  EXPECT_EQ(from.size(), wasSize - what.size());

  for (auto &k : what) {
    EXPECT_EQ(from.end(), from.find(k));
  }
}

TEST(AlgorithmTest, give_me_the_keys_all) {
  WatchDescrContainer from;

  for (auto i = 0; i < 100; ++i) {
    from.insert_or_assign(RandomString(64), AckWatchDescriptior());
  }

  // pick all elements
  AckIDPackSet toModify;
  give_me_the_keys(from, toModify, [](auto &descr) -> bool { return true; });

  for (const auto &pair : from) {
    const auto &[key, descr] = pair;
    EXPECT_NE(toModify.find(AckIDPack(key, WatchDescrContainer::iterator())), toModify.end());
  }
}

TEST(AlgorithmTest, give_me_the_keys_none) {
  WatchDescrContainer from;

  for (auto i = 0; i < 100; ++i) {
    from.insert_or_assign(RandomString(64), AckWatchDescriptior());
  }

  // pick all elements
  AckIDPackSet toModify;
  give_me_the_keys(from, toModify, [](auto &descr) -> bool { return false; });

  EXPECT_EQ(toModify.size(), 0);
}

TEST(AlgorithmTest, give_me_the_keys_some) {
  WatchDescrContainer from;

  for (auto i = 0; i < 100; ++i) {
    from.insert_or_assign(RandomString(64), AckWatchDescriptior());
  }

  const auto nowX = std::chrono::steady_clock::now() + 10s;
  from.insert_or_assign(RandomString(64), AckWatchDescriptior(nowX));
  from.insert_or_assign(RandomString(64), AckWatchDescriptior(nowX));
  from.insert_or_assign(RandomString(64), AckWatchDescriptior(nowX));

  // pick all elements
  AckIDPackSet toModify;
  give_me_the_keys(from, toModify, [nowX](auto &descr) -> bool { return descr._nextAck == nowX; });

  EXPECT_EQ(toModify.size(), 3);
}

TEST(AlgorithmTest, split_request_ids_none) {
  const auto N = 100;
  const auto LEN = 10;

  AckIDSet all, toSend;
  for (auto i = 0; i < N; ++i) {
    all.emplace(RandomString(LEN));
  }

  split_request_ids(all, toSend, 0);
  EXPECT_EQ(toSend.size(), 0);
}

TEST(AlgorithmTest, split_request_ids_all) {
  const auto N = 100;
  const auto LEN = 10;

  AckIDSet all, toSend;
  for (auto i = 0; i < N; ++i) {
    all.emplace(RandomString(LEN));
  }

  const auto allSize = all.size();
  split_request_ids(all, toSend, INT_MAX);

  EXPECT_EQ(toSend.size(), allSize);
}

TEST(AlgorithmTest, split_request_ids_some) {
  const auto N = 100;
  const auto LEN = 10;

  AckIDSet all;
  for (auto i = 0; i < N; ++i) {
    all.emplace(RandomString(LEN));
  }

  const auto allSize = all.size();
  size_t processed = 0;
  while (!all.empty()) {
    AckIDSet toSend;
    split_request_ids(all, toSend, 130);
    processed += toSend.size();
    EXPECT_EQ(toSend.size(), 2);
    EXPECT_EQ(processed + all.size(), allSize);
  }
}

TEST(AlgorithmTest, split_request_ids_some2) {
  const auto N = 100;
  const auto LEN = 20;

  AckIDPackSet all;
  for (auto i = 0; i < N; ++i) {
    all.emplace(AckIDPack(RandomString(LEN), WatchDescrContainer::iterator()));
  }

  const auto allSize = all.size();
  size_t processed = 0;
  while (!all.empty()) {
    AckIDPackSet toSend;
    split_request_ids(all, toSend, 150);
    processed += toSend.size();
    EXPECT_EQ(toSend.size(), 2);
    EXPECT_EQ(processed + all.size(), allSize);
  }
}

TEST(AlgorithmTest, populate_ack_ids) {
  const auto N = 100;
  const auto LEN = 10;

  ::google::protobuf::RepeatedPtrField<std::string> to;

  AckIDSet all;
  for (auto i = 0; i < N; ++i) {
    all.emplace(RandomString(LEN));
  }

  auto allCopy = all;

  populate_ack_ids(to, all);
  EXPECT_EQ(all.size(), 0);

  for (const auto &it : to) {
    EXPECT_NE(allCopy.end(), allCopy.find(it));
  }
}

TEST(AlgorithmTest, populate_ack_ids2) {
  const auto N = 100;
  const auto LEN = 10;

  ::google::protobuf::RepeatedPtrField<std::string> to;

  AckIDPackSet all;
  for (auto i = 0; i < N; ++i) {
    all.emplace(AckIDPack(RandomString(LEN), WatchDescrContainer::iterator()));
  }

  populate_ack_ids(to, all);
  EXPECT_EQ(all.size(), to.size());

  for (const auto &it : to) {
    EXPECT_NE(all.end(), all.find(AckIDPack(it, WatchDescrContainer::iterator())));
  }
}
