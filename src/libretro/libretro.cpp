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
constexpr int TIMING_SAMPLE_RATE = 44100;

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3U) << 11U) | ((g >> 2U) << 5U) | ((b >> 3U) << 0U);
}
#define WHITE rgb565(255, 255, 255)
#define BLACK rgb565(0, 0, 0)

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
std::unique_ptr<Arduous> arduous = make_unique<Arduous>();

void update_video() {
    uint16_t fb[FRAME_WIDTH * FRAME_HEIGHT];
    memset(fb, BLACK, sizeof(uint16_t) * FRAME_WIDTH * FRAME_HEIGHT);
    auto bit_fb = arduous->getFrameBuffer();
    for (int y = 0; y < FRAME_HEIGHT; y++) {
        for (int x = 0; x < FRAME_WIDTH; x++) {
            fb[y * FRAME_WIDTH + x] = bit_fb[y * FRAME_WIDTH + x] ? WHITE : BLACK;
        }
    }
    video_cb((void*)fb, FRAME_WIDTH, FRAME_HEIGHT, FRAME_WIDTH * sizeof(uint16_t));
}

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

bool retro_load_game(const struct retro_game_info* info) {
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

    if (info && info->path) {
        arduous->loadFirmware(info->path);
    }

    return true;
}

void retro_unload_game(void) {}

unsigned retro_get_region(void) { return RETRO_REGION_NTSC; }

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    bool no_rom = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_init(void) {
    struct retro_log_callback log;
    unsigned level = 4;

    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log)) {
        log_cb = log.log;
    } else {
        log_cb = nullptr;
    }

    environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_get_system_info(struct retro_system_info* info) {
    memset(info, 0, sizeof(retro_system_info));
    info->library_name = "arduous";
    info->library_version = "0.1.0";
    info->need_fullpath = false;
    info->valid_extensions = "hex|axf|elf";
}

void retro_get_system_av_info(struct retro_system_av_info* info) {
    int pixel_format = RETRO_PIXEL_FORMAT_RGB565;
    memset(info, 0, sizeof(retro_system_av_info));
    info->timing.fps = TIMING_FPS;
    info->timing.sample_rate = TIMING_SAMPLE_RATE;
    info->geometry.base_width = FRAME_WIDTH;
    info->geometry.base_height = FRAME_HEIGHT;
    info->geometry.max_width = FRAME_WIDTH;
    info->geometry.max_height = FRAME_HEIGHT;
    info->geometry.aspect_ratio = FRAME_ASPECT;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
}

void retro_reset(void) {}

void retro_run(void) {
    arduous->emulateFrame();
    update_video();
}

// libretro unused api functions
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char* code) {}
bool retro_load_game_special(unsigned game_type, const struct retro_game_info* info, size_t num_info) { return false; }
void retro_set_controller_port_device(unsigned port, unsigned device) {}
void* retro_get_memory_data(unsigned id) { return nullptr; }
size_t retro_get_memory_size(unsigned id) { return 0; }
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void* data, size_t size) { return false; }
bool retro_unserialize(const void* data, size_t size) { return false; }
void retro_deinit(void) {}
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {}
