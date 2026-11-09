#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Crime.h"
#include "BTree/BTreeAdapter.h"
#include "BPlusTree/BPlusTree.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <string>

int main() {
    static_assert(BTree<long long, Crime>::order == 16, "B-tree default order must be 16");
    static_assert(BPlusTree<long long, Crime>::order == 16, "B+ tree default order must be 16");
    BTree<long long, Crime> btree;
    BPlusTree<long long, Crime> bplus;
    std::map<long long, Crime> reference;

    for (int i = 1; i <= 1000; ++i) {
        Crime c(i, "Test offense " + std::to_string(i), "1/2/2025", "Gainesville");
        c.reportDate = "1/3/2025";
        assert(btree.insert(i, c));
        assert(bplus.insert(i, c));
        reference[i] = c;
    }
    assert(!btree.insert(100, Crime(100, "duplicate", "", "")));
    assert(!bplus.insert(100, Crime(100, "duplicate", "", "")));
    assert(btree.size() == 1000 && bplus.size() == 1000);

    for (int i = 1; i <= 1000; ++i) {
        Crime* a = btree.search(i);
        Crime* b = bplus.search(i);
        assert(a && b && a->id == i && b->id == i);
    }
    assert(btree.search(2000) == nullptr);
    assert(bplus.search(2000) == nullptr);

    Crime sample(2001, "Dating Violence Simple Battery", "11/02/2026 05:42:23 PM", "1600 BLK NE 12TH AVE");
    sample.reportDate = "11/02/2026 05:43:24 PM";
    sample.incidentType = sample.description;
    assert(btree.insert(2001, sample));
    assert(bplus.insert(2001, sample));
    assert(btree.find(2001) != nullptr && bplus.find(2001) != nullptr);
    assert(btree.find(Date("11/02/2026")).size() == 1);
    assert(bplus.find(Date("11/02/2026")).size() == 1);
    assert(btree.find(CrimeType("dating violence")).size() == 1);
    assert(bplus.find(CrimeType("DATING VIOLENCE")).size() == 1);
    assert(btree.remove(2001));
    assert(bplus.remove(2001));

    const long long largeId = 9621015447LL;
    Crime largeCrime(largeId, "Test large ID", "11/02/2026", "Gainesville");
    assert(btree.insert(largeId, largeCrime));
    assert(bplus.insert(largeId, largeCrime));
    assert(btree.find(largeId) && bplus.find(largeId));
    assert(btree.remove(largeId) && bplus.remove(largeId));

    std::mt19937 rng(3530);
    for (int step = 0; step < 20000; ++step) {
        const int key = static_cast<int>(rng() % 1500) + 1;
        const int operation = static_cast<int>(rng() % 3);
        if (operation == 0) {
            Crime c(key, "Random offense", "2/3/2024", "Test address");
            const bool expected = reference.insert(std::make_pair(key, c)).second;
            assert(btree.insert(key, c) == expected);
            assert(bplus.insert(key, c) == expected);
        } else if (operation == 1) {
            const bool expected = reference.erase(key) != 0;
            assert(btree.remove(key) == expected);
            assert(bplus.remove(key) == expected);
        } else {
            const bool expected = reference.count(key) != 0;
            assert((btree.search(key) != nullptr) == expected);
            assert((bplus.search(key) != nullptr) == expected);
        }
        assert(btree.size() == reference.size());
        assert(bplus.size() == reference.size());
    }

    const auto bEntries = btree.getAll();
    const auto bpEntries = bplus.getAll();
    assert(bEntries.size() == reference.size());
    assert(bpEntries.size() == reference.size());
    auto it = reference.begin();
    for (std::size_t i = 0; i < reference.size(); ++i, ++it) {
        assert(bEntries[i].first == it->first);
        assert(bpEntries[i].first == it->first);
    }

    for (const auto& item : reference) {
        assert(btree.remove(item.first));
        assert(bplus.remove(item.first));
    }
    assert(btree.empty() && bplus.empty());
    assert(btree.search(1) == nullptr && bplus.search(1) == nullptr);

    std::cout << "All B-tree and B+ tree tests passed.\n";
    return 0;
}
