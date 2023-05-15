#undef ENABLE_SKYRIM_AE

#include <EquipEventSink.h>
#include <RE/I/IVirtualMachine.h>
#include <RE/N/NativeFunction.h>

using SKSE::MessagingInterface;

void InitializeLog() {
#ifndef NDEBUG
    auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
    auto path = logger::log_directory();
    if (!path) {
        SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
    }

    *path /= fmt::format("ReservedMagicka.log");
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
    const auto level = spdlog::level::trace;
#else
    const auto level = spdlog::level::info;
#endif

    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
    log->set_level(level);
    log->flush_on(level);

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
}

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

bool BindPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
    vm->RegisterFunction("CalculateReserved"sv, "ERM_PluginFunctions", PapyrusCalculateReserved);
    return true;
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    InitializeLog();
    auto* plugin = SKSE::PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    SKSE::log::info("{} {}.b is loading...", plugin->GetName(), version);

    Init(skse);
    InitializeMessaging();
    RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(EquipEventSink::GetSingleton());
    SKSE::GetPapyrusInterface()->Register(BindPapyrusFunctions);

    SKSE::log::info("{} has finished loading.", plugin->GetName());
    return true;
}
