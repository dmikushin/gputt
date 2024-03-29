/******************************************************************************
MIT License

Copyright (c) 2016 Antti-Pekka Hynninen
Copyright (c) 2016 NVIDIA

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include <list>
#include <mutex>
#include <unordered_map>
#include <utility>

using namespace std;

//
// Simple LRU cache implementation
//
template <typename key_type, typename value_type> class LRUCache {
private:
  struct ValueIterator {
    value_type value;
    typename list<key_type>::iterator it;
  };

  // Size of the cache
  const size_t capacity;

  // Value that is returned when the key is not found
  const value_type null_value;

  // Double linked list of keys. Oldest is at the back
  list<key_type> keys;

  // Cache: (hash table)
  // key = key
  // value = {value, pointer to linked list}
  unordered_map<key_type, ValueIterator> cache;

  // Mutex
  std::mutex cache_lock;

public:
  LRUCache(const size_t capacity, const value_type null_value)
      : capacity(capacity), null_value(null_value) {}

  value_type get(key_type key) {
    std::lock_guard<std::mutex> lock(cache_lock);
    auto it = cache.find(key);
    if (it == cache.end())
      return null_value;
    touch(it);
    return it->second.value;
  }

  void set(key_type key, value_type value) {
    std::lock_guard<std::mutex> lock(cache_lock);
    auto it = cache.find(key);
    if (it != cache.end()) {
      // key found
      it->second.value = value;
      touch(it);
    } else {
      // key not found
      if (cache.size() == capacity) {
        key_type oldest_key = keys.back();
        keys.pop_back();
        cache.erase(cache.find(oldest_key));
      }
      keys.push_front(key);
      ValueIterator vi;
      vi.value = value;
      vi.it = keys.begin();
      pair<key_type, ValueIterator> boo(key, vi);
      cache.insert(boo);
    }
  }

private:
  void touch(typename unordered_map<key_type, ValueIterator>::iterator it) {
    keys.erase(it->second.it);
    keys.push_front(it->first);
    it->second.it = keys.begin();
  }
};
