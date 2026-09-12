#pragma once

#include "ESPressio_Platform.hpp"

namespace ESPressio::Platform::IDF {

/** Compile-time identity of the ESP-IDF backend. */
struct Backend final : ESPressio::Platform::Backend {};

/**
 * Convenience alias for declaring ESP-IDF-backed providers without repeating
 * the backend identity. Concrete providers explicitly declare the capabilities
 * they supply and any composition requirements they impose.
 */
template <typename TCapabilities,
          typename TRequirements = ESPressio::Platform::RequirementSet<>>
using ProviderDeclaration = ESPressio::Platform::ProviderDeclaration<
    Backend,
    TCapabilities,
    TRequirements>;

} // namespace ESPressio::Platform::IDF

#include "ESPressio_Platform_IDFClock.hpp"
