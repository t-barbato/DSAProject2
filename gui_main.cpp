
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

using namespace std;

enum class Page { CrimeRecords, TreeComparison, Performance };
enum class TreeType { BTree, BPlusTree };

struct Crime {
    string description;
    int year;
};

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

int main() {
    // WINDOW AND THEME

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

    // DRAWING HELPERS

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

    // SAMPLE DATA AND APPLICATION STATE

    map<int, Crime> records = {
        {12345, {"Theft", 2024}},
        {67890, {"Burglary", 2023}},
        {80921, {"Assault", 2025}}
    };

    array<TextField, 3> fields = {{
        {{{300, 235}, {220, 48}}, "", "e.g. 12345", "CRIME ID", true},
        {{{540, 235}, {300, 48}}, "", "e.g. Theft", "DESCRIPTION", false},
        {{{860, 235}, {190, 48}}, "", "e.g. 2024", "YEAR", true}
    }};

    Page currentPage = Page::CrimeRecords;
    TreeType selectedTree = TreeType::BPlusTree;
    int activeField = -1;
    int selectedCrimeID = -1;
    int scrollOffset = 0;
    const int visibleRows = 10;
    sf::Clock cursorClock;

    string status = "Ready. Select a field to begin.";
    sf::Color statusColor = muted;
    string resultTitle = "No record selected";
    string resultDetails = "Search for a crime to view its details.";

    const sf::FloatRect crimeNav({15, 125}, {225, 48});
    const sf::FloatRect treeNav({15, 195}, {225, 48});
    const sf::FloatRect performanceNav({15, 265}, {225, 48});
    const sf::FloatRect searchButton({300, 310}, {160, 46});
    const sf::FloatRect insertButton({475, 310}, {160, 46});
    const sf::FloatRect deleteButton({650, 310}, {160, 46});
    const sf::FloatRect tableBounds({300, 568}, {750, 352});
    const sf::FloatRect btreeCard({300, 200}, {350, 200});
    const sf::FloatRect bplusCard({680, 200}, {350, 200});

    // EVENT HANDLING

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
                    activeField = -1;
                    continue;
                }
                if (treeNav.contains(pos)) {
                    currentPage = Page::TreeComparison;
                    activeField = -1;
                    continue;
                }
                if (performanceNav.contains(pos)) {
                    currentPage = Page::Performance;
                    activeField = -1;
                    continue;
                }

                if (currentPage == Page::TreeComparison) {
                    if (btreeCard.contains(pos)) selectedTree = TreeType::BTree;
                    if (bplusCard.contains(pos)) selectedTree = TreeType::BPlusTree;
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
                        for (const auto& record : records) {
                            if (index++ == targetIndex) {
                                selectedCrimeID = record.first;
                                fields[0].value = to_string(record.first);
                                fields[1].value = record.second.description;
                                fields[2].value = to_string(record.second.year);
                                resultTitle = "Crime ID: " + to_string(record.first);
                                resultDetails = record.second.description +
                                                " | Year: " + to_string(record.second.year);
                                status = "Record selected.";
                                statusColor = blue;
                                break;
                            }
                        }
                    }
                }

                if (searchButton.contains(pos)) {
                    try {
                        size_t used = 0;
                        int id = stoi(fields[0].value, &used);
                        if (used != fields[0].value.size() || id < 0)
                            throw invalid_argument("Invalid ID");

                        const auto found = records.find(id);
                        if (found != records.end()) {
                            selectedCrimeID = id;
                            resultTitle = "Crime ID: " + to_string(id);
                            resultDetails = found->second.description +
                                            " | Year: " + to_string(found->second.year);
                            status = "Crime found successfully.";
                            statusColor = green;
                        } else {
                            selectedCrimeID = -1;
                            resultTitle = "Record not found";
                            resultDetails = "No crime exists with that ID.";
                            status = "Search completed.";
                            statusColor = muted;
                        }
                    } catch (const exception&) {
                        status = "Enter a valid numeric crime ID.";
                        statusColor = red;
                    }
                }

                if (insertButton.contains(pos)) {
                    try {
                        size_t idUsed = 0, yearUsed = 0;
                        int id = stoi(fields[0].value, &idUsed);
                        int year = stoi(fields[2].value, &yearUsed);
                        if (idUsed != fields[0].value.size() ||
                            yearUsed != fields[2].value.size() ||
                            id < 0 || year < 1900 || year > 2100 ||
                            fields[1].value.empty())
                            throw invalid_argument("Invalid record");

                        if (records.count(id)) {
                            status = "That crime ID already exists.";
                            statusColor = red;
                        } else {
                            records[id] = {fields[1].value, year};
                            selectedCrimeID = id;
                            resultTitle = "Crime ID: " + to_string(id);
                            resultDetails = fields[1].value +
                                            " | Year: " + to_string(year);
                            status = "Crime inserted successfully!";
                            statusColor = green;
                        }
                    } catch (const exception&) {
                        status = "Enter a valid ID, description, and year.";
                        statusColor = red;
                    }
                }

                if (deleteButton.contains(pos)) {
                    try {
                        size_t used = 0;
                        int id = stoi(fields[0].value, &used);
                        if (used != fields[0].value.size() || id < 0)
                            throw invalid_argument("Invalid ID");

                        if (records.erase(id) > 0) {
                            selectedCrimeID = -1;
                            status = "Crime deleted successfully!";
                            statusColor = green;
                            resultTitle = "Record deleted";
                            resultDetails = "Crime ID " + to_string(id) + " was removed.";
                        } else {
                            status = "Crime ID not found.";
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
                            max(0, int(records.size()) - visibleRows));
                    }
                }
            }

            if (const auto* typed = event->getIf<sf::Event::TextEntered>()) {
                if (currentPage != Page::CrimeRecords || activeField < 0) continue;
                auto& field = fields[activeField];
                const auto code = typed->unicode;
                if (code == 8) {
                    if (!field.value.empty()) field.value.pop_back();
                    cursorClock.restart();
                } else if (code >= 32 && code < 127 && field.value.size() < 25) {
                    const char character = static_cast<char>(code);
                    if (!field.numericOnly ||
                        (character >= '0' && character <= '9')) {
                        field.value += character;
                        cursorClock.restart();
                    }
                }
            }
        }

        // SIDEBAR AND NAVIGATION

        window.clear(background);
        drawBox(0, 0, 255, 1000, sidebar);
        drawText("GAINESVILLE", 25, 30, 23, white);
        drawText("CRIME DATABASE", 25, 62, 16, blue);

        const sf::Color inactiveNav(39, 52, 75);
        drawButton("Crime Records", crimeNav,
                   currentPage == Page::CrimeRecords ? blue : inactiveNav);
        drawButton("Tree Comparison", treeNav,
                   currentPage == Page::TreeComparison ? blue : inactiveNav);
        drawButton("Performance", performanceNav,
                   currentPage == Page::Performance ? blue : inactiveNav);
        drawText("Data Structures Project", 25, 920, 14, muted);
        drawText("COP3530 | UF", 25, 945, 13, muted);

        // CRIME RECORDS PAGE

        if (currentPage == Page::CrimeRecords) {
            drawText("Crime Record Management", 300, 35, 30, white);
            drawText("Search, insert, and delete Gainesville crimes",
                     300, 85, 17, muted);
            drawRoundedBox(300, 130, 750, 58, 12, panel);
            drawText("Records Loaded: " + to_string(records.size()),
                     320, 144, 21, white);
            drawText("DEMO DATA", 895, 148, 14, muted);

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

            // SCROLLABLE CRIME TABLE

            scrollOffset = clamp(scrollOffset, 0,
                max(0, int(records.size()) - visibleRows));
            drawText("Current Records", 300, 532, 22, white);
            drawRoundedBox(300, 568, 750, 352, 10, panel);
            drawText("CRIME ID", 320, 575, 13, muted);
            drawText("DESCRIPTION", 510, 575, 13, muted);
            drawText("YEAR", 890, 575, 13, muted);
            drawBox(300, 599, 750, 1, sf::Color(65, 81, 105));

            int index = 0;
            int displayed = 0;
            for (const auto& record : records) {
                if (index++ < scrollOffset) continue;
                if (displayed >= visibleRows) break;
                const float rowY = 600.f + displayed * 32.f;
                sf::Color rowColor = displayed % 2 == 0 ?
                    sf::Color(30, 41, 59) : sf::Color(37, 49, 68);
                if (record.first == selectedCrimeID)
                    rowColor = sf::Color(38, 69, 111);

                drawBox(302, rowY, 746, 31, rowColor);
                drawText(to_string(record.first), 320, rowY + 5, 16, white);
                drawText(record.second.description, 510, rowY + 5, 16, white);
                drawText(to_string(record.second.year), 890, rowY + 5, 16, muted);
                ++displayed;
            }
            if (records.empty())
                drawText("No crime records available.", 320, 610, 16, muted);
            if (records.size() > visibleRows) {
                const string range = to_string(scrollOffset + 1) + "-" +
                    to_string(min(scrollOffset + visibleRows, int(records.size()))) +
                    " of " + to_string(records.size());
                drawText(range, 925, 540, 13, muted);
            }
        }

        // TREE COMPARISON PAGE

        else if (currentPage == Page::TreeComparison) {
            drawText("Tree Comparison", 300, 35, 32, white);
            drawText("Select the data structure for future crime operations",
                     300, 85, 17, muted);
            drawText("SELECT DATA STRUCTURE", 300, 155, 15, muted);

            auto drawTreeCard = [&](const sf::FloatRect& bounds, const string& name, bool selected) {
                drawRoundedBox(bounds.position.x, bounds.position.y,
                            bounds.size.x, bounds.size.y, 14,
                            selected ? sf::Color(38, 69, 111) : panel);

                const auto mouse = sf::Mouse::getPosition(window);
                const sf::Vector2f pos(float(mouse.x), float(mouse.y));

                if (bounds.contains(pos)) {
                    auto outline = roundedRectangle(bounds.size.x,
                        bounds.size.y, 14.f, sf::Color::Transparent);
                    outline.setPosition(bounds.position);
                    outline.setOutlineThickness(2.f);
                    outline.setOutlineColor(blue);
                    window.draw(outline);
                }

                drawText(name, bounds.position.x + 22,
                        bounds.position.y + 35, 27, white);

                drawText(selected ? "SELECTED" : "Click to select",
                        bounds.position.x + 22, bounds.position.y + 100,
                        16, selected ? green : blue);
            };

            const bool btreeSelected = selectedTree == TreeType::BTree;
            drawTreeCard(btreeCard, "B-Tree", btreeSelected);
            drawTreeCard(bplusCard, "B+ Tree", !btreeSelected);

            drawRoundedBox(300, 435, 730, 145, 12, panel);
            drawText("CURRENT SELECTION", 325, 455, 15, muted);
            drawText(btreeSelected ? "B-Tree" : "B+ Tree", 325, 485, 25, white);
            drawText("Implementation integration pending", 325, 530, 16, muted);

            drawRoundedBox(300, 610, 730, 160, 12, panel);
            drawText("About the Data Structures", 325, 630, 23, white);
            drawText("B-Tree: Stores keys and associated data in tree nodes.",
                     325, 685, 16, muted);
            drawText("B+ Tree: Stores records in leaves, supporting",
                     325, 715, 16, muted);
            drawText("efficient ordered traversal through leaf links.",
                     325, 742, 16, muted);
        }

        // PERFORMANCE PAGE PLACEHOLDER

        else if (currentPage == Page::Performance) {
            drawText("Performance Analysis", 300, 35, 32, white);
            drawText("Compare B-Tree and B+ Tree performance", 300, 85, 17, muted);
            drawRoundedBox(300, 150, 730, 160, 12, panel);
            drawText("Performance placeholder", 325, 175, 25, white);
            drawText("O-time and stuff's gonna go here",
                     325, 235, 17, muted);
        }

        window.display();
    }
    return 0;
}