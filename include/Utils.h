#pragma once

struct OverridesData {
    bool hasPos = false;
    bool hasRot = false;
    bool hasScale = false;

    float pos[3]{};
    float rot[3]{};
    float scale{};
};

struct BOSOriginalData {
    RE::FormID formID{};
    RE::NiPoint3 pos{};
    RE::NiPoint3 rot{};
    float scale{1.0f};
};


struct BOSReference {
    std::string origRefID;
    std::string swapBaseID;
    std::string propertyOverrides;
};

struct BOSTransform {
    std::string origRefID;
    std::string propertyOverrides;
};

struct KIDEntry {
    std::string keyword;
    std::string type;
    std::string objectID;
};

namespace ObjectManipulationOverhaul {
    inline void StartDraggingObject(RE::TESObjectREFR* ref) {
        using func_t = void (*)(RE::TESObjectREFR*);
        static auto ObjectManipulationOverhaul = GetModuleHandle(L"ObjectManipulationOverhaul");
        func_t func = reinterpret_cast<func_t>(GetProcAddress(ObjectManipulationOverhaul, "StartDraggingObject"));
        return func(ref);
    }
}

namespace Utils {

    bool IsDynamicForm(RE::TESForm* form);

    RE::BGSKeyword* FindOrCreateKeyword(std::string kwd);

    std::string NormalizeFormID(RE::TESForm* form);

    RE::TESForm* GetFormFromString(const std::string& str);
    const RE::TESFile* GetMasterFile(RE::TESForm* ref);
    std::string GetObjectTypeName(RE::TESBoundObject* obj);

    OverridesData ParseOverrides(const std::string& str);
    std::string BuildOverrides(const OverridesData& data);

    bool CreateNewBOSFile(const std::string& filePath);
    bool CreateNewKIDFile(const std::string& filePath);

    std::vector<RE::TESForm*> GetIndirectKIDTargets(RE::TESObjectREFR* ref);
}