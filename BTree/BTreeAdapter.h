#ifndef CRIME_BTREE_ADAPTER_H
#define CRIME_BTREE_ADAPTER_H
#include "../Crime.h"
#include "BTree.h"
#include <map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <utility>

template<typename KeyType, typename Value, std::size_t Order = 16>
class BTree {
    static_assert(Order == 16, "the B-tree is order 16");
    mutable btree_impl::BTree tree_;
    std::map<KeyType, Value> records_;
    static std::string lower(std::string s) {
        std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        return s;
    }
    static std::string dateText(const Value& c) {
        if (c.month > 0 && c.day > 0 && c.year > 0) {
            auto pad=[](int n){return (n<10?"0":"")+std::to_string(n);};
            return pad(c.month)+"/"+pad(c.day)+"/"+std::to_string(c.year);
        }
        return "01/01/1900";
    }
    static void gather(const btree_impl::Node* node, std::size_t depth, std::size_t& nodes, std::size_t& leaves, std::size_t& height) {
        if (!node) return;
        ++nodes; height=std::max(height,depth);
        if (node->children.empty()) ++leaves;
        for (const auto* child:node->children) gather(child,depth+1,nodes,leaves,height);
    }
public:
    static constexpr std::size_t order=Order;
    struct Statistics {std::size_t nodes=0,leaves=0,height=0;};
    BTree()=default;
    BTree(const BTree&)=delete;
    BTree& operator=(const BTree&)=delete;
    std::size_t size() const {return records_.size();}
    bool empty() const {return records_.empty();}
    Statistics statistics() const {
        Statistics s;
        gather(tree_.rootNode(),1,s.nodes,s.leaves,s.height);
        return s;
    }
    bool insert(const KeyType& id, const Value& crime) {
        if (records_.count(id)) return false;
        const std::string type=crime.incidentType.empty()?crime.description:crime.incidentType;
        if (!tree_.insertKey(static_cast<long long>(id),type,dateText(crime),crime.address)) return false;
        records_.emplace(id,crime);
        return true;
    }
    bool remove(const KeyType& id) {
        if (!records_.count(id)) return false;
        tree_.deleteKey(static_cast<long long>(id));
        records_.erase(id);
        return true;
    }
    Value* search(const KeyType& id) {if (!tree_.findKey(static_cast<long long>(id))) return nullptr; auto it=records_.find(id);return it==records_.end()?nullptr:&it->second;}
    const Value* search(const KeyType& id) const {if (!tree_.findKey(static_cast<long long>(id))) return nullptr; auto it=records_.find(id);return it==records_.end()?nullptr:&it->second;}
    Value* find(const KeyType& id) {return search(id);}
    const Value* find(const KeyType& id) const {return search(id);}
    std::vector<Value> find(const Date& query) const {
        std::vector<Value> result;
        if (query.value.empty()) return result;
        const auto wanted=lower(query.value);
        for(const auto& item:records_) {
            const auto& c=item.second;
            if(lower(c.offenseDate.empty()?c.date:c.offenseDate).find(wanted)==0 || lower(c.reportDate).find(wanted)==0) result.push_back(c);
        }
        return result;
    }
    std::vector<Value> find(const CrimeType& query) const {
        std::vector<Value> result;
        const auto wanted = lower(query.value);
        if (wanted.empty()) return result;
        for(const auto& item:records_) {
            const auto& c=item.second;
            if (lower(c.incidentType.empty() ? c.description : c.incidentType).find(wanted) != std::string::npos)
                result.push_back(c);
        }
        return result;
    }
    template<typename Function>
    void ForEach(Function f) const {
        for (const auto& item : records_) f(item.first, item.second);
    }
    std::vector<std::pair<KeyType,Value>> getAll() const {return {records_.begin(),records_.end()};}
};
#endif
