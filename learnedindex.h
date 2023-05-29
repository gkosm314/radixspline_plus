#ifndef LEARNED_INDEX_HEADER
#define LEARNED_INDEX_HEADER

#include <atomic>

#include "include/rs/builder.h"
#include "include/rs/radix_spline.h"

template <class KeyType, class ValueType>
class LearnedIndex{
 public:
    LearnedIndex(std::vector<std::pair<KeyType, ValueType>> & k);
    bool lookup(const KeyType &lookup_key, int &offset); // get offset of ">=" key
    bool find(const KeyType &lookup_key, int &offset); // get offset of "==" key
    bool find(const KeyType &lookup_key, int &offset, ValueType &val); // get offset of "==" key and associated value
    
    uint64_t readers_in;
    std::atomic<uint64_t> readers_out;
    
 private:
    std::vector<std::pair<KeyType, ValueType>> * kv_data; //The key-value store over which the active_learned_index approximates.
    rs::RadixSpline<KeyType> rspline;
};

template <class KeyType, class ValueType>
LearnedIndex<KeyType, ValueType>::LearnedIndex(std::vector<std::pair<KeyType, ValueType>> & k){
    // Assumption: keys is a non-empty vector sorted with regard to keys
 
    //Initialize readers' counters
    readers_in = 0;
    readers_out = 0;

    // Keys should be pointing to the initial data
    kv_data = &k;
    
    // Extract minimum and maximum value of the data you want to approximate with the spline
    KeyType min_key = kv_data->front().first;
    KeyType max_key = kv_data->back().first;

    // Construct the spline in a single pass by iterating over the keys
    rs::Builder<KeyType> rsb(min_key, max_key);
    for (const auto& kv_pair : k) rsb.AddKey(kv_pair.first);
    rspline = rsb.Finalize();
}

template <class KeyType, class ValueType>
bool LearnedIndex<KeyType, ValueType>::lookup(const KeyType &lookup_key, int &offset){
    // Finds the next smallest number in keys just greater than or equal to that number and stores it in offset
    // Returns false if such number does not exist, true otherwise
    // Note: offset will be out-of-bounds for the keys vector when the function returns false

    // TODO: check dereference for performance penalty

    // Search bound for local search using RadixSpline
    rs::SearchBound bound = rspline.GetSearchBound(lookup_key);
    
    // Perform binary search inside the error bounds to find the exact position
    auto start = begin(*kv_data) + bound.begin, last = begin(*kv_data) + bound.end;
    auto binary_search_offset = std::lower_bound(start, last, lookup_key,
                    [](const std::pair<KeyType, ValueType>& lhs, const KeyType& rhs){
                        return lhs.first < rhs;
                    });
    offset = binary_search_offset - begin(*kv_data);

    // Return true iff records greater than or equal to the given key exist in the data
    return (offset < kv_data->size());
}

template <class KeyType, class ValueType>
bool LearnedIndex<KeyType, ValueType>::find(const KeyType &lookup_key, int &offset){
    // Finds the exact key, if it exists. Returns true if the key exists, false otherwise.
    // Uses lookup() and stores the smallest key that is greater 
    // Note: offset could be out-of-bounds for the keys vector when the function returns false

    bool keys_greater_or_equal_exist = lookup(lookup_key, offset);

    if(keys_greater_or_equal_exist && (*kv_data)[offset].first == lookup_key) return true;
    else return false;
}

template <class KeyType, class ValueType>
bool LearnedIndex<KeyType, ValueType>::find(const KeyType &lookup_key, int &offset, ValueType &val){
    // Finds the exact key, if it exists. Returns true if the key exists, false otherwise.
    // Uses lookup() and stores the smallest key that is greater 
    // Note: This implementation also returns the value associated with the given key
    // Note: offset could be out-of-bounds for the keys vector when the function returns false

    bool keys_greater_or_equal_exist = lookup(lookup_key, offset);

    if(keys_greater_or_equal_exist && (*kv_data)[offset].first == lookup_key){
        val = (*kv_data)[offset].second;
        return true;
    }
    else return false;
}

#endif