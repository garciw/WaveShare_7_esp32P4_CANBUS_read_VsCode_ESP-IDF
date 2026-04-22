
/*
#include "RadioManager.h"
#include "mp3dec.h" 

#include "ui.h" 
#include "nvs_flash.h" 
#include "nvs.h" 
#include "esp_random.h" 
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "bsp/esp-bsp.h" 

#define millis() (uint32_t)(esp_timer_get_time() / 1000)

// --- Hardware Audio Drivers ---
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h" 
#include "es8311.h" 
#include "esp_http_client.h"

static const char *TAG = "RADIO_HW";
bool isPlaying = false;
bool stationChanged = true;

// ==========================================
// 🔌 WAVESHARE P4 PIN MAPPING
// ==========================================
#define I2S_MCLK      GPIO_NUM_13
#define I2S_SCLK      GPIO_NUM_12
#define I2S_LRCK      GPIO_NUM_10
#define I2S_DOUT      GPIO_NUM_9    // Speaker OUT
#define PA_ENABLE_IO  GPIO_NUM_53

static i2s_chan_handle_t tx_handle = NULL;
static esp_codec_dev_handle_t codec_handle = NULL;

// ==========================================
// 📻 PLAYLIST SETUP
// ==========================================
LV_IMG_DECLARE(ui_img_pondends_png);
LV_IMG_DECLARE(ui_img_181fm_png);

struct Station {
    const char* name;
    const char* url;
    const lv_img_dsc_t* logo;
};

Station playlist[] = {    
    {"PONdENDS", "http://s7.voscast.com:7000/stream/1/", &ui_img_pondends_png},
    {"Groove Salad", "http://listen.181fm.com/181-groovesalad_128k.mp3", &ui_img_181fm_png},
    {"Awesome 80's", "http://listen.181fm.com/181-awesome80s_128k.mp3", &ui_img_181fm_png},
    {"Old School RnB", "http://listen.181fm.com/181-oldschool_128k.mp3", &ui_img_181fm_png},
    {"Soul", "http://listen.181fm.com/181-soul_128k.mp3", &ui_img_181fm_png},
    {"Jammin 181", "http://listen.181fm.com/181-jammin_128k.mp3", &ui_img_181fm_png},
    {"90's Dance", "http://listen.181fm.com/181-90sdance_128k.mp3", &ui_img_181fm_png},
    {"The Breeze", "http://listen.181fm.com/181-breeze_128k.mp3", &ui_img_181fm_png},
    {"Power-ExpliciT", "http://listen.181fm.com/181-powerexplicit_128k.mp3", &ui_img_181fm_png},
};

const int numStations = sizeof(playlist) / sizeof(playlist[0]);
int currentStation = 0;

// ==========================================
// 📊 UI WAVE ANIMATOR
// ==========================================
void wave_anim_timer(lv_timer_t * timer) {
    if (isPlaying) {
        lv_slider_set_value(ui_WaveVisualizer, (esp_random() % 81) + 10, LV_ANIM_ON);
    } else {
        lv_slider_set_value(ui_WaveVisualizer, 0, LV_ANIM_ON);
    }
}

// ==========================================
// 🎛️ HARDWARE INITIALIZATION
// ==========================================
void init_i2s_p4() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 1008; // P4's maximum safe hardware alignment
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK,
            .bclk = I2S_SCLK,
            .ws = I2S_LRCK,
            .dout = I2S_DOUT,
            .din = (gpio_num_t)-1, 
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);
}

void init_codec_p4() {
    gpio_config_t io_conf = {}; 
    io_conf.pin_bit_mask = (1ULL << PA_ENABLE_IO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(PA_ENABLE_IO, 1);   //1

    audio_codec_i2c_cfg_t i2c_cfg = {};
    i2c_cfg.port = 0;
    i2c_cfg.addr = ES8311_CODEC_DEFAULT_ADDR;
    i2c_cfg.bus_handle = bsp_i2c_get_handle(); 
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    audio_codec_i2s_cfg_t i2s_cfg = {};
    i2s_cfg.port = I2S_NUM_0;
    i2s_cfg.tx_handle = tx_handle; 
    const audio_codec_data_if_t *i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    es8311_cfg.ctrl_if = i2c_ctrl_if;
    es8311_cfg.hw_gain.pa_voltage = 5.0; 
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    esp_codec_dev_cfg_t codec_dev_cfg = {};
    codec_dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_OUT;
    codec_dev_cfg.codec_if = out_codec_if;
    codec_dev_cfg.data_if = i2s_data_if; 
    codec_handle = esp_codec_dev_new(&codec_dev_cfg);

    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = 44100;
    fs.channel = 2;
    fs.bits_per_sample = 16;
    esp_codec_dev_open(codec_handle, &fs);

    // 🔊 NEW: CUSTOM HARDWARE VOLUME CURVE (Compiler Fixed!)
    // Pushes the ES8311's internal hardware amplifier to +18dB to maximize clean volume!
    esp_codec_dev_vol_map_t custom_vol_map[] = {
        {0,   -96.0f}, // 0% UI Volume   = Muted
        {50,  -10.0f}, // 50% UI Volume  = -10 dB
        {100,  18.0f}  // 100% UI Volume = +18 dB (Massive Hardware Boost!)
    };
    
    esp_codec_dev_vol_curve_t vol_curve = {};
    vol_curve.vol_map = custom_vol_map;
    vol_curve.count = sizeof(custom_vol_map) / sizeof(esp_codec_dev_vol_map_t);
    
    esp_codec_dev_set_vol_curve(codec_handle, &vol_curve);
}

// ==========================================
// 📡 HTTP STREAMING & DECODING TASK (HELIX)
// ==========================================
void audio_stream_task(void *pvParameters) {
    ESP_LOGI(TAG, "Audio streaming task started.");
    
    // Setup Helix Decoder
    HMP3Decoder hMP3Decoder = MP3InitDecoder();
    if (!hMP3Decoder) {
        ESP_LOGE(TAG, "Failed to initialize Helix MP3 decoder!");
        vTaskDelete(NULL);
    }
    
    esp_http_client_config_t config = {};
    config.buffer_size = 4096; 
    config.timeout_ms = 5000;
    
    esp_http_client_handle_t client = NULL;
    
    const int STREAM_BUF_SIZE = 32768; 
    uint8_t *stream_buf = (uint8_t*)malloc(STREAM_BUF_SIZE);
    short *pcm_buf = (short*)malloc(4608 * 2); // Helix max buffer size
    
    int bytes_in_buf = 0;
    size_t bytes_written = 0;

    while (1) {
        if (isPlaying) {
            if (stationChanged) {
                if (client != NULL) {
                    esp_http_client_cleanup(client);
                    client = NULL;
                }
                
                config.url = playlist[currentStation].url;
                ESP_LOGI(TAG, "Attempting to connect to: %s", config.url);
                
                client = esp_http_client_init(&config);
                esp_http_client_set_header(client, "Icy-MetaData", "0"); // Still blocking text metadata
                
                if (esp_http_client_open(client, 0) == ESP_OK) {
                    esp_http_client_fetch_headers(client);
                    stationChanged = false;
                    bytes_in_buf = 0; 
                    ESP_LOGI(TAG, "Headers OK! Starting HELIX audio playback...");
                } else {
                    esp_http_client_cleanup(client);
                    client = NULL;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
            }
            
            if (!stationChanged && client != NULL) {
                int bytes_to_read = 4096; 
                if (STREAM_BUF_SIZE - bytes_in_buf < 4096) { 
                    bytes_to_read = STREAM_BUF_SIZE - bytes_in_buf;
                }

                int read_len = esp_http_client_read(client, (char *)stream_buf + bytes_in_buf, bytes_to_read);
                
                if (read_len > 0) {
                    bytes_in_buf += read_len;
                    int buf_ptr = 0;
                    
                    // HELIX DECODING LOOP
                    while (bytes_in_buf > 0) {
                        int offset = MP3FindSyncWord(stream_buf + buf_ptr, bytes_in_buf);
                        if (offset < 0) {
                            bytes_in_buf = 0; // Dump buffer, wait for next frame
                            break;
                        }
                        
                        buf_ptr += offset;
                        bytes_in_buf -= offset;
                        
                        unsigned char *read_ptr = stream_buf + buf_ptr;
                        int bytes_left = bytes_in_buf;
                        
                        int err = MP3Decode(hMP3Decoder, &read_ptr, &bytes_left, pcm_buf, 0);
                        
                        if (err == ERR_MP3_NONE) {
                            MP3FrameInfo info;
                            MP3GetLastFrameInfo(hMP3Decoder, &info);
                            
                            if (info.samprate > 0 && info.outputSamps > 0) {
                                i2s_channel_write(tx_handle, pcm_buf, info.outputSamps * sizeof(short), &bytes_written, portMAX_DELAY);
                            }
                            
                            int bytes_consumed = bytes_in_buf - bytes_left;
                            buf_ptr += bytes_consumed;
                            bytes_in_buf = bytes_left;
                            
                        } else if (err == ERR_MP3_INDATA_UNDERFLOW) {
                            break; // Wait for more internet data
                        } else {
                            // Bad frame, skip ahead
                            buf_ptr++;
                            bytes_in_buf--;
                        }
                    }
                    
                    if (bytes_in_buf > 0 && buf_ptr > 0) {
                        memmove(stream_buf, stream_buf + buf_ptr, bytes_in_buf);
                    }
                    
                } else if (read_len < 0) {
                    stationChanged = true; 
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
            }
        } else {
            if (client != NULL) {
                esp_http_client_cleanup(client);
                client = NULL;
                stationChanged = true; 
            }
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    
    free(stream_buf);
    free(pcm_buf);
    MP3FreeDecoder(hMP3Decoder);
    vTaskDelete(NULL);
}

// ==========================================
// 🎛️ UI INTERFACE FUNCTIONS
// ==========================================
void update_radio_ui() {
    lv_label_set_text(ui_LabelStation, playlist[currentStation].name);
    if (playlist[currentStation].logo != NULL) {
        lv_img_set_src(ui_ImageLogo, playlist[currentStation].logo);
    }
    if(isPlaying) lv_label_set_text(ui_LabelTrack, "Playing..."); 
}

void setupRadio() {
    ESP_LOGI(TAG, "Initializing Audio Hardware...");
    
    init_i2s_p4();
    init_codec_p4();

    lv_timer_create(wave_anim_timer, 100, NULL);

nvs_handle_t my_handle;
    if (nvs_open("radio", NVS_READWRITE, &my_handle) == ESP_OK) {
        int32_t tempVol = 100; 
        nvs_get_i32(my_handle, "volume", &tempVol);
        nvs_close(my_handle);
    }

    // 🛑 FIX 2: Ignore memory temporarily and FORCE the hardware to absolute maximum
    if(codec_handle) {
        esp_codec_dev_set_out_mute(codec_handle, false); 
        esp_codec_dev_set_out_vol(codec_handle, 100); // HARDCODED TO 100!
    }
    lv_arc_set_value(ui_ArcVolume, 100);

    update_radio_ui();
    xTaskCreatePinnedToCore(audio_stream_task, "Audio_Stream", 32768, NULL, 2, NULL, 1);
}


void nextRadioStation() { 
    currentStation++;
    if (currentStation >= numStations) currentStation = 0; 
    stationChanged = true;
    lv_label_set_text(ui_LabelTrack, "Loading...");
    update_radio_ui();
}

void prevRadioStation() { 
    currentStation--;
    if (currentStation < 0) currentStation = numStations - 1; 
    stationChanged = true;
    lv_label_set_text(ui_LabelTrack, "Loading...");
    update_radio_ui();
}

void togglePlayPause() {
    isPlaying = !isPlaying;
    if(isPlaying) {
        gpio_set_level(PA_ENABLE_IO, 1);     //1
        stationChanged = true; 
        lv_label_set_text(ui_LabelTrack, "Connecting...");
    } else {
        gpio_set_level(PA_ENABLE_IO, 0);     //0
        lv_label_set_text(ui_LabelTrack, "Paused");
    }
}

void setRadioVolume(uint8_t vol) {
    if (codec_handle) {
		esp_codec_dev_set_out_vol(codec_handle,(float)vol);
		// 🛑 FORCE UNMUTE: Stop the ES8311 from panic-muting when adjusted!
        esp_codec_dev_set_out_mute(codec_handle, false); 
    }
}
///############################################  new code #########################################################

#include "RadioManager.h"
#include "mp3dec.h" 

#include "ui.h" 
#include "nvs_flash.h" 
#include "nvs.h" 
#include "esp_random.h" 
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "bsp/esp-bsp.h" 

#define millis() (uint32_t)(esp_timer_get_time() / 1000)

// --- Hardware Audio Drivers ---
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h" 
#include "es8311.h" 
#include "esp_http_client.h"

static const char *TAG = "RADIO_HW";
bool isPlaying = false;
bool stationChanged = true;

// ==========================================
// 🔌 WAVESHARE P4 PIN MAPPING
// ==========================================
#define I2S_MCLK      GPIO_NUM_13
#define I2S_SCLK      GPIO_NUM_12
#define I2S_LRCK      GPIO_NUM_10
#define I2S_DOUT      GPIO_NUM_9    // Speaker OUT
#define PA_ENABLE_IO  GPIO_NUM_53

static i2s_chan_handle_t tx_handle = NULL;
static esp_codec_dev_handle_t codec_handle = NULL;

// ==========================================
// 📻 PLAYLIST SETUP (Your Custom List!)
// ==========================================
LV_IMG_DECLARE(ui_img_pondends_png);
LV_IMG_DECLARE(ui_img_181fm_png);

struct Station {
    const char* name;
    const char* url;
    const lv_img_dsc_t* logo;
};

Station playlist[] = {    
    {"PONdENDS", "http://s7.voscast.com:7000/stream/1/", &ui_img_pondends_png},
    {"The Beat (HipHop)", "http://listen.181fm.com/181-beat_128k.mp3", &ui_img_181fm_png},
    {"Awesome 80's", "http://listen.181fm.com/181-awesome80s_128k.mp3", &ui_img_181fm_png},
    {"Old School RnB", "http://listen.181fm.com/181-oldschool_128k.mp3", &ui_img_181fm_png},
    {"Soul", "http://listen.181fm.com/181-soul_128k.mp3", &ui_img_181fm_png},
    {"True R&B", "http://listen.181fm.com/181-true_128k.mp3", &ui_img_181fm_png},
    {"Power (Top 40)", "http://listen.181fm.com/181-power_128k.mp3", &ui_img_181fm_png},
    {"Jammin 181", "http://listen.181fm.com/181-jammin_128k.mp3", &ui_img_181fm_png},
    {"90's Dance", "http://listen.181fm.com/181-90sdance_128k.mp3", &ui_img_181fm_png},
    {"The Breeze", "http://listen.181fm.com/181-breeze_128k.mp3", &ui_img_181fm_png},
    {"Power-ExpliciT", "http://listen.181fm.com/181-powerexplicit_128k.mp3", &ui_img_181fm_png},
};

const int numStations = sizeof(playlist) / sizeof(playlist[0]);
int currentStation = 0;

// ==========================================
// 📊 UI WAVE ANIMATOR
// ==========================================
void wave_anim_timer(lv_timer_t * timer) {
    if (isPlaying) {
        lv_slider_set_value(ui_WaveVisualizer, (esp_random() % 81) + 10, LV_ANIM_ON);
    } else {
        lv_slider_set_value(ui_WaveVisualizer, 0, LV_ANIM_ON);
    }
}

// ==========================================
// 🎛️ HARDWARE INITIALIZATION
// ==========================================
void init_i2s_p4() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 16; // 🟢 FIX 1: Doubled from 8 to 16 for a massive hardware bucket!
    chan_cfg.dma_frame_num = 1008; 
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK,
            .bclk = I2S_SCLK,
            .ws = I2S_LRCK,
            .dout = I2S_DOUT,
            .din = (gpio_num_t)-1, 
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);
}

void init_codec_p4() {
    gpio_config_t io_conf = {}; 
    io_conf.pin_bit_mask = (1ULL << PA_ENABLE_IO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    
    gpio_set_level(PA_ENABLE_IO, 1);   // Default to AMP ON

    audio_codec_i2c_cfg_t i2c_cfg = {};
    i2c_cfg.port = 0;
    i2c_cfg.addr = ES8311_CODEC_DEFAULT_ADDR;
    i2c_cfg.bus_handle = bsp_i2c_get_handle(); 
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    audio_codec_i2s_cfg_t i2s_cfg = {};
    i2s_cfg.port = I2S_NUM_0;
    i2s_cfg.tx_handle = tx_handle; 
    const audio_codec_data_if_t *i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    es8311_cfg.ctrl_if = i2c_ctrl_if;
    es8311_cfg.hw_gain.pa_voltage = 5.0; 
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    esp_codec_dev_cfg_t codec_dev_cfg = {};
    codec_dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_OUT;
    codec_dev_cfg.codec_if = out_codec_if;
    codec_dev_cfg.data_if = i2s_data_if; 
    codec_handle = esp_codec_dev_new(&codec_dev_cfg);

    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = 44100;
    fs.channel = 2;
    fs.bits_per_sample = 16;
    esp_codec_dev_open(codec_handle, &fs);

    // 🔊 CUSTOM HARDWARE VOLUME CURVE (Your updated clean curve)
    esp_codec_dev_vol_map_t custom_vol_map[] = {
        {0,   -96.0f}, // Muted
        {30,  -15.0f},
        {50,  -10.0f}, // Half volume
        {80,   8.0f},  // Max safe hardware boost
        {100,  18.0f}  
    };
    
    esp_codec_dev_vol_curve_t vol_curve = {};
    vol_curve.vol_map = custom_vol_map;
    vol_curve.count = sizeof(custom_vol_map) / sizeof(esp_codec_dev_vol_map_t);
    
    esp_codec_dev_set_vol_curve(codec_handle, &vol_curve);
}

// ==========================================
// 📡 HTTP STREAMING & DECODING TASK
// ==========================================
void audio_stream_task(void *pvParameters) {
    ESP_LOGI(TAG, "Audio streaming task started.");
    
    HMP3Decoder hMP3Decoder = MP3InitDecoder();
    if (!hMP3Decoder) vTaskDelete(NULL);
    
    esp_http_client_config_t config = {};
    config.buffer_size = 16384; // 🟢 FIX 2: Increased from 4096. Let the Wi-Fi chip handle the buffering!
    config.timeout_ms = 5000;
    
    esp_http_client_handle_t client = NULL;
    
    const int STREAM_BUF_SIZE = 32768; 
    uint8_t *stream_buf = (uint8_t*)malloc(STREAM_BUF_SIZE);
    short *pcm_buf = (short*)malloc(4608 * 2); 
    
    int bytes_in_buf = 0;
    size_t bytes_written = 0;
    
    // 🛑 NUKED THE BROKEN `is_prebuffering` VARIABLE!

    while (1) {
        if (isPlaying) {
            if (stationChanged) {
                if (client != NULL) {
                    esp_http_client_cleanup(client);
                    client = NULL;
                }
                
                // I2S PURGE: Instantly kill the old station's audio buffer
                i2s_channel_disable(tx_handle);
                i2s_channel_enable(tx_handle);
                
                config.url = playlist[currentStation].url;
                ESP_LOGI(TAG, "Connecting to: %s", config.url);
                
                client = esp_http_client_init(&config);
                esp_http_client_set_header(client, "Icy-MetaData", "0"); 
                
                if (esp_http_client_open(client, 0) == ESP_OK) {
                    esp_http_client_fetch_headers(client);
                    stationChanged = false;
                    bytes_in_buf = 0; 
                } else {
                    esp_http_client_cleanup(client);
                    client = NULL;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
            }
            
            if (!stationChanged && client != NULL) {
                int bytes_to_read = 4096; 
                if (STREAM_BUF_SIZE - bytes_in_buf < 4096) { 
                    bytes_to_read = STREAM_BUF_SIZE - bytes_in_buf;
                }

                int read_len = esp_http_client_read(client, (char *)stream_buf + bytes_in_buf, bytes_to_read);
                
                if (read_len > 0) {
                    bytes_in_buf += read_len;
                    int buf_ptr = 0;
                    
                    // 🛑 NUKED THE BROKEN PRE-BUFFERING `if` STATEMENT BLOCK!
                    
                    while (bytes_in_buf > 0) {
                        int offset = MP3FindSyncWord(stream_buf + buf_ptr, bytes_in_buf);
                        if (offset < 0) {
                            bytes_in_buf = 0; 
                            break;
                        }
                        
                        buf_ptr += offset;
                        bytes_in_buf -= offset;
                        
                        unsigned char *read_ptr = stream_buf + buf_ptr;
                        int bytes_left = bytes_in_buf;
                        
                        int err = MP3Decode(hMP3Decoder, &read_ptr, &bytes_left, pcm_buf, 0);
                        
                        if (err == ERR_MP3_NONE) {
                            MP3FrameInfo info;
                            MP3GetLastFrameInfo(hMP3Decoder, &info);
                            
                            if (info.samprate > 0 && info.outputSamps > 0) {
                                i2s_channel_write(tx_handle, pcm_buf, info.outputSamps * sizeof(short), &bytes_written, portMAX_DELAY);
                            }
                            
                            int bytes_consumed = bytes_in_buf - bytes_left;
                            buf_ptr += bytes_consumed;
                            bytes_in_buf = bytes_left;
                            
                        } else if (err == ERR_MP3_INDATA_UNDERFLOW) {
                            break; 
                        } else {
                            buf_ptr++;
                            bytes_in_buf--;
                        }
                    }
                    
                    if (bytes_in_buf > 0 && buf_ptr > 0) {
                        memmove(stream_buf, stream_buf + buf_ptr, bytes_in_buf);
                    }
                    
                    // 🛑 NUKED THE BROKEN "STARVATION PROTECTION" `memset` LOOP!
                    
                } else if (read_len < 0) {
                    stationChanged = true; 
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
            }
        } else {
            if (client != NULL) {
                esp_http_client_cleanup(client);
                client = NULL;
                stationChanged = true; 
            }
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    
    free(stream_buf);
    free(pcm_buf);
    MP3FreeDecoder(hMP3Decoder);
    vTaskDelete(NULL);
}

// ==========================================
// 🎛️ UI INTERFACE FUNCTIONS
// ==========================================
void update_radio_ui() {
    lv_label_set_text(ui_LabelStation, playlist[currentStation].name);
    if (playlist[currentStation].logo != NULL) {
        lv_img_set_src(ui_ImageLogo, playlist[currentStation].logo);
    }
    if(isPlaying) lv_label_set_text(ui_LabelTrack, "Playing..."); 
}

void setupRadio() {
    ESP_LOGI(TAG, "Initializing Audio Hardware...");
    
    init_i2s_p4();
    init_codec_p4();
    lv_timer_create(wave_anim_timer, 100, NULL);

    // 🟢 Your custom boot volume!
    int boot_volume = 30;
    if(codec_handle) {
        esp_codec_dev_set_out_mute(codec_handle, false); 
        esp_codec_dev_set_out_vol(codec_handle, (float)boot_volume); 
    }
    lv_arc_set_value(ui_ArcVolume, boot_volume);

    update_radio_ui();
    
    xTaskCreatePinnedToCore(audio_stream_task, "Audio_Stream", 32768, NULL, 5, NULL, 1);
}

void nextRadioStation() { 
    currentStation++;
    if (currentStation >= numStations) currentStation = 0; 
    stationChanged = true;
    lv_label_set_text(ui_LabelTrack, "Loading...");
    update_radio_ui();
}

void prevRadioStation() { 
    currentStation--;
    if (currentStation < 0) currentStation = numStations - 1; 
    stationChanged = true;
    lv_label_set_text(ui_LabelTrack, "Loading...");
    update_radio_ui();
}

void togglePlayPause() {
    isPlaying = !isPlaying;
    if(isPlaying) {
        gpio_set_level(PA_ENABLE_IO, 1); // 1 = ON
        stationChanged = true; 
        lv_label_set_text(ui_LabelTrack, "Connecting...");
    } else {
        gpio_set_level(PA_ENABLE_IO, 0); // 0 = OFF
        lv_label_set_text(ui_LabelTrack, "Paused");
    }
}

void setRadioVolume(uint8_t vol) {
    if (codec_handle) {
        esp_codec_dev_set_out_vol(codec_handle, (float)vol);
        esp_codec_dev_set_out_mute(codec_handle, false); 
    }
}
*/

#include "RadioManager.h"
#include "mp3dec.h" 

#include "ui.h" 
#include "nvs_flash.h" 
#include "nvs.h" 
#include "esp_random.h" 
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "bsp/esp-bsp.h" 

#define millis() (uint32_t)(esp_timer_get_time() / 1000)

// --- Hardware Audio Drivers ---
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h" 
#include "es8311.h" 
#include "esp_http_client.h"

static const char *TAG = "RADIO_HW";
bool isPlaying = false;
bool stationChanged = true;

// ==========================================
// 🔌 WAVESHARE P4 PIN MAPPING
// ==========================================
#define I2S_MCLK      GPIO_NUM_13
#define I2S_SCLK      GPIO_NUM_12
#define I2S_LRCK      GPIO_NUM_10
#define I2S_DOUT      GPIO_NUM_9    // Speaker OUT
#define PA_ENABLE_IO  GPIO_NUM_53

static i2s_chan_handle_t tx_handle = NULL;
static esp_codec_dev_handle_t codec_handle = NULL;

// ==========================================
// 📻 PLAYLIST SETUP
// ==========================================
LV_IMG_DECLARE(ui_img_pondends_png);
LV_IMG_DECLARE(ui_img_181fm_png);

struct Station {
    const char* name;
    const char* url;
    const lv_img_dsc_t* logo;
};

Station playlist[] = {    
    {"PONdENDS", "http://s7.voscast.com:7000/stream/1/", &ui_img_pondends_png},
    {"The Beat (HipHop)", "http://listen.181fm.com/181-beat_128k.mp3", &ui_img_181fm_png},
    {"Awesome 80's", "http://listen.181fm.com/181-awesome80s_128k.mp3", &ui_img_181fm_png},
    {"Old School RnB", "http://listen.181fm.com/181-oldschool_128k.mp3", &ui_img_181fm_png},
    {"Soul", "http://listen.181fm.com/181-soul_128k.mp3", &ui_img_181fm_png},
    {"Power (Top 40)", "http://listen.181fm.com/181-power_128k.mp3", &ui_img_181fm_png},
    {"Jammin 181", "http://listen.181fm.com/181-jammin_128k.mp3", &ui_img_181fm_png},
    {"90's Dance", "http://listen.181fm.com/181-90sdance_128k.mp3", &ui_img_181fm_png},
    {"The Breeze", "http://listen.181fm.com/181-breeze_128k.mp3", &ui_img_181fm_png},
    {"Power-ExpliciT", "http://listen.181fm.com/181-powerexplicit_128k.mp3", &ui_img_181fm_png},
};

const int numStations = sizeof(playlist) / sizeof(playlist[0]);
int currentStation = 0;

// ==========================================
// 📊 UI WAVE ANIMATOR
// ==========================================
void wave_anim_timer(lv_timer_t * timer) {
    if (isPlaying) {
        lv_slider_set_value(ui_WaveVisualizer, (esp_random() % 81) + 10, LV_ANIM_ON);
    } else {
        lv_slider_set_value(ui_WaveVisualizer, 0, LV_ANIM_ON);
    }
}

// ==========================================
// 🎛️ HARDWARE INITIALIZATION
// ==========================================
void init_i2s_p4() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 16; 
    chan_cfg.dma_frame_num = 1008; 
    
    // 🟢 THE SILENCER FLAG: If Wi-Fi lags, play clean silence instead of machine-gun stuttering!
    chan_cfg.auto_clear = true; 
    
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_MCLK,
            .bclk = I2S_SCLK,
            .ws = I2S_LRCK,
            .dout = I2S_DOUT,
            .din = (gpio_num_t)-1, 
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);
}

void init_codec_p4() {
    gpio_config_t io_conf = {}; 
    io_conf.pin_bit_mask = (1ULL << PA_ENABLE_IO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    
    gpio_set_level(PA_ENABLE_IO, 1);   // Default to AMP ON

    audio_codec_i2c_cfg_t i2c_cfg = {};
    i2c_cfg.port = 0;
    i2c_cfg.addr = ES8311_CODEC_DEFAULT_ADDR;
    i2c_cfg.bus_handle = bsp_i2c_get_handle(); 
    const audio_codec_ctrl_if_t *i2c_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    audio_codec_i2s_cfg_t i2s_cfg = {};
    i2s_cfg.port = I2S_NUM_0;
    i2s_cfg.tx_handle = tx_handle; 
    const audio_codec_data_if_t *i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    es8311_cfg.ctrl_if = i2c_ctrl_if;
    es8311_cfg.hw_gain.pa_voltage = 5.0; 
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    esp_codec_dev_cfg_t codec_dev_cfg = {};
    codec_dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_OUT;
    codec_dev_cfg.codec_if = out_codec_if;
    codec_dev_cfg.data_if = i2s_data_if; 
    codec_handle = esp_codec_dev_new(&codec_dev_cfg);

    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = 44100;
    fs.channel = 2;
    fs.bits_per_sample = 16;
    esp_codec_dev_open(codec_handle, &fs);

    // 🔊 CUSTOM HARDWARE VOLUME CURVE
    esp_codec_dev_vol_map_t custom_vol_map[] = {
        {0,   -96.0f}, // Muted
        {30,  -15.0f},
        {50,  -10.0f}, // Half volume
        {80,   8.0f},  // Max safe hardware boost
        {100,  18.0f}  
    };
    
    esp_codec_dev_vol_curve_t vol_curve = {};
    vol_curve.vol_map = custom_vol_map;
    vol_curve.count = sizeof(custom_vol_map) / sizeof(esp_codec_dev_vol_map_t);
    
    esp_codec_dev_set_vol_curve(codec_handle, &vol_curve);
}

// ==========================================
// 📡 HTTP STREAMING & DECODING TASK
// ==========================================
void audio_stream_task(void *pvParameters) {
    ESP_LOGI(TAG, "Audio streaming task started.");
    
    HMP3Decoder hMP3Decoder = MP3InitDecoder();
    if (!hMP3Decoder) vTaskDelete(NULL);
    
    esp_http_client_config_t config = {};
    config.buffer_size = 32768; // Let LwIP hold plenty of data in the background
    config.timeout_ms = 5000;
    
    esp_http_client_handle_t client = NULL;
    
    const int STREAM_BUF_SIZE = 32768; 
    uint8_t *stream_buf = (uint8_t*)malloc(STREAM_BUF_SIZE);
    short *pcm_buf = (short*)malloc(4608 * 2); 
    
    int bytes_in_buf = 0;
    size_t bytes_written = 0;
    bool is_muted = false; 

    while (1) {
        if (isPlaying) {
            if (stationChanged) {
                if (codec_handle && !is_muted) {
                    esp_codec_dev_set_out_mute(codec_handle, true);
                    is_muted = true;
                }
                
                if (client != NULL) {
                    esp_http_client_cleanup(client);
                    client = NULL;
                }
                
                // Flush the DMA hardware so the old song doesn't repeat!
                i2s_channel_disable(tx_handle);
                i2s_channel_enable(tx_handle);
                
                config.url = playlist[currentStation].url;
                ESP_LOGI(TAG, "Connecting to: %s", config.url);
                
                client = esp_http_client_init(&config);
                esp_http_client_set_header(client, "Icy-MetaData", "0"); 
                
                if (esp_http_client_open(client, 0) == ESP_OK) {
                    esp_http_client_fetch_headers(client);
                    stationChanged = false;
                    bytes_in_buf = 0; 
                } else {
                    esp_http_client_cleanup(client);
                    client = NULL;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
            }
            
            if (!stationChanged && client != NULL) {
                
                // 🟢 FAST SIPS: Read 2KB at a time so the task never freezes!
                int bytes_to_read = 2048;        
                if (STREAM_BUF_SIZE - bytes_in_buf < 2048) {   
                    bytes_to_read = STREAM_BUF_SIZE - bytes_in_buf;
                }

                int read_len = esp_http_client_read(client, (char *)stream_buf + bytes_in_buf, bytes_to_read);
                
                if (read_len > 0) {
                    bytes_in_buf += read_len;
                    int buf_ptr = 0;
                    
                    while (bytes_in_buf > 0) {
                        int offset = MP3FindSyncWord(stream_buf + buf_ptr, bytes_in_buf);
                        if (offset < 0) {
                            bytes_in_buf = 0; 
                            break;
                        }
                        
                        buf_ptr += offset;
                        bytes_in_buf -= offset;
                        
                        unsigned char *read_ptr = stream_buf + buf_ptr;
                        int bytes_left = bytes_in_buf;
                        
                        int err = MP3Decode(hMP3Decoder, &read_ptr, &bytes_left, pcm_buf, 0);
                        
                        if (err == ERR_MP3_NONE) {
                            MP3FrameInfo info;
                            MP3GetLastFrameInfo(hMP3Decoder, &info);
                            
                            if (info.samprate > 0 && info.outputSamps > 0) {
                                if (is_muted && codec_handle) {
                                    esp_codec_dev_set_out_mute(codec_handle, false);
                                    is_muted = false;
                                }
                                
                                i2s_channel_write(tx_handle, pcm_buf, info.outputSamps * sizeof(short), &bytes_written, portMAX_DELAY);
                            }
                            
                            int bytes_consumed = bytes_in_buf - bytes_left;
                            buf_ptr += bytes_consumed;
                            bytes_in_buf = bytes_left;
                            
                        } else if (err == ERR_MP3_INDATA_UNDERFLOW) {
                            break; 
                        } else {
                            buf_ptr++;
                            bytes_in_buf--;
                        }
                    }
                    
                    if (bytes_in_buf > 0 && buf_ptr > 0) {
                        memmove(stream_buf, stream_buf + buf_ptr, bytes_in_buf);
                    }
                    
                } else if (read_len <= 0) { 
                    if (codec_handle && !is_muted) {
                        esp_codec_dev_set_out_mute(codec_handle, true);
                        is_muted = true;
                    }
                    
                    if (read_len < 0) {
                        stationChanged = true; 
                        vTaskDelay(pdMS_TO_TICKS(2000));
                    } else {
                        vTaskDelay(pdMS_TO_TICKS(10)); 
                    }
                }
            }
        } else {
            if (client != NULL) {
                esp_http_client_cleanup(client);
                client = NULL;
                stationChanged = true; 
            }
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
    
    free(stream_buf);
    free(pcm_buf);
    MP3FreeDecoder(hMP3Decoder);
    vTaskDelete(NULL);
}

// ==========================================
// 🎛️ UI INTERFACE FUNCTIONS
// ==========================================
void update_radio_ui() {
    lv_label_set_text(ui_LabelStation, playlist[currentStation].name);
    if (playlist[currentStation].logo != NULL) {
        lv_img_set_src(ui_ImageLogo, playlist[currentStation].logo);
    }
    if(isPlaying) lv_label_set_text(ui_LabelTrack, "Playing..."); 
}

void setupRadio() {
    ESP_LOGI(TAG, "Initializing Audio Hardware...");
    
    init_i2s_p4();
    init_codec_p4();
    lv_timer_create(wave_anim_timer, 100, NULL);

    int boot_volume = 30;
    if(codec_handle) {
        esp_codec_dev_set_out_mute(codec_handle, false); 
        esp_codec_dev_set_out_vol(codec_handle, (float)boot_volume); 
    }
    lv_arc_set_value(ui_ArcVolume, boot_volume);

    update_radio_ui();
    
    xTaskCreatePinnedToCore(audio_stream_task, "Audio_Stream", 32768, NULL, 5, NULL, 1);
}

void nextRadioStation() { 
    currentStation++;
    if (currentStation >= numStations) currentStation = 0; 
    stationChanged = true;
    lv_label_set_text(ui_LabelTrack, "Loading...");
    update_radio_ui();
}

void prevRadioStation() { 
    currentStation--;
    if (currentStation < 0) currentStation = numStations - 1; 
    stationChanged = true;
    lv_label_set_text(ui_LabelTrack, "Loading...");
    update_radio_ui();
}

void togglePlayPause() {
    isPlaying = !isPlaying;
    if(isPlaying) {
        gpio_set_level(PA_ENABLE_IO, 1); // 1 = ON
        stationChanged = true; 
        lv_label_set_text(ui_LabelTrack, "Connecting...");
    } else {
        gpio_set_level(PA_ENABLE_IO, 0); // 0 = OFF
        lv_label_set_text(ui_LabelTrack, "Paused");
    }
}

void setRadioVolume(uint8_t vol) {
    if (codec_handle) {
        esp_codec_dev_set_out_vol(codec_handle, (float)vol);
        esp_codec_dev_set_out_mute(codec_handle, false); 
    }
}