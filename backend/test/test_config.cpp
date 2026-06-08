#include <catch2/catch_all.hpp>

#include "Config.h"

using namespace PipBoyRemote;

// In the test build (PIPBOY_TESTING=1), Config::Load() returns defaults without
// reading any INI file.  These tests verify that the defaults are correct and
// that the Config struct is well-formed.

TEST_CASE("Config defaults are correct", "[config]")
{
    const Config cfg = Config::Load();

    CHECK(cfg.enabled == true);
    CHECK(cfg.port    == 11104);
    CHECK(cfg.bindIP  == "0.0.0.0");
}

TEST_CASE("Config named constants match documented defaults", "[config]")
{
    CHECK(CONFIG_DEFAULT_PORT    == 11104);
    CHECK(CONFIG_DEFAULT_ENABLED == true);
    CHECK(std::string_view{ CONFIG_DEFAULT_BIND_IP } == "0.0.0.0");
}

TEST_CASE("Config port constant is in valid TCP range", "[config]")
{
    CHECK(CONFIG_DEFAULT_PORT >  0);
    CHECK(CONFIG_DEFAULT_PORT <= 65535);
}
