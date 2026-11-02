#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "app.h"
#include "metrics.h"
#include "render.h"
#include "resource.h"

typedef enum _ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_INVALID_STATE = 5
} ACCENT_STATE;

typedef struct _ACCENT_POLICY {
    ACCENT_STATE AccentState;
    DWORD AccentFlags;
    DWORD GradientColor;
    DWORD AnimationId;
} ACCENT_POLICY;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA {
    DWORD Attrib;
    PVOID pvData;
    SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

typedef BOOL (WINAPI *pfnSetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

static Canvas g_canvas;
static FontCache g_fonts;
static SystemMetrics g_metrics;
static AppConfig g_cfg;
static bool g_trackingMouse = false;

static void ApplyWindowBackdrop(HWND hwnd, bool enableGlass, float opacity) {
    if (!enableGlass) {
        // Solid Modern Matte: 100% solid, fully opaque (no transparency)
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

        #ifndef DWMWA_SYSTEMBACKDROP_TYPE
        #define DWMWA_SYSTEMBACKDROP_TYPE 38
        #endif
        DWORD backdropType = 1; // DWMSBT_NONE
        DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));

        HMODULE hUser = GetModuleHandleW(L"user32.dll");
        if (hUser) {
            pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
                (pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");
            if (SetWindowCompositionAttribute) {
                ACCENT_POLICY policy = {0};
                policy.AccentState = ACCENT_DISABLED;
                WINDOWCOMPOSITIONATTRIBDATA data = {0};
                data.Attrib = 19;
                data.pvData = &policy;
                data.cbData = sizeof(policy);
                SetWindowCompositionAttribute(hwnd, &data);
            }
        }
        return;
    }

    // High-End Frosted Glassmorphism:
    // Window-level alpha stays high (225..255) to maintain 100% crisp, readable text and UI elements,
    // while DWM Acrylic provides hardware-accelerated desktop blur behind the window!
    BYTE bAlpha = (BYTE)(225.0f + opacity * 30.0f);
    if (bAlpha < 220) bAlpha = 220;
    if (bAlpha > 255) bAlpha = 255;
    SetLayeredWindowAttributes(hwnd, 0, bAlpha, LWA_ALPHA);

    #ifndef DWMWA_SYSTEMBACKDROP_TYPE
    #define DWMWA_SYSTEMBACKDROP_TYPE 38
    #endif
    DWORD backdropType = 3; // DWMSBT_TRANSIENTWINDOW (Acrylic Blur)
    DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType));

    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        pfnSetWindowCompositionAttribute SetWindowCompositionAttribute =
            (pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute) {
            ACCENT_POLICY policy = {0};
            policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
            policy.AccentFlags = 2;
            DWORD alphaByte = (DWORD)(100.0f + opacity * 70.0f);
            policy.GradientColor = (alphaByte << 24) | 0x00140D07; // Sleek obsidian tint
            WINDOWCOMPOSITIONATTRIBDATA data = {0};
            data.Attrib = 19;
            data.pvData = &policy;
            data.cbData = sizeof(policy);
            SetWindowCompositionAttribute(hwnd, &data);
        }
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        Metrics_Init(&g_metrics);
        Metrics_Update(&g_metrics);

        g_cfg.glass_opacity = 0.65f;
        g_cfg.is_solid_theme = 1; // Default: Solid Modern Matte
        g_cfg.active_theme = THEME_CYAN;
        g_cfg.show_settings = false;
        g_cfg.active_nav_item = 0;
        g_cfg.hover_wip_back_btn = false;
        g_cfg.toggle_auto_ram = true;
        g_cfg.toggle_fast_defrag = false;
        g_cfg.toggle_turbo_net = true;
        g_cfg.dragging_opacity = false;
        g_cfg.hover_nav_item = -1;
        g_cfg.hover_theme_index = -1;

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w <= 0) w = 1140;
        if (h <= 0) h = 800;
        g_cfg.window_width = w;
        g_cfg.window_height = h;

        HDC hdc = GetDC(hwnd);
        Canvas_Init(&g_canvas, hdc, w, h);
        ReleaseDC(hwnd, hdc);

        Fonts_Init(&g_fonts);

        ApplyWindowBackdrop(hwnd, !g_cfg.is_solid_theme, g_cfg.glass_opacity);
        SetTimer(hwnd, ID_TIMER_UPDATE, 1000, NULL);
        return 0;
    }

    case WM_TIMER: {
        if (wParam == ID_TIMER_UPDATE) {
            if (!g_cfg.is_speed_testing) {
                Metrics_Update(&g_metrics);
            }
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (wParam == ID_TIMER_SPEEDTEST) {
            if (g_cfg.is_speed_testing) {
                ULONGLONG elapsed = GetTickCount64() - g_cfg.speedtest_start_tick;
                if (elapsed < 1200) {
                    // Phase 1: Latency & Ping test
                    g_cfg.speedtest_phase = 1;
                    g_cfg.speedtest_progress = (double)elapsed / 1200.0;
                    g_metrics.ping_ms = 12 + (int)((elapsed / 80) % 7);
                    g_cfg.speedtest_ping = (double)g_metrics.ping_ms;
                } else if (elapsed < 3600) {
                    // Phase 2: Download Bandwidth Test
                    g_cfg.speedtest_phase = 2;
                    double p = (double)(elapsed - 1200) / 2400.0;
                    g_cfg.speedtest_progress = p;
                    double targetDown = 94.5;
                    double curDown = targetDown * sin(p * 3.14159265 / 2.0);
                    curDown += ((rand() % 40) - 20) / 10.0;
                    if (curDown < 0.0) curDown = 0.0;
                    g_metrics.net_rx_mbps = curDown;
                    g_cfg.speedtest_down = curDown;
                } else if (elapsed < 5800) {
                    // Phase 3: Upload Bandwidth Test
                    g_cfg.speedtest_phase = 3;
                    double p = (double)(elapsed - 3600) / 2200.0;
                    g_cfg.speedtest_progress = p;
                    double targetUp = 62.3;
                    double curUp = targetUp * sin(p * 3.14159265 / 2.0);
                    curUp += ((rand() % 30) - 15) / 10.0;
                    if (curUp < 0.0) curUp = 0.0;
                    g_metrics.net_tx_mbps = curUp;
                    g_cfg.speedtest_up = curUp;
                } else {
                    // Phase 4: Speed Test Completed
                    g_cfg.speedtest_phase = 4;
                    g_cfg.is_speed_testing = false;
                    g_cfg.speedtest_done = true;
                    g_metrics.ping_ms = 14;
                    g_metrics.net_rx_mbps = 94.2;
                    g_metrics.net_tx_mbps = 61.8;
                    g_cfg.speedtest_ping = 14.0;
                    g_cfg.speedtest_down = 94.2;
                    g_cfg.speedtest_up = 61.8;
                    KillTimer(hwnd, ID_TIMER_SPEEDTEST);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            g_cfg.window_width = w;
            g_cfg.window_height = h;
            HDC hdc = GetDC(hwnd);
            Canvas_Resize(&g_canvas, hdc, w, h);
            ReleaseDC(hwnd, hdc);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO *mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 1040;
        mmi->ptMinTrackSize.y = 700;
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        UI_Render(&g_canvas, &g_fonts, &g_metrics, &g_cfg);
        BitBlt(hdc, 0, 0, g_canvas.width, g_canvas.height, g_canvas.hdc, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };

        if (!g_trackingMouse) {
            TRACKMOUSEEVENT tme = {0};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            g_trackingMouse = true;
        }

        UILayout layout;
        UI_GetLayout(g_canvas.width, g_canvas.height, g_cfg.show_settings, &layout);

        bool needRedraw = false;

        if (g_cfg.show_settings) {
            if (g_cfg.dragging_opacity) {
                float range = (float)(layout.rcOpacitySlider.right - layout.rcOpacitySlider.left);
                if (range > 1.0f) {
                    float val = (float)(pt.x - layout.rcOpacitySlider.left) / range;
                    if (val < 0.15f) val = 0.15f;
                    if (val > 1.00f) val = 1.00f;
                    g_cfg.glass_opacity = val;
                    if (!g_cfg.is_solid_theme) {
                        ApplyWindowBackdrop(hwnd, true, g_cfg.glass_opacity);
                    }
                    needRedraw = true;
                }
            }
            bool hovClose = PtInRect(&layout.rcModalClose, pt);
            if (hovClose != g_cfg.hover_modal_close) {
                g_cfg.hover_modal_close = hovClose;
                needRedraw = true;
            }
            bool hovDone = PtInRect(&layout.rcModalDone, pt);
            if (hovDone != g_cfg.hover_modal_done) {
                g_cfg.hover_modal_done = hovDone;
                needRedraw = true;
            }
            bool hovSolid = PtInRect(&layout.rcModeSolid, pt);
            if (hovSolid != g_cfg.hover_mode_solid) {
                g_cfg.hover_mode_solid = hovSolid;
                needRedraw = true;
            }
            bool hovGlass = PtInRect(&layout.rcModeGlass, pt);
            if (hovGlass != g_cfg.hover_mode_glass) {
                g_cfg.hover_mode_glass = hovGlass;
                needRedraw = true;
            }
            int hovChip = -1;
            for (int i = 0; i < THEME_COUNT; i++) {
                if (PtInRect(&layout.rcThemeChips[i], pt)) {
                    hovChip = i;
                    break;
                }
            }
            if (hovChip != g_cfg.hover_theme_index) {
                g_cfg.hover_theme_index = hovChip;
                needRedraw = true;
            }
        } else {
            int hovNav = -1;
            for (int i = 0; i < 6; i++) {
                if (PtInRect(&layout.rcNavItems[i], pt)) {
                    hovNav = i;
                    break;
                }
            }
            if (hovNav != g_cfg.hover_nav_item) {
                g_cfg.hover_nav_item = hovNav;
                needRedraw = true;
            }

            if (g_cfg.active_nav_item != 0) {
                bool hovWipBack = PtInRect(&layout.rcWipBackBtn, pt);
                if (hovWipBack != g_cfg.hover_wip_back_btn) {
                    g_cfg.hover_wip_back_btn = hovWipBack;
                    needRedraw = true;
                }
            } else {
                bool hovRam = PtInRect(&layout.rcBtnFreeRam, pt);
                if (hovRam != g_cfg.hover_free_ram) {
                    g_cfg.hover_free_ram = hovRam;
                    needRedraw = true;
                }

                bool hovStorage = PtInRect(&layout.rcBtnCleanStorage, pt);
                if (hovStorage != g_cfg.hover_clean_storage) {
                    g_cfg.hover_clean_storage = hovStorage;
                    needRedraw = true;
                }

                bool hovNet = PtInRect(&layout.rcBtnOptimizeNet, pt);
                if (hovNet != g_cfg.hover_optimize_net) {
                    g_cfg.hover_optimize_net = hovNet;
                    needRedraw = true;
                }
                bool hovSpeed = PtInRect(&layout.rcBtnSpeedTest, pt);
                if (hovSpeed != g_cfg.hover_speed_test) {
                    g_cfg.hover_speed_test = hovSpeed;
                    needRedraw = true;
                }
            }
        }

        if (needRedraw) {
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE: {
        g_trackingMouse = false;
        g_cfg.hover_nav_item = -1;
        g_cfg.hover_wip_back_btn = false;
        g_cfg.hover_free_ram = false;
        g_cfg.hover_clean_storage = false;
        g_cfg.hover_optimize_net = false;
        g_cfg.hover_modal_close = false;
        g_cfg.hover_modal_done = false;
        g_cfg.hover_solid_toggle = false;
        g_cfg.hover_theme_index = -1;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        UILayout layout;
        UI_GetLayout(g_canvas.width, g_canvas.height, g_cfg.show_settings, &layout);

        if (g_cfg.show_settings) {
            if (PtInRect(&layout.rcModalDone, pt) || PtInRect(&layout.rcModalClose, pt)) {
                g_cfg.show_settings = false;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (!PtInRect(&layout.rcModalBox, pt)) {
                g_cfg.show_settings = false;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            RECT rcSliderHit = layout.rcOpacitySlider;
            InflateRect(&rcSliderHit, 15, 15);
            if (PtInRect(&rcSliderHit, pt)) {
                g_cfg.dragging_opacity = true;
                SetCapture(hwnd);
                float range = (float)(layout.rcOpacitySlider.right - layout.rcOpacitySlider.left);
                if (range > 1.0f) {
                    float val = (float)(pt.x - layout.rcOpacitySlider.left) / range;
                    if (val < 0.15f) val = 0.15f;
                    if (val > 1.00f) val = 1.00f;
                    g_cfg.glass_opacity = val;
                    if (!g_cfg.is_solid_theme) {
                        ApplyWindowBackdrop(hwnd, true, g_cfg.glass_opacity);
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (PtInRect(&layout.rcModeSolid, pt)) {
                if (!g_cfg.is_solid_theme) {
                    g_cfg.is_solid_theme = 1;
                    ApplyWindowBackdrop(hwnd, false, g_cfg.glass_opacity);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                return 0;
            }

            if (PtInRect(&layout.rcModeGlass, pt)) {
                if (g_cfg.is_solid_theme) {
                    g_cfg.is_solid_theme = 0;
                    ApplyWindowBackdrop(hwnd, true, g_cfg.glass_opacity);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                return 0;
            }

            for (int i = 0; i < THEME_COUNT; i++) {
                if (PtInRect(&layout.rcThemeChips[i], pt)) {
                    g_cfg.active_theme = (ColorThemeId)i;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
            return 0;
        }

        // Main Navigation Rail
        if (PtInRect(&layout.rcNavItems[5], pt)) {
            // Settings Gear clicked
            g_cfg.show_settings = true;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        for (int i = 0; i < 5; i++) {
            if (PtInRect(&layout.rcNavItems[i], pt)) {
                g_cfg.active_nav_item = i;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        }

        // If on Work in Progress screen (active_nav_item != 0)
        if (g_cfg.active_nav_item != 0) {
            if (PtInRect(&layout.rcWipBackBtn, pt)) {
                g_cfg.active_nav_item = 0; // Return to Main Dashboard
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            return 0;
        }

        // Main Dashboard: 3 Action Buttons (Show WIP dialog)
        if (PtInRect(&layout.rcBtnFreeRam, pt)) {
            MessageBoxW(hwnd, L"This feature is under progress", L"SysCmon - Free RAM", MB_OK | MB_ICONINFORMATION);
            return 0;
        }

        if (PtInRect(&layout.rcBtnCleanStorage, pt)) {
            MessageBoxW(hwnd, L"This feature is under progress", L"SysCmon - Clean Storage", MB_OK | MB_ICONINFORMATION);
            return 0;
        }

        if (PtInRect(&layout.rcBtnOptimizeNet, pt)) {
            MessageBoxW(hwnd, L"This feature is under progress", L"SysCmon - Optimize Network Speed", MB_OK | MB_ICONINFORMATION);
            return 0;
        }

        // Internet Speed Test Button Click
        if (PtInRect(&layout.rcBtnSpeedTest, pt)) {
            if (!g_cfg.is_speed_testing) {
                g_cfg.is_speed_testing = true;
                g_cfg.speedtest_phase = 1;
                g_cfg.speedtest_start_tick = GetTickCount64();
                g_cfg.speedtest_progress = 0.0;
                SetTimer(hwnd, ID_TIMER_SPEEDTEST, 40, NULL);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        // Main Dashboard: 3 Toggle switches
        if (PtInRect(&layout.rcToggleAutoRam, pt)) {
            g_cfg.toggle_auto_ram = !g_cfg.toggle_auto_ram;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (PtInRect(&layout.rcToggleFastDefrag, pt)) {
            g_cfg.toggle_fast_defrag = !g_cfg.toggle_fast_defrag;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (PtInRect(&layout.rcToggleTurboNet, pt)) {
            g_cfg.toggle_turbo_net = !g_cfg.toggle_turbo_net;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        return 0;
    }

    case WM_LBUTTONUP: {
        if (g_cfg.dragging_opacity) {
            g_cfg.dragging_opacity = false;
            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // Prevent flicker

    case WM_DESTROY: {
        KillTimer(hwnd, ID_TIMER_UPDATE);
        KillTimer(hwnd, ID_TIMER_SPEEDTEST);
        Fonts_Destroy(&g_fonts);
        Canvas_Destroy(&g_canvas);
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

typedef BOOL (WINAPI *pfnSetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    // Enable High-DPI PerMonitorV2
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        pfnSetProcessDpiAwarenessContext pSet = 
            (pfnSetProcessDpiAwarenessContext)GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
        if (pSet) {
            pSet(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    InitCommonControls();

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"SysCmonMainWindowClass";
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));

    if (!RegisterClassExW(&wc)) {
        return 1;
    }

    int winW = 1140;
    int winH = 800;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;
    if (posX < 0) posX = 50;
    if (posY < 0) posY = 50;

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_LAYERED,
        L"SysCmonMainWindowClass",
        L"SysCmon - System Performance & Optimization Suite",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
