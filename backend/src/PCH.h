#pragma once

// Standard library
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// CommonLibF4: F4SE integration layer and reverse-engineered game APIs
#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

// JSON serialization (nlohmann/json, header-only)
#include <nlohmann/json.hpp>

// uWebSockets (non-TLS; local network only)
#include <uwebsockets/App.h>
