#include <vector>
#include <optional>
#include <cstdint>
#include <new>
#include "main.h"

struct alignas(32) CacheEntry{
    uint32_t key = 0;
    uint32_t payload = 0; 
    uint32_t is_occupied = 0; // Use native 32-bit integer for fast boolean check
};

class Cache {
    private:
        inline uint32_t fast_hash_32(uint32_t x) const noexcept {
            x ^= x >> 16;
            x *= 0x85ebca6b;
            x ^= x >> 13;
            x *= 0xc2b2ae35;
            x ^= x >> 16;
            return x;
        }

        size_t max_entries;
        size_t entry_mask;
        
        std::vector<CacheEntry> table;
        std::vector<size_t> eviction_queue;
        size_t queue_head = 0;
        size_t queue_tail = 0;

    public:
        Cache();

        std::optional<uint32_t> get(uint32_t key) noexcept;

        void put(uint32_t key, uint32_t value) noexcept;
};