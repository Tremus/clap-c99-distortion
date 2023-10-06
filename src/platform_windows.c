#include "fallbacktimer.c"
#include <stdio.h>

static int globalOpenGUICount = 0;

void GUICreate(const clap_c99_distortion_plug *plugin)
{
    // On windows you need to call RegisterClass before you're allowed to CreateWindow
    if (globalOpenGUICount++ == 0)
    {
        WNDCLASS wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = plugin->plugin.desc->id;
        RegisterClass(&wc);
    }
    plugin->gui->main_view = CreateWindow(
        plugin->plugin.desc->id, plugin->plugin.desc->name, WS_CHILDWINDOW | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, 0, GUI_WIDTH, GUI_HEIGHT, GetDesktopWindow(), NULL, NULL, NULL);
    assert(plugin->gui->main_view);
}

void GUIDestroy(const clap_c99_distortion_plug *plugin)
{
    DestroyWindow(plugin->gui->main_view);
    if (--globalOpenGUICount == 0)
        UnregisterClass(plugin->plugin.desc->id, NULL);
}

void GUISetParent(clap_c99_distortion_plug *plugin, const clap_window_t *window)
{
    SetParent(plugin->gui->main_view, (HWND)window->win32);
}

void GUISetVisible(clap_c99_distortion_plug *plugin, bool visible)
{
    ShowWindow((plugin)->gui->main_view, (visible) ? SW_SHOW : SW_HIDE);
}

static HWND g_fallbacktimer_hwnd = NULL;
static UINT_PTR g_fallbacktimer_timer = 0;

uint64_t fallback_timer_platform_get_ticks_ms() { return GetTickCount64(); }

LRESULT fallbacktimer_WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
        fallback_timer_callback();
        return 1;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void get_fallback_timer_classname(char *cn_buf, const clap_plugin_t *plugin)
{
    snprintf(cn_buf, (UINT64)CLAP_NAME_SIZE, "%s-fallbacktimer", plugin->desc->id);
}

void fallback_timer_platform_globals_init(const clap_plugin_t *cplug)
{
    char classname[CLAP_NAME_SIZE];
    get_fallback_timer_classname(classname, cplug);

    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = (WNDPROC)&fallbacktimer_WindowProc;
    wc.lpszClassName = classname;
    RegisterClass(&wc);

    g_fallbacktimer_hwnd = CreateWindow(classname, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, 0);
    assert(g_fallbacktimer_hwnd != NULL);
    g_fallbacktimer_timer = SetTimer(g_fallbacktimer_hwnd, 0, 10, NULL);
    assert(g_fallbacktimer_timer != 0);
}

void fallback_timer_platform_globals_deinit(const clap_plugin_t *cplug)
{
    if (g_fallbacktimer_timer && g_fallbacktimer_hwnd)
        KillTimer(g_fallbacktimer_hwnd, g_fallbacktimer_timer);
    if (g_fallbacktimer_hwnd)
        DestroyWindow(g_fallbacktimer_hwnd);
    g_fallbacktimer_hwnd = NULL;
    g_fallbacktimer_timer = 0;

    char classname[CLAP_NAME_SIZE];
    get_fallback_timer_classname(classname, cplug);

    UnregisterClass(classname, NULL);
}