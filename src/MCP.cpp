#include "MCP.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "Events.h"
#include "Utils.h"
#include "BOS.h"
#include "KID.h"

namespace MCP {

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework is not installed. Cannot register menu.");
            return;
        }
        SKSEMenuFramework::SetSection("In-Game Patcher");
        SKSEMenuFramework::AddSectionItem("Menu Patcher", RenderSettings);
        SKSEMenuFramework::AddSectionItem("BOS File", RenderBOSFile);
        SKSEMenuFramework::AddSectionItem("KID File", RenderKIDFile);
        SKSEMenuFramework::AddSectionItem("Log", RenderLog);

        logger::info("SKSE Menu Framework registered.");
    }

    void __stdcall RenderSettings() {
        ImGui::SeparatorText("General Options");
        if (ImGui::CollapsingHeader("General Options Help")) {
            ImGui::BulletText("Show Editor Markers: toggles visibility of markers (idle, furniture, spawns, etc).");
            ImGui::BulletText("     After changing, reload cells (fast travel recommended).");
            ImGui::BulletText("Patching Mode: enables object interaction via SkyPrompt.");
            ImGui::BulletText("     To patch: look directly at an object or select it in Console.");
            ImGui::BulletText("     Console selection has priority over look-at.");
        }
        ImGui::Separator();

        ImGui::Checkbox("Patching Mode", &PatchingMode);

        REL::Relocation<std::uint8_t*> bShowMarkers{REL::ID(381022)};
        REL::Relocation<std::uint8_t*> bLoadMarkers{REL::ID(381019)};

        bool showMarkers = *bShowMarkers;
        if (ImGui::Checkbox("Show Editor Markers", &showMarkers)) {
            *bLoadMarkers = showMarkers;
            *bShowMarkers = showMarkers;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Requires cell reload to take effect.");
        }

        RE::TESObjectREFR* ref = RE::Console::GetSelectedRef().get();
        if (ref) {
            if (ref->As<RE::Actor>()) {
                ImGui::Text("Selected Ref: %08X - Actor {} (SPID unsupported)", ref->GetFormID(),
                            ref->GetName());
            } else {
                ImGui::Text("Selected Ref: %08X", ref->GetFormID());
            }
            auto base = ref->GetBaseObject();
            if (base) {
                bool isKIDSupported = base->Is(RE::FormType::Weapon) || base->Is(RE::FormType::Armor) ||
                                      base->Is(RE::FormType::Ammo) || base->Is(RE::FormType::MagicEffect) ||
                                      base->Is(RE::FormType::AlchemyItem) || base->Is(RE::FormType::Scroll) ||
                                      base->Is(RE::FormType::Location) || base->Is(RE::FormType::Ingredient) ||
                                      base->Is(RE::FormType::Book) || base->Is(RE::FormType::Misc) ||
                                      base->Is(RE::FormType::KeyMaster) || base->Is(RE::FormType::SoulGem) ||
                                      base->Is(RE::FormType::Spell) || base->Is(RE::FormType::Activator) ||
                                      base->Is(RE::FormType::Flora) || base->Is(RE::FormType::Furniture) ||
                                      base->Is(RE::FormType::Race) || base->Is(RE::FormType::TalkingActivator) ||
                                      base->Is(RE::FormType::Enchantment);

                bool isIndirectKIDSupported = false;
                if (base->Is(RE::FormType::ActorCharacter) || base->Is(RE::FormType::Weapon) ||
                    base->Is(RE::FormType::Armor) || base->Is(RE::FormType::AlchemyItem) ||
                    base->Is(RE::FormType::Scroll) || base->Is(RE::FormType::Book)) {
                    isIndirectKIDSupported = true;  // can list Race, Spells, MagicEffect, Location
                }
                isIndirectKIDSupported = false;  // disabling for now

                if (isKIDSupported || isIndirectKIDSupported) {
                    if (ImGui::CollapsingHeader("KID Support", ImGuiTreeNodeFlags_DefaultOpen)) {
                        if (ImGui::CollapsingHeader("KID Support Help")) {
                            ImGui::Text("Keywords can be patched onto this object's base form.");
                            ImGui::Text("These edits are saved into the KID patch file whe Add Keyword is pressed.");
                        }

                        static char customKeywordBuf[128]{};
                        static int selectedKeyword = -1;
                        static char keywordFilter[128]{};

                        // Filter box
                        ImGui::InputTextWithHint("Filter", "Type to filter keywords...", keywordFilter,
                                                 sizeof(keywordFilter));

                        // Collect all game keywords once
                        static std::vector<RE::BGSKeyword*> allKeywords;
                        if (allKeywords.empty()) {
                            auto dataHandler = RE::TESDataHandler::GetSingleton();
                            if (dataHandler) {
                                for (auto* kw : dataHandler->GetFormArray<RE::BGSKeyword>()) {
                                    allKeywords.push_back(kw);
                                }
                            }
                        }

                        // Dropdown with filter
                        if (ImGui::BeginCombo("Game Keywords",
                                              (selectedKeyword >= 0 && selectedKeyword < (int)allKeywords.size())
                                                  ? allKeywords[selectedKeyword]->GetFormEditorID()
                                                  : "None")) {
                            for (int i = 0; i < (int)allKeywords.size(); i++) {
                                auto* kw = allKeywords[i];
                                if (base->HasKeywordByEditorID(kw->GetFormEditorID())) {
                                    continue;  // already has it
                                }
                                const char* name = kw->GetFormEditorID();
                                if (!name) name = "<no editorID>";

                                if (keywordFilter[0] != '\0' && !std::strstr(name, keywordFilter)) {
                                    continue;  // filter out
                                }

                                bool isSelected = (i == selectedKeyword);
                                if (ImGui::Selectable(name, isSelected)) {
                                    selectedKeyword = i;
                                }
                                if (isSelected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }

                        // Custom keyword text field
                        ImGui::InputText("Custom Keyword", customKeywordBuf, sizeof(customKeywordBuf));

                        // Add button
                        if (isKIDSupported) {
                            if (ImGui::Button("Add Keyword to Base Object")) {
                                if (selectedKeyword >= 0) {
                                    auto* kw = allKeywords[selectedKeyword];
                                    if (kw) {
                                        auto type = Utils::GetObjectTypeName(base);
                                        base->As<RE::BGSKeywordForm>()->AddKeyword(kw);
                                        KIDIniManager::GetSingleton()->AddEntry(
                                            {kw->GetFormEditorID(), type, Utils::NormalizeFormID(base)});
                                    }
                                } else if (customKeywordBuf[0] != '\0') {
                                    auto type = Utils::GetObjectTypeName(base);
                                    auto kwd = Utils::FindOrCreateKeyword(customKeywordBuf);
                                    base->As<RE::BGSKeywordForm>()->AddKeyword(kwd);
                                    customKeywordBuf[0] = '\0';
                                    KIDIniManager::GetSingleton()->AddEntry(
                                        {kwd->GetFormEditorID(), type, Utils::NormalizeFormID(base)});
                                }
                            }
                        }
                        if (isIndirectKIDSupported) {
                            // If actor selectable Race, Spells, MagicEffects, Location
                            // If weapon/armor selectable enchantments
                            auto indirectTargets = Utils::GetIndirectKIDTargets(ref);
                            if (!indirectTargets.empty()) {
                                if (ImGui::CollapsingHeader("Indirect KID Targets")) {
                                    for (auto* target : indirectTargets) {
                                        const char* name = target ? target->GetName() : "<invalid>";
                                        ImGui::Text("%s", name ? name : "<unnamed>");
                                        ImGui::SameLine();
                                        if (ImGui::Button("Add Keyword")) {
                                            if (selectedKeyword >= 0) {
                                                auto* kw = allKeywords[selectedKeyword];
                                                if (kw) {
                                                    auto type = Utils::GetObjectTypeName(target->As<RE::TESBoundObject>());
                                                    target->As<RE::BGSKeywordForm>()->AddKeyword(kw);
                                                    KIDIniManager::GetSingleton()->AddEntry(
                                                        {kw->GetFormEditorID(), type, Utils::NormalizeFormID(target)});
                                                }
                                            } else if (customKeywordBuf[0] != '\0') {
                                                auto type = Utils::GetObjectTypeName(target->As<RE::TESBoundObject>());
                                                auto kwd = Utils::FindOrCreateKeyword(customKeywordBuf);
                                                target->As<RE::BGSKeywordForm>()->AddKeyword(kwd);
                                                customKeywordBuf[0] = '\0';
                                                KIDIniManager::GetSingleton()->AddEntry(
                                                    {kwd->GetFormEditorID(), type, Utils::NormalizeFormID(target)});
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        ImGui::BulletText("Add Keyword to Base Object: attaches a new keyword to this base object.");
                        ImGui::BulletText(
                            "   Keywords are applied to the *base object*, so all references of this type inherit "
                            "them.");
                        //ImGui::BulletText("Add Keyword: attaches a new keyword to the selected object.");
                    }
                }
            }
            if (Utils::IsDynamicForm(ref)) {
                ImGui::Text("Selected Ref: %08X (Dynamic - BOS ignored)", ref->GetFormID());
            } else {
                if (ImGui::CollapsingHeader("BOS Support", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::CollapsingHeader("BOS Support Help")) {
                        ImGui::BulletText("Change object Position, Rotation, Scale or Disable it.");
                        ImGui::BulletText(
                            "These edits are saved into the BOS patch file when Save Transform is pressed.");
                        ImGui::BulletText("Tip: You can resize this window for easier editing.");
                    }

                    auto BosMgr = BOSIniManager::GetSingleton();

                    BosMgr->RememberOriginal(ref);

                    bool changed = false;
                    static bool unsavedChanges = false;

                    // Read current values
                    RE::NiPoint3 pos = ref->GetPosition();
                    RE::NiPoint3 rot = ref->GetAngle();
                    float scale = std::round(ref->GetScale() * 1000.0f) / 1000.0f;
                    

                    RE::NiPoint3 originalPos = {0, 0, 0};
                    RE::NiPoint3 originalRot = {0, 0, 0};
                    float originalScale = 0.0f;

                    auto original = BosMgr->originalStates.find(ref->GetFormID());

                    if (original != BosMgr->originalStates.end()) {
                        originalPos = original->second.pos;
                        originalRot = original->second.rot;
                        originalScale = std::round(original->second.scale * 1000.0f) / 1000.0f;
                    }
                    ImVec4 green = ImVec4(0.3f, 0.6f, 0.3f, 0.5f);
                    ImVec4 yellow = ImVec4(0.8f, 0.8f, 0.3f, 0.5f);
                    ImVec4 color;

                    static float posStepValue = 10.0f;
                    ImGui::Text("Position | Step: ");
                    ImGui::SameLine();
                    ImGui::InputFloat("", &posStepValue, 1.0f, 10.0f, "%.3f");

                    color = (std::abs(pos.x - originalPos.x) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("X##pos", &pos.x, posStepValue, posStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(pos.y - originalPos.y) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Y##pos", &pos.y, posStepValue, posStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(pos.z - originalPos.z) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Z##pos", &pos.z, posStepValue, posStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();

                    static float rotStepValue = 0.1f;
                    ImGui::Text("Rotation | Step: ");
                    ImGui::SameLine();
                    ImGui::InputFloat("", &rotStepValue, 0.1f, 1.0f, "%.3f");

                    color = (std::abs(rot.x - originalRot.x) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("X##rot", &rot.x, rotStepValue, rotStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(rot.y - originalRot.y) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Y##rot", &rot.y, rotStepValue, rotStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(rot.z - originalRot.z) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Z##rot", &rot.z, rotStepValue, rotStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();

                    static float scaleStepValue = 0.1f;
                    ImGui::Text("Scale | Step: ");
                    ImGui::SameLine();
                    ImGui::InputFloat("", &scaleStepValue, 0.1f, 1.0f, "%.3f");

                    color = (std::abs(scale - originalScale) < 0.001f) ? green : yellow;
                    ImGui::Text("Current Scale: %.3f", scale);
                    ImGui::Text("Original Scale: %.3f", originalScale);
                    ImGui::Text("Difference: %.5f", scale - originalScale);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("##scale", &scale, scaleStepValue, scaleStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();

                    if (changed) {
                        unsavedChanges = true;
                        ref->SetPosition(pos);
                        ref->data.angle = rot;
                        ref->GetReferenceRuntimeData().refScale = static_cast<std::uint16_t>(scale * 100.0f);
                    }

                    ImGui::Separator();

                    // Action buttons

                    if (unsavedChanges) {
                        ImGui::TextColored(yellow, "Unsaved Changes");
                    }
                    if (ImGui::Button("Save Transform")) {
                        unsavedChanges = false;
                        BosMgr->TransformObject(ref);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Remove Object")) {
                        unsavedChanges = false;
                        BosMgr->RemoveObject(ref);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Transform")) {
                        unsavedChanges = false;
                        BosMgr->ResetObject(ref);
                    }

                    ImGui::BulletText(
                        "Save Transform: save current object position and rotation into the BOS file.");
                    ImGui::BulletText("Remove Object: hides this reference at runtime and marks it in BOS.");
                    ImGui::BulletText(
                        "Reset Transform: restores the object to its original state, remove BOS edits.");
                }
            }
        } else {
            ImGui::Text("No Ref Selected");
        }
    }

    void __stdcall RenderBOSFile() {
        ImGui::SeparatorText("BOS Runtime Editor");
        if (ImGui::CollapsingHeader("BOS Runtime Editor Help")) {
            ImGui::BulletText("Only this session-added changes are shown here.");
            ImGui::BulletText("Saving will append new entries into your BOS-SWAP file.");
            ImGui::BulletText("Existing data in the file will remain unchanged.");
            ImGui::BulletText("This tool does not overwrite or remove original entries.");
            ImGui::BulletText("Always keep a backup of your BOS-SWAP files before editing.");
        }
        ImGui::Separator();

        auto bosMgr = BOSIniManager::GetSingleton();

        std::string currentFile = bosMgr->GetFile();
        if (currentFile.starts_with("Data//") || currentFile.starts_with("Data\\")) {
            currentFile = currentFile.substr(5);
        }
        ImGui::Text("Current BOS File: %s", currentFile.c_str());

        // Collect all *_SWAP.ini files in Data folder (once)
        static std::vector<std::string> swapFiles;
        static bool filesScanned = false;

        if (!filesScanned) {
            filesScanned = true;
            swapFiles.clear();

            for (const auto& entry : std::filesystem::directory_iterator("Data")) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    auto name = path.filename().string();
                    if (name.size() > 9 && name.ends_with("_SWAP.ini")) {
                        swapFiles.push_back(name);
                    }
                }
            }
            std::sort(swapFiles.begin(), swapFiles.end());
        }

        // Dropdown to choose file
        static int selectedFile = -1;
        for (size_t i = 0; i < swapFiles.size(); i++) {
            if (swapFiles[i] == currentFile) {
                selectedFile = static_cast<int>(i);
                break;
            }
        }

        if (ImGui::BeginCombo("Select BOS File", selectedFile >= 0 ? swapFiles[selectedFile].c_str() : "<None>")) {
            for (int i = 0; i < (int)swapFiles.size(); i++) {
                bool isSelected = (i == selectedFile);
                if (ImGui::Selectable(swapFiles[i].c_str(), isSelected)) {
                    selectedFile = i;
                    bosMgr->SetFile("Data//" + swapFiles[i]);  // update BOS file
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        static bool creatingNewFile = false;
        static char newFileBuf[128]{};

        if (!creatingNewFile) {
            if (ImGui::Button("New File")) {
                creatingNewFile = true;
                newFileBuf[0] = '\0';
            }
        } else {
            ImGui::Text("Enter new file name (without _SWAP.ini):");
            ImGui::InputText("New File Name", newFileBuf, sizeof(newFileBuf));
            ImGui::SameLine();
            if (ImGui::Button("Create")) {
                if (std::strlen(newFileBuf) > 0) {
                    std::string newFileName = std::string(newFileBuf) + "_SWAP.ini";
                    std::string fullPath = "Data//" + newFileName;

                    if (Utils::CreateNewBOSFile(fullPath)) {
                        bosMgr->SetFile(fullPath);
                        swapFiles.push_back(newFileName);
                        std::sort(swapFiles.begin(), swapFiles.end());
                    }
                    creatingNewFile = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                creatingNewFile = false;
            }
        }

        if (ImGui::CollapsingHeader("Removed Objects")) {
            int idx = 0;
            for (auto it = bosMgr->newReferences.begin(); it != bosMgr->newReferences.end();) {
                ImGui::PushID(idx);

                ImGui::Text("RefID: %s", it->second.origRefID.c_str());
                auto ref = Utils::GetFormFromString(it->second.origRefID.c_str());
                if (ref) {
                    ImGui::Text("Object: %s", ref->GetFormEditorID());
                }

                OverridesData overrides = Utils::ParseOverrides(it->second.propertyOverrides);

                if (ImGui::Button("Revert")) {
                    if (ref) {
                        bosMgr->ResetObject(ref->AsReference());
                    }
                    ImGui::PopID();
                    break; 
                }

                ImGui::Separator();
                ImGui::PopID();
                ++idx;
                ++it;
            }
            ImGui::BulletText("Revert: re-enable object, restores the object to its original state, remove BOS edits.");
        }

        if (ImGui::CollapsingHeader("Transformed Objects")) {
            int idx = 0;
            for (auto it = bosMgr->newTransforms.begin(); it != bosMgr->newTransforms.end();) {
                ImGui::PushID(idx);

                ImGui::Text("RefID: %s", it->second.origRefID.c_str());
                auto ref = Utils::GetFormFromString(it->second.origRefID.c_str())->AsReference();
                if (ref) {
                    ImGui::Text("Object: %s", ref->GetFormEditorID());
                }

                OverridesData overrides = Utils::ParseOverrides(it->second.propertyOverrides);
                bool changed = false;

                if (ImGui::CollapsingHeader("Transform")) {
                    RE::NiPoint3 originalPos = {0, 0, 0};
                    RE::NiPoint3 originalRot = {0, 0, 0};
                    float originalScale = 0;

                    auto original = bosMgr->originalStates.find(ref->GetFormID());

                    if (original != bosMgr->originalStates.end()) {
                        originalPos = original->second.pos;
                        originalRot = original->second.rot;
                        originalScale = std::round(original->second.scale * 1000.0f) / 1000.0f;
                    }

                    ImVec4 green = ImVec4(0.3f, 0.6f, 0.3f, 0.5f);
                    ImVec4 yellow = ImVec4(0.8f, 0.8f, 0.3f, 0.5f);
                    ImVec4 color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

                    // --Position ---
                    if (!overrides.hasPos) {
                        overrides.pos[0] = originalPos.x;
                        overrides.pos[1] = originalPos.y;
                        overrides.pos[2] = originalPos.z;
                    }
                    static float posStepValue = 10.0f;
                    ImGui::Text("Position | Step:");
                    ImGui::SameLine();
                    ImGui::InputFloat("", &posStepValue, 1.0f, 10.0f, "%.3f");

                    color = (std::abs(overrides.pos[0] - originalPos.x) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("X##pos", &overrides.pos[0], posStepValue, posStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(overrides.pos[1] - originalPos.y) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Y##pos", &overrides.pos[1], posStepValue, posStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(overrides.pos[2] - originalPos.z) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Z##pos", &overrides.pos[2], posStepValue, posStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();

                    // --Rotation ---
                    if (!overrides.hasRot) {
                        overrides.rot[0] = originalRot.x;
                        overrides.rot[1] = originalRot.y;
                        overrides.rot[2] = originalRot.z;
                    }
                    static float rotStepValue = 0.1f;
                    ImGui::Text("Rotation | Step:");
                    ImGui::SameLine();
                    ImGui::InputFloat("", &rotStepValue, 0.1f, 1.0f, "%.3f");
                    color = (std::abs(overrides.rot[0] - originalRot.x) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("X##rot", &overrides.rot[0], rotStepValue, rotStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(overrides.rot[1] - originalRot.y) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Y##rot", &overrides.rot[1], rotStepValue, rotStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                    color = (std::abs(overrides.rot[2] - originalRot.z) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("Z##rot", &overrides.rot[2], rotStepValue, rotStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();

                    // --Scale ---
                    if (!overrides.hasScale) {
                        overrides.scale = originalScale;
                    }
                    static float scaleStepValue = 0.1f;
                    ImGui::Text("Scale | Step:");
                    ImGui::SameLine();
                    ImGui::InputFloat("", &scaleStepValue, 0.1f, 1.0f, "%.3f");
                    color = (std::abs(overrides.scale - originalScale) < 0.001f) ? green : yellow;
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, color);
                    if (ImGui::InputFloat("##scale", &overrides.scale, scaleStepValue, scaleStepValue * 10)) {
                        changed = true;
                    }
                    ImGui::PopStyleColor();
                }

                if (changed) {
                    if (ref) {
                        ref->SetPosition(overrides.pos[0], overrides.pos[1], overrides.pos[2]);
                        ref->data.angle = RE::NiPoint3(overrides.rot[0], overrides.rot[1], overrides.rot[2]);
                        ref->GetReferenceRuntimeData().refScale = static_cast<std::uint16_t>(overrides.scale * 100.0f);
                    }
                    auto newOverrides = Utils::BuildOverrides(overrides);
                    BOSTransform newTr;
                    newTr.origRefID = it->second.origRefID;
                    newTr.propertyOverrides = newOverrides;
                    bosMgr->AddTransform(newTr);
                    bosMgr->Save();
                }

                if (ImGui::Button("Reset Transform")) {
                    bosMgr->ResetObject(ref);
                    ImGui::PopID();
                    break;
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Object")) {
                    bosMgr->ResetObject(ref);  // first reset to original position
                    bosMgr->RemoveObject(ref);
                    ImGui::PopID();
                    break;
                }

                ImGui::Separator();
                ImGui::PopID();
                ++idx;
                ++it;
            }
            ImGui::BulletText("Remove Object: hides this reference at runtime and marks it in BOS.");
            ImGui::BulletText("Reset Transform: restores the object to its original state, removing BOS edits.");
        }
    }

    void __stdcall RenderKIDFile() {
        ImGui::SeparatorText("KID Runtime Editor");
        if (ImGui::CollapsingHeader("KID Runtime Editor Help")) {
            ImGui::BulletText("Only this session-added keywords are shown here.");
            ImGui::BulletText("Saving will append new entries into your KID file.");
            ImGui::BulletText("Existing data in the file will remain unchanged.");
            ImGui::BulletText("This tool does not overwrite or remove original entries.");
            ImGui::BulletText("Always keep a backup of your KID files before editing.");
        }
        ImGui::Separator();

        auto kidMgr = KIDIniManager::GetSingleton();

        std::string currentFile = kidMgr->GetFile();
        if (currentFile.starts_with("Data//") || currentFile.starts_with("Data\\")) {
            currentFile = currentFile.substr(5);
        }
        ImGui::Text("Current KID File: %s", currentFile.c_str());

        // Collect all *_KID.ini files in Data folder (once)
        static std::vector<std::string> kidFiles;
        static bool filesScanned = false;

        if (!filesScanned) {
            filesScanned = true;
            kidFiles.clear();

            for (const auto& entry : std::filesystem::directory_iterator("Data")) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    auto name = path.filename().string();
                    if (name.size() > 8 && name.ends_with("_KID.ini")) {
                        kidFiles.push_back(name);
                    }
                }
            }
            std::sort(kidFiles.begin(), kidFiles.end());
        }

        // Dropdown to choose file
        static int selectedFile = -1;
        for (size_t i = 0; i < kidFiles.size(); i++) {
            if (kidFiles[i] == currentFile) {
                selectedFile = static_cast<int>(i);
                break;
            }
        }

        if (ImGui::BeginCombo("Select KID File", selectedFile >= 0 ? kidFiles[selectedFile].c_str() : "<None>")) {
            for (int i = 0; i < (int)kidFiles.size(); i++) {
                bool isSelected = (i == selectedFile);
                if (ImGui::Selectable(kidFiles[i].c_str(), isSelected)) {
                    selectedFile = i;
                    kidMgr->SetFile("Data//" + kidFiles[i]);  // update KID file
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // New file creation
        static bool creatingNewFile = false;
        static char newFileBuf[128]{};

        if (!creatingNewFile) {
            if (ImGui::Button("New File")) {
                creatingNewFile = true;
                newFileBuf[0] = '\0';
            }
        } else {
            ImGui::Text("Enter new file name (without _KID.ini):");
            ImGui::InputText("New File Name", newFileBuf, sizeof(newFileBuf));
            ImGui::SameLine();
            if (ImGui::Button("Create")) {
                if (std::strlen(newFileBuf) > 0) {
                    std::string newFileName = std::string(newFileBuf) + "_KID.ini";
                    std::string fullPath = "Data//" + newFileName;

                    if (Utils::CreateNewKIDFile(fullPath)) {
                        kidMgr->SetFile(fullPath);
                        kidFiles.push_back(newFileName);
                        std::sort(kidFiles.begin(), kidFiles.end());
                    }
                    creatingNewFile = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                creatingNewFile = false;
            }
        }

        // Entries list
        if (ImGui::CollapsingHeader("KID Entries")) {
            int idx = 0;
            auto entries = kidMgr->GetEntries();
            for (auto it = entries.begin(); it != entries.end();) {
                ImGui::PushID(idx);

                ImGui::Text("FormID: %s", it->objectID.c_str());
                ImGui::Text("Keyword: %s", it->keyword.c_str());

                if (ImGui::Button("Remove")) {
                    if (auto form = Utils::GetFormFromString(it->objectID)) {
                        if (auto base = form->As<RE::BGSKeywordForm>()) {
                            if (auto keyword = RE::TESForm::LookupByEditorID(it->keyword)) {
                                auto keywordform = keyword->As<RE::BGSKeyword>();
                                if (base->HasKeyword(keywordform)) {
                                    base->RemoveKeyword(keywordform);
                                }
                            }
                        }
                        kidMgr->RemoveEntry(form, it->keyword);
                    }
                    it = entries.erase(it);
                    ImGui::PopID();
                    break;
                }

                ImGui::Separator();
                ImGui::PopID();
                ++idx;
                ++it;
            }
            ImGui::BulletText("Remove: remove keyword from base object, remove KID edits.");
        }
    }
}

void __stdcall MCP::RenderLog() {
    ImGui::Checkbox("Trace", &MCPLog::log_trace);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &MCPLog::log_info);
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &MCPLog::log_warning);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &MCPLog::log_error);
    ImGui::InputText("Custom Filter", MCPLog::custom, 255);

    // if"Generate Log" button is pressed, read the log file
    if (ImGui::Button("Generate Log")) {
        logLines = MCPLog::ReadLogFile();
    }

    // Display each line in a new ImGui::Text() element
    for (const auto& line : logLines) {
        if (line.find("trace") != std::string::npos && !MCPLog::log_trace) continue;
        if (line.find("info") != std::string::npos && !MCPLog::log_info) continue;
        if (line.find("warning") != std::string::npos && !MCPLog::log_warning) continue;
        if (line.find("error") != std::string::npos && !MCPLog::log_error) continue;
        if (line.find(MCPLog::custom) == std::string::npos && MCPLog::custom != "") continue;
        ImGui::Text(line.c_str());
    }
}

namespace MCPLog {
    std::filesystem::path GetLogPath() {
        const auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
        auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
        return logFilePath;
    }

    std::vector<std::string> ReadLogFile() {
        std::vector<std::string> logLines;

        // Open the log file
        std::ifstream file(GetLogPath().c_str());
        if (!file.is_open()) {
            // Handle error
            return logLines;
        }

        // Read and store each line from the file
        std::string line;
        while (std::getline(file, line)) {
            logLines.push_back(line);
        }

        file.close();

        return logLines;
    }

}