#include <catch_amalgamated.hpp>

#include <buffer_pool/lru_k.hpp>
#include <misc/config.hpp>

TEST_CASE("LRU k test", "[lru_k]") {
    std::optional<hivedb::frame_id_t> frame;

    constexpr auto k = 2;
    constexpr hivedb::frame_id_t max_frames = 7;

    hivedb::lru_k replacer(k, max_frames);

    replacer.recordAccess(1);
    replacer.recordAccess(2);
    replacer.recordAccess(3);
    replacer.recordAccess(4);
    replacer.recordAccess(5);
    replacer.recordAccess(6);
    replacer.setEvictable(1, true);
    replacer.setEvictable(2, true);
    replacer.setEvictable(3, true);
    replacer.setEvictable(4, true);
    replacer.setEvictable(5, true);
    replacer.setEvictable(6, false);
    //We should have frames [1, 2, 3, 4, 5]. Frame 6 is marked as non evictable.

    //The size of the replacer should be the num of frames that can be evicted
    REQUIRE(5 == replacer.size());

    // Record an access for frame 1. Now frame 1 has two accesses total.
    replacer.recordAccess(1);
    // All other frames now share the maximum backward k-distance. Since we use timestamps to break ties, where the first
    // to be evicted is the frame with the oldest timestamp, the order of eviction should be [2, 3, 4, 5, 1].

    // Evict three pages from the replacer.
    // To break ties, we use LRU with respect to the oldest timestamp, or the least recently used frame.
    REQUIRE(2 == replacer.evict().value());
    REQUIRE(3 == replacer.evict().value());
    REQUIRE(4 == replacer.evict().value());
    REQUIRE(2 == replacer.size());
    // Now the replacer has the frames [5, 1].

    // Insert new frames [3, 4], and update the access history for 5. Now, the ordering is [3, 1, 5, 4].
    replacer.recordAccess(3);
    replacer.recordAccess(4);
    replacer.recordAccess(5);
    replacer.recordAccess(4);
    replacer.setEvictable(3, true);
    replacer.setEvictable(4, true);
    REQUIRE(4 == replacer.size());

    // Look for a frame to evict. We expect frame 3 to be evicted next.
    REQUIRE(3 == replacer.evict());
    REQUIRE(3 == replacer.size());

    // Set 6 to be evictable. 6 Should be evicted next since it has the maximum backward k-distance.
    replacer.setEvictable(6, true);
    REQUIRE(4 == replacer.size());
    REQUIRE(6 == replacer.evict().value());
    REQUIRE(3 == replacer.size());

    // Mark frame 1 as non-evictable. We now have [5, 4].
    replacer.setEvictable(1, false);

    // We expect frame 5 to be evicted next.
    REQUIRE(2 == replacer.size());
    REQUIRE(5 == replacer.evict().value());
    REQUIRE(1 == replacer.size());

    // Update the access history for frame 1 and make it evictable. Now we have [4, 1].
    replacer.recordAccess(1);
    replacer.recordAccess(1);
    replacer.setEvictable(1, true);
    REQUIRE(2 == replacer.size());

    // Evict the last two frames.
    REQUIRE(4 == replacer.evict().value());
    REQUIRE(1 == replacer.size());
    REQUIRE(1 == replacer.evict().value());
    REQUIRE(0 == replacer.size());

    // Insert frame 1 again and mark it as non-evictable.
    replacer.recordAccess(1);
    replacer.setEvictable(1, false);
    REQUIRE(0 == replacer.size());

    // A failed eviction should not change the size of the replacer.
    frame = replacer.evict();
    REQUIRE(false == frame.has_value());

    // Mark frame 1 as evictable again and evict it.
    replacer.setEvictable(1, true);
    REQUIRE(1 == replacer.size());
    REQUIRE(1 == replacer.evict().value());
    REQUIRE(0 == replacer.size());

    // There is nothing left in the replacer, so make sure this doesn't do something strange.
    frame = replacer.evict();
    REQUIRE(false == frame.has_value());
    REQUIRE(0 == replacer.size());
}
