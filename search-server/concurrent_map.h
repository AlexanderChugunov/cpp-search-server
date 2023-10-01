#pragma once

#include <future>
#include <mutex>
#include <map>
#include <vector>


template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

public:
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex)
            , ref(bucket.map[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& bucket = buckets[static_cast<uint64_t>(key) % buckets.size()];
        return { key, bucket };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

private:
    std::vector<Bucket> buckets;
};
