#include <iostream>
#include <vector>
#include "Node.h"
#include <optional>

#pragma once

using namespace std;

class BTree {
   Node* root = nullptr;
   int size = 0;
   int height = 0;
   int m = 16; //Number of children


   public:

   BTree() = default;

   void splitChild(Node* parent, int index);

   void searchCrime(vector<Key>& vec, Node* node, string crime);

   void searchDate(vector<Key>& vec, Node* node, string date);

   optional<Key> searchKey(Node *root, long long key);

   void deleteLeaf(Node* leaf);

   bool eraseFrom(Node* node, long long id);

   void repairChild(Node* parent, int index);

   bool insert(Node* root, Key &newkey);

   bool insertKey(const long long &id, const string &name, const string &date, const string &location);

   optional<Key> findKey(const long long &id);

   vector<Key> findCrime(string crime);

   vector<Key> findDate(string date);

   void deleteKey(const long long &id);
   ~BTree();
};


