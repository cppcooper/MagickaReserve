#pragma once
#ifndef EQUIPEVENTSINK_H
#define EQUIPEVENTSINK_H
#undef ENABLE_SKYRIM_AE

#include <cmath>
#include <RE/S/SkyrimVM.h>
#include <RE/T/TESEquipEvent.h>


extern void PapyrusCalculateReserved(RE::BSScript::IVirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag* tag);
extern void InitializeEffectMagnitude();

class EquipEventSink : public RE::BSTEventSink<RE::TESEquipEvent> {
    EquipEventSink() = default;
    EquipEventSink(const EquipEventSink&) = delete;
    EquipEventSink(EquipEventSink&&) = delete;
    EquipEventSink& operator=(const EquipEventSink&) = delete;
    EquipEventSink& operator=(EquipEventSink&&) = delete;
public:
    static EquipEventSink* GetSingleton() {
        static EquipEventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* event, RE::BSTEventSource<RE::TESEquipEvent>* eventSource);
};


#endif
