#include "Utils.h"

#include <sstream>

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
        uint8_t modIndex = static_cast<uint8_t>(formID >> 24);

        auto dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) return nullptr;

        if (modIndex == 0xFE) {
            uint16_t eslIndex = (formID >> 12) & 0xFFF;
            return dataHandler->LookupLoadedLightModByIndex(eslIndex);
        }

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

    std::string toLower(const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::optional<SkyPromptAPI::ButtonID> KeyNameToButtonID(RE::INPUT_DEVICE device, std::string keyStr) {
        std::unordered_map<std::string, uint32_t> keyboardKeys = {
            {"escape", 0x01},      {"num1", 0x02},        {"num2", 0x03},
            {"num3", 0x04},        {"num4", 0x05},        {"num5", 0x06},
            {"num6", 0x07},        {"num7", 0x08},        {"num8", 0x09},
            {"num9", 0x0A},        {"num0", 0x0B},        {"minus", 0x0C},
            {"equals", 0x0D},      {"backspace", 0x0E},   {"tab", 0x0F},
            {"q", 0x10},           {"w", 0x11},           {"e", 0x12},
            {"r", 0x13},           {"t", 0x14},           {"y", 0x15},
            {"u", 0x16},           {"i", 0x17},           {"o", 0x18},
            {"p", 0x19},           {"bracketleft", 0x1A}, {"bracketright", 0x1B},
            {"enter", 0x1C},       {"leftcontrol", 0x1D}, {"a", 0x1E},
            {"s", 0x1F},           {"d", 0x20},           {"f", 0x21},
            {"g", 0x22},           {"h", 0x23},           {"j", 0x24},
            {"k", 0x25},           {"l", 0x26},           {"semicolon", 0x27},
            {"apostrophe", 0x28},  {"tilde", 0x29},       {"leftshift", 0x2A},
            {"backslash", 0x2B},   {"z", 0x2C},           {"x", 0x2D},
            {"c", 0x2E},           {"v", 0x2F},           {"b", 0x30},
            {"n", 0x31},           {"m", 0x32},           {"comma", 0x33},
            {"period", 0x34},      {"slash", 0x35},       {"rightshift", 0x36},
            {"kp_multiply", 0x37}, {"leftalt", 0x38},     {"spacebar", 0x39},
            {"capslock", 0x3A},    {"f1", 0x3B},          {"f2", 0x3C},
            {"f3", 0x3D},          {"f4", 0x3E},          {"f5", 0x3F},
            {"f6", 0x40},          {"f7", 0x41},          {"f8", 0x42},
            {"f9", 0x43},          {"f10", 0x44},         {"numlock", 0x45},
            {"scrolllock", 0x46},  {"kp_7", 0x47},        {"kp_8", 0x48},
            {"kp_9", 0x49},        {"kp_subtract", 0x4A}, {"kp_4", 0x4B},
            {"kp_5", 0x4C},        {"kp_6", 0x4D},        {"kp_plus", 0x4E},
            {"kp_1", 0x4F},        {"kp_2", 0x50},        {"kp_3", 0x51},
            {"kp_0", 0x52},        {"kp_decimal", 0x53},  {"f11", 0x57},
            {"f12", 0x58},         {"kp_enter", 0x9C},    {"rightcontrol", 0x9D},
            {"kp_divide", 0xB5},   {"printscreen", 0xB7}, {"rightalt", 0xB8},
            {"pause", 0xC5},       {"home", 0xC7},        {"up", 0xC8},
            {"pageup", 0xC9},      {"left", 0xCB},        {"right", 0xCD},
            {"end", 0xCF},         {"down", 0xD0},        {"pagedown", 0xD1},
            {"insert", 0xD2},      {"delete", 0xD3},      {"leftwin", 0xDB},
            {"rightwin", 0xDC}};

        std::unordered_map<std::string, uint32_t> mouseKeys = {
            {"leftbutton", 0x00}, {"rightbutton", 0x01}, {"middlebutton", 0x02}, {"button3", 0x03},
            {"button4", 0x04},    {"button5", 0x05},     {"button6", 0x06},      {"button7", 0x07},
            {"wheelup", 0x08},    {"wheeldown", 0x09}};

        std::unordered_map<std::string, uint32_t> gamepadKeys = {{"up", 0x0001},
                                                                 {"down", 0x0002},
                                                                 {"left", 0x0004},
                                                                 {"right", 0x0008},
                                                                 {"start", 0x0010},
                                                                 {"back", 0x0020},
                                                                 {"leftthumb", 0x0040},
                                                                 {"rightthumb", 0x0080},
                                                                 {"leftshoulder", 0x0100},
                                                                 {"rightshoulder", 0x0200},
                                                                 {"a", 0x1000},
                                                                 {"b", 0x2000},
                                                                 {"x", 0x4000},
                                                                 {"y", 0x8000},
                                                                 {"lefttrigger", 0x0009},
                                                                 {"righttrigger", 0x000A}};

        std::string lower = toLower(keyStr);

        switch (device) {
            case RE::INPUT_DEVICE::kMouse:
                if (auto it = mouseKeys.find(lower); it != mouseKeys.end()) return it->second;
                break;
            case RE::INPUT_DEVICE::kKeyboard:
                if (auto it = keyboardKeys.find(lower); it != keyboardKeys.end()) return it->second;
                break;
            case RE::INPUT_DEVICE::kGamepad:
                if (auto it = gamepadKeys.find(lower); it != gamepadKeys.end()) return it->second;
                break;
            default:
                break;
        }
        return std::nullopt;
    }

    bool IsPluginLoaded(const std::string& pluginName) {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return false;
        }

        auto* mod = dataHandler->LookupModByName(pluginName);
        return mod;
    }

    void LoadKeyConfig(const std::string& filePath) {
        OMO_action_bindings.clear();

        std::ifstream file(filePath);
        if (!file.is_open()) {
            logger::warn("Could not open key configuration file: {}", filePath);
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Ignore comments and empty lines
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string action, deviceStr, keyStr;
            if (!(std::getline(iss, action, ',') && std::getline(iss, deviceStr, ',') &&
                  std::getline(iss, keyStr, ','))) {
                continue;  // malformed line
            }

            // Trim spaces
            auto trim = [](std::string& s) {
                s.erase(s.begin(),
                        std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
                s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                        s.end());
            };
            trim(action);
            trim(deviceStr);
            trim(keyStr);

            logger::info("Binding action '{}' to {} key '{}'", action, deviceStr, keyStr);

            RE::INPUT_DEVICE device;
            if (deviceStr == "Mouse") {
                device = RE::INPUT_DEVICE::kMouse;
            } else if (deviceStr == "Keyboard") {
                device = RE::INPUT_DEVICE::kKeyboard;
            } else if (deviceStr == "Gamepad") {
                device = RE::INPUT_DEVICE::kGamepad;
            } else {
                logger::warn("Unknown device: {}", deviceStr);
                continue;
            }

            // Convert key string to ButtonID
            std::optional<SkyPromptAPI::ButtonID> key = KeyNameToButtonID(device, keyStr);
            if (!key) {
                logger::warn("Unknown key: {}", keyStr);
                continue;
            }

            OMO_action_bindings[action].emplace_back(device, *key);
        }
    }
}