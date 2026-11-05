#include <iostream>
#include <vector>
#include <sstream>
#include <string>
using namespace std;

struct Node {
    int ID;
    string crime;
    string address;
    string date; // 06/08/2025
    int day;
    int month;
    int year;

    vector<Node*> children;

    Node(int id, string offense,string date, string location) {
        this->ID = id;
        this->crime = offense;
        this->date = date;
        this->address = location;
        convertDates(date);
    }
    void addChild(Node* child) {
        this->children.push_back(child);
    }

    void convertDates(string date) {
        string sday, smonth, syear;
        stringstream ss(date);
        getline(ss, smonth, '/');
        getline(ss, sday, '/');
        getline(ss, syear, '/');
        month = stoi(smonth);
        day = stoi(sday);
        year = stoi(syear);

    }
};