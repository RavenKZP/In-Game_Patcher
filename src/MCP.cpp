#include "MCP.h"

#include "SKSEMCP/SKSEMenuFramework.hpp"
#include "Events.h"
#include "Utils.h"
#include "BOS.h"
#include "BOSConflictResolver.h"
#include "KID.h"
#include "Translations.h"


namespace TrStMCP = Translations::Strings::MCP;

namespace MCP {

    void Register() {
        if (!SKSEMenuFramework::IsInstalled()) {
            logger::warn("SKSE Menu Framework is not installed. Cannot register menu.");
            return;
        }
        SKSEMenuFramework::SetSection("In-Game Patcher");
        SKSEMenuFramework::AddSectionItem(TrStMCP::menu_title, RenderSettings);
        SKSEMenuFramework::AddSectionItem(TrStMCP::BOS_file, RenderBOSFile);
        SKSEMenuFramework::AddSectionItem(TrStMCP::BOS_resolver, RenderdBOSResolver);
        SKSEMenuFramework::AddSectionItem(TrStMCP::KID_file, RenderKIDFile);
        SKSEMenuFramework::AddSectionItem(TrStMCP::log, RenderLog);

        logger::info("SKSE Menu Framework registered.");
    }

    void __stdcall RenderSettings() {

        ImGui::SeparatorText(TrStMCP::general_options.c_str());
        std::string HeaderText = TrStMCP::general_options + " " + TrStMCP::help;
        if (ImGui::CollapsingHeader(HeaderText.c_str())) {
            ImGui::TextWrapped(TrStMCP::general_options_help_text.c_str());
        }
        ImGui::Separator();

        ImGui::Checkbox(TrStMCP::patching_mode.c_str(), &PatchingMode);
        
        RE::TESObjectREFR* ref = nullptr;
        
        // The following if-else is required because the address of GetSelectedRef function changes between AE
        // vesions
        if (Version.compare(REL::Version(1, 6, 1130)) == std::strong_ordering::less){
            ref = RE::Console::GetSelectedRef640().get();  // AE offset: 405935
        } else {
            ref = RE::Console::GetSelectedRef().get(); // AE offset: 504099
        }

        if (ref) {
            if (ref->As<RE::Actor>()) {
                std::string text = TrStMCP::selected_ref + ": %08X - " + TrStMCP::actor + " {} ";

                ImGui::Text(text.c_str(), ref->GetFormID(), ref->GetName());
            } else {
                std::string text = TrStMCP::selected_ref + ": %08X";
                ImGui::Text(text.c_str(), ref->GetFormID());
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
                    if (ImGui::CollapsingHeader(TrStMCP::KID_support.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                        HeaderText = TrStMCP::KID_support + " " + TrStMCP::help;
                        if (ImGui::CollapsingHeader(HeaderText.c_str())) {
                            ImGui::TextWrapped(TrStMCP::KID_support_help_text.c_str());
                        }

                        static char customKeywordBuf[128]{};
                        static int selectedKeyword = -1;
                        static char keywordFilter[128]{};

                        // Filter box
                        ImGui::InputTextWithHint(TrStMCP::filter.c_str(), TrStMCP::filter_text.c_str(), keywordFilter,
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
                        if (ImGui::BeginCombo(TrStMCP::game_keywords.c_str(),
                                              (selectedKeyword >= 0 && selectedKeyword < (int)allKeywords.size())
                                                  ? allKeywords[selectedKeyword]->GetFormEditorID()
                                                  : TrStMCP::none.c_str())) {
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
                        ImGui::InputText(TrStMCP::custom_keyword.c_str(), customKeywordBuf, sizeof(customKeywordBuf));

                        // Add button
                        if (isKIDSupported) {
                            if (ImGui::Button(TrStMCP::add_keyword.c_str())) {
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
                                if (ImGui::CollapsingHeader(TrStMCP::indirect_KID.c_str())) {
                                    for (auto* target : indirectTargets) {
                                        const char* name = target ? target->GetName() : "<invalid>";
                                        ImGui::Text("%s", name ? name : "<unnamed>");
                                        ImGui::SameLine();
                                        if (ImGui::Button(TrStMCP::add_keyword.c_str())) {
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
                    }
                }
            }
            if (Utils::IsDynamicForm(ref)) {
                std::string text = TrStMCP::selected_ref + ": %08X" + TrStMCP::BOS_ignored;
                ImGui::Text(text.c_str(), ref->GetFormID());
            } else {
                if (ImGui::CollapsingHeader(TrStMCP::BOS_support.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    std::string headerText = TrStMCP::BOS_support + " " + TrStMCP::help;
                    if (ImGui::CollapsingHeader(headerText.c_str())) {
                        ImGui::TextWrapped(TrStMCP::BOS_support_help_text.c_str());
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
                    std::string posLabel = TrStMCP::position + " | " + TrStMCP::step + ": ";
                    ImGui::Text(posLabel.c_str());
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
                    std::string rotLabel = TrStMCP::rotation + " | " + TrStMCP::step + ": ";
                    ImGui::Text(rotLabel.c_str());
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
                    std::string scaleLabel = TrStMCP::scale + " | " + TrStMCP::step + ": ";
                    ImGui::Text(scaleLabel.c_str());
                    ImGui::SameLine();
                    ImGui::InputFloat("", &scaleStepValue, 0.1f, 1.0f, "%.3f");

                    color = (std::abs(scale - originalScale) < 0.001f) ? green : yellow;
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
                        ImGui::TextColored(yellow, TrStMCP::unsaved_changes.c_str());
                    }
                    if (ImGui::Button(TrStMCP::save_transform.c_str())) {
                        unsavedChanges = false;
                        BosMgr->TransformObject(ref);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(TrStMCP::remove_object.c_str())) {
                        unsavedChanges = false;
                        BosMgr->RemoveObject(ref);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(TrStMCP::reset_transform.c_str())) {
                        unsavedChanges = false;
                        BosMgr->ResetObject(ref);
                    }

                    ImGui::BulletText(TrStMCP::save_BOS_text.c_str());
                    ImGui::BulletText(TrStMCP::remove_BOS_text.c_str());
                    ImGui::BulletText(TrStMCP::reset_transform.c_str());
                }
            }
        } else {
            ImGui::Text(TrStMCP::no_selected_ref.c_str());
        }
    }

    void __stdcall RenderBOSFile() {

        ImGui::SeparatorText(TrStMCP::BOS_editor.c_str());
        std::string headerText = TrStMCP::BOS_editor + " " + TrStMCP::help;
        if (ImGui::CollapsingHeader(headerText.c_str())) {
            ImGui::TextWrapped(TrStMCP::BOS_editor_help_text.c_str());
        }
        ImGui::Separator();

        auto bosMgr = BOSIniManager::GetSingleton();

        std::string currentFile = bosMgr->GetFile();
        if (currentFile.starts_with("Data//") || currentFile.starts_with("Data\\")) {
            currentFile = currentFile.substr(5);
        }
        std::string fileText = TrStMCP::current_file + currentFile;
        ImGui::Text(fileText.c_str());

        // Collect all *_SWAP.ini files in Data folder (once)
        static std::vector<std::string> swapFiles;
        static bool filesScanned = false;

        if (!filesScanned) {
            filesScanned = true;
            swapFiles.clear();

            for (const auto& entry : std::filesystem::directory_iterator("Data")) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    auto u8name = path.filename().u8string();
                    std::string name(u8name.begin(), u8name.end());
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

        if (ImGui::BeginCombo(TrStMCP::select_file.c_str(),
                              selectedFile >= 0 ? swapFiles[selectedFile].c_str() : TrStMCP::none.c_str())) {
            for (int i = 0; i < (int)swapFiles.size(); i++) {
                bool isSelected = (i == selectedFile);
                if (ImGui::Selectable(swapFiles[i].c_str(), isSelected)) {
                    selectedFile = i;
                    bosMgr->SetFile("Data//" + swapFiles[i]);
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
            if (ImGui::Button(TrStMCP::create_new_file.c_str())) {
                creatingNewFile = true;
                newFileBuf[0] = '\0';
            }
        } else {
            headerText = TrStMCP::enter_file_name + " _SWAP.ini):";
            ImGui::Text(headerText.c_str());
            ImGui::InputText(TrStMCP::new_file_name.c_str(), newFileBuf, sizeof(newFileBuf));
            ImGui::SameLine();
            if (ImGui::Button(TrStMCP::create.c_str())) {
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
            if (ImGui::Button(TrStMCP::cancel.c_str())) {
                creatingNewFile = false;
            }
        }

        if (ImGui::CollapsingHeader(TrStMCP::transformed_objects.c_str())) {
            int idx = 0;
            for (auto it = bosMgr->newTransforms.begin(); it != bosMgr->newTransforms.end();) {
                ImGui::PushID(idx);

                ImGui::Text("RefID: %s", it->second.origRefID.c_str());
                auto ref = Utils::GetFormFromString(it->second.origRefID.c_str())->AsReference();
                if (ref) {
                    ImGui::Text("EditID: %s", ref->AsReference()->GetBaseObject()->GetFormEditorID());
                }

                OverridesData overrides = Utils::ParseOverrides(it->second.propertyOverrides);
                bool changed = false;

                if (ImGui::CollapsingHeader(TrStMCP::transform.c_str())) {
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
                    std::string posLabel = TrStMCP::position + " | " + TrStMCP::step + ": ";
                    ImGui::Text(posLabel.c_str());
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
                    std::string rotLabel = TrStMCP::rotation + " | " + TrStMCP::step + ": ";
                    ImGui::Text(rotLabel.c_str());
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
                    std::string scaleLabel = TrStMCP::scale + " | " + TrStMCP::step + ": ";
                    ImGui::Text(scaleLabel.c_str());
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

                if (ImGui::Button(TrStMCP::reset_transform.c_str())) {
                    bosMgr->ResetObject(ref);
                    ImGui::PopID();
                    break;
                }
                ImGui::SameLine();
                if (ImGui::Button(TrStMCP::remove_object.c_str())) {
                    bosMgr->ResetObject(ref);  // first reset to original position
                    bosMgr->RemoveObject(ref);
                    ImGui::PopID();
                    break;
                }
                ImGui::SameLine();
                if (ImGui::Button(TrStMCP::revert.c_str())) {
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
            ImGui::BulletText(TrStMCP::auto_save_text.c_str());
            ImGui::BulletText(TrStMCP::remove_BOS_text.c_str());
            ImGui::BulletText(TrStMCP::reset_BOS_transform.c_str());
            ImGui::BulletText(TrStMCP::revert_BOS_text.c_str());
        }
    }
    
    void __stdcall RenderdBOSResolver() {
        static bool scaned = false;
        static auto resolver = BOSConflictResolver::GetSingleton();
        if (ImGui::Button("Scan for Conflicts") || scaned == false) {
            resolver->Scan("Data");
            scaned = true;
        }

        ImGui::Separator();

        const auto& conflicts = resolver->GetConflicts();

        if (conflicts.empty()) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "No conflicts found!");
        } else {
            for (auto& [section, secConflicts] : conflicts) {
                // Tree node for each section
                if (ImGui::CollapsingHeader(section.c_str())) {
                    for (auto& [lhs, entries] : secConflicts) {
                        if (entries.size() <= 1) continue;  // no real conflict

                        // Highlight LHS in red if conflicting
                        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", lhs.c_str());

                        ImGui::Indent();
                        for (auto& e : entries) {
                            ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
                            ImGui::TextColored(color, "%s (line %d) -> %s", e.file.c_str(), e.line, e.rhs.c_str());
                            ImGui::Text("    %s|%s", e.lhs.c_str(), e.rhs.c_str());

                            if (ImGui::Button(("Keep This##" + std::to_string(e.line) + e.file).c_str())) {
                                for (auto& other : entries) {
                                    if (&other != &e) {
                                        resolver->CommentOut(other);
                                        resolver->Scan("Data");
                                    }
                                }
                                resolver->Apply(e);
                            }
                            if (section == "Transforms") {
                                ImGui::SameLine();
                                if (ImGui::Button(("Inspect##" + std::to_string(e.line) + e.file).c_str())) {
                                    resolver->Apply(e);
                                    resolver->Inspect(e);
                                }
                                ImGui::SameLine();
                                if (ImGui::Button(("Show##" + std::to_string(e.line) + e.file).c_str())) {
                                    resolver->Apply(e);
                                }
                            }

                        }
                        ImGui::Unindent();
                    }
                }
            }
        }
    }

    void __stdcall RenderKIDFile() {

        ImGui::SeparatorText(TrStMCP::KID_editor.c_str());
        std::string headerText = TrStMCP::KID_editor + " " + TrStMCP::help;
        if (ImGui::CollapsingHeader(headerText.c_str())) {
            ImGui::TextWrapped(TrStMCP::KID_editor_text.c_str());
        }
        ImGui::Separator();

        auto kidMgr = KIDIniManager::GetSingleton();

        std::string currentFile = kidMgr->GetFile();
        if (currentFile.starts_with("Data//") || currentFile.starts_with("Data\\")) {
            currentFile = currentFile.substr(5);
        }
        std::string fileText = TrStMCP::current_file + currentFile;
        ImGui::Text(fileText.c_str());

        // Collect all *_KID.ini files in Data folder (once)
        static std::vector<std::string> kidFiles;
        static bool filesScanned = false;

        if (!filesScanned) {
            filesScanned = true;
            kidFiles.clear();

            for (const auto& entry : std::filesystem::directory_iterator("Data")) {
                if (entry.is_regular_file()) {
                    auto path = entry.path();
                    auto u8name = path.filename().u8string();
                    std::string name(u8name.begin(), u8name.end());
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

        if (ImGui::BeginCombo(TrStMCP::select_file.c_str(),
                              selectedFile >= 0 ? kidFiles[selectedFile].c_str() : TrStMCP::none.c_str())) {
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
            if (ImGui::Button(TrStMCP::create_new_file.c_str())) {
                creatingNewFile = true;
                newFileBuf[0] = '\0';
            }
        } else {
            headerText = TrStMCP::enter_file_name + " _KID.ini):";
            ImGui::Text(headerText.c_str());
            ImGui::InputText(TrStMCP::new_file_name.c_str(), newFileBuf, sizeof(newFileBuf));
            ImGui::SameLine();
            if (ImGui::Button(TrStMCP::create.c_str())) {
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
            if (ImGui::Button(TrStMCP::cancel.c_str())) {
                creatingNewFile = false;
            }
        }

        // Entries list
        if (ImGui::CollapsingHeader(TrStMCP::KID_entries.c_str())) {
            int idx = 0;
            auto entries = kidMgr->GetEntries();
            for (auto it = entries.begin(); it != entries.end();) {
                ImGui::PushID(idx);

                ImGui::Text("FormID: %s", it->objectID.c_str());
                ImGui::Text("Keyword: %s", it->keyword.c_str());

                if (ImGui::Button(TrStMCP::remove.c_str())) {
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
            ImGui::BulletText(TrStMCP::remove_KID_text.c_str());
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