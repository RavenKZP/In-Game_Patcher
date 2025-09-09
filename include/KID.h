#pragma once

#pragma once

#include "ClibUtil/singleton.hpp"
#include "RE/Skyrim.h"
#include "Utils.h"

class KIDIniManager : public clib_util::singleton::ISingleton<KIDIniManager> {
public:
    bool Save() const;

    bool SetFile(const std::string& newFilePath);
    void RemoveFromFile(const std::string& id, const std::string& keyword) const;
    std::string GetFile() const { return filePath; }

    void AddEntry(const KIDEntry& entry);
    void RemoveEntry(RE::TESForm* form, const std::string& keyword);

    const std::vector<KIDEntry>& GetEntries() const { return newEntries; }

private:
    std::string filePath = "Data\\InGamePatcher_KID.ini";
    std::vector<KIDEntry> newEntries;
};