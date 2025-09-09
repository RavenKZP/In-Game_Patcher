#include "Utils.h"

namespace Utils {

    bool IsDynamicForm(RE::TESForm* form) {
        if (!form) return false;
        return ((form->GetFormID() >> 24) & 0xFF) == 0xFF;
    }

    RE::BGSKeyword* FindOrCreateKeyword(std::string kwd) {
        auto form = RE::TESForm::LookupByEditorID(kwd);
        if (form) {
            return form->As<RE::BGSKeyword>();
        }
        logger::warn("No keyword {} found, creating it", kwd);
        const auto factory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::BGSKeyword>();
        RE::BGSKeyword* createdkeyword = nullptr;
        if (createdkeyword = factory ? factory->Create() : nullptr; createdkeyword) {
            createdkeyword->formEditorID = kwd;
        }
        return createdkeyword;
    }

    RE::TESForm* GetFormFromString(const std::string& str) {
        // Expect format: 0xXXXXXX~Plugin.esp
        auto sep = str.find('~');
        if (sep == std::string::npos) {
            return nullptr;
        }

        std::string idPart = str.substr(0, sep);
        std::string pluginName = str.substr(sep + 1);

        // Convert hex string to formID
        RE::FormID formID = static_cast<RE::FormID>(std::stoul(idPart, nullptr, 16));

        // Look up plugin file
        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return nullptr;
        }

        auto file = dataHandler->LookupModByName(pluginName);
        if (!file) {
            return nullptr;
        }

        // Compute full FormID with correct load index
        RE::FormID actualFormID = formID;
        if (file->IsLight()) {
            // ESL (0xFE)
            actualFormID = (0xFE000000 | (file->GetPartialIndex() << 12) | (formID & 0xFFF));
        } else {
            // Standard plugin
            actualFormID |= (file->GetCompileIndex() << 24);
        }

        return RE::TESForm::LookupByID(actualFormID);
    }

    OverridesData ParseOverrides(const std::string& str) {
        OverridesData data{};

        auto extractFloats = [](const std::string& s, float* out, int count) {
            std::stringstream ss(s);
            std::string num;
            int i = 0;
            while (std::getline(ss, num, ',') && i < count) {
                out[i++] = std::stof(num);
            }
        };

        if (auto p = str.find("posA("); p != std::string::npos) {
            auto end = str.find(')', p);
            if (end != std::string::npos) {
                std::string values = str.substr(p + 5, end - (p + 5));
                extractFloats(values, data.pos, 3);
                data.hasPos = true;
            }
        }
        if (auto p = str.find("rotA("); p != std::string::npos) {
            auto end = str.find(')', p);
            if (end != std::string::npos) {
                std::string values = str.substr(p + 5, end - (p + 5));
                extractFloats(values, data.rot, 3);
                data.hasRot = true;
            }
        }
        if (auto p = str.find("scale("); p != std::string::npos) {
            auto end = str.find(')', p);
            if (end != std::string::npos) {
                std::string values = str.substr(p + 6, end - (p + 6));
                data.scale = std::stof(values);
                data.hasScale = true;
            }
        }
        return data;
    }

    std::string BuildOverrides(const OverridesData& data) {
        std::ostringstream out;
        if (data.hasPos) {
            out << "posA(" << data.pos[0] << "," << data.pos[1] << "," << data.pos[2] << ")";
        }
        if (data.hasRot) {
            if (!out.str().empty()) out << ",";
            out << "rotA(" << data.rot[0] << "," << data.rot[1] << "," << data.rot[2] << ")";
        }
        if (data.hasScale) {
            if (!out.str().empty()) out << ",";
            out << "scale(" << data.scale << ")";
        }
        return out.str();
    }

    const RE::TESFile* GetMasterFile(RE::TESForm* ref) {
        if (!ref) return nullptr;

        uint32_t formID = ref->GetFormID();
        uint8_t modIndex = static_cast<uint8_t>(formID >> 24);  // upper byte

        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) return nullptr;

        // Handle ESL (0xFE) or dynamic (0xFF) if needed
        if (modIndex == 0xFE || modIndex == 0xFF) {
            // For ESL: the next byte contains the ESL index
            if (modIndex == 0xFE) {
                uint8_t eslIndex = (formID >> 12) & 0xFF;
                return dataHandler->LookupLoadedLightModByIndex(eslIndex);
            }
            // Dynamic refs don’t have a master file
            return nullptr;
        }

        // Standard plugin
        return dataHandler->LookupLoadedModByIndex(modIndex);
    }

    bool CreateNewBOSFile(const std::string& filePath) {
        if (std::filesystem::exists(filePath)) {
            return false;  // don’t overwrite existing
        }

        std::ofstream ofs(filePath);
        if (!ofs.is_open()) {
            return false;
        }

        ofs << "[References]\n\n";
        ofs << "[Transforms]\n";

        return true;
    }

    bool CreateNewKIDFile(const std::string& filePath) {
        if (std::filesystem::exists(filePath)) {
            return false;  // don’t overwrite existing
        }

        std::ofstream ofs(filePath);
        if (!ofs.is_open()) {
            return false;
        }

        return true;
    }
    
    std::string NormalizeFormID(RE::TESForm* form) {
        if (!form) {
            return {};
        }

        auto file = Utils::GetMasterFile(form);
        if (!file) {
            return {};
        }

        RE::FormID formID = form->GetFormID();

        uint8_t modIndex = (formID >> 24) & 0xFF;
        uint32_t localID = formID & 0x00FFFFFF;

        if (modIndex == 0xFF) {
            // Dynamic form -> skip
            return {};
        }

        std::string hexPart;

        if (modIndex == 0xFE) {
            // ESL/light plugin -> only last 3 hex digits
            uint32_t eslID = localID & 0xFFF;
            hexPart = std::format("0x{:03X}", eslID);
        } else {
            // Normal plugin -> 6 hex digits
            hexPart = std::format("0x{:06X}", localID);
        }

        // Attach plugin filename
        return std::format("{}~{}", hexPart, file->GetFilename());
    }

    std::string GetObjectTypeName(RE::TESBoundObject* obj) {
        if (!obj) {
            return "None";
        }

        switch (obj->GetFormType()) {
            case RE::FormType::Weapon:
                return "Weapon";
            case RE::FormType::Armor:
                return "Armor";
            case RE::FormType::Ammo:
                return "Ammo";
            case RE::FormType::MagicEffect:
                return "Magic Effect";
            case RE::FormType::AlchemyItem:
                return "Potion";
            case RE::FormType::Scroll:
                return "Scroll";
            case RE::FormType::Location:
                return "Location";
            case RE::FormType::Ingredient:
                return "Ingredient";
            case RE::FormType::Book:
                return "Book";
            case RE::FormType::Misc:
                return "Misc Item";
            case RE::FormType::KeyMaster:
                return "Key";
            case RE::FormType::SoulGem:
                return "Soul Gem";
            case RE::FormType::Spell:
                return "Spell";
            case RE::FormType::Activator:
                return "Activator";
            case RE::FormType::Flora:
                return "Flora";
            case RE::FormType::Furniture:
                return "Furniture";
            case RE::FormType::Race:
                return "Race";
            case RE::FormType::TalkingActivator:
                return "Talking Activator";
            case RE::FormType::Enchantment:
                return "Enchantment";
            default:
                return "Unknown";
        }
    }

    std::vector<RE::TESForm*> GetIndirectKIDTargets(RE::TESObjectREFR* ref) {
        // To add more types, add them here
        std::vector<RE::TESForm*> targets;
        if (!ref) return targets;

        auto base = ref->GetBaseObject();
        if (!base) return targets;

        if (base->Is(RE::FormType::ActorCharacter)) {
            if (auto actor = ref->As<RE::Actor>()) {
                if (actor->GetRace()) targets.push_back(actor->GetRace());
            }
        }

        return targets;
    }
}