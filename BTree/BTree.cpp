#include "BTree.h"

#include <ranges>


void BTree::splitChild(Node *parent, int index) {
    Node* left = parent->children[index];
    Node* right = new Node();
    int middle = left->keys.size() / 2;
    Key promoted = left->keys[middle];
    for (int i = left->keys.size()-1; i > middle; i--) {
        right->addKey(left->keys[i]);
        left->keys.pop_back();
    }
    left->keys.pop_back();

    if (!left->isLeaf()) {
        int firstchild = middle + 1;
        int i = firstchild;
        while ( i < left->children.size() ) {
            right->addChild(left->children[i]);
            i++;
        }
        while (left->children.size() > firstchild) {
            left->children.pop_back();
        }
    }
    parent->keys.insert(parent->keys.begin()+index, promoted);
    parent->children.insert(parent->children.begin()+index+1, right);
}

void BTree::searchCrime(vector<Key> &vec, Node* node, string crime){
    if (node==nullptr) {
        return;
    }
    for (int i = 0; i < node->keys.size(); i++) {
        if (node->keys[i].crime == crime) {
            vec.push_back(node->keys[i]);
        }
    }
    if (node->isLeaf()) {
        return;
    }
    for (int i = 0; i < node->children.size(); i++) {
        searchCrime(vec, node->children[i], crime);
    }
}

void BTree::searchDate(vector<Key> &vec, Node *node, string date) {
    if (node == nullptr) {
        return;
    }
    for (int i = 0; i < node->keys.size(); i++) {
        if (node->keys[i].date == date) {
            vec.push_back(node->keys[i]);
        }
    }
    if (node->isLeaf()) {
        return;
    }
    for (int i = 0; i < node->children.size(); i++) {
        searchDate(vec, node->children[i], date);
    }
    return;
}

optional<Key> BTree::searchKey(Node *root, long long key) {
    Node* node = root;
    while  (node != nullptr) {
        int i = 0;
        while (i < node->keys.size() && node->keys[i].ID < key) {
            i++;
        }
        if (i < node->keys.size() && node->keys[i].ID == key) {
            return node->keys[i];
        }
        if (node->isLeaf()) {
            break;
        }
        node = node->children[i];
        }
    return nullopt;
}

bool BTree::insert(Node *root, Key &newkey) {
    if (root->isLeaf()) {
        root->addKey(newkey);
        size++;
        return true;
    }
    long long id = newkey.ID;
    int keyCount = root->keys.size();
    int index = -1;
    if (id < root->keys[0].ID) {
        index = 0;
    }
    else if (id > root->keys.back().ID) {
        index = keyCount;
    }
    for (int i = 0; i < root->keys.size()-1; i++) {
        if (id > root->keys[i].ID && id < root->keys[i+1].ID) {
            index = i + 1;
            break;
        }
    }

    if (index == -1) {
        return false;
    }
    Node* child = root->children[index];

    if (!insert(child, newkey)) {
        return false;
    }
    if (child->keys.size()==m) {
        splitChild(root, index);
    }
    return true;
}

bool BTree::insertKey(const long long &id, const string &crime, const string &date, const string &location) {
    Key newKey(id, crime, date, location);
    if (root == nullptr) {
        root = new Node();
        root->addKey(newKey);
        size = 1;
        height = 1;
        return true;
    }
    if (findKey(id) != nullopt) {
        cout << "Already in Database" << endl;
        return false;
    }
    if  (!insert(root, newKey)) {
        return false;
    }
    if (root->keys.size() == m) {
        Node* newRoot = new Node();
        newRoot->addChild(root);

        splitChild(newRoot, 0);
        root = newRoot;
        height++;
    }
    return true;
}

optional<Key> BTree::findKey(const long long &id) {
    if (size == 0) {
        return nullopt;
    }
    return searchKey(root, id);
}

vector<Key> BTree::findCrime(string crime) {
    vector<Key> vec = {};
    searchCrime(vec, root, crime);
    return vec;
}

vector<Key> BTree::findDate(string date) {
    vector<Key> vec = {};
    searchDate(vec, root, date);
    return vec;
}

void BTree::deleteKey(const long long &id) {
    if (root == nullptr) {
        return;
    }
    if (!eraseFrom(root, id)) {
        return;
    }

    size--;

    if (root->keys.empty()) {
        Node* oldroot = root;
        if (oldroot->isLeaf()) {
            root = nullptr;
        }
        else {
            root = oldroot->children.front();
        }
        delete oldroot;
        --height;
    }
}

void BTree::deleteLeaf(Node *leaf) {
    if (leaf == nullptr) {
        return;
    }
    while (!leaf->children.empty()) {
        deleteLeaf(leaf->children.back());
        leaf->children.pop_back();
    }
    delete leaf;
}

bool BTree::eraseFrom(Node *node, long long id) {
    int i = 0;
    while (i < node->keys.size() && node->keys[i].ID < id) {
        i++;
    }

    bool found = i < node->keys.size() && node->keys[i].ID == id;

    if (node->isLeaf()) {
        if (!found) {
            return false;
        }

        node->keys.erase(node->keys.begin() + i);
        return true;
    }
    Node* child = node->children[i];

    if (found) {
        Node* parent = child;
        while (!parent->isLeaf()) {
            parent = parent->children.back();
        }
        node->keys[i] = parent->keys.back();
        id = node->keys[i].ID;
    }

    if (!eraseFrom(child, id)) {
        return false;
    }

    int min = (m-1)/2;
    if (child->keys.size() < min) {
        repairChild(node, i);
    }
    return true;
}

void BTree::repairChild(Node *parent, int index) {
    int minSize = (m-1)/2;
    Node* child = parent->children[index];

    if (index > 0) {
        Node* left = parent->children[index-1];
        if (left->keys.size() > minSize) {
            child->keys.insert(child->keys.begin(), parent->keys[index-1]);
            parent->keys[index-1] = left->keys.back();
            left->keys.pop_back();

            if (!left->isLeaf()) {
                child->children.insert(child->children.begin(), left->children.back());
                left->children.pop_back();
            }
            return;
        }
    }
    if (index+1 < parent->children.size()) {
        Node* right = parent->children[index+1];
        if (right->keys.size() > minSize) {
            child->keys.push_back(parent->keys[index]);
            parent->keys[index] = right->keys.front();
            right->keys.erase(right->keys.begin());

            if (!right->isLeaf()) {
                child->children.push_back(right->children.front());
                right->children.erase(right->children.begin());
            }
            return;
        }
    }

    int separator = 0;
    if (index > 0) {
        separator = index-1;
    }
    Node* left = parent->children[separator];
    Node* right = parent->children[separator + 1];
    left->keys.push_back(parent->keys[separator]);
    left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
    left->children.insert(left->children.end(), right->children.begin(), right->children.end());
    parent->keys.erase(parent->keys.begin() + separator);
    parent->children.erase(parent->children.begin() + separator + 1);
    delete right;
}

BTree::~BTree() {
    deleteLeaf(root);
}
