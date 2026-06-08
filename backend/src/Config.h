#pragma once

#include <cstdint>
#include <string>

namespace PipBoyRemote
{
    // Default values for all configurable settings.
    constexpr std::uint16_t  CONFIG_DEFAULT_PORT    = 11104;
    constexpr const char*    CONFIG_DEFAULT_BIND_IP = "0.0.0.0";
    constexpr bool           CONFIG_DEFAULT_ENABLED = true;

    // INI file location, relative to the game executable directory.
    // F4SE plugins and their INI files live side-by-side under Data/F4SE/Plugins/.
    constexpr const wchar_t* CONFIG_INI_PATH    = L".\\Data\\F4SE\\Plugins\\PipBoyRemote.ini";
    constexpr const wchar_t* CONFIG_INI_SECTION = L"PipBoyRemote";

    struct Config
    {
        bool          enabled = CONFIG_DEFAULT_ENABLED;
        std::uint16_t port    = CONFIG_DEFAULT_PORT;
        std::string   bindIP  = CONFIG_DEFAULT_BIND_IP;

        // Reads settings from the INI file. Returns defaults for any missing key.
        // In test builds (PIPBOY_TESTING=1) this is stubbed out and returns defaults.
        static Config Load();
    };
}
