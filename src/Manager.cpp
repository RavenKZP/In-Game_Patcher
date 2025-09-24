#include "Manager.h"
#include "Events.h"
#include "Utils.h"
#include "BOS.h"
#include "Translations.h"

namespace TrStSkPr = Translations::Strings::SkyPrompt;

void PatcherPromptSink::RegisterSkyPrompt(std::string lang) {
    clientID = SkyPromptAPI::RequestClientID();
    if(SkyPromptAPI::RequestTheme(clientID, "InGamePatcher")) {
        logger::info("SkyPrompt theme InGamePatcher registered");
    } else {
        logger::error("SkyPrompt theme InGamePatcher registration failed");
    }
}

std::span<const SkyPromptAPI::Prompt> PatcherPromptSink::GetPrompts() const {
    if (console) {
        if (PatchingMode) {
            return ExitPrompt;
        } else {
            return EnterPrompt;
        }
    }
    if (dragging) {
        if (advancedHints) {
            return AdvancedHintPrompts;
        } else {
            return HintPrompts;
        }
    }
    return PatcherPrompts;
}

void PatcherPromptSink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {
    if (event.type == SkyPromptAPI::PromptEventType::kAccepted) {
        static RE::TESObjectREFR* dragginObj = nullptr;
        if (event.prompt.eventID == 1) {
            if (event.prompt.actionID == 1) {
                auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(event.prompt.refid);
                if (ref) {
                    SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                    BOSIniManager::GetSingleton()->RememberOriginal(ref);
                    if (OMO_installed) {
                        ObjectManipulationOverhaul::StartDraggingObject(ref);
                        dragginObj = ref;
                        dragging = true;
                        logger::info("Started dragging object: {}", ref->GetName());
                    } else {
                        RE::DebugNotification("Object Manipulation Overhaul is not installed");
                    }
                    if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                        logger::error("Failed to send prompt to SkyPrompt after starting drag");
                    }
                }
            } else if (event.prompt.actionID == 4) {
                logger::info("Entered Patch Mode");
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                PatchingMode = true;
                if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                    logger::error("Failed to send prompt to SkyPrompt after entering patch mode");
                }
            } else if (event.prompt.actionID == 5) {
                logger::info("Exited Patch Mode");
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                PatchingMode = false;
                if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                    logger::error("Failed to send prompt to SkyPrompt after exiting patch mode");
                }
            } else if (event.prompt.actionID == 6) {
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                if (dragginObj) {
                    logger::info("Stoped dragging object: {}", dragginObj->GetName());
                }
                dragginObj = nullptr;
                dragging = false;
                if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                    logger::error("Failed to send prompt to SkyPrompt after stopping drag");
                }
            }
        }
        else if (event.prompt.eventID == 2) {
             if (event.prompt.actionID == 2) {
                auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(event.prompt.refid);
                 if (ref) {
                     BOSIniManager::GetSingleton()->RememberOriginal(ref);
                     BOSIniManager::GetSingleton()->RemoveObject(ref);
                 }
                 SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
             } else if (event.prompt.actionID == 6) {
                 SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                 dragginObj = nullptr;
                 dragging = false;
                 if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                     logger::error("Failed to send prompt to SkyPrompt after stopping drag");
                 }
             }
        }
        else if(event.prompt.eventID == 3) {
            if (event.prompt.actionID == 3) {
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(event.prompt.refid);
                if (ref) {
                    BOSIniManager::GetSingleton()->TransformObject(ref);
                }
                if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                    logger::error("Failed to send prompt to SkyPrompt after saving object");
                }
            }
        } else if (event.prompt.eventID == 4) {
            if (event.prompt.actionID == 4) {
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(event.prompt.refid);
                if (ref) {
                    BOSIniManager::GetSingleton()->ResetObject(ref);
                }
                if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                    logger::error("Failed to send prompt to SkyPrompt after resetting object");
                }
            }
        } else if (event.prompt.eventID == 5 && event.prompt.actionID == 6) {
            SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
            advancedHints = !advancedHints;
            if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
                logger::error("Failed to send prompt to SkyPrompt after toggling advanced hints");
            }
        }
    } else if (event.type == SkyPromptAPI::PromptEventType::kDeclined) {
        SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
    } else if (event.type == SkyPromptAPI::PromptEventType::kTimingOut) {
        if (!SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID)) {
            logger::error("Failed to send prompt to SkyPrompt on timeout");
        }
    }
}

void PatcherPromptSink::SetRef(RE::TESObjectREFR* ref) {
    auto formID = ref->GetFormID();
    PatcherPrompts = {SkyPromptAPI::Prompt(TrStSkPr::move, 1, 1, SkyPromptAPI::PromptType::kHold, formID),
                      SkyPromptAPI::Prompt(TrStSkPr::remove, 2, 2, SkyPromptAPI::PromptType::kHold, formID),
                      SkyPromptAPI::Prompt(TrStSkPr::save, 3, 3, SkyPromptAPI::PromptType::kHold, formID),
                      SkyPromptAPI::Prompt(TrStSkPr::reset, 4, 4, SkyPromptAPI::PromptType::kHold, formID)};
}

void PatcherPromptSink::InitPrompts() {
    EnterPrompt = {SkyPromptAPI::Prompt(TrStSkPr::enter, 1, 4, SkyPromptAPI::PromptType::kHold)};
    ExitPrompt = {SkyPromptAPI::Prompt(TrStSkPr::exit, 1, 5, SkyPromptAPI::PromptType::kHold)};

    PatcherPrompts = {SkyPromptAPI::Prompt(TrStSkPr::move,1, 1, SkyPromptAPI::PromptType::kHold),
                      SkyPromptAPI::Prompt(TrStSkPr::remove, 2, 2, SkyPromptAPI::PromptType::kHold),
                      SkyPromptAPI::Prompt(TrStSkPr::save, 3, 3, SkyPromptAPI::PromptType::kHold),
                      SkyPromptAPI::Prompt(TrStSkPr::reset, 4, 4, SkyPromptAPI::PromptType::kHold)};

    static const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> cancel_button_key{
        {{RE::INPUT_DEVICE::kMouse, 0x01}}};
    static const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> accept_button_key{
        {{RE::INPUT_DEVICE::kMouse, 0x00}}};
    static const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> ctrl_button_key{
        {{RE::INPUT_DEVICE::kKeyboard, 0x1D}}};
    static const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> shift_button_key{
        {{RE::INPUT_DEVICE::kKeyboard, 0x2A}}};
    static const std::vector<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>> space_button_key{
        {{RE::INPUT_DEVICE::kKeyboard, 0x39}}};

    if (OMO_action_bindings.find("Commit") == OMO_action_bindings.end()) {
        logger::warn("No key binding found for 'Commit', defaulting to Left Mouse Button");
        OMO_action_bindings["Commit"] = accept_button_key;
    }
    if (OMO_action_bindings.find("Cancel") == OMO_action_bindings.end()) {
        logger::warn("No key binding found for 'Cancel', defaulting to Right Mouse Button");    
        OMO_action_bindings["Cancel"] = cancel_button_key;
    }
    if (OMO_action_bindings.find("Move") == OMO_action_bindings.end()) {
        logger::warn("No key binding found for 'Move', defaulting to Left Control");
        OMO_action_bindings["Move"] = ctrl_button_key;
    }
    if (OMO_action_bindings.find("Rotate") == OMO_action_bindings.end()) {
        logger::warn("No key binding found for 'Rotate', defaulting to Left Shift");
        OMO_action_bindings["Rotate"] = shift_button_key;
    }
    if (OMO_action_bindings.find("ToggleAdvancedMode") == OMO_action_bindings.end()) {
        logger::warn("No key binding found for 'ToggleAdvancedMode', defaulting to Space");
        OMO_action_bindings["ToggleAdvancedMode"] = space_button_key;
    }

    HintPrompts = {
        SkyPromptAPI::Prompt(TrStSkPr::accept, 1, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Commit"]),
        SkyPromptAPI::Prompt(TrStSkPr::cancel, 2, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Cancel"]),
        SkyPromptAPI::Prompt(TrStSkPr::zrotate, 3, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Rotate"]),
        SkyPromptAPI::Prompt(TrStSkPr::zmove, 4, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Move"]),
        SkyPromptAPI::Prompt(TrStSkPr::basic, 5, 6, SkyPromptAPI::PromptType::kHint, 0,
                             OMO_action_bindings["ToggleAdvancedMode"])};

    AdvancedHintPrompts = {
        SkyPromptAPI::Prompt(TrStSkPr::accept, 1, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Commit"]),
        SkyPromptAPI::Prompt(TrStSkPr::cancel, 2, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Cancel"]),
        SkyPromptAPI::Prompt(TrStSkPr::xyzrotate, 3, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Rotate"]),
        SkyPromptAPI::Prompt(TrStSkPr::xyzmove, 4, 6, SkyPromptAPI::PromptType::kHint, 0, OMO_action_bindings["Move"]),
        SkyPromptAPI::Prompt(TrStSkPr::advanced, 5, 6, SkyPromptAPI::PromptType::kHint, 0,
                             OMO_action_bindings["ToggleAdvancedMode"])};
    
}