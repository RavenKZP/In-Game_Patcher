#pragma once
#include "ClibUtil/singleton.hpp"

namespace EventSinks {
    void Install();
};

class MenuEventSink : public clib_util::singleton::ISingleton<MenuEventSink>,
                      public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_dispatcher) override;
};