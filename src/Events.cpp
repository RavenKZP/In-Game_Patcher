#include "Events.h"
#include "Hooks.h"
#include "Settings.h"
#include "Utils.h"
#include "Manager.h"

#include "SkyPrompt/API.hpp"

RE::BSEventNotifyControl MenuEventSink::ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                                        RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_dispatcher) {
    if (!a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (a_event->menuName == RE::Console::MENU_NAME) {
        auto patcherPrompt = PatcherPromptSink::GetSingleton();
        SkyPromptAPI::RemovePrompt(patcherPrompt, patcherPrompt->clientID);
        if (a_event->opening) {
            patcherPrompt->console = true;
            if (!SkyPromptAPI::SendPrompt(patcherPrompt, patcherPrompt->clientID)) {
                logger::error("Failed to send prompt to SkyPrompt from Console open event");
            }
        } else {
            patcherPrompt->console = false;
            if (PatchingMode) {
                if (!SkyPromptAPI::SendPrompt(patcherPrompt, patcherPrompt->clientID)) {
                    logger::error("Failed to send prompt to SkyPrompt from Console close event");
                }
            }
        }
    }

    return RE::BSEventNotifyControl::kContinue;
}

void EventSinks::Install() {
    auto ui = RE::UI::GetSingleton();
    ui->AddEventSink(MenuEventSink::GetSingleton());

    logger::info("Event sinks installed");
}