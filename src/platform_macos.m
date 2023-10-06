#include "common.h"
#include "array.h"

#import <Cocoa/Cocoa.h>

@interface MainView : NSView
@property(nonatomic) clap_c99_distortion_plug *plugin;
@end

@implementation MainView
@end

void GUICreate(clap_c99_distortion_plug *plug)
{
    NSRect frame;
    frame.origin.x = 0;
    frame.origin.y = 0;
    frame.size.width = GUI_WIDTH;
    frame.size.height = GUI_HEIGHT;

    MainView *main_view = [[MainView alloc] initWithFrame:frame];
    main_view.plugin = plug;
    plug->gui->main_view = main_view;
}

void GUIDestroy(clap_c99_distortion_plug *plug) { [((MainView *)plug->gui->main_view) release]; }

void GUISetParent(clap_c99_distortion_plug *plug, const clap_window_t *window)
{
    MainView *main = (MainView *)plug->gui->main_view;
    NSView *parent = (NSView *)window->cocoa;
    if (main.superview != NULL)
        [main removeFromSuperview];
    [parent addSubview:main];
}

void GUISetVisible(clap_c99_distortion_plug *plug, bool visible)
{
    MainView *main = (MainView *)plug->gui->main_view;
    [main setHidden:(visible ? NO : YES)];
}

typedef struct
{
    uint32_t period; // if period is 0 the entry is unused (and can be reused)
    clap_id timer_id;
    uint64_t nexttick;
} Timer;

typedef struct
{
    const clap_plugin_t *plugin;
    Timer *timers;
    uint32_t id_counter;
} PluginTimers;

static CFRunLoopTimerRef g_osx_timer = NULL;
static PluginTimers *g_plugintimers = NULL;

uint64_t get_ticks_ms() { return (clock() * 1000) / CLOCKS_PER_SEC; }

static void fallback_timer_callback(CFRunLoopTimerRef t, void *info)
{
    for (PluginTimers *pt = g_plugintimers; pt != xarr_end(g_plugintimers); pt++)
    {
        uint64_t now = get_ticks_ms();

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

void fallback_timer_plugin_init(const clap_plugin_t *plugobject)
{
    if (xarr_len(g_plugintimers) == 0)
    {
        xarr_setcap(g_plugintimers, 16);
        CFRunLoopTimerContext context = {};
        context.info = &g_plugintimers;
        g_osx_timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() + 0.01, 0.01, 0, 0,
                                           fallback_timer_callback, &context);
        if (g_osx_timer)
            CFRunLoopAddTimer(CFRunLoopGetCurrent(), g_osx_timer, kCFRunLoopCommonModes);
    }

    PluginTimers pt = {.plugin = plugobject, .timers = NULL, .id_counter = 0};
    xarr_push(g_plugintimers, pt);
}

void fallback_timer_plugin_deinit(const clap_plugin_t *plugobject)
{
    for (size_t i = 0; i < xarr_len(g_plugintimers); i++)
    {
        if (g_plugintimers[i].plugin == plugobject)
        {
            xarr_free(g_plugintimers[i].timers);
            xarr_delete(g_plugintimers, i);
            break;
        }
    }

    if (xarr_len(g_plugintimers) == 0)
    {
        if (g_osx_timer)
        {
            CFRunLoopTimerInvalidate(g_osx_timer);
            CFRelease(g_osx_timer);
        }
        g_osx_timer = NULL;
        xarr_free(g_plugintimers);
    }
}

void fallback_timer_register(const clap_plugin_t *plugobject, uint32_t period_ms, clap_id *timer_id)
{
    if (period_ms < 16)
        period_ms = 16;

    PluginTimers *pt = g_plugintimers;
    while (pt != xarr_end(g_plugintimers) && pt->plugin != plugobject)
        pt++;
    assert(pt != xarr_end(g_plugintimers));

    clap_id newid = pt->id_counter++;

    Timer t = {period_ms, newid, get_ticks_ms() + period_ms};
    *timer_id = newid;
    xarr_push(pt->timers, t);
}

void fallback_timer_unregister(const clap_plugin_t *plugobject, clap_id timer_id)
{
    PluginTimers *pt = g_plugintimers;
    while (pt != xarr_end(g_plugintimers) && pt->plugin != plugobject)
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