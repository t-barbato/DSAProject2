#include "Trees/BPlusTree.h"
#include <iostream>
#include <string>
using namespace std;

struct Crime {
    string description;
    int year;
};

int main() {
    BPlusTree<int, Crime> crimeTree;

    crimeTree.insert(12345, {"Theft", 2024});
    crimeTree.insert(67890, {"Burglary", 2023});

    Crime* result = crimeTree.search(12345);

    if (result != nullptr) {
        cout << result->description << endl;
    }

    crimeTree.remove(12345);

    return 0;
}