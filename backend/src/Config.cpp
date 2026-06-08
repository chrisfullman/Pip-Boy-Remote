#include "Config.h"

#ifndef PIPBOY_TESTING
// Full Windows implementation — only compiled outside the unit-test target.
#    include <Windows.h>

namespace PipBoyRemote
{
    Config Config::Load()
    {
        Config cfg;

        // Enable flag
        cfg.enabled = ::GetPrivateProfileIntW(
            CONFIG_INI_SECTION, L"Enable", static_cast<int>(CONFIG_DEFAULT_ENABLED), CONFIG_INI_PATH
        ) != 0;

        // WebSocket port — clamp to valid range
        const int portRaw = ::GetPrivateProfileIntW(
            CONFIG_INI_SECTION, L"WebSocketPort", CONFIG_DEFAULT_PORT, CONFIG_INI_PATH
        );
        if (portRaw > 0 && portRaw <= 65535) {
            cfg.port = static_cast<std::uint16_t>(portRaw);
        }

        // Bind IP address — read as wide string, convert to narrow ASCII
        wchar_t ipBuffer[64] = {};
        ::GetPrivateProfileStringW(
            CONFIG_INI_SECTION, L"BindIP", L"0.0.0.0", ipBuffer, static_cast<DWORD>(std::size(ipBuffer)), CONFIG_INI_PATH
        );
        // IP addresses are ASCII-safe, so a simple narrow conversion is fine here.
        cfg.bindIP.clear();
        for (const wchar_t c : std::wstring_view{ ipBuffer }) {
            if (c == L'\0') { break; }
            cfg.bindIP += static_cast<char>(c);
        }

        return cfg;
    }
}

#else
// Stub for unit-test builds — just returns defaults.
namespace PipBoyRemote
{
    Config Config::Load()
    {
        return Config{};
    }
}
#endif
