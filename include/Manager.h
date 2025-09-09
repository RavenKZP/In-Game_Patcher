#pragma once

#include "ClibUtil/singleton.hpp"
#include "SkyPrompt/API.hpp"


class PatcherPromptSink : public clib_util::singleton::ISingleton<PatcherPromptSink>, public SkyPromptAPI::PromptSink {
public:
    void RegisterSkyPrompt();
    std::span<const SkyPromptAPI::Prompt> GetPrompts() const override;
    void ProcessEvent(SkyPromptAPI::PromptEvent event) const override;

    void SetRef(RE::TESObjectREFR* ref);

    void InitPrompts();

    std::array<SkyPromptAPI::Prompt, 4> PatcherPrompts;
    std::array<SkyPromptAPI::Prompt, 1> EnterPrompt;
    std::array<SkyPromptAPI::Prompt, 1> ExitPrompt;
    std::array<SkyPromptAPI::Prompt, 5> HintPrompts;
    std::array<SkyPromptAPI::Prompt, 5> AdvancedHintPrompts;

    SkyPromptAPI::ClientID clientID = 0;
    mutable bool console = false;
    mutable bool advancedHints = false;
    mutable bool dragging = false;
};