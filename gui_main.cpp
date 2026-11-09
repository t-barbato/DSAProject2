
#include <SFML/Graphics.hpp>
#include "Crime.h"
#include "CsvLoader.h"
#include "BTree/BTreeAdapter.h"
#include "BPlusTree/BPlusTree.h"
#include <algorithm>
#include <cctype>
#include <array>
#include <cmath>
#include <cstdint>
#include <chrono>
#include <fstream>
#include <set>
#include <vector>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

using namespace std;

enum class Page { CrimeRecords, TreeComparison };
enum class TreeType { BTree, BPlusTree };

static string lowercase(string value) {
    transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return value;
}

static string formattedDate(const Crime& crime) {
    if (crime.month < 1 || crime.month > 12 || crime.day < 1 || crime.day > 31 || crime.year < 1)
        return "-";
    auto twoDigits = [](int n) { return (n < 10 ? "0" : "") + to_string(n); };
    return twoDigits(crime.month) + "/" + twoDigits(crime.day) + "/" + to_string(crime.year);
}

static bool validDate(const string& date) {
    if (date.size() != 10 || date[2] != '/' || date[5] != '/') return false;
    for (size_t i = 0; i < date.size(); ++i) {
        if (i == 2 || i == 5) continue;
        if (date[i] < '0' || date[i] > '9') return false;
    }
    const int month = stoi(date.substr(0, 2));
    const int day = stoi(date.substr(3, 2));
    const int year = stoi(date.substr(6, 4));
    if (year < 1900 || year > 2100 || month < 1 || month > 12) return false;
    const int daysInMonth[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return day >= 1 && day <= daysInMonth[month] + (month == 2 && leap ? 1 : 0);
}

static string crimeDetails(const Crime& crime) {
    string details = crime.description;
    const string date = formattedDate(crime);
    if (date != "-") details += " | " + date;
    if (!crime.address.empty()) details += " | " + crime.address;
    return details;
}

struct TextField {
    sf::FloatRect bounds;
    string value;
    string placeholder;
    string label;
    bool numericOnly;
};

sf::ConvexShape roundedRectangle(float width, float height,
                                 float radius, sf::Color color) {
    constexpr int segments = 8;
    constexpr float pi = 3.14159265358979323846f;
    radius = min(radius, min(width, height) / 2.f);

    sf::ConvexShape shape(4 * (segments + 1));
    const sf::Vector2f centers[4] = {
        {radius, radius},
        {width - radius, radius},
        {width - radius, height - radius},
        {radius, height - radius}
    };

    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i <= segments; ++i) {
            const float angle = (180.f + corner * 90.f +
                                 i * 90.f / segments) * pi / 180.f;
            shape.setPoint(corner * (segments + 1) + i, {
                centers[corner].x + radius * cos(angle),
                centers[corner].y + radius * sin(angle)
            });
        }
    }
    shape.setFillColor(color);
    return shape;
}

static string directoryOf(const string& path) {
    const size_t pos = path.find_last_of("/\\");
    return pos == string::npos ? string() : path.substr(0, pos);
}

static string joinPath(const string& directory, const string& filename) {
    if (directory.empty()) return filename;
    return directory + "/" + filename;
}

static bool readableFile(const string& path) {
    ifstream file(path.c_str());
    return file.good();
}

static string findCsvPath(int argc, char* argv[]) {
    if (argc > 1) return argv[1];

    const string filename = "Crime_Responses_20261108.csv";
    vector<string> roots = {".", directoryOf(argv[0]), directoryOf(__FILE__)};
    set<string> checked;

    for (const string& root : roots) {
        string folder = root;
        for (int depth = 0; depth < 6 && !folder.empty(); ++depth) {
            const string candidate = joinPath(folder, filename);
            if (checked.insert(candidate).second && readableFile(candidate))
                return candidate;
            const string parent = directoryOf(folder);
            if (parent.empty() || parent == folder) break;
            folder = parent;
        }
    }
    return filename;
}

int main(int argc, char* argv[]) {

    sf::RenderWindow window(sf::VideoMode({1100, 1000}),
                            "Gainesville Crime Database");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        cerr << "Unable to load Arial font.\n";
        return 1;
    }

    const sf::Color background(15, 23, 42);
    const sf::Color sidebar(22, 32, 52);
    const sf::Color panel(30, 41, 59);
    const sf::Color blue(59, 130, 246);
    const sf::Color white(241, 245, 249);
    const sf::Color muted(148, 163, 184);
    const sf::Color green(34, 197, 94);
    const sf::Color red(239, 68, 68);

    auto drawBox = [&](float x, float y, float w, float h, sf::Color color) {
        sf::RectangleShape rectangle({w, h});
        rectangle.setPosition({x, y});
        rectangle.setFillColor(color);
        window.draw(rectangle);
    };

    auto drawRoundedBox = [&](float x, float y, float w, float h,
                              float radius, sf::Color color) {
        auto shape = roundedRectangle(w, h, radius, color);
        shape.setPosition({x, y});
        window.draw(shape);
    };

    auto drawText = [&](const string& textValue, float x, float y,
                        unsigned size, sf::Color color) {
        sf::Text text(font, textValue, size);
        text.setPosition({x, y});
        text.setFillColor(color);
        window.draw(text);
    };

    auto drawButton = [&](const string& label, const sf::FloatRect& bounds,
                          sf::Color baseColor) {
        const auto mouse = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos(static_cast<float>(mouse.x),
                              static_cast<float>(mouse.y));
        const bool hovered = bounds.contains(mousePos);
        const bool pressed = hovered &&
            sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        const float scale = pressed ? 0.97f : (hovered ? 1.04f : 1.f);
        const float w = bounds.size.x * scale;
        const float h = bounds.size.y * scale;
        const float x = bounds.position.x + (bounds.size.x - w) / 2.f;
        const float y = bounds.position.y + (bounds.size.y - h) / 2.f;

        sf::Color color = baseColor;
        if (hovered) {
            color.r = static_cast<uint8_t>(min(255, int(color.r) + 25));
            color.g = static_cast<uint8_t>(min(255, int(color.g) + 25));
            color.b = static_cast<uint8_t>(min(255, int(color.b) + 25));
        }

        auto shape = roundedRectangle(w, h, 12.f, color);
        shape.setPosition({x, y});
        if (hovered) {
            shape.setOutlineThickness(2.f);
            shape.setOutlineColor(sf::Color(255, 255, 255, 65));
        }
        window.draw(shape);

        sf::Text text(font, label, 18);
        text.setFillColor(white);
        const auto textBounds = text.getLocalBounds();
        text.setPosition({
            x + (w - textBounds.size.x) / 2.f - textBounds.position.x,
            y + (h - textBounds.size.y) / 2.f - textBounds.position.y
        });
        window.draw(text);
    };

    string loadMessage;
    const string csvPath = findCsvPath(argc, argv);
    BTree<long long, Crime> btree;
    BPlusTree<long long, Crime> bplus;
    const bool dataLoaded = crime_csv::loadEach(csvPath, [&](const Crime& crime) {
        const bool insertedB = btree.insert(crime.id, crime);
        const bool insertedBP = bplus.insert(crime.id, crime);
        if (insertedB != insertedBP) {
            if (insertedB) btree.remove(crime.id);
            if (insertedBP) bplus.remove(crime.id);
            return false;
        }
        return insertedB && insertedBP;
    }, loadMessage);
    if (!dataLoaded) {
        cerr << loadMessage << "\n";
        loadMessage = "CSV load failed: " + loadMessage;
    }

    double btreeInsertNs = 0.0, bplusInsertNs = 0.0;
    double btreeSearchNs = 0.0, bplusSearchNs = 0.0;
    double btreeDeleteNs = 0.0, bplusDeleteNs = 0.0;
    bool benchmarkReady = false;
    string comparisonId;
    string comparisonResult = "Enter a crime ID to compare searches.";
    double comparisonBTreeNs = 0.0, comparisonBPlusNs = 0.0;
    bool comparisonSearched = false;
    const sf::FloatRect compareInput({320, 482}, {220, 43});
    const sf::FloatRect compareSearchButton({555, 482}, {190, 43});
    const sf::FloatRect compareBenchmarkButton({770, 482}, {240, 43});
    bool compareInputFocused = false;

    auto runBenchmark = [&]() {
        vector<pair<long long, Crime> > data;
        data.reserve(20000);
        bplus.ForEach([&](long long id, const Crime& crime) {
            if (data.size() < 20000) data.push_back(make_pair(id, crime));
        });
        if (data.empty()) return;
        auto timeInsert = [&](bool plus) {
            const auto start = chrono::steady_clock::now();
            if (plus) { BPlusTree<long long, Crime> t; for (const auto& x : data) t.insert(x.first, x.second); }
            else { BTree<long long, Crime> t; for (const auto& x : data) t.insert(x.first, x.second); }
            return chrono::duration<double, nano>(chrono::steady_clock::now() - start).count() / data.size();
        };
        auto timeSearch = [&](bool plus) {
            BTree<long long, Crime> bt; BPlusTree<long long, Crime> bp;
            if (plus) for (const auto& x : data) bp.insert(x.first, x.second);
            else for (const auto& x : data) bt.insert(x.first, x.second);
            volatile long long checksum = 0;
            const size_t reps = 10000;
            const auto start = chrono::steady_clock::now();
            for (size_t i = 0; i < reps; ++i) {
                const Crime* found = plus ? bp.find(data[i % data.size()].first) : bt.find(data[i % data.size()].first);
                if (found) checksum += found->id;
            }
            (void)checksum;
            return chrono::duration<double, nano>(chrono::steady_clock::now() - start).count() / reps;
        };
        auto timeDelete = [&](bool plus) {
            BTree<long long, Crime> bt; BPlusTree<long long, Crime> bp;
            if (plus) for (const auto& x : data) bp.insert(x.first, x.second);
            else for (const auto& x : data) bt.insert(x.first, x.second);
            const auto start = chrono::steady_clock::now();
            if (plus) for (const auto& x : data) bp.remove(x.first);
            else for (const auto& x : data) bt.remove(x.first);
            return chrono::duration<double, nano>(chrono::steady_clock::now() - start).count() / data.size();
        };
        btreeInsertNs = timeInsert(false); bplusInsertNs = timeInsert(true);
        btreeSearchNs = timeSearch(false); bplusSearchNs = timeSearch(true);
        btreeDeleteNs = timeDelete(false); bplusDeleteNs = timeDelete(true);
        benchmarkReady = true;
    };

    array<TextField, 3> fields = {{
        {{{300, 255}, {220, 48}}, "", "e.g. 12345", "CRIME ID", true},
        {{{540, 255}, {300, 48}}, "", "e.g. Theft", "DESCRIPTION", false},
        {{{860, 255}, {190, 48}}, "", "MM/DD/YYYY", "DATE", false}
    }};

    Page currentPage = Page::CrimeRecords;
    TreeType selectedTree = TreeType::BPlusTree;
    int activeField = -1;
    long long selectedCrimeID = -1;
    int scrollOffset = 0;
    bool filteredResults = false;
    vector<long long> matchingIds;
    const int visibleRows = 10;
    sf::Clock cursorClock;

    string status = dataLoaded ? loadMessage : "CSV not loaded. Check the file location.";
    sf::Color statusColor = dataLoaded ? green : red;
    string resultTitle = "No record selected";
    string resultDetails = "Search for a crime to view its details.";

    const sf::FloatRect crimeNav({15, 125}, {225, 48});
    const sf::FloatRect treeNav({15, 195}, {225, 48});
    const sf::FloatRect searchButton({300, 310}, {160, 46});
    const sf::FloatRect insertButton({475, 310}, {160, 46});
    const sf::FloatRect deleteButton({650, 310}, {160, 46});
    const sf::FloatRect tableBounds({300, 568}, {750, 352});
    const sf::FloatRect btreeCard({300, 200}, {350, 200});
    const sf::FloatRect bplusCard({680, 200}, {350, 200});

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouse->button != sf::Mouse::Button::Left) continue;
                sf::Vector2f pos(static_cast<float>(mouse->position.x),
                                 static_cast<float>(mouse->position.y));

                if (crimeNav.contains(pos)) {
                    currentPage = Page::CrimeRecords;
                    compareInputFocused = false;
                    activeField = -1;
                    continue;
                }
                if (treeNav.contains(pos)) {
                    currentPage = Page::TreeComparison;
                    activeField = -1;
                    compareInputFocused = false;
                    continue;
                }

                if (currentPage == Page::TreeComparison) {
                    if (btreeCard.contains(pos)) selectedTree = TreeType::BTree;
                    if (bplusCard.contains(pos)) selectedTree = TreeType::BPlusTree;
                    compareInputFocused = compareInput.contains(pos);
                    if (compareSearchButton.contains(pos)) {
                        try {
                            size_t used = 0;
                            const long long id = stoll(comparisonId, &used);
                            if (used != comparisonId.size() || id < 0)
                                throw invalid_argument("Invalid ID");

                            constexpr int repetitions = 10000;
                            volatile long long sink = 0;
                            auto measure = [&](bool plus) {
                                const auto start = chrono::steady_clock::now();
                                for (int i = 0; i < repetitions; ++i) {
                                    const Crime* record = plus ? bplus.find(id) : btree.find(id);
                                    if (record) sink += record->id;
                                }
                                return chrono::duration<double, nano>(
                                    chrono::steady_clock::now() - start).count() / repetitions;
                            };
                            comparisonBTreeNs = measure(false);
                            comparisonBPlusNs = measure(true);
                            const Crime* b = btree.find(id);
                            const Crime* bp = bplus.find(id);
                            comparisonResult = (b != nullptr) == (bp != nullptr) ?
                                (b ? "Found in both indexes" : "Not found in either index") :
                                "WARNING: index results differ";
                            comparisonSearched = true;
                            (void)sink;
                        } catch (const exception&) {
                            comparisonResult = "Enter a valid nonnegative crime ID.";
                            comparisonSearched = false;
                        }
                    }
                    if (compareBenchmarkButton.contains(pos)) runBenchmark();
                    continue;
                }
                if (currentPage != Page::CrimeRecords) continue;

                activeField = -1;
                for (int i = 0; i < 3; ++i) {
                    if (fields[i].bounds.contains(pos)) {
                        activeField = i;
                        cursorClock.restart();
                    }
                }

                if (tableBounds.contains(pos)) {
                    const float relativeY = pos.y - tableBounds.position.y;
                    if (relativeY >= 32.f && relativeY < tableBounds.size.y) {
                        const int clickedRow = int((relativeY - 32.f) / 32.f);
                        const int targetIndex = scrollOffset + clickedRow;
                        int index = 0;
                        auto selectRow = [&](long long id, const Crime& crime) {
                            if (index++ == targetIndex) {
                                selectedCrimeID = id;
                                fields[0].value = to_string(id);
                                fields[1].value = crime.description;
                                fields[2].value = formattedDate(crime) == "-" ? "" : formattedDate(crime);
                                resultTitle = "Crime ID: " + to_string(id);
                                resultDetails = crimeDetails(crime);
                                status = "Record selected.";
                                statusColor = blue;
                            }
                        };
                        if (filteredResults) {
                            if (targetIndex >= 0 && static_cast<size_t>(targetIndex) < matchingIds.size()) {
                                const long long id = matchingIds[static_cast<size_t>(targetIndex)];
                                const Crime* crime = bplus.find(id);
                                if (crime) { index = targetIndex; selectRow(id, *crime); }
                            }
                        } else bplus.ForEach(selectRow);
                    }
                }

                if (searchButton.contains(pos)) {
                    matchingIds.clear();
                    scrollOffset = 0;
                    selectedCrimeID = -1;

                    const bool hasId = !fields[0].value.empty();
                    const bool hasType = !fields[1].value.empty();
                    const bool hasDate = !fields[2].value.empty();
                    long long queryId = -1;

                    if (hasId) {
                        try {
                            size_t used = 0;
                            queryId = stoll(fields[0].value, &used);
                            if (used != fields[0].value.size() || queryId < 0)
                                throw invalid_argument("Invalid ID");
                        } catch (const exception&) {
                            status = "Enter a valid numeric crime ID.";
                            statusColor = red;
                            continue;
                        }
                    }
                    if (hasDate && !validDate(fields[2].value)) {
                        status = "Enter a valid date (MM/DD/YYYY).";
                        statusColor = red;
                        continue;
                    }

                    filteredResults = hasId || hasType || hasDate;
                    if (!filteredResults) {
                        resultTitle = "All crime records";
                        resultDetails = "Showing the complete database.";
                        status = "Showing all crime records.";
                        statusColor = muted;
                        continue;
                    }

                    const string typeQuery = lowercase(fields[1].value);
                    auto matches = [&](long long id, const Crime& crime) {
                        if (hasId && id != queryId) return false;
                        if (hasDate && formattedDate(crime) != fields[2].value) return false;
                        if (hasType && lowercase(crime.incidentType.empty() ?
                            crime.description : crime.incidentType).find(typeQuery) == string::npos)
                            return false;
                        return true;
                    };

                    if (hasId) {
                        const Crime* found = selectedTree == TreeType::BTree ?
                            btree.find(queryId) : bplus.find(queryId);
                        if (found && matches(queryId, *found)) matchingIds.push_back(queryId);
                    } else {
                        bplus.ForEach([&](long long id, const Crime& crime) {
                            if (matches(id, crime)) matchingIds.push_back(id);
                        });
                    }
                    resultTitle = to_string(matchingIds.size()) + " matching crime record(s)";
                    resultDetails = "Filtered by populated search fields.";
                    status = "Search completed. Scroll the results table below.";
                    statusColor = matchingIds.empty() ? muted : green;
                }

                if (insertButton.contains(pos)) {
                    try {
                        size_t idUsed = 0;
                        long long id = stoll(fields[0].value, &idUsed);
                        if (idUsed != fields[0].value.size() ||
                            id < 0 || !validDate(fields[2].value) ||
                            fields[1].value.empty())
                            throw invalid_argument("Invalid record");

                        if (bplus.find(id) != nullptr) {
                            status = "That crime ID already exists.";
                            statusColor = red;
                        } else {
                            Crime crime(id, fields[1].value, fields[2].value, "");
                            const bool insertedB = btree.insert(id, crime);
                            const bool insertedBP = bplus.insert(id, crime);
                            if (insertedB && insertedBP) {
                                selectedCrimeID = id;
                                resultTitle = "Crime ID: " + to_string(id);
                                resultDetails = crimeDetails(crime);
                                status = "Crime inserted successfully into both trees!";
                                statusColor = green;
                                benchmarkReady = false;
                                filteredResults = false; matchingIds.clear(); scrollOffset = 0;
                            } else {
                                if (insertedB) btree.remove(id);
                                if (insertedBP) bplus.remove(id);
                                status = "Insertion failed; indexes remain synchronized.";
                                statusColor = red;
                            }
                        }
                    } catch (const exception&) {
                        status = "Enter a valid ID, description, and date (MM/DD/YYYY).";
                        statusColor = red;
                    }
                }

                if (deleteButton.contains(pos)) {
                    try {
                        size_t used = 0;
                        long long id = stoll(fields[0].value, &used);
                        if (used != fields[0].value.size() || id < 0)
                            throw invalid_argument("Invalid ID");

                        const bool removedB = btree.remove(id);
                        const bool removedBP = bplus.remove(id);
                        if (removedB && removedBP) {
                            selectedCrimeID = -1;
                            status = "Crime deleted successfully from both trees!";
                            statusColor = green;
                            resultTitle = "Record deleted";
                            resultDetails = "Crime ID " + to_string(id) + " was removed.";
                            benchmarkReady = false;
                            filteredResults = false; matchingIds.clear(); scrollOffset = 0;
                        } else {
                            status = "Crime ID not found or index mismatch.";
                            statusColor = red;
                        }
                    } catch (const exception&) {
                        status = "Enter a valid numeric crime ID.";
                        statusColor = red;
                    }
                }
            }

            if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (currentPage == Page::CrimeRecords) {
                    sf::Vector2f pos(static_cast<float>(wheel->position.x),
                                     static_cast<float>(wheel->position.y));
                    if (tableBounds.contains(pos)) {
                        if (wheel->delta < 0) ++scrollOffset;
                        if (wheel->delta > 0) --scrollOffset;
                        scrollOffset = clamp(scrollOffset, 0,
                            max(0, int(filteredResults ? matchingIds.size() : bplus.size()) - visibleRows));
                    }
                }
            }

            if (const auto* typed = event->getIf<sf::Event::TextEntered>()) {
                if (currentPage == Page::TreeComparison && compareInputFocused) {
                    const auto code = typed->unicode;
                    if (code == 8 && !comparisonId.empty()) comparisonId.pop_back();
                    else if (code >= '0' && code <= '9' && comparisonId.size() < 19)
                        comparisonId += static_cast<char>(code);
                    continue;
                }
                if (currentPage != Page::CrimeRecords || activeField < 0) continue;
                auto& field = fields[activeField];
                const auto code = typed->unicode;
                if (code == 8) {
                    if (!field.value.empty()) field.value.pop_back();
                    cursorClock.restart();
                } else if (code >= 32 && code < 127 && field.value.size() < 25) {
                    const char character = static_cast<char>(code);
                    if ((activeField == 2 && field.value.size() < 10 &&
                         ((field.value.size() == 2 || field.value.size() == 5) ? character == '/' : (character >= '0' && character <= '9'))) ||
                        (activeField != 2 && (!field.numericOnly ||
                         (character >= '0' && character <= '9')))) {
                        field.value += character;
                        cursorClock.restart();
                    }
                }
            }
        }

        window.clear(background);
        drawBox(0, 0, 255, 1000, sidebar);
        drawText("GAINESVILLE", 25, 30, 23, white);
        drawText("CRIME DATABASE", 25, 62, 16, blue);

        const sf::Color inactiveNav(39, 52, 75);
        drawButton("Crime Records", crimeNav,
                   currentPage == Page::CrimeRecords ? blue : inactiveNav);
        drawButton("Tree Comparison", treeNav,
                   currentPage == Page::TreeComparison ? blue : inactiveNav);
        drawText("Data Structures Project", 25, 920, 14, muted);
        drawText("COP3530 | UF", 25, 945, 13, muted);

        if (currentPage == Page::CrimeRecords) {
            drawText("Crime Record Management", 300, 35, 30, white);
            drawText("Search, insert, and delete Gainesville crimes",
                     300, 85, 17, muted);
            drawRoundedBox(300, 130, 750, 58, 12, panel);
            drawText("Records Loaded: " + to_string(bplus.size()),
                     320, 144, 21, white);
            drawText(dataLoaded ? "CSV DATASET" : "NO DATA", 895, 148, 14, muted);

            const auto mouse = sf::Mouse::getPosition(window);
            sf::Vector2f mousePos(float(mouse.x), float(mouse.y));
            for (int i = 0; i < 3; ++i) {
                const auto& field = fields[i];
                const auto& bounds = field.bounds;
                const bool focused = activeField == i;
                const bool hovered = bounds.contains(mousePos);
                const sf::Color borderColor = focused ? blue :
                    (hovered ? sf::Color(100, 116, 139) : sf::Color(65, 81, 105));

                drawText(field.label, bounds.position.x,
                         bounds.position.y - 26, 13, muted);
                drawRoundedBox(bounds.position.x, bounds.position.y,
                               bounds.size.x, bounds.size.y, 10.f, borderColor);
                drawRoundedBox(bounds.position.x + 2, bounds.position.y + 2,
                               bounds.size.x - 4, bounds.size.y - 4,
                               8.f, background);

                const string display = field.value.empty() ?
                    field.placeholder : field.value;
                drawText(display, bounds.position.x + 20,
                         bounds.position.y + 11, 17,
                         field.value.empty() ? muted : white);

                if (focused && int(cursorClock.getElapsedTime().asSeconds() * 2) % 2 == 0) {
                    sf::Text typedText(font, field.value, 17);
                    const float cursorX = min(
                        bounds.position.x + 20 +
                        typedText.getLocalBounds().size.x + 2,
                        bounds.position.x + bounds.size.x - 12);
                    drawBox(cursorX, bounds.position.y + 12, 2, 24, blue);
                }
            }

            drawButton("Search", searchButton, blue);
            drawButton("Insert", insertButton, green);
            drawButton("Delete", deleteButton, red);
            drawText(status, 300, 370, 16, statusColor);
            drawRoundedBox(300, 400, 750, 115, 12, panel);
            drawText("SEARCH RESULT", 320, 412, 14, muted);
            drawText(resultTitle, 320, 438, 21, white);
            drawText(resultDetails, 320, 478, 16, muted);

            scrollOffset = clamp(scrollOffset, 0,
                max(0, int(filteredResults ? matchingIds.size() : bplus.size()) - visibleRows));
            drawText(filteredResults ? "Search Results" : "Current Records", 300, 532, 22, white);
            drawRoundedBox(300, 568, 750, 352, 10, panel);
            drawText("CRIME ID", 320, 575, 13, muted);
            drawText("DESCRIPTION", 510, 575, 13, muted);
            drawText("DATE", 890, 575, 13, muted);
            drawBox(300, 599, 750, 1, sf::Color(65, 81, 105));

            int index = 0;
            int displayed = 0;
            auto renderRow = [&](long long id, const Crime& crime) {
                if (index++ < scrollOffset || displayed >= visibleRows) return;
                const float rowY = 600.f + displayed * 32.f;
                sf::Color rowColor = displayed % 2 == 0 ?
                    sf::Color(30, 41, 59) : sf::Color(37, 49, 68);
                if (id == selectedCrimeID) rowColor = sf::Color(38, 69, 111);
                drawBox(302, rowY, 746, 31, rowColor);
                drawText(to_string(id), 320, rowY + 5, 16, white);
                drawText(crime.description, 510, rowY + 5, 16, white);
                drawText(formattedDate(crime), 890, rowY + 5, 16, muted);
                ++displayed;
            };
            if (filteredResults) {
                for (size_t i = static_cast<size_t>(scrollOffset);
                     i < matchingIds.size() && displayed < visibleRows; ++i) {
                    const Crime* crime = bplus.find(matchingIds[i]);
                    if (crime) { index = static_cast<int>(i); renderRow(matchingIds[i], *crime); }
                }
            } else bplus.ForEach(renderRow);
            const size_t totalRows = filteredResults ? matchingIds.size() : bplus.size();
            if (totalRows == 0)
                drawText(filteredResults ? "No matching crimes found." : "No crime records available.", 320, 610, 16, muted);
            if (totalRows > visibleRows) {
                const string range = to_string(scrollOffset + 1) + "-" +
                    to_string(min<size_t>(scrollOffset + visibleRows, totalRows)) +
                    " of " + to_string(totalRows);
                drawText(range, 925, 540, 13, muted);
            }
        }

        else if (currentPage == Page::TreeComparison) {
            drawText("Tree Comparison", 300, 35, 32, white);
            drawText("Order-16 indexes | same records, different structures", 300, 85, 17, muted);
            drawText("CLICK A TREE TO SELECT IT FOR CRIME RECORD SEARCHES", 300, 145, 13, muted);

            const auto bStats = btree.statistics();
            const auto bpStats = bplus.statistics();
            auto drawTreeCard = [&](const sf::FloatRect& bounds, const string& name,
                                    bool selected, size_t records, size_t height,
                                    size_t nodes, size_t leaves) {
                drawRoundedBox(bounds.position.x, bounds.position.y,
                               bounds.size.x, bounds.size.y, 14,
                               selected ? sf::Color(38, 69, 111) : panel);
                const float x = bounds.position.x + 20;
                const float y = bounds.position.y;
                drawText(name, x, y + 14, 25, white);
                drawText(selected ? "SELECTED FOR SEARCH" : "Click to select", x, y + 52,
                         14, selected ? green : blue);
                drawText("Records: " + to_string(records), x, y + 85, 17, white);
                drawText("Height (levels): " + to_string(height), x, y + 112, 16, muted);
                drawText("Nodes: " + to_string(nodes), x, y + 139, 16, muted);
                drawText("Leaf nodes: " + to_string(leaves), x, y + 166, 16, muted);
            };
            drawTreeCard(btreeCard, "B-Tree", selectedTree == TreeType::BTree,
                         btree.size(), bStats.height, bStats.nodes, bStats.leaves);
            drawTreeCard(bplusCard, "B+ Tree", selectedTree == TreeType::BPlusTree,
                         bplus.size(), bpStats.height, bpStats.nodes, bpStats.leaves);

            drawRoundedBox(300, 425, 750, 175, 12, panel);
            drawText("Compare the same ID in both trees", 320, 440, 20, white);
            drawRoundedBox(318, 480, 224, 47, 9, compareInputFocused ? blue : muted);
            drawRoundedBox(320, 482, 220, 43, 8, background);
            drawText(comparisonId.empty() ? "Enter crime ID" : comparisonId,
                     332, 491, 17, comparisonId.empty() ? muted : white);
            drawButton("Compare Search", compareSearchButton, blue);
            drawButton("Run Benchmarks", compareBenchmarkButton, sf::Color(50, 105, 150));
            drawText(comparisonResult, 320, 539, 16,
                     comparisonResult.find("WARNING") == 0 ? red : muted);
            if (comparisonSearched) {
                drawText("B-Tree: " + to_string(static_cast<long long>(comparisonBTreeNs)) +
                         " ns/search", 320, 566, 14, white);
                drawText("B+ Tree: " + to_string(static_cast<long long>(comparisonBPlusNs)) +
                         " ns/search", 655, 566, 14, white);
            }

            drawRoundedBox(300, 620, 750, 148, 12, panel);
            drawText("Average benchmark time (ns per operation)", 320, 635, 19, white);
            drawText("Operation", 320, 667, 15, muted);
            drawText("B-Tree", 615, 667, 15, blue);
            drawText("B+ Tree", 825, 667, 15, green);
            if (benchmarkReady) {
                auto benchmarkRow = [&](const string& name, double b, double bp, float y) {
                    drawText(name, 320, y, 16, white);
                    drawText(to_string(static_cast<long long>(b)), 615, y, 16, white);
                    drawText(to_string(static_cast<long long>(bp)), 825, y, 16, white);
                };
                benchmarkRow("Insert", btreeInsertNs, bplusInsertNs, 692);
                benchmarkRow("Search", btreeSearchNs, bplusSearchNs, 716);
                benchmarkRow("Delete", btreeDeleteNs, bplusDeleteNs, 740);
            } else {
                drawText("Click Run Benchmarks to compare operations.", 320, 704, 16, muted);
            }

            drawRoundedBox(300, 790, 750, 155, 12, panel);
            drawText("Why these structures differ", 320, 803, 20, white);
            drawText("B-Tree: Keys and records are stored in internal and leaf nodes.",
                     320, 837, 15, muted);
            drawText("B+ Tree: Records are stored in linked leaves; internal nodes route searches.",
                     320, 864, 15, muted);
            drawText("Both indexes stay synchronized on insert/delete. Lower timing is faster.",
                     320, 895, 15, muted);
            drawText("Timing varies by hardware, dataset and measurement noise.",
                     320, 920, 13, muted);
        }

        window.display();
    }
    return 0;
}
