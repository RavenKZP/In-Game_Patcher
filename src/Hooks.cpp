#include "Hooks.h"
#include "Events.h"
#include "Manager.h"
#include "Raycast.h"
#include "Utils.h"

RE::NiObject* GetPlayer3d() {
    auto refr = RE::PlayerCharacter::GetSingleton();
    if (!refr) {
        return nullptr;
    }
    if (!refr->loadedData) {
        return nullptr;
    }
    if (!refr->loadedData->data3D) {
        return nullptr;
    }
    return refr->loadedData->data3D.get();
}

namespace Hooks {
	void Install() {
		UpdateHook::Update_ =
            REL::Relocation<std::uintptr_t>(RE::VTABLE_PlayerCharacter[0]).write_vfunc(0xAD, UpdateHook::Update);
	}

	void Hooks::UpdateHook::Update(RE::Actor* a_this, float a_delta) {
		Update_(a_this, a_delta);

        static RE::TESObjectREFR* previousObject = nullptr;

        if (PatcherPromptSink::GetSingleton()->dragging) {
            return;
        }

        if (!PatchingMode) {
            if (previousObject) {
                // Reset old object's tint
                if (auto obj3d = previousObject->Get3D()) {
                    auto color = RE::NiColorA(0, 0, 0, 0);
                    obj3d->TintScenegraph(color);
                }
            }
            previousObject = nullptr;
            return;
        }

        if (a_this && a_this->IsPlayerRef()) {
            auto player3d = GetPlayer3d();

            const auto evaluator = [player3d](RE::NiAVObject* mesh) {
                if (mesh == player3d) {
                    return false;
                }
                return true;
            };

            RE::TESObjectREFR* consoleRef = RE::Console::GetSelectedRef().get();

            // Priority: console ref > raycast ref
            RE::TESObjectREFR* currentRef = nullptr;

            if (consoleRef && !Utils::IsDynamicForm(consoleRef)) {
                currentRef = consoleRef;
                if (auto obj3d = currentRef->Get3D()) {
                    static auto color = RE::NiColorA(0, 1.0f, 0, 0.5f);
                    obj3d->TintScenegraph(color);
                }
            } else {
                auto result = RayCast::Cast(evaluator);
                if (result.object && !Utils::IsDynamicForm(result.object)) {
                    currentRef = result.object;
                    if (auto obj3d = currentRef->Get3D()) {
                        static auto color = RE::NiColorA(0, 1.0f, 0, 0.5f);
                        obj3d->TintScenegraph(color);
                    }
                }
            }

            // Handle changes
            if (currentRef != previousObject) {
                if (previousObject) {
                    // Reset old object's tint
                    if (auto obj3d = previousObject->Get3D()) {
                        auto color = RE::NiColorA(0, 0, 0, 0);
                        obj3d->TintScenegraph(color);
                    }
                }

                auto PatcherPrompt = PatcherPromptSink::GetSingleton();
                // Remove old prompt if any
                if (previousObject) {
                    SkyPromptAPI::RemovePrompt(PatcherPrompt, PatcherPrompt->clientID);
                }

                if (currentRef) {
                    if (currentRef->As<RE::Actor>()) {
                        // Don't show prompt for actors
                    } else {
                        PatcherPrompt->SetRef(currentRef);
                        SkyPromptAPI::SendPrompt(PatcherPrompt, PatcherPrompt->clientID);
                    }
                }
            }

            previousObject = currentRef;

        }
    }
}  // namespace Hooks