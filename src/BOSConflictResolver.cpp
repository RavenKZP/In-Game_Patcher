#include "BOSConflictResolver.h"
#include "Utils.h"

void BOSConflictResolver::Scan(const std::string& dataPath) {
    conflicts.clear();

    for (const auto& entry : std::filesystem::directory_iterator(dataPath)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            if (path.extension() == ".ini" && path.filename().string().find("_SWAP") != std::string::npos) {
                ProcessFile(path);
            }
        }
    }
}

void BOSConflictResolver::ProcessFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    std::string currentSection;
    int lineNum = 0;

    while (std::getline(file, line)) {
        ++lineNum;

        // Strip whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        // Section header
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        // Normal entry
        ParseLine(currentSection, line, path.filename().string(), lineNum);
    }
}

void BOSConflictResolver::ParseLine(const std::string& section, const std::string& line, const std::string& file,
                                    int lineNum) {
    auto sep = line.find('|');
    std::string lhs, rhs;

    if (sep != std::string::npos) {
        lhs = line.substr(0, sep);
        rhs = line.substr(sep + 1);
    }

    Entry e{lhs, rhs, "Data\\" + file, lineNum};

    auto& vec = conflicts[section][lhs];
    vec.push_back(e);
}

bool BOSConflictResolver::CommentOut(const Entry& entry) {
    std::ifstream in(entry.file);
    if (!in.is_open()) return false;

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    in.close();

    if (entry.line - 1 < 0 || entry.line - 1 >= (int)lines.size()) return false;

    // Prepend `;`
    if (!lines[entry.line - 1].empty() && lines[entry.line - 1][0] != ';') {
        lines[entry.line - 1] = ";" + lines[entry.line - 1];
    }

    std::ofstream out(entry.file, std::ios::trunc);
    if (!out.is_open()) return false;
    for (auto& l : lines) {
        out << l << "\n";
    }
    return true;
}

void BOSConflictResolver::Inspect(const Entry& entry) {

    auto form = Utils::GetFormFromString(entry.lhs);

    if (auto refr = form ? form->As<RE::TESObjectREFR>() : nullptr) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            player->MoveTo(refr);
        }
    }
}

void BOSConflictResolver::Apply(const Entry& entry) {

    auto form = Utils::GetFormFromString(entry.lhs);
    if (!form) return;
    std::string overrides;
    if (entry.rhs.find('|') != std::string::npos) {
        // Reference override
        auto sep = entry.rhs.find('|');
        overrides = entry.rhs.substr(sep + 1);
    } else {
        // Transform only
        overrides = entry.rhs;
    }
    auto data = Utils::ParseOverrides(overrides);
    if (auto refr = form->As<RE::TESObjectREFR>()) {
        if (data.hasPos)
            refr->SetPosition(data.pos[0], data.pos[1], data.pos[2]);
        if (data.hasRot) {
            data.rot[0] = RE::deg_to_rad(data.rot[0]);
            data.rot[1] = RE::deg_to_rad(data.rot[1]);
            data.rot[2] = RE::deg_to_rad(data.rot[2]);
            refr->data.angle = RE::NiPoint3(data.rot[0], data.rot[1], data.rot[2]);
        }
        if (data.hasScale)
            refr->GetReferenceRuntimeData().refScale = static_cast<std::uint16_t>(data.scale * 100.0f);
    }
}
