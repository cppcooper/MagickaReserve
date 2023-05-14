#include <EquipEventSink.h>
#include <RE/A/ActorEquipManager.h>
#include <RE/A/Actor.h>
#include <RE/P/PlayerCharacter.h>
#include <RE/T/TESForm.h>
#include <RE/T/TESGlobal.h>
#include <RE/T/TESObjectARMO.h>


float MagnitudeAggregate = 0.f;

RE::SpellItem* GetSpell() {
    //RE::PlayerCharacter::GetSingleton()->GetActiv
    static auto theSpell = RE::TESForm::LookupByEditorID<RE::SpellItem>("ERM_ReserveMagicka");
    return theSpell;
}

float GetMagPerMag() {
    static auto mgm = RE::TESForm::LookupByEditorID<RE::TESGlobal>("ERM_MagickaPerMagnitude");
    return mgm->value;
}

float GetReducPerEnchLevel() {
    static auto rpel = RE::TESForm::LookupByEditorID<RE::TESGlobal>("ERM_ReductionPerEnchLevel");
    return rpel->value;
}

float GetReductionPercent() {
    float p = GetReducPerEnchLevel() * RE::PlayerCharacter::GetSingleton()->AsActorValueOwner()->GetActorValue(RE::ActorValue::kEnchanting);
    return std::min(p, 1.f);
}

float GetMagnitude(RE::Effect* x) {
    std::string fxName = x->baseEffect->GetName();
    SKSE::log::info("{}", fxName);
    if (fxName == "Regenerate Magicka") {
        return 0.27f * x->GetMagnitude();
    } else if (fxName != "Fortify Magicka") {
        return x->GetMagnitude();
    }
    return 0.f;
}

float Aggregate(RE::TESObjectARMO* thisArmor) {
    float magTotal = 0.f;
    if (thisArmor) {
        uint32_t count = 0;
        for (auto x: thisArmor->formEnchanting->effects) {
            auto m = GetMagnitude(x);
            magTotal += m;
            SKSE::log::info("  magnitude: {}", m);
            if (m > 0.f) {
                count++;
            }
        }
        count = std::max(count, 1u);
        magTotal /= (float) count;
    }
    return magTotal;
}

void ReApplySpell(RE::Actor* ref) {
    if(ref) {
        ref->RemoveSpell(GetSpell());
        GetSpell()->effects[0]->effectItem.magnitude = MagnitudeAggregate;
        ref->AddSpell(GetSpell());
    }
}

RE::BSEventNotifyControl EquipEventSink::ProcessEvent(const RE::TESEquipEvent *event, RE::BSTEventSource<RE::TESEquipEvent> *eventSource) {
    auto player = RE::PlayerCharacter::GetSingleton();
    SKSE::log::info("Processing equip event event for base object '{}'", event->baseObject);
    //auto form = RE::TESForm::LookupByID(event->baseObject);  //LookupByID<RE::TESObjectARMO>(event->baseObject);
    auto thisArmor = RE::TESForm::LookupByID<RE::TESObjectARMO>(event->baseObject);
    //SKSE::log::info("form: {}", form->GetName());
    //auto thisArmor = form->As<RE::TESObjectARMO>();
    if (thisArmor && thisArmor->formEnchanting) {
        SKSE::log::info("Item is '{}'", thisArmor->GetFullName());
        float magTotal = Aggregate(thisArmor);
        magTotal *= event->equipped ? 1 : -1;
        magTotal = (1 - GetReductionPercent()) * magTotal * GetMagPerMag();
        float magicka = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka);
        MagnitudeAggregate += magTotal;
        ReApplySpell(player);
        SKSE::log::info(" Magicka Reserved: {}", MagnitudeAggregate);
        SKSE::log::info("{} > {}", magTotal, magicka);
        if (magTotal > magicka) {
            SKSE::log::warn(" PC doesn't have enough magicka for this item to be equipped.");
            RE::ActorEquipManager::GetSingleton()->UnequipObject(player, thisArmor);
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

void InitializeEffectMagnitude() {
    auto player = RE::PlayerCharacter::GetSingleton();
    MagnitudeAggregate = 0.f;
    for (auto &[boundObject, pair] : player->GetInventory()) {
        auto &[quantity, entryData] = pair;
        SKSE::log::info("Inventory: {}", entryData->GetDisplayName());
        if (entryData->IsEnchanted() && entryData->IsWorn() && boundObject->IsArmor()) {
            SKSE::log::info(" {} is enchanted, and being worn", entryData->GetDisplayName());
            float magTotal = Aggregate(boundObject->As<RE::TESObjectARMO>());
            magTotal = (1 - GetReductionPercent()) * magTotal * GetMagPerMag();
            float magicka = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka);
            MagnitudeAggregate += magTotal;
            ReApplySpell(player);
            SKSE::log::info(" Magicka Reserved: {}", MagnitudeAggregate);
            SKSE::log::info("{} > {}", magTotal, magicka);
            if (magTotal > magicka) {
                SKSE::log::warn(" PC doesn't have enough magicka for this item to be equipped.");
                RE::ActorEquipManager::GetSingleton()->UnequipObject(player, boundObject);
            }
        }
    }
}

void PapyrusCalculateReserved(RE::BSScript::IVirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag* tag) {
    InitializeEffectMagnitude();
}