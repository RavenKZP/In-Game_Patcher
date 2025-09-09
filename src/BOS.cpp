#include "BOS.h"
#include "Utils.h"

#include <fstream>
#include <unordered_map>
#include <string>

bool BOSIniManager::Save() const {
    std::ifstream in(filePath);
    std::vector<std::string> lines;
    if (in.is_open()) {
        std::string l;
        while (std::getline(in, l)) {
            lines.push_back(l);
        }
        in.close();
    }

    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    bool inReferences = false;
    bool inTransforms = false;
    bool refsWritten = false;
    bool transWritten = false;

    for (size_t i = 0; i < lines.size(); i++) {
        std::string line = lines[i];
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

        if (trimmed == "[References]") {
            inReferences = true;
            inTransforms = false;
            out << line << "\n";

            // Write our references immediately after section header
            for (auto& [id, r] : newReferences) {
                out << r.origRefID << "|" << r.swapBaseID << "|" << r.propertyOverrides << "\n";
            }
            refsWritten = true;
            continue;
        }

        if (trimmed == "[Transforms]") {
            inReferences = false;
            inTransforms = true;
            out << line << "\n";

            // Write our transforms immediately after section header
            for (auto& [id, t] : newTransforms) {
                out << t.origRefID << "|" << t.propertyOverrides << "\n";
            }
            transWritten = true;
            continue;
        }

        // Skip duplicates (if this line corresponds to an overridden ref/transform)
        if (inReferences) {
            // check if line matches any newReferences origRefID
            auto pos = line.find('|');
            if (pos != std::string::npos) {
                std::string refID = line.substr(0, pos);
                if (newReferences.find(refID) != newReferences.end()) {
                    continue;  // skip old line
                }
            }
        }
        if (inTransforms) {
            auto pos = line.find('|');
            if (pos != std::string::npos) {
                std::string refID = line.substr(0, pos);
                if (newTransforms.find(refID) != newTransforms.end()) {
                    continue;  // skip old line
                }
            }
        }

        out << line << "\n";
    }

    // If sections didn’t exist, create them
    if (!refsWritten && !newReferences.empty()) {
        out << "\n[References]\n";
        for (auto& [id, r] : newReferences) {
            out << r.origRefID << "|" << r.swapBaseID << "|" << r.propertyOverrides << "\n";
        }
    }
    if (!transWritten && !newTransforms.empty()) {
        out << "\n[Transforms]\n";
        for (auto& [id, t] : newTransforms) {
            out << t.origRefID << "|" << t.propertyOverrides << "\n";
        }
    }

    return true;
}

void BOSIniManager::RemoveFromFile(const std::string& id, bool isReference) const {
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

    bool inSection = false;
    const std::string sectionHeader = isReference ? "[References]" : "[Transforms]";

    for (auto& line : lines) {
        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
        trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

        if (trimmed == sectionHeader) {
            inSection = true;
            out << line << "\n";
            continue;
        }

        if (trimmed.size() > 2 && trimmed.front() == '[' && trimmed.back() == ']') {
            // Leaving the section
            inSection = false;
            out << line << "\n";
            continue;
        }

        if (inSection) {
            auto pos = line.find('|');
            if (pos != std::string::npos) {
                std::string refID = line.substr(0, pos);
                if (refID == id) {
                    continue;  // Skip this line -> effectively removes it
                }
            }
        }

        out << line << "\n";
    }
}

bool BOSIniManager::SetFile(std::string newFilePatch) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        logger::error("File {} not found!", filePath);
        return false;
    }
    filePath = newFilePatch;
    newReferences.clear();
    newTransforms.clear();
    return true;
}


void BOSIniManager::AddReference(const BOSReference& ref) {
    newReferences[ref.origRefID] = ref;
    Save();
}
void BOSIniManager::AddTransform(const BOSTransform& tr) {
    newTransforms[tr.origRefID] = tr;
    Save();
}

void BOSIniManager::RemoveReference(RE::TESForm* ref) {
    if (!ref) return;
    std::string id = Utils::NormalizeFormID(ref);
    RemoveFromFile(id, true);
    newReferences.erase(id);
}

void BOSIniManager::RemoveTransform(RE::TESForm* ref) {
    if (!ref) return;
    std::string id = Utils::NormalizeFormID(ref);
    RemoveFromFile(id, false);
    newTransforms.erase(id);
}

 
void BOSIniManager::RemoveObject(RE::TESObjectREFR* ref) {
     BOSReference referenceToDisable;
     referenceToDisable.origRefID = Utils::NormalizeFormID(ref);
     referenceToDisable.swapBaseID = "0x3B~Skyrim.esm";
     std::string PositionX = std::to_string(ref->GetPositionX());
     std::string PositionY = std::to_string(ref->GetPositionY());
     std::string PositionZ = "-30000.0";
     referenceToDisable.propertyOverrides = "posA(" + PositionX + "," + PositionY + "," + PositionZ + ")";
     RemoveTransform(ref);
     AddReference(referenceToDisable);
     ref->Disable();
     logger::info("Removed object: {}", ref->GetName());
 }

 void BOSIniManager::TransformObject(RE::TESObjectREFR* ref) {
     BOSTransform newTransform;
     newTransform.origRefID = Utils::NormalizeFormID(ref);

     std::string PositionX = std::to_string(ref->GetPositionX());
     std::string PositionY = std::to_string(ref->GetPositionY());
     std::string PositionZ = std::to_string(ref->GetPositionZ());

     std::string RotationX = std::to_string(ref->GetAngleX());
     std::string RotationY = std::to_string(ref->GetAngleY());
     std::string RotationZ = std::to_string(ref->GetAngleZ());

     std::string Scale = std::to_string(ref->GetScale());

     auto originalIt = originalStates.find(ref->GetFormID());
     newTransform.propertyOverrides = "";
     bool posChanged = false;
     bool rotChanged = false;
     bool scaleChanged = false;
     if (originalIt->second.pos.x != ref->GetPositionX() ||
         originalIt->second.pos.y != ref->GetPositionY() ||
         originalIt->second.pos.z != ref->GetPositionZ()) {
         posChanged = true;
         newTransform.propertyOverrides += "posA(" + PositionX + "," + PositionY + "," + PositionZ + ")";
     }
     if (originalIt->second.rot.x != ref->GetAngleX() ||
         originalIt->second.rot.y != ref->GetAngleY() ||
         originalIt->second.rot.z != ref->GetAngleZ()) {
         rotChanged = true;
         if (posChanged) {
             newTransform.propertyOverrides += ",";
         }
         newTransform.propertyOverrides += "rotA(" + RotationX + "," + RotationY + "," + RotationZ + ")";
     }
     if (originalIt->second.scale != ref->GetScale()) {
         scaleChanged = true;
         if (posChanged || rotChanged) {
             newTransform.propertyOverrides += ",";
         }
         newTransform.propertyOverrides += ",scale(" + Scale + ")";
     }
     
     AddTransform(newTransform);
     logger::info("Saved object to BOS: {}", ref->GetName());
 }

 void BOSIniManager::RememberOriginal(RE::TESObjectREFR* ref) {
     if (!ref) return;

     RE::FormID id = ref->GetFormID();
     if (originalStates.contains(id)) {
         return;  // already saved
     }

     BOSOriginalData data;
     data.formID = id;
     data.pos = ref->GetPosition();
     data.rot = ref->GetAngle();
     data.scale = ref->GetScale();

     originalStates[id] = data;
 }

 void BOSIniManager::ResetObject(RE::TESObjectREFR* ref) {
     if (!ref) return;

     RE::FormID id = ref->GetFormID();
     auto it = originalStates.find(id);
     if (it == originalStates.end()) {
         logger::warn("No original data stored for {}", id);
     } else {
         const auto& orig = it->second;
         ref->SetPosition(orig.pos.x, orig.pos.y, orig.pos.z);
         ref->data.angle = orig.rot;
         ref->GetReferenceRuntimeData().refScale = static_cast<std::uint16_t>(orig.scale * 100.0f);
     }

     if (ref->IsDisabled()) {
         ref->Enable(false);
     }
     RemoveReference(ref);
     RemoveTransform(ref);

     logger::info("Reset object {} to original transform", ref->GetName());
 }