#ifndef CRIME_H
#define CRIME_H

#include <string>
#include <ostream>

struct Date {
    std::string value;
    explicit Date(const std::string& text) : value(text) {}
};

struct CrimeType {
    std::string value;
    explicit CrimeType(const std::string& text) : value(text) {}
};

struct Crime {
    long long id = 0;
    std::string incidentType;
    std::string description;
    std::string date;
    std::string offenseDate;
    std::string reportDate;
    std::string address;
    std::string city;
    std::string state;
    std::string latitude;
    std::string longitude;
    std::string location;
    std::string reportHourOfDay;
    std::string reportDayOfWeek;
    std::string offenseHourOfDay;
    std::string offenseDayOfWeek;
    int day = 0, month = 0, year = 0;

    Crime() = default;
    Crime(long long crimeId, const std::string& offense,
          const std::string& crimeDate, const std::string& crimeAddress)
        : id(crimeId), incidentType(offense), description(offense),
          date(crimeDate), offenseDate(crimeDate), address(crimeAddress) {
        parseDateParts();
    }

    void parseDateParts() {
        day = month = year = 0;
        const std::string& source = offenseDate.empty() ? date : offenseDate;
        try {
            const std::size_t first = source.find('/');
            if (first != std::string::npos) {
                const std::size_t second = source.find('/', first + 1);
                if (second == std::string::npos) return;
                month = std::stoi(source.substr(0, first));
                day = std::stoi(source.substr(first + 1, second - first - 1));
                year = std::stoi(source.substr(second + 1, 4));
            } else if (source.size() >= 10 && source[4] == '-' && source[7] == '-') {
                year = std::stoi(source.substr(0, 4));
                month = std::stoi(source.substr(5, 2));
                day = std::stoi(source.substr(8, 2));
            }
        } catch (...) { day = month = year = 0; }
    }
};

inline std::ostream& operator<<(std::ostream& out, const Crime& crime) {
    out << "ID: " << crime.id
        << " | Incident Type: " << (crime.incidentType.empty() ? crime.description : crime.incidentType)
        << " | Offense Date: " << (crime.offenseDate.empty() ? crime.date : crime.offenseDate)
        << " | Report Date: " << crime.reportDate
        << " | Address: " << crime.address;
    if (!crime.city.empty() || !crime.state.empty())
        out << " | " << crime.city
            << (crime.city.empty() || crime.state.empty() ? "" : ", ") << crime.state;
    return out;
}

#endif
