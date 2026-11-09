#include "Crime.h"
#include "CsvLoader.h"
#include "BTree/BTreeAdapter.h"
#include "BPlusTree/BPlusTree.h"

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

static bool readLine(const string& prompt, string& value) {
    cout << prompt;
    return static_cast<bool>(getline(cin, value));
}

static bool readId(long long& id) {
    string input;
    if (!readLine("Crime ID: ", input)) return false;
    try {
        size_t used = 0;
        id = stoll(input, &used);
        return used == input.size() && id >= 0;
    } catch (...) { return false; }
}

static void printRecord(const Crime& crime) { cout << crime << "\n"; }

static void printMatches(const vector<Crime>& matches) {
    if (matches.empty()) {
        cout << "No matching crime records found.\n";
        return;
    }
    const size_t limit = 30;
    for (size_t i = 0; i < matches.size() && i < limit; ++i) printRecord(matches[i]);
    cout << "Matches: " << matches.size();
    if (matches.size() > limit) cout << " (showing first " << limit << ")";
    cout << "\n";
}

template <typename Tree>
static double benchmarkInsert(const vector<pair<long long, Crime> >& data) {
    Tree tree;
    const auto start = chrono::steady_clock::now();
    for (const auto& item : data) tree.insert(item.first, item.second);
    const auto finish = chrono::steady_clock::now();
    return data.empty() ? 0.0 :
        chrono::duration<double, nano>(finish - start).count() / data.size();
}

template <typename Tree>
static double benchmarkSearch(const vector<pair<long long, Crime> >& data) {
    Tree tree;
    for (const auto& item : data) tree.insert(item.first, item.second);
    if (data.empty()) return 0.0;
    volatile int checksum = 0;
    const size_t repetitions = 10000;
    const auto start = chrono::steady_clock::now();
    for (size_t i = 0; i < repetitions; ++i) {
        const Crime* found = tree.find(data[i % data.size()].first);
        if (found) checksum += found->id;
    }
    const auto finish = chrono::steady_clock::now();
    (void)checksum;
    return chrono::duration<double, nano>(finish - start).count() / repetitions;
}

template <typename Tree>
static double benchmarkDelete(const vector<pair<long long, Crime> >& data) {
    Tree tree;
    for (const auto& item : data) tree.insert(item.first, item.second);
    if (data.empty()) return 0.0;
    const auto start = chrono::steady_clock::now();
    for (const auto& item : data) tree.remove(item.first);
    const auto finish = chrono::steady_clock::now();
    return chrono::duration<double, nano>(finish - start).count() / data.size();
}

static void showPerformance(const vector<pair<long long, Crime> >& data) {
    cout << "\nBenchmarking a sample of " << data.size() << " crime records...\n"
         << "Average nanoseconds per operation (this machine).\n\n"
         << "                         B-Tree          B+ Tree\n";
    cout << "insert(build)       " << benchmarkInsert<BTree<long long, Crime> >(data)
         << " ns       " << benchmarkInsert<BPlusTree<long long, Crime> >(data) << " ns\n";
    cout << "find(ID), 10,000     " << benchmarkSearch<BTree<long long, Crime> >(data)
         << " ns       " << benchmarkSearch<BPlusTree<long long, Crime> >(data) << " ns\n";
    cout << "remove(all)         " << benchmarkDelete<BTree<long long, Crime> >(data)
         << " ns       " << benchmarkDelete<BPlusTree<long long, Crime> >(data) << " ns\n";
}

int main(int argc, char* argv[]) {
    string loadMessage;
    const string csvPath = argc > 1 ? argv[1] : "Crime_Responses_20261108.csv";
    BTree<long long, Crime> btree;
    BPlusTree<long long, Crime> bplus;
    cout << "Loading CSV and building order-16 indexes...\n";
    if (!crime_csv::loadEach(csvPath, [&](const Crime& crime) {
            const bool insertedB = btree.insert(crime.id, crime);
            const bool insertedBP = bplus.insert(crime.id, crime);
            if (insertedB != insertedBP) {
                if (insertedB) btree.remove(crime.id);
                if (insertedBP) bplus.remove(crime.id);
                return false;
            }
            return insertedB && insertedBP;
        }, loadMessage)) {
        cerr << loadMessage << "\n";
        cerr << "Place Crime_Responses_20261108.csv beside the executable or pass a CSV path.\n";
        return 1;
    }
    cout << loadMessage << "\n";
    cout << "Both indexes contain " << bplus.size() << " records.\n";

    bool useBPlus = true;
    while (true) {
        cout << "\n========== GAINESVILLE CRIME DATABASE ==========\n"
             << "Records: " << bplus.size() << " | Active index: "
             << (useBPlus ? "B+ Tree" : "B-Tree") << " | Order: 16\n"
             << "1. find(ID)\n"
             << "2. find(Date)\n"
             << "3. find(CrimeType)\n"
             << "4. Insert crime record\n"
             << "5. Delete crime record\n"
             << "6. Display first 30 records\n"
             << "7. Select tree (B-tree / B+ tree)\n"
             << "8. Compare performance\n"
             << "0. Exit\n"
             << "Selection: ";
        string choice;
        if (!getline(cin, choice)) break;
        if (choice == "0") break;

        if (choice == "1") {
            long long id;
            if (!readId(id)) { cout << "Invalid crime ID.\n"; continue; }
            const Crime* found = useBPlus ? bplus.find(id) : btree.find(id);
            if (found) printRecord(*found);
            else cout << "No crime found with ID " << id << ".\n";
        } else if (choice == "2") {
            string date;
            if (!readLine("Date (e.g. 11/02/2026): ", date)) break;
            const vector<Crime> found = useBPlus ? bplus.find(Date(date)) : btree.find(Date(date));
            printMatches(found);
        } else if (choice == "3") {
            string type;
            if (!readLine("Crime type (e.g. Battery, Theft, Runaway): ", type)) break;
            const vector<Crime> found = useBPlus ? bplus.find(CrimeType(type)) : btree.find(CrimeType(type));
            printMatches(found);
        } else if (choice == "4") {
            long long id;
            if (!readId(id)) { cout << "Invalid crime ID.\n"; continue; }
            if (bplus.find(id)) { cout << "That crime ID already exists.\n"; continue; }
            Crime crime;
            crime.id = id;
            if (!readLine("Incident Type: ", crime.incidentType) ||
                !readLine("Offense Date: ", crime.offenseDate) ||
                !readLine("Report Date: ", crime.reportDate) ||
                !readLine("Address: ", crime.address) ||
                !readLine("City: ", crime.city) ||
                !readLine("State: ", crime.state)) break;
            crime.description = crime.incidentType;
            crime.date = crime.offenseDate;
            crime.parseDateParts();
            const bool insertedB = btree.insert(id, crime);
            const bool insertedBP = bplus.insert(id, crime);
            if (insertedB && insertedBP) {
                cout << "Inserted into both trees.\n";
            } else {
                if (insertedB) btree.remove(id);
                if (insertedBP) bplus.remove(id);
                cout << "Insertion failed; indexes remain synchronized.\n";
            }
        } else if (choice == "5") {
            long long id;
            if (!readId(id)) { cout << "Invalid crime ID.\n"; continue; }
            const bool deletedB = btree.remove(id);
            const bool deletedBP = bplus.remove(id);
            if (deletedB && deletedBP) {
                cout << "Deleted crime " << id << " from both trees.\n";
            } else {
                cout << "Crime ID not found or index inconsistency detected.\n";
            }
        } else if (choice == "6") {
            size_t shown = 0;
            bplus.ForEach([&](long long, const Crime& crime) {
                if (shown < 30) printRecord(crime);
                ++shown;
            });
            cout << "Total records: " << bplus.size() << "\n";
        } else if (choice == "7") {
            cout << "1. B-tree\n2. B+ tree\nSelection: ";
            getline(cin, choice);
            if (choice == "1") useBPlus = false;
            else if (choice == "2") useBPlus = true;
            else cout << "Invalid selection.\n";
        } else if (choice == "8") {
            vector<pair<long long, Crime> > sample;
            sample.reserve(20000);
            bplus.ForEach([&](long long id, const Crime& crime) {
                if (sample.size() < 20000) sample.push_back(make_pair(id, crime));
            });
            showPerformance(sample);
        } else {
            cout << "Unknown menu option.\n";
        }
    }
    cout << "Goodbye.\n";
    return 0;
}
