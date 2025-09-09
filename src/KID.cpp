#include "KID.h"

#include <unordered_set>

bool KIDIniManager::Save() const {
    // Read existing lines (if file exists)
    std::ifstream in(filePath);
    std::vector<std::string> lines;
    if (in.is_open()) {
        std::string l;
        while (std::getline(in, l)) {
            lines.push_back(l);
        }
    }
    in.close();

    // Deduplication set
    std::unordered_set<std::string> seen;

    // Open for rewrite
    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    // Write back existing lines first (skip duplicates inside the file itself)
    for (const auto& line : lines) {
        if (seen.insert(line).second) {  // true if not already in set
            out << line << "\n";
        }
    }

    // Append new runtime entries (skip duplicates against existing lines)
    for (const auto& e : newEntries) {
        std::string newLine = "Keyword = " + e.keyword + "|" + e.type + "|" + e.objectID;
        if (seen.insert(newLine).second) {
            out << newLine << "\n";
        }
    }

    return true;
}

void KIDIniManager::RemoveFromFile(const std::string& objectID, const std::string& keywordEditorID) const {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        return;
    }

    std::vector<std::string> lines;
    std::string l;
    while (std::getline(in, l)) {
        lines.push_back(l);
    }
    in.close();

    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    for (auto& line : lines) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

        // Only process lines that start with "Keyword ="
        if (trimmed.rfind("Keyword =", 0) == 0) {
            std::string rhs = trimmed.substr(9);         // skip "Keyword ="
            rhs.erase(0, rhs.find_first_not_of(" \t"));  // trim leading spaces

            size_t first = rhs.find('|');
            size_t second = rhs.find('|', first + 1);

            if (first != std::string::npos && second != std::string::npos) {
                std::string kwEditorID = rhs.substr(0, first);
                std::string objID = rhs.substr(second + 1);

                if (kwEditorID == keywordEditorID && objID == objectID) {
                    continue;  // skip this line
                }
            }
        }

        out << line << "\n";  // keep line
    }
}

bool KIDIniManager::SetFile(const std::string& newFilePath) {
    std::ifstream file(newFilePath);
    if (!file.is_open()) {
        logger::error("File {} not found!", newFilePath);
        return false;
    }
    filePath = newFilePath;
    newEntries.clear();
    return true;
}

void KIDIniManager::AddEntry(const KIDEntry& entry) {
    newEntries.push_back(entry);
    Save();
}

void KIDIniManager::RemoveEntry(RE::TESForm* form, const std::string& keyword) {
    if (!form) return;

    std::string id = Utils::NormalizeFormID(form);
    newEntries.erase(std::remove_if(newEntries.begin(), newEntries.end(),
                                    [&](const KIDEntry& e) { return e.objectID == id && e.keyword == keyword; }),
                     newEntries.end());
    RemoveFromFile(id, keyword);
}
