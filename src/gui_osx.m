#include "common.h"

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
