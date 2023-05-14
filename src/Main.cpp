#undef ENABLE_SKYRIM_AE

#include <EquipEventSink.h>
#include <RE/I/IVirtualMachine.h>
#include <RE/N/NativeFunction.h>

using SKSE::MessagingInterface;

namespace {

    /**
     * Register to listen for messages.
     *
     * <p>
     * SKSE has a messaging system to allow for loosely coupled messaging. This means you don't need to know about or
     * link with a message sender to receive their messages. SKSE itself will send messages for common Skyrim lifecycle
     * events, such as when SKSE plugins are done loading, or when all ESP plugins are loaded.
     * </p>
     *
     * <p>
     * Here we register a listener for SKSE itself (because we have not specified a message source). Plugins can send
     * their own messages that other plugins can listen for as well, although that is not demonstrated in this example
     * and is not common.
     * </p>
     *
     * <p>
     * The data included in the message is provided as only a void pointer. It's type depends entirely on the type of
     * message, and some messages have no data (<code>dataLen</code> will be zero).
     * </p>
     */
    void InitializeMessaging() {
        if (!SKSE::GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message) {
            switch (message->type) {
                // Skyrim lifecycle events.
                case MessagingInterface::kPostLoad: // Called after all plugins have finished running SKSEPlugin_Load.
                    // It is now safe to do multithreaded operations, or operations against other plugins.
                case MessagingInterface::kPostPostLoad: // Called after all kPostLoad message handlers have run.
                case MessagingInterface::kInputLoaded: // Called when all game data has been found.
                case MessagingInterface::kDataLoaded: // All ESM/ESL/ESP plugins have loaded, main menu is now active.
                case MessagingInterface::kPreLoadGame: // Player selected a game to load, but it hasn't loaded yet.
                    break;
                case MessagingInterface::kNewGame: // Player starts a new game from main menu.
                case MessagingInterface::kPostLoadGame: // Player's selected save game has finished loading.
                    // Data will be a boolean indicating whether the load was successful.
                    SKSE::log::info("Initializing the effect magnitude");
                    InitializeEffectMagnitude();
                case MessagingInterface::kSaveGame: // The player has saved a game.
                    // Data will be the save name.
                case MessagingInterface::kDeleteGame: // The player deleted a saved game from within the load menu.
                    break;
            }
        })) {
            SKSE::stl::report_and_fail("Unable to register message listener.");
        }
    }
}

bool BindPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("CalculateReserved"sv, "ERM_PluginFunctions", PapyrusCalculateReserved);
    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    auto* plugin = SKSE::PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    SKSE::log::info("{} {} is loading...", plugin->GetName(), version);

    Init(skse);
    InitializeMessaging();
    RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(EquipEventSink::GetSingleton());
    SKSE::GetPapyrusInterface()->Register(BindPapyrusFunctions);

    SKSE::log::info("{} has finished loading.", plugin->GetName());
    return true;
}
