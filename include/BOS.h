#pragma once

#include "Utils.h"

#include "ClibUtil/singleton.hpp"

class BOSIniManager : public clib_util::singleton::ISingleton<BOSIniManager> {
public:

    bool Save() const;

    bool SetFile(std::string newFilePatch);
    void RemoveFromFile(const std::string& id) const;
    std::string GetFile() const { return filePath; }

    void AddTransform(const BOSTransform& tr);
    void RemoveTransform(RE::TESForm* ref);

    void RemoveObject(RE::TESObjectREFR* ref);
    void TransformObject(RE::TESObjectREFR* ref);

    const std::unordered_map<std::string, BOSTransform>& GetTransforms() const { return newTransforms; }

    void RememberOriginal(RE::TESObjectREFR* ref);
    void ResetObject(RE::TESObjectREFR* ref);

    std::unordered_map<std::string, BOSTransform> newTransforms;

    std::unordered_map<RE::FormID, BOSOriginalData> originalStates;

private:
    std::string filePath = "Data\\InGamePatcher_SWAP.ini";

};
