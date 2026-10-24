#include <vector>
#include <optional>
#include <cstdint>
#include <new>
#include "main.h"
#include "Cache.hpp"

Cache::Cache() {
    // Find the largest power of 2 number of entries that stays under 15 MB limit.
    // Power of 2 allows replacing slow '%' division with ultra-fast '&' bitwise masking.
    constexpr size_t total_bytes = 15 * 1024 * 1024;
    constexpr size_t max_possible = total_bytes / sizeof(CacheEntry);
    
    max_entries = 1;
    while ((max_entries << 1) <= max_possible) {
        max_entries <<= 1;
    }
    
    entry_mask = max_entries - 1; // Used for bitwise modulo: index = hash & entry_mask

    // Allocate flat, contiguous blocks up-front
    table.resize(max_entries);
    eviction_queue.resize(max_entries);
}

std::optional<uint32_t> Cache::get(uint32_t key) noexcept {
    size_t index = fast_hash_32(key) & entry_mask;
    size_t start_index = index;

    while (table[index].is_occupied) {
        if (table[index].key == key) {
            return table[index].payload; // Cache hit
        }
        index = (index + 1) & entry_mask; // Sequential next item is likely already in L1 cache
        if (index == start_index) break;
    }
    return std::nullopt; // Cache miss
}

void Cache::put(uint32_t key, uint32_t value) noexcept {
    size_t index = fast_hash_32(key) & entry_mask;
    size_t start_index = index;

    while (table[index].is_occupied) {
        if (table[index].key == key) {
            table[index].payload = value;
            return;
        }
        index = (index + 1) & entry_mask;
        if (index == start_index) {
            // Table is full, evict via flat ring-buffer index
            size_t evict_idx = eviction_queue[queue_tail];
            table[evict_idx].is_occupied = 0;
            index = evict_idx; 
            queue_tail = (queue_tail + 1) & entry_mask;
            break;
        }
    }

    table[index].key = key;
    table[index].payload = value;
    table[index].is_occupied = 1;

    eviction_queue[queue_head] = index;
    queue_head = (queue_head + 1) & entry_mask;
}