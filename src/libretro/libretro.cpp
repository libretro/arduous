#include "libretro/libretro.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "arduous/arduous.h"
#include "sim_elf.h"

constexpr int FRAME_WIDTH = 128;
constexpr int FRAME_HEIGHT = 64;
constexpr float FRAME_ASPECT = 2.0f;

#undef USE_RGB565
#define USE_RGB565 1

#ifdef USE_RGB565
typedef uint16_t pixel_t;

static inline pixel_t arduous_rgb(uint8_t r, uint8_t g, uint8_t b) {
#ifdef PS2
    return ((b & 0xf8) << 7) | ((g & 0xf8) << 2) | ((r & 0xf8) >> 3);
#else
    return ((r >> 3U) << 11U) | ((g >> 2U) << 5U) | ((b >> 3U) << 0U);
#endif
}
#else
typedef uint32_t pixel_t;

static inline pixel_t arduous_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16U) | (g << 8U) | (b);
}
#endif

static pixel_t fb[FRAME_WIDTH * FRAME_HEIGHT];
static int16_t audio_buffer[TIMING_SAMPLE_RATE / TIMING_FPS * 2];

static inline pixel_t arduous_gray(uint8_t y) {
    return arduous_rgb(y, y, y);
}

static void fallback_log(enum retro_log_level level, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
std::unique_ptr<Arduous> arduous;

void update_video() {
    memset(fb, 0, sizeof(pixel_t) * FRAME_WIDTH * FRAME_HEIGHT);
    auto bit_fb = arduous->getVideoFrameBuffer();
    std::pair<uint8_t, uint8_t> cols = arduous->getVideoColors();
    pixel_t fg = arduous_gray(cols.first), bg = arduous_gray(cols.second);
    
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            fb[y * FRAME_WIDTH + x] = bit_fb[y * FRAME_WIDTH + x] ? fg : bg;
        }
    }
    video_cb((void *)fb, FRAME_WIDTH, FRAME_HEIGHT, FRAME_WIDTH * sizeof(pixel_t));
}

void update_audio() {
    memcpy(audio_buffer, arduous->getAudioBuffer().data(), TIMING_SAMPLE_RATE / TIMING_FPS * 2);
    audio_batch_cb(audio_buffer, TIMING_SAMPLE_RATE / TIMING_FPS);
}

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

bool retro_load_game(const struct retro_game_info* info) {
#ifdef USE_RGB565
    int pixel_format = RETRO_PIXEL_FORMAT_RGB565;
#else
    int pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
#endif    
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format)) {
        log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
        return false;
    }

    struct retro_input_descriptor desc[] = {
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A"},
        {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B"},
        {0, RETRO_DEVICE_NONE, 0, 0, nullptr},
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    if (info && info->data) {
        arduous->loadHexBuffer((const char *)info->data, info->size);
    }

    return true;
}

void retro_unload_game(void) {}

unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    bool no_rom = false;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_init(void) {
    struct retro_log_callback log;
    unsigned level = 4;

    arduous = make_unique<Arduous>();

    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log)) {
        log_cb = log.log;
    } else {
        log_cb = fallback_log;
    }

    environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_get_system_info(struct retro_system_info* info) {
    memset(info, 0, sizeof(retro_system_info));
    info->library_name = "arduous";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
    info->library_version = "0.1.0" GIT_VERSION;
    info->need_fullpath = false;
    info->valid_extensions = "hex";  // TODO(jmaroeder): handle .arduboy ZIP files
}

void retro_get_system_av_info(struct retro_system_av_info* info) {
    memset(info, 0, sizeof(retro_system_av_info));
    info->timing.fps = TIMING_FPS;
    info->timing.sample_rate = TIMING_SAMPLE_RATE;
    info->geometry.base_width = FRAME_WIDTH;
    info->geometry.base_height = FRAME_HEIGHT;
    info->geometry.max_width = FRAME_WIDTH;
    info->geometry.max_height = FRAME_HEIGHT;
    info->geometry.aspect_ratio = FRAME_ASPECT;
}

void retro_reset(void) { arduous->reset(); }

void retro_run(void) {
    ArduousButtonState buttonState;
    input_poll_cb();
    buttonState.buttonUp = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
    buttonState.buttonDown = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
    buttonState.buttonLeft = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
    buttonState.buttonRight = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    buttonState.buttonA = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
    buttonState.buttonB = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
    arduous->setButtonState(buttonState);
    arduous->emulateFrame();
    update_video();
    update_audio();
}

size_t retro_serialize_size(void) { return arduous->getSaveSize(); }
bool retro_serialize(void* data, size_t size) { return arduous->save(data, size); }
bool retro_unserialize(const void* data, size_t size) { return arduous->load(data, size); }

size_t retro_get_memory_size(unsigned id) {
    switch(id)
    {
    case RETRO_MEMORY_SYSTEM_RAM: // System Memory
	return arduous->getRamSize();
    case RETRO_MEMORY_SAVE_RAM: // EEPROM
	return arduous->getEEPROMSize();
    case RETRO_MEMORY_VIDEO_RAM: // VRAM
	return arduous->getVideoRAMSize();
    }
    return 0;
}

void* retro_get_memory_data(unsigned id) {
    switch(id)
    {
    case RETRO_MEMORY_SYSTEM_RAM: // System Memory
	return arduous->getRam();
    case RETRO_MEMORY_SAVE_RAM: // EEPROM
	return arduous->getEEPROM();
    case RETRO_MEMORY_VIDEO_RAM: // VRAM
	return arduous->getVideoRAM();
    }
    return 0;
}

// libretro unused api functions
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char* code) {}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info) { return false; }
void retro_set_controller_port_device(unsigned port, unsigned device) {}
void retro_deinit(void) {}
