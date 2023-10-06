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

static CFRunLoopTimerRef g_osx_timer = NULL;

uint64_t fallback_timer_platform_get_ticks_ms() { return (clock() * 1000) / CLOCKS_PER_SEC; }

void osx_timer_cb(CFRunLoopTimerRef t, void *info) { fallback_timer_callback(); }

void fallback_timer_platform_globals_init(const clap_plugin_t *cplug)
{
    CFRunLoopTimerContext context = {};
    context.info = &g_plugintimers;
    g_osx_timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent() + 0.01, 0.01, 0, 0,
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