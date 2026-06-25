#include <catch2/catch_test_macros.hpp>

#include "buffer/arc_replacer.hpp"

TEST_CASE("ARC tracks evictable resident frames", "[arc]") {
  im110::ArcReplacer replacer{3};

  replacer.RecordAccess(0, 10);
  replacer.RecordAccess(1, 11);
  replacer.SetEvictable(0, true);
  replacer.SetEvictable(1, true);

  REQUIRE(replacer.Size() == 2);

  const auto victim = replacer.Evict();
  REQUIRE(victim.has_value());
  REQUIRE((*victim == 0 || *victim == 1));
  REQUIRE(replacer.Size() == 1);
}

TEST_CASE("ARC does not evict pinned frames", "[arc]") {
  im110::ArcReplacer replacer{2};

  replacer.RecordAccess(0, 20);
  replacer.RecordAccess(1, 21);
  replacer.SetEvictable(0, false);
  replacer.SetEvictable(1, false);

  REQUIRE(replacer.Size() == 0);
  REQUIRE_FALSE(replacer.Evict().has_value());

  replacer.SetEvictable(1, true);
  REQUIRE(replacer.Size() == 1);
  REQUIRE(replacer.Evict() == 1);
}

TEST_CASE("ARC promotes repeated resident access", "[arc]") {
  im110::ArcReplacer replacer{2};

  replacer.RecordAccess(0, 30);
  replacer.RecordAccess(1, 31);
  replacer.RecordAccess(0, 30);
  replacer.SetEvictable(0, true);
  replacer.SetEvictable(1, true);

  const auto first_victim = replacer.Evict();
  REQUIRE(first_victim.has_value());
  REQUIRE(*first_victim == 1);
}
