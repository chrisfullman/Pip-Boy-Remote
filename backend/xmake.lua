-- PipBoyRemote - Fallout 4 F4SE backend plugin
-- Builds a Windows x64 DLL targeting the Fallout 4 NG and AE runtimes via CommonLibF4.

set_xmakever("3.0.0")
set_project("PipBoyRemote")
set_arch("x64")
set_languages("c++23")
set_warnings("allextra", "error")
set_encodings("utf-8")

add_rules("mode.debug", "mode.releasedbg", "mode.release")
add_rules("plugin.vsxmake.autoupdate")

-- ============================================================
-- External packages (resolved via xrepo on Windows CI)
-- ============================================================
add_requires("nlohmann_json v3.12.0")
add_requires("uwebsockets v20.77.0", { configs = { zip = false } })

-- Catch2 is only needed for the test target.
if is_plat("windows") then
    add_requires("catch2 v3.14.0")
end

-- ============================================================
-- CommonLibF4 (git submodule, provides commonlibf4.plugin rule)
-- ============================================================
includes("commonlibf4")

-- ============================================================
-- Main plugin target: PipBoyRemote.dll
-- ============================================================
target("PipBoyRemote", function()
    -- Applies the CommonLibF4 plugin rule which:
    --   * Sets kind("shared") and arch("x64")
    --   * Generates the F4SEPlugin_Version export from the template
    --   * Wires up install/package helpers
    add_rules("commonlibf4.plugin", {
        name   = "PipBoyRemote",
        author = "",
    })
    set_version("0.1.0")
    set_license("MIT")

    -- Source files (Plugin.cpp excluded from test target so it is listed explicitly)
    add_files("src/Config.cpp")
    add_files("src/GameStatePoller.cpp")
    add_files("src/JsonMessages.cpp")
    add_files("src/Plugin.cpp")
    add_files("src/WebSocketServer.cpp")

    add_includedirs("src")

    add_packages("nlohmann_json", "uwebsockets")

    -- Windows SDK libraries required by uSockets / uWebSockets
    add_syslinks("ws2_32", "Iphlpapi", "userenv")

    -- PCH shared with all source files
    set_pcxxheader("src/PCH.h")
end)

-- ============================================================
-- Test target (Windows-only; runs on the CI Windows runner)
-- ============================================================
if is_plat("windows") then
    target("PipBoyRemote_Tests", function()
        set_kind("binary")
        set_default(false)

        -- Only the pure-logic sources that have no game-engine dependencies
        add_files("src/Config.cpp")
        add_files("src/JsonMessages.cpp")
        add_files("test/test_main.cpp")
        add_files("test/test_config.cpp")
        add_files("test/test_json_messages.cpp")

        add_includedirs("src", "test")
        add_packages("catch2", "nlohmann_json")

        -- Mirror the MSVC flags used by commonlib-shared so headers parse cleanly
        add_cxxflags(
            "/EHsc",
            "/permissive-",
            "cl::/bigobj",
            "cl::/Zc:preprocessor"
        )

        -- Define a stub so Config.cpp compiles without calling real Windows INI APIs
        add_defines("PIPBOY_TESTING=1")
    end)
end
