#include "RadioManager.h"
#include "ui.h" 
#include "nvs_flash.h" 
#include "nvs.h" 
#include "esp_random.h" 
#include "esp_log.h"

// --- STUBBED DATA ---
static const char *TAG = "RADIO";
bool isPlaying = false;
int currentStation = 0;
bool stationChanged = false;

void setupRadio() {
    ESP_LOGI(TAG, "Radio UI Initialized (Audio Hardware Pending)");
    
    // Initialize NVS for volume/station memory
    nvs_handle_t my_handle;
    if (nvs_open("radio", NVS_READWRITE, &my_handle) == ESP_OK) {
        int32_t tempVol = 5;
        nvs_get_i32(my_handle, "volume", &tempVol);
        lv_arc_set_value(ui_ArcVolume, (int)tempVol);
        nvs_close(my_handle);
    }

    // Set UI to a "Ready" state
    lv_label_set_text(ui_LabelStation, "Radio Ready");
    lv_label_set_text(ui_LabelTrack, "Awaiting Wi-Fi Bridge");
}

void nextRadioStation() {
    ESP_LOGI(TAG, "Next Station clicked");
    // UI Update only for now
    lv_label_set_text(ui_LabelTrack, "Switching...");
}

void prevRadioStation() {
    ESP_LOGI(TAG, "Prev Station clicked");
}

void togglePlayPause() {
    isPlaying = !isPlaying;
    ESP_LOGI(TAG, "Play/Pause: %s", isPlaying ? "PLAY" : "PAUSE");
}

void setRadioVolume(uint8_t vol) {
    // Save volume to NVS so it persists across car restarts
    nvs_handle_t my_handle;
    if (nvs_open("radio", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_set_i32(my_handle, "volume", (int32_t)vol);
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}

// Visualizer Timer - Pulses the UI even without audio for demo
void wave_anim_timer(lv_timer_t * timer) {
    if (isPlaying) {
        lv_slider_set_value(ui_WaveVisualizer, (esp_random() % 81) + 10, LV_ANIM_ON);
    } else {
        lv_slider_set_value(ui_WaveVisualizer, 0, LV_ANIM_ON);
    }
}