#include "common.h"
#include "array.h"
#include <assert.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define PLATFORM_TIMER_MIN USER_TIMER_MINIMUM
#else
#error "TODO: OSX & Linux"
#endif

typedef struct
{
    uint32_t period;
    clap_id timer_id;
    uint64_t nexttick;
} Timer;

typedef struct
{
    const clap_plugin_t *plugin;
    Timer *timers;
    uint32_t id_counter;
} PluginTimers;

static PluginTimers *g_plugintimers = NULL;

uint64_t fallback_timer_platform_get_ticks_ms();

void fallback_timer_platform_globals_init(const clap_plugin_t *);
void fallback_timer_platform_globals_deinit(const clap_plugin_t *);

void fallback_timer_plugin_init(const clap_plugin_t *cplug)
{
    if (xarr_cap(g_plugintimers) == 0)
    {
        xarr_setcap(g_plugintimers, 16);

        fallback_timer_platform_globals_init(cplug);
    }

    PluginTimers pt = {.plugin = cplug, .timers = NULL, .id_counter = 0};
    xarr_push(g_plugintimers, pt);
}

void fallback_timer_plugin_deinit(const clap_plugin_t *cplug)
{
    for (size_t i = 0; i < xarr_len(g_plugintimers); i++)
    {
        if (g_plugintimers[i].plugin == cplug)
        {
            clap_c99_distortion_plug *p = cplug->plugin_data;
            xarr_free(g_plugintimers[i].timers);
            xarr_delete(g_plugintimers, i);
            break;
        }
    }

    if (xarr_len(g_plugintimers) == 0)
    {
        fallback_timer_platform_globals_deinit(cplug);
        xarr_free(g_plugintimers);
    }
}

void fallback_timer_register(const clap_plugin_t *cplug, uint32_t period_ms, clap_id *timer_id)
{
    if (period_ms < PLATFORM_TIMER_MIN)
        period_ms = PLATFORM_TIMER_MIN;

    PluginTimers *pt = g_plugintimers;
    while (pt != xarr_end(g_plugintimers) && pt->plugin != cplug)
        pt++;
    assert(pt != xarr_end(g_plugintimers));

    clap_id newid = pt->id_counter++;

    Timer t = {.period = period_ms,
               .timer_id = newid,
               .nexttick = fallback_timer_platform_get_ticks_ms() + period_ms};
    *timer_id = newid;
    xarr_push(pt->timers, t);
}

void fallback_timer_unregister(const clap_plugin_t *cplug, clap_id timer_id)
{
    PluginTimers *pt = g_plugintimers;
    while (pt != xarr_end(g_plugintimers) && pt->plugin != cplug)
        pt++;
    assert(pt != xarr_end(g_plugintimers));

    for (size_t i = 0; i < xarr_len(pt->timers); i++)
    {
        if (pt->timers[i].timer_id == timer_id)
        {
            xarr_delete(pt->timers, i);
            break;
        }
    }
}

static void fallback_timer_callback()
{
    uint64_t now = fallback_timer_platform_get_ticks_ms();
    for (PluginTimers *pt = g_plugintimers; pt != xarr_end(g_plugintimers); pt++)
    {
        for (Timer *t = pt->timers; t != xarr_end(pt->timers); t++)
        {
            if (t->nexttick < now)
            {
                t->nexttick = now + t->period;
                const clap_plugin_timer_support_t *ext =
                    pt->plugin->get_extension(pt->plugin, CLAP_EXT_TIMER_SUPPORT);
                ext->on_timer(pt->plugin, t->timer_id);
            }
        }
    }
}