#pragma once
// Galaxy brains at Microsoft will warn you about strncpy in c99, telling you to use strncpy_s instead.
// When you do, your code may not compile on other compilers because strncpy_s is not in c99!
#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <clap/clap.h>
#include <nanovg_compat.h>

#define GUI_WIDTH 640
#define GUI_HEIGHT 360

typedef struct
{
    void *plug;
    void *window;
    NVGcontext *nvg;
    float pixel_scale;
    int main_fbo;

    clap_id draw_timer_ID;
} clap_c99_gui;

typedef struct
{
    clap_plugin_t plugin;
    const clap_host_t *host;
    const clap_host_latency_t *hostLatency;
    const clap_host_log_t *hostLog;
    const clap_host_thread_check_t *hostThreadCheck;
    const clap_host_params_t *hostParams;
    const clap_host_timer_support_t *hostTimerSupport;

    clap_c99_gui *gui;

    float drive;
    float mix;
    int32_t mode;
} clap_c99_distortion_plug;

float get_pixel_scale(void *window);

void fallback_timer_plugin_init(const clap_plugin_t *);
void fallback_timer_plugin_deinit(const clap_plugin_t *);
void fallback_timer_register(const clap_plugin_t *, uint32_t period_ms, clap_id *);
void fallback_timer_unregister(const clap_plugin_t *, clap_id);