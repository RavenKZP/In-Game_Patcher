#include "Manager.h"
#include "Events.h"
#include "Utils.h"
#include "BOS.h"

void PatcherPromptSink::RegisterSkyPrompt() { 
    clientID = SkyPromptAPI::RequestClientID();
    SkyPromptAPI::RequestTheme(clientID, "InGamePatcher");
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

#include <format>  // C++20
#include <string>

void PatcherPromptSink::ProcessEvent(SkyPromptAPI::PromptEvent event) const {
    if (event.type == SkyPromptAPI::PromptEventType::kAccepted) {
        static RE::TESObjectREFR* dragginObj = nullptr;
        if (event.prompt.eventID == 1) {
            if (event.prompt.actionID == 1) {
                auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(event.prompt.refid);
                if (ref) {
                    SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                    BOSIniManager::GetSingleton()->RememberOriginal(ref);
                    ObjectManipulationOverhaul::StartDraggingObject(ref);
                    dragginObj = ref;
                    dragging = true;
                    logger::info("Started dragging object: {}", ref->GetName());
                    SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
                }
            } else if (event.prompt.actionID == 4) {
                logger::info("Entered Patch Mode");
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                PatchingMode = true;
                SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
            } else if (event.prompt.actionID == 5) {
                logger::info("Exited Patch Mode");
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                PatchingMode = false;
                SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
            } else if (event.prompt.actionID == 6) {
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                if (dragginObj) {
                    BOSIniManager::GetSingleton()->TransformObject(dragginObj);
                }
                dragginObj = nullptr;
                dragging = false;
                SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
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
                 SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
             }
        }
        else if(event.prompt.eventID == 3) {
            if (event.prompt.actionID == 3) {
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(event.prompt.refid);
                if (ref) {
                    BOSIniManager::GetSingleton()->TransformObject(ref);
                }
                SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
            }
        } else if (event.prompt.eventID == 4) {
            if (event.prompt.actionID == 4) {
                SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
                auto ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(event.prompt.refid);
                if (ref) {
                    BOSIniManager::GetSingleton()->ResetObject(ref);
                }
                SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
            }
        } else if (event.prompt.eventID == 5 && event.prompt.actionID == 6) {
            SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
            advancedHints = !advancedHints;
            SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
        }
    } else if (event.type == SkyPromptAPI::PromptEventType::kDeclined) {
        SkyPromptAPI::RemovePrompt(PatcherPromptSink::GetSingleton(), clientID);
    } else if (event.type == SkyPromptAPI::PromptEventType::kTimeout) {
        SkyPromptAPI::SendPrompt(PatcherPromptSink::GetSingleton(), clientID);
    }
}

void PatcherPromptSink::SetRef(RE::TESObjectREFR* ref) {
    auto formID = ref->GetFormID();
    PatcherPrompts = {SkyPromptAPI::Prompt("Move Object", 1, 1, SkyPromptAPI::PromptType::kHold, formID),
                      SkyPromptAPI::Prompt("Remove Object", 2, 2, SkyPromptAPI::PromptType::kHold, formID),
                      SkyPromptAPI::Prompt("Save Position", 3, 3, SkyPromptAPI::PromptType::kHold, formID),
                      SkyPromptAPI::Prompt("Reset Position", 4, 4, SkyPromptAPI::PromptType::kHold, formID)};
}

void PatcherPromptSink::InitPrompts() {
    EnterPrompt = {SkyPromptAPI::Prompt("Enter Patch Mode", 1, 4, SkyPromptAPI::PromptType::kHold)};
    ExitPrompt = {SkyPromptAPI::Prompt("Exit Patch Mode", 1, 5, SkyPromptAPI::PromptType::kHold)};

    PatcherPrompts = {SkyPromptAPI::Prompt("Move Object", 1, 1, SkyPromptAPI::PromptType::kHold),
                      SkyPromptAPI::Prompt("Remove Object", 2, 2, SkyPromptAPI::PromptType::kHold),
                      SkyPromptAPI::Prompt("Save Position", 3, 3, SkyPromptAPI::PromptType::kHold),
                      SkyPromptAPI::Prompt("Reset Position", 4, 4, SkyPromptAPI::PromptType::kHold)};

    
    static const std::array<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>, 1> cancel_button_key{
        {{RE::INPUT_DEVICE::kMouse, 0x01}}};
    static const std::array<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>, 1> accept_button_key{
        {{RE::INPUT_DEVICE::kMouse, 0x00}}};
    static const std::array<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>, 1> ctrl_button_key{
        {{RE::INPUT_DEVICE::kKeyboard, 0x1D}}};
    static const std::array<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>, 1> shift_button_key{
        {{RE::INPUT_DEVICE::kKeyboard, 0x2A}}};
    static const std::array<std::pair<RE::INPUT_DEVICE, SkyPromptAPI::ButtonID>, 1> space_button_key{
        {{RE::INPUT_DEVICE::kKeyboard, 0x39}}};

    HintPrompts = {
        SkyPromptAPI::Prompt("Accept", 1, 6, SkyPromptAPI::PromptType::kHint, 0, accept_button_key),
        SkyPromptAPI::Prompt("Cancel", 2, 6, SkyPromptAPI::PromptType::kHint, 0, cancel_button_key),
        SkyPromptAPI::Prompt("Z Rotate", 3, 6, SkyPromptAPI::PromptType::kHint, 0, ctrl_button_key),
        SkyPromptAPI::Prompt("Z Move", 4, 6, SkyPromptAPI::PromptType::kHint, 0, shift_button_key),
        SkyPromptAPI::Prompt("Basic", 5, 6, SkyPromptAPI::PromptType::kHint, 0, space_button_key)};

    AdvancedHintPrompts = {SkyPromptAPI::Prompt("Accept", 1, 6, SkyPromptAPI::PromptType::kHint, 0, accept_button_key),
                           SkyPromptAPI::Prompt("Cancel", 2, 6, SkyPromptAPI::PromptType::kHint, 0, cancel_button_key),
                           SkyPromptAPI::Prompt("XYZ Rotate", 3, 6, SkyPromptAPI::PromptType::kHint, 0, ctrl_button_key),
                           SkyPromptAPI::Prompt("XYZ Move", 4, 6, SkyPromptAPI::PromptType::kHint, 0, shift_button_key),
                           SkyPromptAPI::Prompt("Advanced", 5, 6, SkyPromptAPI::PromptType::kHint, 0, space_button_key)};
    
}