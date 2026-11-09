#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "../Crime.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <cstddef>
#include <utility>
#include <vector>

template <typename Key, typename Value, std::size_t Order = 16>
class BPlusTree {
    static_assert(Order >= 3, "B+ tree order must be at least 3");

private:
    struct Node {
        bool leaf;
        std::vector<Key> keys;
        std::vector<Value> values;
        std::vector<Node*> children;
        Node* next;

        explicit Node(bool isLeaf) : leaf(isLeaf), next(nullptr) {}
    };

    struct InsertResult {
        bool inserted;
        Node* newRight;
        InsertResult(bool didInsert = false, Node* right = nullptr)
            : inserted(didInsert), newRight(right) {}
    };

    Node* root_;
    std::size_t size_;

    static constexpr std::size_t maxLeafKeys() { return Order - 1; }
    static constexpr std::size_t minLeafKeys() { return Order / 2; }
    static constexpr std::size_t minInternalChildren() { return (Order + 1) / 2; }

    static void destroy(Node* node) {
        if (!node) return;
        if (!node->leaf) {
            for (Node* child : node->children) destroy(child);
        }
        delete node;
    }

    Node* findLeaf(const Key& key) const {
        Node* current = root_;
        while (current && !current->leaf) {
            std::size_t i = static_cast<std::size_t>(
                std::upper_bound(current->keys.begin(), current->keys.end(), key)
                - current->keys.begin());
            current = current->children[i];
        }
        return current;
    }

    static const Key& minimumKey(const Node* node) {
        while (!node->leaf) node = node->children.front();
        return node->keys.front();
    }

    static void refreshNodeKeys(Node* node) {
        if (!node || node->leaf) return;
        node->keys.clear();
        for (std::size_t i = 1; i < node->children.size(); ++i)
            node->keys.push_back(minimumKey(node->children[i]));
    }

    InsertResult insertRecursive(Node* node, const Key& key, const Value& value) {
        if (node->leaf) {
            auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
            std::size_t pos = static_cast<std::size_t>(it - node->keys.begin());
            if (it != node->keys.end() && !(*it < key) && !(key < *it)) {
                return InsertResult(false, nullptr);
            }
            node->keys.insert(it, key);
            node->values.insert(node->values.begin() + static_cast<std::ptrdiff_t>(pos), value);

            if (node->keys.size() <= maxLeafKeys()) return InsertResult(true, nullptr);

            Node* right = new Node(true);
            const std::size_t splitAt = (Order + 1) / 2;
            right->keys.assign(node->keys.begin() + static_cast<std::ptrdiff_t>(splitAt), node->keys.end());
            right->values.assign(node->values.begin() + static_cast<std::ptrdiff_t>(splitAt), node->values.end());
            node->keys.resize(splitAt);
            node->values.resize(splitAt);
            right->next = node->next;
            node->next = right;
            return InsertResult(true, right);
        }

        std::size_t childIndex = static_cast<std::size_t>(
            std::upper_bound(node->keys.begin(), node->keys.end(), key) - node->keys.begin());
        InsertResult result = insertRecursive(node->children[childIndex], key, value);
        if (!result.inserted) return result;
        if (!result.newRight) {
            refreshNodeKeys(node);
            return result;
        }

        node->children.insert(node->children.begin() + static_cast<std::ptrdiff_t>(childIndex + 1),
                              result.newRight);
        refreshNodeKeys(node);
        if (node->children.size() <= Order) return InsertResult(true, nullptr);

        Node* right = new Node(false);
        const std::size_t leftChildCount = (Order + 1) / 2;
        right->children.assign(node->children.begin() + static_cast<std::ptrdiff_t>(leftChildCount),
                               node->children.end());
        node->children.resize(leftChildCount);
        refreshNodeKeys(node);
        refreshNodeKeys(right);
        return InsertResult(true, right);
    }

    static std::size_t childPosition(const Node* parent, const Node* child) {
        for (std::size_t i = 0; i < parent->children.size(); ++i)
            if (parent->children[i] == child) return i;
        return parent->children.size();
    }

    static void rebalanceChild(Node* parent, std::size_t index) {
        Node* child = parent->children[index];
        const bool leaf = child->leaf;
        const std::size_t minimum = leaf ? minLeafKeys() : minInternalChildren();
        const std::size_t occupancy = leaf ? child->keys.size() : child->children.size();
        if (occupancy >= minimum) return;

        Node* left = index > 0 ? parent->children[index - 1] : nullptr;
        Node* right = index + 1 < parent->children.size() ? parent->children[index + 1] : nullptr;

        if (left) {
            std::size_t leftOccupancy = leaf ? left->keys.size() : left->children.size();
            if (leftOccupancy > minimum) {
                if (leaf) {
                    child->keys.insert(child->keys.begin(), left->keys.back());
                    child->values.insert(child->values.begin(), left->values.back());
                    left->keys.pop_back();
                    left->values.pop_back();
                } else {
                    child->children.insert(child->children.begin(), left->children.back());
                    left->children.pop_back();
                    refreshNodeKeys(child);
                    refreshNodeKeys(left);
                }
                return;
            }
        }

        if (right) {
            std::size_t rightOccupancy = leaf ? right->keys.size() : right->children.size();
            if (rightOccupancy > minimum) {
                if (leaf) {
                    child->keys.push_back(right->keys.front());
                    child->values.push_back(right->values.front());
                    right->keys.erase(right->keys.begin());
                    right->values.erase(right->values.begin());
                } else {
                    child->children.push_back(right->children.front());
                    right->children.erase(right->children.begin());
                    refreshNodeKeys(child);
                    refreshNodeKeys(right);
                }
                return;
            }
        }

        if (left) {
            if (leaf) {
                left->keys.insert(left->keys.end(), child->keys.begin(), child->keys.end());
                left->values.insert(left->values.end(), child->values.begin(), child->values.end());
                left->next = child->next;
            } else {
                left->children.insert(left->children.end(), child->children.begin(), child->children.end());
                child->children.clear();
                refreshNodeKeys(left);
            }
            parent->children.erase(parent->children.begin() + static_cast<std::ptrdiff_t>(index));
            delete child;
        } else if (right) {
            if (leaf) {
                child->keys.insert(child->keys.end(), right->keys.begin(), right->keys.end());
                child->values.insert(child->values.end(), right->values.begin(), right->values.end());
                child->next = right->next;
            } else {
                child->children.insert(child->children.end(), right->children.begin(), right->children.end());
                right->children.clear();
                refreshNodeKeys(child);
            }
            parent->children.erase(parent->children.begin() + static_cast<std::ptrdiff_t>(index + 1));
            delete right;
        }
    }

    bool eraseRecursive(Node* node, const Key& key) {
        if (node->leaf) {
            auto it = std::lower_bound(node->keys.begin(), node->keys.end(), key);
            if (it == node->keys.end() || *it < key || key < *it) return false;
            std::size_t pos = static_cast<std::size_t>(it - node->keys.begin());
            node->keys.erase(it);
            node->values.erase(node->values.begin() + static_cast<std::ptrdiff_t>(pos));
            return true;
        }

        std::size_t index = static_cast<std::size_t>(
            std::upper_bound(node->keys.begin(), node->keys.end(), key) - node->keys.begin());
        Node* child = node->children[index];
        if (!eraseRecursive(child, key)) return false;
        rebalanceChild(node, index);
        refreshNodeKeys(node);
        return true;
    }

    void collect(std::vector<std::pair<Key, Value> >& out) const {
        if (!root_) return;
        Node* leaf = root_;
        while (!leaf->leaf) leaf = leaf->children.front();
        while (leaf) {
            for (std::size_t i = 0; i < leaf->keys.size(); ++i)
                out.push_back(std::make_pair(leaf->keys[i], leaf->values[i]));
            leaf = leaf->next;
        }
    }

    static std::string lowerText(const std::string& text) {
        std::string result = text;
        for (std::size_t i = 0; i < result.size(); ++i)
            result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
        return result;
    }

public:
    static constexpr std::size_t order = Order;
    BPlusTree() : root_(new Node(true)), size_(0) {}
    ~BPlusTree() { destroy(root_); }

    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;

    struct Statistics {
        std::size_t nodes = 0;
        std::size_t leaves = 0;
        std::size_t height = 0;
    };

    Statistics statistics() const {
        Statistics result;
        if (size_ == 0) return result;
        auto visit = [&](const auto& self, const Node* node, std::size_t level) -> void {
            if (!node) return;
            ++result.nodes;
            result.height = std::max(result.height, level);
            if (node->leaf) ++result.leaves;
            for (const Node* child : node->children) self(self, child, level + 1);
        };
        visit(visit, root_, 1);
        return result;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    bool insert(const Key& key, const Value& value) {
        InsertResult result = insertRecursive(root_, key, value);
        if (!result.inserted) return false;
        if (result.newRight) {
            Node* newRoot = new Node(false);
            newRoot->children.push_back(root_);
            newRoot->children.push_back(result.newRight);
            root_ = newRoot;
        }
        ++size_;
        refreshNodeKeys(root_);
        return true;
    }

    Value* search(const Key& key) {
        Node* leaf = findLeaf(key);
        if (!leaf) return nullptr;
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it == leaf->keys.end() || *it < key || key < *it) return nullptr;
        return &leaf->values[static_cast<std::size_t>(it - leaf->keys.begin())];
    }

    const Value* search(const Key& key) const {
        Node* leaf = findLeaf(key);
        if (!leaf) return nullptr;
        auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it == leaf->keys.end() || *it < key || key < *it) return nullptr;
        return &leaf->values[static_cast<std::size_t>(it - leaf->keys.begin())];
    }

    bool remove(const Key& key) {
        if (!eraseRecursive(root_, key)) return false;
        --size_;
        if (!root_->leaf && root_->children.size() == 1) {
            Node* oldRoot = root_;
            root_ = root_->children.front();
            oldRoot->children.clear();
            delete oldRoot;
        }
        refreshNodeKeys(root_);
        return true;
    }

    Value* find(const Key& id) { return search(id); }
    const Value* find(const Key& id) const { return search(id); }

    std::vector<Value> find(const Date& query) const {
        std::vector<Value> result;
        const std::string wanted = lowerText(query.value);
        if (wanted.empty()) return result;
        ForEach([&](const Key&, const Value& value) {
            const std::string offense = value.offenseDate.empty() ? value.date : value.offenseDate;
            if (lowerText(offense).find(wanted) == 0 ||
                lowerText(value.reportDate).find(wanted) == 0)
                result.push_back(value);
        });
        return result;
    }

    std::vector<Value> find(const CrimeType& query) const {
        std::vector<Value> result;
        const std::string wanted = lowerText(query.value);
        if (wanted.empty()) return result;
        ForEach([&](const Key&, const Value& value) {
            const std::string type = value.incidentType.empty() ? value.description : value.incidentType;
            if (lowerText(type).find(wanted) != std::string::npos)
                result.push_back(value);
        });
        return result;
    }

    template <typename Function>
    void ForEach(Function function) const {
        if (!root_) return;
        Node* leaf = root_;
        while (!leaf->leaf) leaf = leaf->children.front();
        while (leaf) {
            for (std::size_t i = 0; i < leaf->keys.size(); ++i)
                function(leaf->keys[i], leaf->values[i]);
            leaf = leaf->next;
        }
    }

    std::vector<std::pair<Key, Value> > getAll() const {
        std::vector<std::pair<Key, Value> > result;
        result.reserve(size_);
        collect(result);
        return result;
    }
};

#endif
