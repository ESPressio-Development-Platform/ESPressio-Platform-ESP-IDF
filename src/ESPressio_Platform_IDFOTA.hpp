#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

#include "ESPressio_PlatformOTA.hpp"
#include "ESPressio_Platform_IDF.hpp"

namespace ESPressio::Platform::IDF {

namespace OTADetail {

inline OTA::Result MapResult(esp_err_t result) noexcept {
    if (result == ESP_OK) return {OTA::Status::Success, 0};
    if (result == ESP_ERR_INVALID_ARG) return {OTA::Status::Invalid, result};
    if (result == ESP_ERR_NO_MEM) return {OTA::Status::CapacityUnavailable, result};
    if (result == ESP_ERR_INVALID_STATE) return {OTA::Status::Busy, result};
    if (result == ESP_ERR_NOT_FOUND) return {OTA::Status::Unsupported, result};
    return {OTA::Status::Failed, result};
}

inline OTA::BootTargetIdentifier TargetFor(const esp_partition_t* partition) noexcept {
    return partition == nullptr
        ? OTA::BootTargetIdentifier{}
        : OTA::BootTargetIdentifier{partition->address};
}

inline const esp_partition_t* FindApplicationTarget(OTA::BootTargetIdentifier target) noexcept {
    if (!target) return nullptr;
    auto iterator = esp_partition_find(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    while (iterator != nullptr) {
        const auto* partition = esp_partition_get(iterator);
        if (partition != nullptr && partition->address == target.Value()) {
            esp_partition_iterator_release(iterator);
            return partition;
        }
        iterator = esp_partition_next(iterator);
    }
    return nullptr;
}

inline std::uint64_t FnvAppendByte(std::uint64_t hash, std::uint8_t byte) noexcept {
    constexpr std::uint64_t prime = 1099511628211ULL;
    return (hash ^ byte) * prime;
}

inline std::uint64_t FnvAppend32(std::uint64_t hash, std::uint32_t value) noexcept {
    for (std::uint8_t shift = 0U; shift < 32U; shift += 8U) {
        hash = FnvAppendByte(hash, static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
    return hash;
}

} // namespace OTADetail

/**
 * ESP-IDF application-image staging provider.
 *
 * Bytes are written only to ESP-IDF's selected inactive OTA application
 * partition. Finalize validates/closes the staged image but deliberately does
 * not select it for boot; activation remains a separate BootControl operation.
 * Aborting an incomplete session never changes the selected boot partition.
 */
class OTAApplicationImageStaging final
    : public ProviderDeclaration<CapabilitySet<Capability::ApplicationImageStaging>> {
    esp_ota_handle_t handle_{0};
    const esp_partition_t* target_{nullptr};
    std::uint64_t expectedBytes_{0U};
    std::uint64_t writtenBytes_{0U};
    bool active_{false};

    void ClearSession(bool clearTarget) noexcept {
        handle_ = 0;
        expectedBytes_ = 0U;
        writtenBytes_ = 0U;
        active_ = false;
        if (clearTarget) target_ = nullptr;
    }

public:
    OTA::PreflightResult PreflightApplicationImage(std::uint64_t exactBytes) noexcept {
        if (active_ || exactBytes == 0U) {
            return {active_ ? OTA::Status::Busy : OTA::Status::Invalid,
                    exactBytes, 0U, 0};
        }
        const auto* candidate = esp_ota_get_next_update_partition(nullptr);
        if (candidate == nullptr) {
            return {OTA::Status::Unsupported, exactBytes, 0U, ESP_ERR_NOT_FOUND};
        }
        const auto maximum = static_cast<std::uint64_t>(candidate->size);
        if (exactBytes > maximum || exactBytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            return {OTA::Status::CapacityUnavailable, exactBytes, maximum, 0};
        }
        return {OTA::Status::Success, exactBytes, maximum, 0};
    }

    OTA::Result BeginApplicationImage(std::uint64_t exactBytes) noexcept {
        if (active_) return {OTA::Status::Busy, ESP_ERR_INVALID_STATE};
        const auto preflight = PreflightApplicationImage(exactBytes);
        if (!preflight) return {preflight.Code, preflight.NativeCode};

        target_ = esp_ota_get_next_update_partition(nullptr);
        if (target_ == nullptr) return {OTA::Status::Unsupported, ESP_ERR_NOT_FOUND};

        esp_ota_handle_t handle = 0;
        const auto result = esp_ota_begin(
            target_, static_cast<std::size_t>(exactBytes), &handle);
        if (result != ESP_OK) {
            target_ = nullptr;
            return OTADetail::MapResult(result);
        }

        handle_ = handle;
        expectedBytes_ = exactBytes;
        writtenBytes_ = 0U;
        active_ = true;
        return {OTA::Status::Success, 0};
    }

    OTA::WriteResult WriteApplicationImage(
        const std::uint8_t* data,
        std::size_t size) noexcept {
        if (!active_ || data == nullptr || size == 0U) {
            return {OTA::Status::Invalid, 0U, ESP_ERR_INVALID_ARG};
        }
        if (static_cast<std::uint64_t>(size) > expectedBytes_ - writtenBytes_) {
            return {OTA::Status::CapacityUnavailable, 0U, 0};
        }
        const auto result = esp_ota_write(handle_, data, size);
        if (result != ESP_OK) {
            const auto mapped = OTADetail::MapResult(result);
            return {mapped.Code, 0U, mapped.NativeCode};
        }
        writtenBytes_ += static_cast<std::uint64_t>(size);
        return {OTA::Status::Success, size, 0};
    }

    OTA::Result FinalizeApplicationImage() noexcept {
        if (!active_ || writtenBytes_ != expectedBytes_) {
            return {OTA::Status::Invalid, ESP_ERR_INVALID_STATE};
        }
        const auto result = esp_ota_end(handle_);
        if (result != ESP_OK) {
            ClearSession(true);
            return OTADetail::MapResult(result);
        }
        ClearSession(false);
        return {OTA::Status::Success, 0};
    }

    OTA::Result AbortApplicationImage() noexcept {
        if (!active_) {
            ClearSession(true);
            return {OTA::Status::Success, 0};
        }
        const auto result = esp_ota_abort(handle_);
        ClearSession(true);
        return OTADetail::MapResult(result);
    }

    OTA::BootTargetIdentifier ApplicationImageTarget() const noexcept {
        return OTADetail::TargetFor(target_);
    }
};

/** Portable boot-target inspection/selection backed by the ESP-IDF partition API. */
class OTABootControl final
    : public ProviderDeclaration<CapabilitySet<Capability::BootControl>> {
public:
    OTA::BootTargetIdentifier CurrentBootTarget() const noexcept {
        return OTADetail::TargetFor(esp_ota_get_running_partition());
    }

    OTA::BootTargetIdentifier CommittedBootTarget() const noexcept {
        // OTA reads this before candidate activation and durably retains the
        // previous target. At that boundary the ESP-IDF boot partition is the
        // committed target; later trial recovery uses the persisted identity.
        return OTADetail::TargetFor(esp_ota_get_boot_partition());
    }

    OTA::BootTargetIdentifier NextBootTarget() const noexcept {
        return OTADetail::TargetFor(esp_ota_get_boot_partition());
    }

    OTA::Result SelectNextBootTarget(OTA::BootTargetIdentifier target) noexcept {
        const auto* partition = OTADetail::FindApplicationTarget(target);
        if (partition == nullptr) return {OTA::Status::Invalid, ESP_ERR_NOT_FOUND};
        return OTADetail::MapResult(esp_ota_set_boot_partition(partition));
    }
};

/** ESP-IDF rollback/trial validity mechanics without taking restart ownership. */
class OTATrialBoot final
    : public ProviderDeclaration<CapabilitySet<Capability::TrialBoot>> {
public:
    bool IsCurrentBootTrial() const noexcept {
        const auto* running = esp_ota_get_running_partition();
        if (running == nullptr) return false;
        esp_ota_img_states_t state{};
        if (esp_ota_get_state_partition(running, &state) != ESP_OK) return false;
        return state == ESP_OTA_IMG_PENDING_VERIFY;
    }

    OTA::Result MarkCurrentBootValid() noexcept {
        return OTADetail::MapResult(esp_ota_mark_app_valid_cancel_rollback());
    }

    OTA::Result MarkCurrentBootInvalid() noexcept {
        // The non-rebooting IDF primitive is intentional: SystemRestart is
        // Coordinator-owned, so a TrialBoot provider must not restart itself.
        return OTADetail::MapResult(esp_ota_mark_app_invalid_rollback());
    }
};

/** Controlled system restart provider. The semantic reason remains bounded/portable. */
class OTASystemRestart final
    : public ProviderDeclaration<CapabilitySet<Capability::SystemRestart>> {
public:
    OTA::Result Restart(OTA::RestartReason reason) noexcept {
        (void)reason;
        esp_restart();
        // Real hardware normally does not return; native/fake backends do.
        return {OTA::Status::Success, 0};
    }
};

/**
 * Deterministic opaque partition-layout inspection.
 *
 * Layout identity is a backend-private FNV-1a digest over every partition
 * table entry's type/subtype/address/size/label. OTA sees only the resulting
 * opaque StorageLayoutIdentifier. AvailableBytes reports the current inactive
 * application OTA slot capacity because that is the directly stageable capacity
 * represented by this concrete provider.
 */
class OTAStorageLayoutInspection final
    : public ProviderDeclaration<CapabilitySet<Capability::StorageLayoutInspection>> {
public:
    OTA::Result InspectStorageLayout(OTA::StorageLayoutInfo& output) const noexcept {
        constexpr std::uint64_t offsetBasis = 14695981039346656037ULL;
        std::uint64_t hash = offsetBasis;
        std::uint64_t total = 0U;
        std::size_t count = 0U;

        auto iterator = esp_partition_find(
            ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
        while (iterator != nullptr) {
            const auto* partition = esp_partition_get(iterator);
            if (partition == nullptr) {
                esp_partition_iterator_release(iterator);
                return {OTA::Status::Failed, ESP_ERR_INVALID_STATE};
            }

            hash = OTADetail::FnvAppendByte(hash, static_cast<std::uint8_t>(partition->type));
            hash = OTADetail::FnvAppendByte(hash, static_cast<std::uint8_t>(partition->subtype));
            hash = OTADetail::FnvAppend32(hash, partition->address);
            hash = OTADetail::FnvAppend32(hash, static_cast<std::uint32_t>(partition->size));
            for (std::size_t i = 0U; i < sizeof(partition->label); ++i) {
                const auto byte = static_cast<std::uint8_t>(partition->label[i]);
                hash = OTADetail::FnvAppendByte(hash, byte);
                if (byte == 0U) break;
            }

            const auto size = static_cast<std::uint64_t>(partition->size);
            total = size > std::numeric_limits<std::uint64_t>::max() - total
                ? std::numeric_limits<std::uint64_t>::max()
                : total + size;
            ++count;
            iterator = esp_partition_next(iterator);
        }

        if (count == 0U) return {OTA::Status::Unsupported, ESP_ERR_NOT_FOUND};
        if (hash == 0U) hash = 1U;
        const auto* candidate = esp_ota_get_next_update_partition(nullptr);
        output.Layout = OTA::StorageLayoutIdentifier{hash};
        output.TotalBytes = total;
        output.AvailableBytes = candidate == nullptr
            ? 0U
            : static_cast<std::uint64_t>(candidate->size);
        return {OTA::Status::Success, 0};
    }
};

static_assert(OTA::IsApplicationImageStagingProviderV<OTAApplicationImageStaging>,
              "ESP-IDF application staging must satisfy the portable OTA contract");
static_assert(OTA::IsBootControlProviderV<OTABootControl>,
              "ESP-IDF boot control must satisfy the portable OTA contract");
static_assert(OTA::IsTrialBootProviderV<OTATrialBoot>,
              "ESP-IDF trial boot must satisfy the portable OTA contract");
static_assert(OTA::IsSystemRestartProviderV<OTASystemRestart>,
              "ESP-IDF restart must satisfy the portable OTA contract");
static_assert(OTA::IsStorageLayoutInspectionProviderV<OTAStorageLayoutInspection>,
              "ESP-IDF storage layout inspection must satisfy the portable OTA contract");

} // namespace ESPressio::Platform::IDF
