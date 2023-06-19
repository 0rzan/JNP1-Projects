#include "hash.h"
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <utility>
#include <algorithm>

namespace jnp1 {
    namespace {
        using sequence = std::pair<std::unique_ptr<std::uint64_t[]>, std::size_t>;

        sequence seq_create(uint64_t const * seq, size_t size) {
            sequence result {std::make_unique<uint64_t[]>(size), size};
            std::copy_n(seq, size, result.first.get());
            return result;
        }

        struct hash_fun {
            hash_function_t hash_function;
            std::size_t operator() (sequence const &seq) const noexcept{
                return hash_function(seq.first.get(), seq.second);
            } 
        };

        using set_seq = std::unordered_set<sequence, hash_fun>;
        using sets_id = std::unordered_map<unsigned long, set_seq>;

        sets_id &get_sets() {
            static sets_id sets;
            return sets;
        }          
    }

    unsigned long hash_create(hash_function_t hash_function) {
        auto &sets = get_sets();
        static unsigned long next_id = 0;
        unsigned long id = next_id++;
        sets.insert({id, set_seq(0, hash_fun{hash_function})});
        return id;
    }

    void hash_delete(unsigned long id) {
        auto &sets = get_sets();
        auto it = sets.find(id);
        if(it == sets.end()) {
            // TODO
            return;
        }
        sets.erase(it);
    }

    size_t hash_size(unsigned long id) {
        auto &sets = get_sets();
        auto it = sets.find(id);
        if(it == sets.end()) {
            // TODO
            return 0;
        }
        return it->second.size();
    }

    bool hash_insert(unsigned long id, uint64_t const * seq, size_t size) {
        auto &sets = get_sets();
        auto it = sets.find(id);
        // te czesc z flaga trzeba bedzie zrobic jako pomocnicza funkcje wypisujaca bledy
        bool ok = true;
        if(it == sets.end()) {
            // TODO
            ok = false;
        }
        if(seq == NULL) {
            // TODO
            ok = false;
        }
        if(size == 0) {
            // TODO
            ok = false;
        }
        if(!ok) {
            return false;
        }
        return it->second.insert(seq_create(seq, size)).second;
    }

    bool hash_remove(unsigned long id, uint64_t const * seq, size_t size) {
        auto &sets = get_sets();
        auto it = sets.find(id);
        bool ok = true;
        if(it == sets.end()) {
            // TODO
            ok = false;
        }
        if(seq == NULL) {
            // TODO
            ok = false;
        }
        if(size == 0) {
            // TODO
            ok = false;
        }
        if(!ok) {
            return false;
        }
        auto it_to_set = it->second.find(seq_create(seq, size));
        auto result = it->second.end() != it_to_set;
        if(result) {
            it->second.erase(it_to_set);
        }
        return result;
    }

    void hash_clear(unsigned long id) {
        auto &sets = get_sets();
        auto it = sets.find(id);
        if(it == sets.end()) {
            // TODO
            return;
        }
        it->second.clear();
    }

    bool hash_test(unsigned long id, uint64_t const * seq, size_t size) {
        auto &sets = get_sets();
        auto it = sets.find(id);
        bool ok = true;
        if(it == sets.end()) {
            // TODO
            ok = false;
        }
        if(seq == NULL) {
            // TODO
            ok = false;
        }
        if(size == 0) {
            // TODO
            ok = false;
        }
        if(!ok) {
            return false;
        }
        auto result = it->second.end() != it->second.find(seq_create(seq, size));
        return result;
    }
}