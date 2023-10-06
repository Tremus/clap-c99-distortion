#include "fallbacktimer.c"

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

    MainView *mainview = [[MainView alloc] initWithFrame:frame];
    mainview.plugin = plug;
    plug->gui->window = mainview;
}

void GUIDestroy(clap_c99_distortion_plug *plug) { [((MainView *)plug->gui->window) release]; }

void GUISetParent(clap_c99_distortion_plug *plug, const clap_window_t *window)
{
    MainView *main = (MainView *)plug->gui->window;
    NSView *parent = (NSView *)window->cocoa;
    if (main.superview != NULL)
        [main removeFromSuperview];
    [parent addSubview:main];
}

void GUISetVisible(clap_c99_distortion_plug *plug, bool visible)
{
    MainView *main = (MainView *)plug->gui->window;
    [main setHidden:(visible ? NO : YES)];
}

float get_pixel_scale(void *nsvew)
{
    float scale = [[(MainView *)nsvew window] screen].backingScaleFactor;
    assert(scale >= 1);
    return scale;
}

static CFRunLoopTimerRef g_osx_timer = NULL;

uint64_t fallback_timer_platform_get_ticks_ms() { return (clock() * 1000) / CLOCKS_PER_SEC; }

void osx_timer_cb(CFRunLoopTimerRef t, void *info) { fallback_timer_callback(); }

void fallback_timer_platform_globals_init(const clap_plugin_t *cplug)
{
    CFRunLoopTimerContext context = {};
    context.info = &g_plugintimers;
    double interval = (double)PLATFORM_TIMER_MIN * 0.001;
    g_osx_timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() + interval, interval, 0, 0,
                                       osx_timer_cb, &context);
    if (g_osx_timer)
        CFRunLoopAddTimer(CFRunLoopGetCurrent(), g_osx_timer, kCFRunLoopCommonModes);
}

void fallback_timer_platform_globals_deinit(const clap_plugin_t *cplug)
{
    if (g_osx_timer)
    {
        CFRunLoopTimerInvalidate(g_osx_timer);
        CFRelease(g_osx_timer);
    }
    g_osx_timer = NULL;
}