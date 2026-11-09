#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#pragma once
using namespace std;

struct Key {
    long long ID;
    string crime;
    string address;
    string date; // 06/08/2025
    int day;
    int month;
    int year;

    Key(const long long &id, const string &offense,const string &date, const string &location) {
        this->ID = id;
        this->crime = offense;
        this->date = date;
        this->address = location;
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