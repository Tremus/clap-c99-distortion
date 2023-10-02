#include <clap/clap.h>
#include <nanovg_compat.h>

#define GUI_WIDTH 640
#define GUI_HEIGHT 360

typedef struct
{
    void *plug;
    void *main_view;
    void *parent_window;
    NVGcontext *nvg;
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