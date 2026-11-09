#ifndef CRIME_CSV_LOADER_H
#define CRIME_CSV_LOADER_H

#include "Crime.h"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace crime_csv {
inline std::string trim(const std::string& value) {
    std::size_t first = 0, last = value.size();
    while (first < last && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) --last;
    return value.substr(first, last - first);
}

inline std::string normalize(std::string value) {
    std::string result;
    for (unsigned char c : value)
        if (std::isalnum(c)) result += static_cast<char>(std::tolower(c));
    return result;
}

inline std::vector<std::string> parseLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '"') {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else quoted = !quoted;
        } else if (c == ',' && !quoted) {
            fields.push_back(trim(field));
            field.clear();
        } else field += c;
    }
    fields.push_back(trim(field));
    return fields;
}

inline int findColumn(const std::vector<std::string>& header,
                      const std::vector<std::string>& aliases) {
    for (std::size_t i = 0; i < header.size(); ++i) {
        const std::string name = normalize(header[i]);
        for (const std::string& alias : aliases)
            if (name == normalize(alias)) return static_cast<int>(i);
    }
    return -1;
}

inline std::string fieldAt(const std::vector<std::string>& row, int column) {
    return column >= 0 && static_cast<std::size_t>(column) < row.size()
        ? trim(row[static_cast<std::size_t>(column)]) : std::string();
}

template <typename Callback>
inline bool loadEach(const std::string& path, Callback callback,
                     std::string& message) {
    std::ifstream input(path.c_str());
    if (!input) {
        message = "Could not open CSV file: " + path;
        return false;
    }

    std::string line;
    if (!std::getline(input, line)) {
        message = "CSV file is empty.";
        return false;
    }
    if (line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF)
        line.erase(0, 3);

    const std::vector<std::string> header = parseLine(line);
    const int idCol = findColumn(header, {"ID", "crime_id", "incident_id", "case_number"});
    const int typeCol = findColumn(header, {"Incident Type", "crime_type", "offense", "description"});
    const int reportDateCol = findColumn(header, {"Report Date", "reported_date", "call_date"});
    const int offenseDateCol = findColumn(header, {"Offense Date", "occurred_date", "incident_date", "date"});
    const int addressCol = findColumn(header, {"Address", "street_address", "location_address"});
    const int cityCol = findColumn(header, {"City"});
    const int stateCol = findColumn(header, {"State"});
    const int latitudeCol = findColumn(header, {"Latitude", "lat"});
    const int longitudeCol = findColumn(header, {"Longitude", "lon", "lng"});
    const int locationCol = findColumn(header, {"Location", "geolocation"});
    const int reportHourCol = findColumn(header, {"Report Hour of Day"});
    const int reportDowCol = findColumn(header, {"Report Day of Week"});
    const int offenseHourCol = findColumn(header, {"Offense Hour of Day"});
    const int offenseDowCol = findColumn(header, {"Offense Day of Week"});

    if (idCol < 0 || typeCol < 0 || offenseDateCol < 0 || addressCol < 0) {
        message = "CSV is missing a required column. Expected ID, Incident Type, "
                  "Offense Date, and Address.";
        return false;
    }

    std::size_t loaded = 0, skipped = 0, duplicateIds = 0;
    std::unordered_set<long long> seenIds;
    auto process = [&](const std::vector<std::string>& row) {
        const std::string idText = fieldAt(row, idCol);
        try {
            std::size_t used = 0;
            const long long parsed = std::stoll(idText, &used);
            if (used != idText.size() || parsed < 0 || parsed > LLONG_MAX) {
                ++skipped;
                return;
            }

            Crime crime;
            crime.id = static_cast<long long>(parsed);
            crime.incidentType = fieldAt(row, typeCol);
            crime.description = crime.incidentType;
            crime.reportDate = fieldAt(row, reportDateCol);
            crime.offenseDate = fieldAt(row, offenseDateCol);
            crime.date = crime.offenseDate;
            crime.address = fieldAt(row, addressCol);
            crime.city = fieldAt(row, cityCol);
            crime.state = fieldAt(row, stateCol);
            crime.latitude = fieldAt(row, latitudeCol);
            crime.longitude = fieldAt(row, longitudeCol);
            crime.location = fieldAt(row, locationCol);
            crime.reportHourOfDay = fieldAt(row, reportHourCol);
            crime.reportDayOfWeek = fieldAt(row, reportDowCol);
            crime.offenseHourOfDay = fieldAt(row, offenseHourCol);
            crime.offenseDayOfWeek = fieldAt(row, offenseDowCol);
            crime.parseDateParts();

            if (!seenIds.insert(crime.id).second) {
                ++duplicateIds;
                return;
            }
            if (callback(crime)) ++loaded;
            else ++skipped;
        } catch (...) {
            ++skipped;
        }
    };

    while (std::getline(input, line)) {
        if (!trim(line).empty()) process(parseLine(line));
    }

    std::ostringstream out;
    out << "Loaded " << loaded << " unique crime IDs";
    if (duplicateIds) out << " (" << duplicateIds << " duplicate IDs ignored after the first row)";
    if (skipped) out << " (" << skipped << " invalid rows skipped)";
    out << ".";
    message = out.str();
    return loaded > 0;
}

inline bool load(const std::string& path, std::map<long long, Crime>& records,
                 std::string& message) {
    records.clear();
    return loadEach(path, [&](const Crime& crime) {
        records[crime.id] = crime;
        return true;
    }, message);
}
}

#endif
