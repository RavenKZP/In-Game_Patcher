#include "logger.h"
#include "Events.h"
#include "Manager.h"
#include "Hooks.h"
#include "MCP.h"
#include "Utils.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {

        //This will create new files only if missing
        Utils::CreateNewBOSFile("Data\\InGamePatcher_SWAP.ini");
        Utils::CreateNewKIDFile("Data\\InGamePatcher_KID.ini");

        Hooks::Install();
        EventSinks::Install();
        MCP::Register();
        auto PatcherPrompt = PatcherPromptSink::GetSingleton();
        PatcherPrompt->RegisterSkyPrompt();
        PatcherPrompt->InitPrompts();
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        // Post-load
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    Version = skse->RuntimeVersion();
    logger::info("Game version: {}", Version.string());
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
