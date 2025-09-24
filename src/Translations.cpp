#include "Translations.h"

#include "Utils.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

std::string GetGameLanguage() {
    if (const RE::Setting* languageSetting = RE::GetINISetting("sLanguage:General")) {
        std::string lang = languageSetting->GetString();
        std::ranges::transform(lang, lang.begin(), ::toupper);
        if (const auto it = Translations::languageMap.find(lang); it != Translations::languageMap.end()) {
            return it->first;
        }
        logger::warn("Detected language is not supported: {}", lang);
    } else {
        logger::error("Failed to get sLanguage setting.");
    }
    return "ENGLISH";
}

std::string Translations::GetValidLanguage() {
    std::string detectedLang = GetGameLanguage();
    if (const auto it = languageMap.find(detectedLang); it != languageMap.end()) {
        return it->first;
    }
    logger::warn("Detected language is not supported: {}", detectedLang);

    return "ENGLISH";
}

namespace {
    void LoadTranslationSection(const rapidjson::Value& sectionObj,
                                const std::map<std::string, std::string*>& section) {
        for (const auto& [key, value] : section) {
            if (sectionObj.HasMember(key.c_str())) {
                *value = sectionObj[key.c_str()].GetString();
            } else {
                logger::warn("Translation file does not contain key {}", key);
            }
        }
    }
};

bool Translations::LoadTranslations(const std::string& lang) {
    // check if mod_name_LANG.json exists
    const std::filesystem::path path = std::filesystem::path(translations_folder) / ("In-Game_Patcher_" + lang + ".json");

    if (!exists(path)) {
        if (lang != "ENGLISH") {
            logger::warn("Translation file {} does not exist.", path.string());
        }
        return false;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        logger::error("Failed to open translation file {}", path.string());
        return false;
    }

    rapidjson::IStreamWrapper isw(file);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        logger::error("Failed to parse translation file {}", path.string());
        return false;
    }

    for (const auto& [section_name, section] : requiredTranslations) {
        if (doc.HasMember(section_name.c_str())) {
            const auto& sectionObj = doc[section_name.c_str()];
            LoadTranslationSection(sectionObj, section);
        } else {
            logger::warn("Translation file does not contain section {}", section_name);
        }
    }

    return true;
}