#include "common.h"
#include <windows.h>
#include <assert.h>

static int globalOpenGUICount = 0;

LRESULT CALLBACK GUIWindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    clap_c99_distortion_plug *plugin = (clap_c99_distortion_plug *)GetWindowLongPtr(window, 0);

    switch (message)
    {
    case WM_PAINT:
        // TODO
        break;
    case WM_LBUTTONDOWN:
        SetCapture(window);
        break;
    case WM_LBUTTONUP:
        ReleaseCapture();
        break;
    default:
        return DefWindowProc(window, message, wParam, lParam);
    }

    return 0;
}

void GUICreate(const clap_c99_distortion_plug *plugin)
{
    // On windows you need to call RegisterClass before you're allowed to CreateWindow
    if (globalOpenGUICount++ == 0)
    {
        WNDCLASS wc;
        memset(&wc, 0, sizeof(wc));
        wc.lpfnWndProc = GUIWindowProcedure;
        wc.lpszClassName = plugin->plugin.desc->id;
        RegisterClass(&wc);
    }

    plugin->gui->main_view = CreateWindow(
        plugin->plugin.desc->id, plugin->plugin.desc->name, WS_CHILDWINDOW | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, 0, GUI_WIDTH, GUI_HEIGHT, GetDesktopWindow(), NULL, NULL, NULL);
    SetWindowLongPtr(plugin->gui->main_view, 0, (LONG_PTR)plugin);
}

void GUIDestroy(const clap_c99_distortion_plug *plugin) {}
void GUISetParent(clap_c99_distortion_plug *plugin, const clap_window_t *window) {}
void GUISetVisible(clap_c99_distortion_plug *plugin, bool visible) {}

void fallback_timer_plugin_init(const clap_plugin_t *plugin) {}
void fallback_timer_plugin_deinit(const clap_plugin_t *plugin) {}
void fallback_timer_register(const clap_plugin_t *plugin, uint32_t period_ms, clap_id *id) {}
void fallback_timer_unregister(const clap_plugin_t *plugin, clap_id id) {}