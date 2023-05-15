#include <EquipEventSink.h>
#include <RE/A/ActorEquipManager.h>
#include <RE/A/Actor.h>
#include <RE/P/PlayerCharacter.h>
#include <RE/T/TESForm.h>
#include <RE/T/TESGlobal.h>
#include <RE/T/TESObjectARMO.h>
#include <unordered_map>

std::unordered_map<RE::FormID, std::shared_ptr<RE::InventoryEntryData>> Inventory;

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

float Aggregate(RE::TESObjectARMO* thisArmor, std::unique_ptr<RE::InventoryEntryData> &entryData) {
    float magTotal = 0.f;
    uint32_t count = 0;
    if (thisArmor && thisArmor->formEnchanting) {
        for (auto x: thisArmor->formEnchanting->effects) {
            auto m = GetMagnitude(x);
            magTotal += m;
            SKSE::log::info("  magnitude: {}", m);
            if (m > 0.f) {
                count++;
            }
        }
    }
    if (entryData && entryData->extraLists) {
        for (auto &dataList : *entryData->extraLists) {
            auto xench = dataList->GetByType<RE::ExtraEnchantment>();
            if (!xench || !xench->enchantment) {
                SKSE::log::info("Skipping, no xench or xench->enchantment");
                continue;
            }
            SKSE::log::info("Enchantment found in extraLists");
            for (auto x : xench->enchantment->effects) {
                auto m = GetMagnitude(x);
                magTotal += m;
                SKSE::log::info("  magnitude: {}", m);
                if (m > 0.f) {
                    count++;
                }
            }
        }
    }
    count = std::max(count, 1u);
    magTotal /= (float) count;
    return magTotal;
}

void ReApplySpell(RE::Actor* ref) {
    if(ref) {
        ref->RemoveSpell(GetSpell());
        GetSpell()->effects[0]->effectItem.magnitude = MagnitudeAggregate;
        ref->AddSpell(GetSpell());
    }
}

void HandleItem(RE::TESBoundObject* boundObject, std::unique_ptr<RE::InventoryEntryData> &entryData, bool equipped) {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (boundObject->IsArmor()) {
        float magTotal = Aggregate(boundObject->As<RE::TESObjectARMO>(), entryData);
        magTotal *= equipped ? 1 : -1;
        magTotal = (1 - GetReductionPercent()) * magTotal * GetMagPerMag();
        if (magTotal != 0) {
            float magicka = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka);
            MagnitudeAggregate += magTotal;
            ReApplySpell(player);
            SKSE::log::info(" Magicka Reserved: {}", MagnitudeAggregate);
            SKSE::log::info("{} > {} = {}", magTotal, magicka, magTotal > magicka);
            if (magTotal > magicka) {
                SKSE::log::warn(" PC doesn't have enough magicka for this item to be equipped.");
                RE::ActorEquipManager::GetSingleton()->UnequipObject(player, boundObject);
            }
        }
    }
}

RE::BSEventNotifyControl EquipEventSink::ProcessEvent(const RE::TESEquipEvent *event, RE::BSTEventSource<RE::TESEquipEvent> *eventSource) {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (event->actor.get() == player) {
        auto inventory = RE::PlayerCharacter::GetSingleton()->GetInventory();
        SKSE::log::info("\nProcessing ~~{}equip~~ event event for base object '{}'", event->equipped ? "" : "un", event->baseObject);
        auto thisArmor = RE::TESForm::LookupByID<RE::TESObjectARMO>(event->baseObject);
        if (thisArmor) {
            auto iter = inventory.find(thisArmor);
            bool suc = iter != inventory.end();
            if (suc) {
                auto &pair = iter->second;
                //SKSE::log::info(" entryData name: {}", pair.second->GetDisplayName());
                HandleItem(iter->first, pair.second, event->equipped);
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

void InitializeEffectMagnitude() {
    auto player = RE::PlayerCharacter::GetSingleton();
    MagnitudeAggregate = 0.f;
    SKSE::log::info("\nInitializing Magnitude");
    for (auto &[boundObject, pair] : player->GetInventory()) {
        auto &[quantity, entryData] = pair;
        if(entryData->IsWorn()) {
            SKSE::log::info(" initialization reading worn item '{}'", boundObject->GetName());
            HandleItem(boundObject, entryData, true);
        }
    }
}

void PapyrusCalculateReserved(RE::BSScript::IVirtualMachine* a_vm, RE::VMStackID a_stackID, RE::StaticFunctionTag* tag) {
    InitializeEffectMagnitude();
}