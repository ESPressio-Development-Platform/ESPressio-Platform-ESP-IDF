#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ESPressio_Platform_IDFOTA.hpp"

using namespace ESPressio::Platform;
using namespace ESPressio::Platform::IDF;

namespace {

std::array<esp_partition_t, 4> Partitions{};
const esp_partition_t* Running{nullptr};
const esp_partition_t* Boot{nullptr};
const esp_partition_t* Candidate{nullptr};
esp_ota_img_states_t RunningState{ESP_OTA_IMG_VALID};
bool OtaActive{false};
esp_ota_handle_t ActiveHandle{0U};
std::size_t ExpectedBytes{0U};
std::size_t WrittenBytes{0U};
std::size_t BeginCalls{0U};
std::size_t WriteCalls{0U};
std::size_t EndCalls{0U};
std::size_t AbortCalls{0U};
std::size_t SetBootCalls{0U};
std::size_t MarkValidCalls{0U};
std::size_t MarkInvalidCalls{0U};
std::size_t RestartCalls{0U};

bool Matches(const esp_partition_t& partition, esp_partition_type_t type,
             esp_partition_subtype_t subtype, const char* label) {
    if (type != ESP_PARTITION_TYPE_ANY && partition.type != type) return false;
    if (subtype != ESP_PARTITION_SUBTYPE_ANY && partition.subtype != subtype) return false;
    if (label != nullptr && std::strcmp(partition.label, label) != 0) return false;
    return true;
}

void SetPartition(std::size_t index, esp_partition_type_t type,
                  esp_partition_subtype_t subtype, std::uint32_t address,
                  std::size_t size, const char* label) {
    Partitions[index] = {};
    Partitions[index].type = type;
    Partitions[index].subtype = subtype;
    Partitions[index].address = address;
    Partitions[index].size = size;
    std::strncpy(Partitions[index].label, label, sizeof(Partitions[index].label) - 1U);
}

} // namespace

struct fake_esp_partition_iterator final {
    std::size_t index{0U};
    esp_partition_type_t type{ESP_PARTITION_TYPE_ANY};
    esp_partition_subtype_t subtype{ESP_PARTITION_SUBTYPE_ANY};
    const char* label{nullptr};
};

namespace {
fake_esp_partition_iterator Iterator{};

esp_partition_iterator_t Seek(std::size_t start) {
    for (std::size_t i = start; i < Partitions.size(); ++i) {
        if (Matches(Partitions[i], Iterator.type, Iterator.subtype, Iterator.label)) {
            Iterator.index = i;
            return &Iterator;
        }
    }
    return nullptr;
}
} // namespace

esp_partition_iterator_t esp_partition_find(
    esp_partition_type_t type,
    esp_partition_subtype_t subtype,
    const char* label) {
    Iterator = {0U, type, subtype, label};
    return Seek(0U);
}

const esp_partition_t* esp_partition_get(esp_partition_iterator_t iterator) {
    return iterator == nullptr || iterator->index >= Partitions.size()
        ? nullptr : &Partitions[iterator->index];
}

esp_partition_iterator_t esp_partition_next(esp_partition_iterator_t iterator) {
    return iterator == nullptr ? nullptr : Seek(iterator->index + 1U);
}

void esp_partition_iterator_release(esp_partition_iterator_t) {}

const esp_partition_t* esp_ota_get_running_partition() { return Running; }
const esp_partition_t* esp_ota_get_boot_partition() { return Boot; }
const esp_partition_t* esp_ota_get_next_update_partition(const esp_partition_t*) { return Candidate; }

esp_err_t esp_ota_begin(const esp_partition_t* partition, std::size_t image_size, esp_ota_handle_t* out_handle) {
    ++BeginCalls;
    if (OtaActive) return ESP_ERR_INVALID_STATE;
    if (partition == nullptr || out_handle == nullptr || image_size == 0U || image_size > partition->size) {
        return ESP_ERR_INVALID_ARG;
    }
    OtaActive = true;
    ActiveHandle = 0x1234U;
    ExpectedBytes = image_size;
    WrittenBytes = 0U;
    *out_handle = ActiveHandle;
    return ESP_OK;
}

esp_err_t esp_ota_write(esp_ota_handle_t handle, const void* data, std::size_t size) {
    ++WriteCalls;
    if (!OtaActive || handle != ActiveHandle || data == nullptr || size == 0U) return ESP_ERR_INVALID_ARG;
    if (size > ExpectedBytes - WrittenBytes) return ESP_ERR_INVALID_ARG;
    WrittenBytes += size;
    return ESP_OK;
}

esp_err_t esp_ota_end(esp_ota_handle_t handle) {
    ++EndCalls;
    if (!OtaActive || handle != ActiveHandle || WrittenBytes != ExpectedBytes) return ESP_ERR_INVALID_STATE;
    OtaActive = false;
    return ESP_OK;
}

esp_err_t esp_ota_abort(esp_ota_handle_t handle) {
    ++AbortCalls;
    if (!OtaActive || handle != ActiveHandle) return ESP_ERR_INVALID_ARG;
    OtaActive = false;
    return ESP_OK;
}

esp_err_t esp_ota_set_boot_partition(const esp_partition_t* partition) {
    ++SetBootCalls;
    if (partition == nullptr || partition->type != ESP_PARTITION_TYPE_APP) return ESP_ERR_INVALID_ARG;
    Boot = partition;
    return ESP_OK;
}

esp_err_t esp_ota_get_state_partition(const esp_partition_t* partition, esp_ota_img_states_t* state) {
    if (partition == nullptr || state == nullptr || partition != Running) return ESP_ERR_INVALID_ARG;
    *state = RunningState;
    return ESP_OK;
}

esp_err_t esp_ota_mark_app_valid_cancel_rollback() {
    ++MarkValidCalls;
    RunningState = ESP_OTA_IMG_VALID;
    return ESP_OK;
}

esp_err_t esp_ota_mark_app_invalid_rollback() {
    ++MarkInvalidCalls;
    RunningState = ESP_OTA_IMG_INVALID;
    return ESP_OK;
}

void esp_restart() { ++RestartCalls; }

int main() {
    static_assert(OTA::IsApplicationImageStagingProviderV<OTAApplicationImageStaging>);
    static_assert(OTA::IsBootControlProviderV<OTABootControl>);
    static_assert(OTA::IsTrialBootProviderV<OTATrialBoot>);
    static_assert(OTA::IsSystemRestartProviderV<OTASystemRestart>);
    static_assert(OTA::IsStorageLayoutInspectionProviderV<OTAStorageLayoutInspection>);
    static_assert(!OTA::IsFilesystemImageStagingProviderV<OTAApplicationImageStaging>);
    static_assert(!OTA::IsStorageLayoutTransitionProviderV<OTAStorageLayoutInspection>);

    SetPartition(0U, ESP_PARTITION_TYPE_DATA, 0x00U, 0x9000U, 0x2000U, "otadata");
    SetPartition(1U, ESP_PARTITION_TYPE_APP, 0x10U, 0x10000U, 0x100000U, "ota_0");
    SetPartition(2U, ESP_PARTITION_TYPE_APP, 0x11U, 0x110000U, 0x100000U, "ota_1");
    SetPartition(3U, ESP_PARTITION_TYPE_DATA, 0x82U, 0x210000U, 0x10000U, "nvs");
    Running = &Partitions[1];
    Boot = &Partitions[1];
    Candidate = &Partitions[2];

    OTAApplicationImageStaging staging;
    const auto preflight = staging.PreflightApplicationImage(6U);
    if (!preflight || preflight.RequiredBytes != 6U || preflight.MaximumBytes != Candidate->size) return 1;
    if (staging.PreflightApplicationImage(Candidate->size + 1U).Code != OTA::Status::CapacityUnavailable) return 2;
    if (!staging.BeginApplicationImage(6U) || staging.ApplicationImageTarget() != OTA::BootTargetIdentifier{Candidate->address}) return 3;
    const std::array<std::uint8_t, 6> bytes{1U, 2U, 3U, 4U, 5U, 6U};
    if (staging.WriteApplicationImage(bytes.data(), 2U).ConsumedBytes != 2U) return 4;
    if (staging.WriteApplicationImage(bytes.data() + 2U, 4U).ConsumedBytes != 4U) return 5;
    if (!staging.FinalizeApplicationImage() || BeginCalls != 1U || WriteCalls != 2U || EndCalls != 1U) return 6;
    if (Boot != Running || SetBootCalls != 0U) return 7;

    // Interrupted staging must abort without activating the candidate.
    if (!staging.BeginApplicationImage(4U)) return 8;
    if (staging.WriteApplicationImage(bytes.data(), 2U).ConsumedBytes != 2U) return 9;
    if (!staging.AbortApplicationImage() || AbortCalls != 1U || Boot != Running) return 10;
    if (staging.ApplicationImageTarget()) return 11;

    OTABootControl boot;
    if (boot.CurrentBootTarget() != OTA::BootTargetIdentifier{Running->address}) return 12;
    if (boot.CommittedBootTarget() != OTA::BootTargetIdentifier{Boot->address}) return 13;
    if (!boot.SelectNextBootTarget(OTA::BootTargetIdentifier{Candidate->address}) || Boot != Candidate) return 14;
    if (boot.NextBootTarget() != OTA::BootTargetIdentifier{Candidate->address}) return 15;
    if (boot.SelectNextBootTarget(OTA::BootTargetIdentifier{0xDEADBEEFU}).Code != OTA::Status::Invalid) return 16;

    OTATrialBoot trial;
    Running = Candidate;
    RunningState = ESP_OTA_IMG_PENDING_VERIFY;
    if (!trial.IsCurrentBootTrial()) return 17;
    if (!trial.MarkCurrentBootValid() || RunningState != ESP_OTA_IMG_VALID || MarkValidCalls != 1U) return 18;
    RunningState = ESP_OTA_IMG_PENDING_VERIFY;
    if (!trial.MarkCurrentBootInvalid() || RunningState != ESP_OTA_IMG_INVALID || MarkInvalidCalls != 1U) return 19;
    if (RestartCalls != 0U) return 20;

    OTASystemRestart restart;
    if (!restart.Restart(OTA::RestartReason::ActivateCandidate) || RestartCalls != 1U) return 21;

    OTAStorageLayoutInspection layout;
    OTA::StorageLayoutInfo first{};
    OTA::StorageLayoutInfo second{};
    if (!layout.InspectStorageLayout(first) || !layout.InspectStorageLayout(second)) return 22;
    if (!first.Layout || first.Layout != second.Layout) return 23;
    const std::uint64_t expectedTotal = 0x2000U + 0x100000U + 0x100000U + 0x10000U;
    if (first.TotalBytes != expectedTotal || first.AvailableBytes != Candidate->size) return 24;

    return 0;
}
