#pragma once

namespace Hooks {
    void Install();

    struct UpdateHook {
        static void Update(RE::Actor* a_this, float a_delta);
        static inline REL::Relocation<decltype(Update)> Update_;
    };
}