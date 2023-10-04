// This file is here to demonstrate how to wire a CLAP plugin
// You can use it as a starting point, however if you are implementing a C++
// plugin, I'd encourage you to use the C++ glue layer instead:
// https://github.com/free-audio/clap-helpers/blob/main/include/clap/helpers/plugin.hh

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <math.h>
#include <assert.h>
#include <nanovg_compat.h>

static const clap_plugin_descriptor_t s_c99dist_desc = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "org.surge-synth-team.clap-c99-distortion",
    .name = "Bad Distortion written in C99",
    .vendor = "Surge Synth Team",
    .url = "https://surge-synth-team.org/",
    .manual_url = "",
    .support_url = "",
    .version = "1.0.0",
    .description = "A few sloppy distortion algorithms using naive waveshapers",
    .features =
        (const char *[]){CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, NULL},
};

enum ClipType
{
    HARD = 0,
    SOFT = 1,
    FOLD = 2
};

enum ParamIds
{
    pid_DRIVE = 2112,
    pid_MIX = 8675309,
    pid_MODE = 5150
};

static void c99dist_process_event(clap_c99_distortion_plug *plug, const clap_event_header_t *hdr);

/////////////////////
// clap_plugin_gui //
/////////////////////

void GUICreate(const clap_c99_distortion_plug *);
void GUIDestroy(const clap_c99_distortion_plug *);
void GUISetParent(clap_c99_distortion_plug *, const clap_window_t *);
void GUISetVisible(clap_c99_distortion_plug *, bool);
void GUIDraw(clap_c99_distortion_plug *plug)
{
    NVGcontext *nvg = plug->gui->nvg;
    nvgBindFramebuffer(nvg, 0);
    nvgBeginFrame(nvg, GUI_WIDTH, GUI_HEIGHT, 2.0f);
    nvgClearWithColor(nvg, nvgRGBAf(1.0f, 0.0f, 1.0f, 1.0f));
    nvgEndFrame(nvg);
}

#if defined(_WIN32)
#include "gui_w32.cpp"
#elif defined(__APPLE__)
#define GUI_API CLAP_WINDOW_API_COCOA
#else
#error "TODO: support linux"
#endif

static bool c99dist_gui_is_api_supported(const clap_plugin_t *plugin, const char *api,
                                         bool isFloating)
{
    return 0 == strcmp(api, GUI_API) && !isFloating;
}

static bool c99dist_gui_get_preferred_api(const clap_plugin_t *plugin, const char **api,
                                          bool *isFloating)
{
    *api = GUI_API;
    *isFloating = false;
    return true;
}

static bool c99dist_gui_create(const clap_plugin_t *_plugin, const char *api, bool isFloating)
{
    if (!c99dist_gui_is_api_supported(_plugin, api, isFloating))
        return false;

    clap_c99_distortion_plug *plug = _plugin->plugin_data;

    clap_c99_gui *gui = calloc(1, sizeof(clap_c99_gui));
    plug->gui = gui;
    gui->plug = plug;

    GUICreate(plug);
    assert(gui->main_view != NULL);
    gui->nvg = nvgCreateContext(gui->main_view, 0, GUI_WIDTH, GUI_HEIGHT);
    assert(gui->nvg != NULL);
    gui->main_fbo = nvgCreateFramebuffer(gui->nvg, GUI_WIDTH, GUI_HEIGHT, 0);

    if (plug->hostTimerSupport && plug->hostTimerSupport->register_timer)
        plug->hostTimerSupport->register_timer(plug->host, 16, &gui->draw_timer_ID);
    else
    {
        fallback_timer_plugin_init(_plugin);
        fallback_timer_register(_plugin, 16, &gui->draw_timer_ID);
    }

    return true;
}

static void c99dist_gui_destroy(const clap_plugin_t *_plugin)
{
    clap_c99_distortion_plug *plug = _plugin->plugin_data;

    if (plug->hostTimerSupport && plug->hostTimerSupport->unregister_timer)
        plug->hostTimerSupport->unregister_timer(plug->host, plug->gui->draw_timer_ID);
    else
    {
        fallback_timer_unregister(_plugin, plug->gui->draw_timer_ID);
        fallback_timer_plugin_deinit(_plugin);
    }

    nvgDeleteFramebuffer(plug->gui->nvg, plug->gui->main_fbo);
    nvgDeleteContext(plug->gui->nvg);
    GUIDestroy(plug);
    free(plug->gui);
    plug->gui = NULL;
}
static bool c99dist_gui_set_scale(const clap_plugin_t *plugin, double scale) { return false; }
static bool c99dist_gui_get_size(const clap_plugin_t *plugin, uint32_t *width, uint32_t *height)
{
    *width = GUI_WIDTH;
    *height = GUI_HEIGHT;
    return true;
}

static bool c99dist_gui_can_resize(const clap_plugin_t *plugin) { return false; }

static bool c99dist_gui_get_resize_hints(const clap_plugin_t *plugin,
                                         clap_gui_resize_hints_t *hints)
{
    return false;
}

static bool c99dist_gui_adjust_size(const clap_plugin_t *plugin, uint32_t *width, uint32_t *height)
{
    return c99dist_gui_get_size(plugin, width, height);
}
static bool c99dist_gui_set_size(const clap_plugin_t *plugin, uint32_t width, uint32_t height)
{
    return true;
}

static bool c99dist_gui_set_parent(const clap_plugin_t *_plugin, const clap_window_t *window)
{
    assert(0 == strcmp(window->api, GUI_API));
    GUISetParent((clap_c99_distortion_plug *)_plugin->plugin_data, window);
    return true;
}

static bool c99dist_gui_set_transient(const clap_plugin_t *plugin, const clap_window_t *window)
{
    return false;
}

static void c99dist_gui_suggest_title(const clap_plugin_t *plugin, const char *title) {}

static bool c99dist_gui_show(const clap_plugin_t *_plugin)
{
    GUISetVisible((clap_c99_distortion_plug *)_plugin->plugin_data, true);
    return true;
}
static bool c99dist_gui_hide(const clap_plugin_t *_plugin)
{
    GUISetVisible((clap_c99_distortion_plug *)_plugin->plugin_data, false);
    return true;
}

static const clap_plugin_gui_t s_c99dist_gui = {
    .is_api_supported = c99dist_gui_is_api_supported,
    .get_preferred_api = c99dist_gui_get_preferred_api,
    .create = c99dist_gui_create,
    .destroy = c99dist_gui_destroy,
    .set_scale = c99dist_gui_set_scale,
    .get_size = c99dist_gui_get_size,
    .can_resize = c99dist_gui_can_resize,
    .get_resize_hints = c99dist_gui_get_resize_hints,
    .adjust_size = c99dist_gui_adjust_size,
    .set_size = c99dist_gui_set_size,
    .set_parent = c99dist_gui_set_parent,
    .set_transient = c99dist_gui_set_transient,
    .suggest_title = c99dist_gui_suggest_title,
    .show = c99dist_gui_show,
    .hide = c99dist_gui_hide,
};

///////////////////////////////
// clap_plugin_timer_support //
///////////////////////////////

static void c99dist_timer_support_on_timer(const clap_plugin_t *_plugin, clap_id timerID)
{
    clap_c99_distortion_plug *plug = _plugin->plugin_data;

    // If the GUI is open and at least one parameter value has changed...
    if (plug->gui && timerID == plug->gui->draw_timer_ID)
        GUIDraw(plug);
}

static const clap_plugin_timer_support_t s_c99dist_timer_support = {
    .on_timer = c99dist_timer_support_on_timer,
};

/////////////////////////////
// clap_plugin_audio_ports //
/////////////////////////////

static uint32_t c99dist_audio_ports_count(const clap_plugin_t *plugin, bool is_input) { return 1; }

static bool c99dist_audio_ports_get(const clap_plugin_t *plugin, uint32_t index, bool is_input,
                                    clap_audio_port_info_t *info)
{
    if (index > 0)
        return false;
    info->id = 0;
    if (is_input)
        snprintf(info->name, sizeof(info->name), "%s", "Stereo In");
    else
        snprintf(info->name, sizeof(info->name), "%s", "Distorted Output");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
}

static const clap_plugin_audio_ports_t s_c99dist_audio_ports = {
    .count = c99dist_audio_ports_count,
    .get = c99dist_audio_ports_get,
};

//////////////////
// clap_latency //
//////////////////

uint32_t c99dist_latency_get(const clap_plugin_t *plugin) { return 0; }

static const clap_plugin_latency_t s_c99dist_latency = {
    .get = c99dist_latency_get,
};

//////////////////
// clap_params //
//////////////////

uint32_t c99dist_param_count(const clap_plugin_t *plugin) { return 3; }
bool c99dist_param_get_info(const clap_plugin_t *plugin, uint32_t param_index,
                            clap_param_info_t *param_info)
{
    switch (param_index)
    {
    case 0: // drive
        param_info->id = pid_DRIVE;
        strncpy(param_info->name, "Drive", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 0.;
        param_info->min_value = -1;
        param_info->max_value = 6;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        param_info->cookie = NULL;
        break;
    case 1: // mix
        param_info->id = pid_MIX;
        strncpy(param_info->name, "MIX", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 0.5;
        param_info->min_value = 0;
        param_info->max_value = 1;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        param_info->cookie = NULL;
        break;
    case 2: // mode
        param_info->id = pid_MODE;
        strncpy(param_info->name, "Mode", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 0.;
        param_info->min_value = 0;
        param_info->max_value = 2;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
        param_info->cookie = NULL;
        break;
    default:
        return false;
    }
    return true;
}
bool c99dist_param_get_value(const clap_plugin_t *plugin, clap_id param_id, double *value)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;

    switch (param_id)
    {
    case pid_DRIVE:
        *value = plug->drive;
        return true;
        break;

    case pid_MIX:
        *value = plug->mix;
        return true;
        break;

    case pid_MODE:
        *value = plug->mode;
        return true;
        break;
    }

    return false;
}
bool c99dist_param_value_to_text(const clap_plugin_t *plugin, clap_id param_id, double value,
                                 char *display, uint32_t size)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;
    switch (param_id)
    {
    case pid_DRIVE:
    case pid_MIX:
        snprintf(display, size, "%f", value);
        return true;
        break;
    case pid_MODE:
    {
        int v = (int)value;
        switch (v)
        {
        case 0:
            snprintf(display, size, "Hard Clip");
            break;
        case 1:
            snprintf(display, size, "Soft Clip (Tanh)");
            break;
        case 2:
            snprintf(display, size, "Simple Folder");
            break;
        }

        return true;
    }
    break;
    }
    return false;
}
bool c99dist_text_to_value(const clap_plugin_t *plugin, clap_id param_id, const char *display,
                           double *value)
{
    // I'm not going to bother to support this
    return false;
}
void c99dist_flush(const clap_plugin_t *plugin, const clap_input_events_t *in,
                   const clap_output_events_t *out)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;
    int s = in->size(in);
    int q;
    for (q = 0; q < s; ++q)
    {
        const clap_event_header_t *hdr = in->get(in, q);

        c99dist_process_event(plug, hdr);
    }
}

static const clap_plugin_params_t s_c99dist_params = {.count = c99dist_param_count,
                                                      .get_info = c99dist_param_get_info,
                                                      .get_value = c99dist_param_get_value,
                                                      .value_to_text = c99dist_param_value_to_text,
                                                      .text_to_value = c99dist_text_to_value,
                                                      .flush = c99dist_flush};

bool c99dist_state_save(const clap_plugin_t *plugin, const clap_ostream_t *stream)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;

    // We need to save 2 doubles and an int to save our state plus a version. This is, of course, a
    // terrible implementation of state. You should do better.
    assert(sizeof(float) == 4);
    assert(sizeof(int32_t) == 4);

    int buffersize = 16;
    char buffer[16];

    int32_t version = 1;
    memcpy(buffer, &version, sizeof(int32_t));
    memcpy(buffer + 4, &(plug->drive), sizeof(float));
    memcpy(buffer + 8, &(plug->mix), sizeof(float));
    memcpy(buffer + 12, &(plug->mode), sizeof(int32_t));

    int written = 0;
    char *curr = buffer;
    while (written != buffersize)
    {
        int thiswrite = stream->write(stream, curr, buffersize - written);
        if (thiswrite < 0)
            return false;
        curr += thiswrite;
        written += thiswrite;
    }

    return true;
}

bool c99dist_state_load(const clap_plugin_t *plugin, const clap_istream_t *stream)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;

    int buffersize = 16;
    char buffer[16];

    int read = 0;
    char *curr = buffer;
    while (read != buffersize)
    {
        int thisread = stream->read(stream, curr, buffersize - read);
        if (thisread < 0)
            return false;
        curr += thisread;
        read += thisread;
    }

    int32_t version;
    memcpy(&version, buffer, sizeof(int32_t));
    memcpy(&plug->drive, buffer + 4, sizeof(float));
    memcpy(&plug->mix, buffer + 8, sizeof(float));
    memcpy(&plug->mode, buffer + 12, sizeof(int32_t));

    return true;
}
static const clap_plugin_state_t s_c99dist_state = {.save = c99dist_state_save,
                                                    .load = c99dist_state_load};

/////////////////
// clap_plugin //
/////////////////

static bool c99dist_init(const struct clap_plugin *plugin)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;

    // Fetch host's extensions here
    plug->hostLatency = plug->host->get_extension(plug->host, CLAP_EXT_LATENCY);
    plug->hostLog = plug->host->get_extension(plug->host, CLAP_EXT_LOG);
    plug->hostThreadCheck = plug->host->get_extension(plug->host, CLAP_EXT_THREAD_CHECK);
    plug->hostParams = plug->host->get_extension(plug->host, CLAP_EXT_PARAMS);
    plug->hostTimerSupport = plug->host->get_extension(plug->host, CLAP_EXT_TIMER_SUPPORT);

    plug->drive = 0.f;
    plug->mix = 0.5f;
    plug->mode = HARD;
    return true;
}

static void c99dist_destroy(const struct clap_plugin *plugin)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;
    free(plug);
}

static bool c99dist_activate(const struct clap_plugin *plugin, double sample_rate,
                             uint32_t min_frames_count, uint32_t max_frames_count)
{
    return true;
}

static void c99dist_deactivate(const struct clap_plugin *plugin) {}

static bool c99dist_start_processing(const struct clap_plugin *plugin) { return true; }

static void c99dist_stop_processing(const struct clap_plugin *plugin) {}

static void c99dist_reset(const struct clap_plugin *plugin) {}

static void c99dist_process_event(clap_c99_distortion_plug *plug, const clap_event_header_t *hdr)
{
    if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID)
    {
        switch (hdr->type)
        {
        case CLAP_EVENT_PARAM_VALUE:
        {
            const clap_event_param_value_t *ev = (const clap_event_param_value_t *)hdr;
            // TODO: handle parameter change
            switch (ev->param_id)
            {
            case pid_DRIVE:
                plug->drive = ev->value;
                break;
            case pid_MIX:
                plug->mix = ev->value;
                break;
            case pid_MODE:
                plug->mode = (int)(ev->value);
                break;
            }
            break;
        }
        }
    }
}

static clap_process_status c99dist_process(const struct clap_plugin *plugin,
                                           const clap_process_t *process)
{
    clap_c99_distortion_plug *plug = plugin->plugin_data;
    const uint32_t nframes = process->frames_count;
    const uint32_t nev = process->in_events->size(process->in_events);
    uint32_t ev_index = 0;
    uint32_t next_ev_frame = nev > 0 ? 0 : nframes;

    for (uint32_t i = 0; i < nframes;)
    {
        /* handle every events that happrens at the frame "i" */
        while (ev_index < nev && next_ev_frame == i)
        {
            const clap_event_header_t *hdr = process->in_events->get(process->in_events, ev_index);
            if (hdr->time != i)
            {
                next_ev_frame = hdr->time;
                break;
            }

            c99dist_process_event(plug, hdr);
            ++ev_index;

            if (ev_index == nev)
            {
                // we reached the end of the event list
                next_ev_frame = nframes;
                break;
            }
        }

        /* process every samples until the next event */
        for (; i < next_ev_frame; ++i)
        {
            // fetch input samples
            const float in_l = process->audio_inputs[0].data32[0][i];
            const float in_r = process->audio_inputs[0].data32[1][i];

            float out_l, out_r;
            out_l = 0;
            out_r = 0;

            float tl = in_l * (1.0 + plug->drive);
            float tr = in_r * (1.0 + plug->drive);

            // Obviously this is inefficient but
            switch (plug->mode)
            {
            case HARD:
            {
                tl = (tl > 1 ? 1 : tl < -1 ? -1 : tl);
                tr = (tr > 1 ? 1 : tr < -1 ? -1 : tr);
            }
            break;
            case SOFT:
            {
                tl = (tl > 1 ? 1 : tl < -1 ? -1 : tl);
                tl = 1.5 * tl - 0.5 * tl * tl * tl;

                tr = (tr > 1 ? 1 : tr < -1 ? -1 : tr);
                tr = 1.5 * tr - 0.5 * tr * tr * tr;
            }
            break;
            case FOLD:
            {
                tl = sin(2.0 * M_PI * tl);
                tr = sin(2.0 * M_PI * tr);
            }
            break;
            }

            float mix = plug->mix;
            out_l = mix * tl + (1.0 - mix) * in_l;
            out_r = mix * tr + (1.0 - mix) * in_r;

            // store output samples
            process->audio_outputs[0].data32[0][i] = out_l;
            process->audio_outputs[0].data32[1][i] = out_r;
        }
    }

    return CLAP_PROCESS_CONTINUE;
}

static const void *c99dist_get_extension(const struct clap_plugin *plugin, const char *id)
{
    if (!strcmp(id, CLAP_EXT_LATENCY))
        return &s_c99dist_latency;
    if (!strcmp(id, CLAP_EXT_AUDIO_PORTS))
        return &s_c99dist_audio_ports;
    if (!strcmp(id, CLAP_EXT_PARAMS))
        return &s_c99dist_params;
    if (!strcmp(id, CLAP_EXT_STATE))
        return &s_c99dist_state;
    if (!strcmp(id, CLAP_EXT_GUI))
        return &s_c99dist_gui;
    if (!strcmp(id, CLAP_EXT_TIMER_SUPPORT))
        return &s_c99dist_timer_support;
    return NULL;
}

static void c99dist_on_main_thread(const struct clap_plugin *plugin) {}

clap_plugin_t *c99dist_create(const clap_host_t *host)
{
    clap_c99_distortion_plug *p = calloc(1, sizeof(*p));
    p->host = host;
    p->plugin.desc = &s_c99dist_desc;
    p->plugin.plugin_data = p;
    p->plugin.init = c99dist_init;
    p->plugin.destroy = c99dist_destroy;
    p->plugin.activate = c99dist_activate;
    p->plugin.deactivate = c99dist_deactivate;
    p->plugin.start_processing = c99dist_start_processing;
    p->plugin.stop_processing = c99dist_stop_processing;
    p->plugin.reset = c99dist_reset;
    p->plugin.process = c99dist_process;
    p->plugin.get_extension = c99dist_get_extension;
    p->plugin.on_main_thread = c99dist_on_main_thread;

    // Don't call into the host here

    return &p->plugin;
}

/////////////////////////
// clap_plugin_factory //
/////////////////////////

static struct
{
    const clap_plugin_descriptor_t *desc;
    clap_plugin_t *(*create)(const clap_host_t *host);
} s_plugins[] = {
    {
        .desc = &s_c99dist_desc,
        .create = c99dist_create,
    },
};

static uint32_t plugin_factory_get_plugin_count(const struct clap_plugin_factory *factory)
{
    return sizeof(s_plugins) / sizeof(s_plugins[0]);
}

static const clap_plugin_descriptor_t *
plugin_factory_get_plugin_descriptor(const struct clap_plugin_factory *factory, uint32_t index)
{
    return s_plugins[index].desc;
}

static const clap_plugin_t *plugin_factory_create_plugin(const struct clap_plugin_factory *factory,
                                                         const clap_host_t *host,
                                                         const char *plugin_id)
{
    if (!clap_version_is_compatible(host->clap_version))
    {
        return NULL;
    }

    const int N = sizeof(s_plugins) / sizeof(s_plugins[0]);
    for (int i = 0; i < N; ++i)
        if (!strcmp(plugin_id, s_plugins[i].desc->id))
            return s_plugins[i].create(host);

    return NULL;
}

static const clap_plugin_factory_t s_plugin_factory = {
    .get_plugin_count = plugin_factory_get_plugin_count,
    .get_plugin_descriptor = plugin_factory_get_plugin_descriptor,
    .create_plugin = plugin_factory_create_plugin,
};

////////////////
// clap_entry //
////////////////

static bool entry_init(const char *plugin_path)
{
    // called only once, and very first
    return true;
}

static void entry_deinit(void)
{
    // called before unloading the DSO
}

static const void *entry_get_factory(const char *factory_id)
{
    if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
        return &s_plugin_factory;
    return NULL;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = entry_init,
    .deinit = entry_deinit,
    .get_factory = entry_get_factory,
};
