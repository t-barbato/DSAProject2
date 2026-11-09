#include <algorithm>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Key.h"
#pragma once

using namespace std;



struct Node {
    vector<Node*> children;
    vector<Key> keys;

    void addChild(Node* child) {
        this->children.push_back(child);
    }
    void addKey(Key key) {
        this->keys.push_back(key);
        sort(this->keys.begin(), this->keys.end(),
            [](const Key a, const Key b) {
                return a.ID < b.ID;
            });
    }

    bool isLeaf() {
        return children.empty();
    };
};
