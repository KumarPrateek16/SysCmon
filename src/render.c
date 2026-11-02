#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "app.h"
#include "render.h"

const ColorThemeDef g_ColorThemes[THEME_COUNT] = {
    // 0: Cyan (Cyberpunk Neon)
    {
        .name = "Cyberpunk Cyan",
        .primary = RGB(0, 210, 255),
        .accentLight = RGB(128, 240, 255),
        .accentDark = RGB(0, 110, 160),
        .glow = RGB(0, 210, 255),
        .textAccent = RGB(0, 210, 255)
    },
    // 1: Emerald (Matrix Green)
    {
        .name = "Emerald Matrix",
        .primary = RGB(16, 185, 129),
        .accentLight = RGB(110, 231, 183),
        .accentDark = RGB(4, 120, 87),
        .glow = RGB(16, 185, 129),
        .textAccent = RGB(52, 211, 153)
    },
    // 2: Amethyst (Royal Neon Violet)
    {
        .name = "Royal Amethyst",
        .primary = RGB(168, 85, 247),
        .accentLight = RGB(216, 180, 254),
        .accentDark = RGB(126, 34, 206),
        .glow = RGB(168, 85, 247),
        .textAccent = RGB(192, 132, 252)
    },
    // 3: Amber (Sunset Flame)
    {
        .name = "Sunset Amber",
        .primary = RGB(245, 158, 11),
        .accentLight = RGB(253, 230, 138),
        .accentDark = RGB(180, 83, 9),
        .glow = RGB(245, 158, 11),
        .textAccent = RGB(251, 191, 36)
    },
    // 4: Frost (Arctic Ice)
    {
        .name = "Arctic Frost",
        .primary = RGB(56, 189, 248),
        .accentLight = RGB(224, 242, 254),
        .accentDark = RGB(3, 105, 161),
        .glow = RGB(56, 189, 248),
        .textAccent = RGB(186, 230, 253)
    }
};

COLORREF StateColor(SystemState st) {
    switch (st) {
        case SYS_STATE_CRITICAL: return RGB(239, 68, 68);   // Red
        case SYS_STATE_WARNING:  return RGB(250, 204, 21);  // Yellow
        case SYS_STATE_NORMAL:
        default:                 return RGB(56, 189, 248);  // Light Blue
    }
}

COLORREF StateGlow(SystemState st) {
    switch (st) {
        case SYS_STATE_CRITICAL: return RGB(248, 113, 113);
        case SYS_STATE_WARNING:  return RGB(253, 224, 71);
        case SYS_STATE_NORMAL:
        default:                 return RGB(125, 211, 252);
    }
}

COLORREF StateFill(SystemState st) {
    switch (st) {
        case SYS_STATE_CRITICAL: return RGB(48, 14, 18);
        case SYS_STATE_WARNING:  return RGB(45, 38, 8);
        case SYS_STATE_NORMAL:
        default:                 return RGB(10, 35, 52);
    }
}

const wchar_t* StateText(SystemState st) {
    switch (st) {
        case SYS_STATE_CRITICAL: return L"CRITICAL";
        case SYS_STATE_WARNING:  return L"WARNING";
        case SYS_STATE_NORMAL:
        default:                 return L"NORMAL";
    }
}

void Canvas_Init(Canvas *c, HDC hdcScreen, int width, int height) {
    c->width = width;
    c->height = height;

    ZeroMemory(&c->bmi, sizeof(BITMAPINFO));
    c->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    c->bmi.bmiHeader.biWidth = width;
    c->bmi.bmiHeader.biHeight = -height;
    c->bmi.bmiHeader.biPlanes = 1;
    c->bmi.bmiHeader.biBitCount = 32;
    c->bmi.bmiHeader.biCompression = BI_RGB;

    c->hdc = CreateCompatibleDC(hdcScreen);
    c->hBitmap = CreateDIBSection(c->hdc, &c->bmi, DIB_RGB_COLORS, (void**)&c->pixels, NULL, 0);
    c->hOldBitmap = (HBITMAP)SelectObject(c->hdc, c->hBitmap);
}

void Canvas_Resize(Canvas *c, HDC hdcScreen, int width, int height) {
    if (c->width == width && c->height == height && c->hBitmap != NULL) return;
    Canvas_Destroy(c);
    Canvas_Init(c, hdcScreen, width, height);
}

void Canvas_Destroy(Canvas *c) {
    if (c->hdc) {
        SelectObject(c->hdc, c->hOldBitmap);
        DeleteDC(c->hdc);
        c->hdc = NULL;
    }
    if (c->hBitmap) {
        DeleteObject(c->hBitmap);
        c->hBitmap = NULL;
    }
    c->pixels = NULL;
}

void Fonts_Init(FontCache *fc) {
    fc->hFontTitle = CreateFontW(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontHeader = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontSubheader = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontBody = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontSmall = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontBigNum = CreateFontW(-28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontDialNum = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontBadge = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontBtn = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontParamBadge = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontIcon = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    fc->hFontWipIcon = CreateFontW(-42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
    fc->hFontStatValue = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fc->hFontStatLabel = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

void Fonts_Destroy(FontCache *fc) {
    if (fc->hFontTitle) DeleteObject(fc->hFontTitle);
    if (fc->hFontHeader) DeleteObject(fc->hFontHeader);
    if (fc->hFontSubheader) DeleteObject(fc->hFontSubheader);
    if (fc->hFontBody) DeleteObject(fc->hFontBody);
    if (fc->hFontSmall) DeleteObject(fc->hFontSmall);
    if (fc->hFontBigNum) DeleteObject(fc->hFontBigNum);
    if (fc->hFontDialNum) DeleteObject(fc->hFontDialNum);
    if (fc->hFontBadge) DeleteObject(fc->hFontBadge);
    if (fc->hFontBtn) DeleteObject(fc->hFontBtn);
    if (fc->hFontParamBadge) DeleteObject(fc->hFontParamBadge);
    if (fc->hFontIcon) DeleteObject(fc->hFontIcon);
    if (fc->hFontWipIcon) DeleteObject(fc->hFontWipIcon);
    if (fc->hFontStatValue) DeleteObject(fc->hFontStatValue);
    if (fc->hFontStatLabel) DeleteObject(fc->hFontStatLabel);
}

void UI_GetLayout(int w, int h, bool showSettings, UILayout *layout) {
    int sidebarW = 64;
    layout->rcSidebar.left = 0;
    layout->rcSidebar.top = 0;
    layout->rcSidebar.right = sidebarW;
    layout->rcSidebar.bottom = h;

    // Sidebar items
    int iconH = 44;
    for (int i = 0; i < 5; i++) {
        layout->rcNavItems[i].left = 12;
        layout->rcNavItems[i].top = 75 + (i * 50);
        layout->rcNavItems[i].right = sidebarW - 12;
        layout->rcNavItems[i].bottom = layout->rcNavItems[i].top + iconH;
    }
    // Settings icon at bottom
    layout->rcNavItems[5].left = 12;
    layout->rcNavItems[5].bottom = h - 18;
    layout->rcNavItems[5].top = layout->rcNavItems[5].bottom - iconH;
    layout->rcNavItems[5].right = sidebarW - 12;

    // Note: Top-right overall badge removed per user request for clean de-cluttered titlebar

    // Main Columns
    int contentX = sidebarW + 20;
    int contentW = w - contentX - 20;
    int colGap = 14;
    int col1W = (int)(contentW * 0.63);
    int col2W = contentW - col1W - colGap;
    int col2X = contentX + col1W + colGap;

    // 3 Action Cards (Home Dashboard)
    int cTop = 56;
    int cardH = 84;
    int cardGap = 10;
    int btnW = 112;
    int btnH = 30;
    int toggleW = 36;
    int toggleH = 18;

    // Card 1: Speed Up / Free RAM
    layout->rcCardSpeedUp.left = contentX;
    layout->rcCardSpeedUp.top = cTop;
    layout->rcCardSpeedUp.right = contentX + col1W;
    layout->rcCardSpeedUp.bottom = cTop + cardH;

    layout->rcBtnFreeRam.right = layout->rcCardSpeedUp.right - 18;
    layout->rcBtnFreeRam.left = layout->rcBtnFreeRam.right - btnW;
    layout->rcBtnFreeRam.top = layout->rcCardSpeedUp.top + 12;
    layout->rcBtnFreeRam.bottom = layout->rcBtnFreeRam.top + btnH;

    layout->rcToggleAutoRam.right = layout->rcCardSpeedUp.right - 18;
    layout->rcToggleAutoRam.left = layout->rcToggleAutoRam.right - toggleW;
    layout->rcToggleAutoRam.top = layout->rcBtnFreeRam.bottom + 10;
    layout->rcToggleAutoRam.bottom = layout->rcToggleAutoRam.top + toggleH;

    // Card 2: Disk Cleanup
    layout->rcCardDiskClean.left = contentX;
    layout->rcCardDiskClean.top = layout->rcCardSpeedUp.bottom + cardGap;
    layout->rcCardDiskClean.right = contentX + col1W;
    layout->rcCardDiskClean.bottom = layout->rcCardDiskClean.top + cardH;

    layout->rcBtnCleanStorage.right = layout->rcCardDiskClean.right - 18;
    layout->rcBtnCleanStorage.left = layout->rcBtnCleanStorage.right - btnW;
    layout->rcBtnCleanStorage.top = layout->rcCardDiskClean.top + 12;
    layout->rcBtnCleanStorage.bottom = layout->rcBtnCleanStorage.top + btnH;

    layout->rcToggleFastDefrag.right = layout->rcCardDiskClean.right - 18;
    layout->rcToggleFastDefrag.left = layout->rcToggleFastDefrag.right - toggleW;
    layout->rcToggleFastDefrag.top = layout->rcBtnCleanStorage.bottom + 10;
    layout->rcToggleFastDefrag.bottom = layout->rcToggleFastDefrag.top + toggleH;

    // Card 3: Network Speed Optimization
    layout->rcCardNetOpt.left = contentX;
    layout->rcCardNetOpt.top = layout->rcCardDiskClean.bottom + cardGap;
    layout->rcCardNetOpt.right = contentX + col1W;
    layout->rcCardNetOpt.bottom = layout->rcCardNetOpt.top + cardH;

    layout->rcBtnOptimizeNet.right = layout->rcCardNetOpt.right - 18;
    layout->rcBtnOptimizeNet.left = layout->rcBtnOptimizeNet.right - btnW;
    layout->rcBtnOptimizeNet.top = layout->rcCardNetOpt.top + 12;
    layout->rcBtnOptimizeNet.bottom = layout->rcBtnOptimizeNet.top + btnH;

    layout->rcToggleTurboNet.right = layout->rcCardNetOpt.right - 18;
    layout->rcToggleTurboNet.left = layout->rcToggleTurboNet.right - toggleW;
    layout->rcToggleTurboNet.top = layout->rcBtnOptimizeNet.bottom + 10;
    layout->rcToggleTurboNet.bottom = layout->rcToggleTurboNet.top + toggleH;

    // Bottom Performance Graphs Card
    int bottomCardsTop = layout->rcCardNetOpt.bottom + 12; // 316
    layout->rcCardPerf.left = contentX;
    layout->rcCardPerf.top = bottomCardsTop;
    layout->rcCardPerf.right = contentX + col1W;
    layout->rcCardPerf.bottom = h - 18;

    // Right Column Cards in user-specified order:
    // Top    : Internet Throughput
    // Middle : Hardware Telemetry
    // Bottom : System {Normal or Warning or Critical}
    int rightTop = cTop;
    int rightBottom = h - 18;
    int rightTotalH = rightBottom - rightTop;
    int rightGap = 12;

    int hInternet = (int)(rightTotalH * 0.29);
    if (hInternet < 190) hInternet = 190;
    layout->rcCardInternet.left = col2X;
    layout->rcCardInternet.top = rightTop;
    layout->rcCardInternet.right = col2X + col2W;
    layout->rcCardInternet.bottom = rightTop + hInternet;

    layout->rcBtnSpeedTest.right = layout->rcCardInternet.right - 14;
    layout->rcBtnSpeedTest.left = layout->rcBtnSpeedTest.right - 116;
    layout->rcBtnSpeedTest.top = layout->rcCardInternet.top + 14;
    layout->rcBtnSpeedTest.bottom = layout->rcBtnSpeedTest.top + 32;

    int hHardware = (int)(rightTotalH * 0.38);
    if (hHardware < 235) hHardware = 235;
    layout->rcCardHardware.left = col2X;
    layout->rcCardHardware.top = layout->rcCardInternet.bottom + rightGap;
    layout->rcCardHardware.right = col2X + col2W;
    layout->rcCardHardware.bottom = layout->rcCardHardware.top + hHardware;

    layout->rcCardSystemState.left = col2X;
    layout->rcCardSystemState.top = layout->rcCardHardware.bottom + rightGap;
    layout->rcCardSystemState.right = col2X + col2W;
    layout->rcCardSystemState.bottom = rightBottom;

    // Work In Progress Screen
    int wipW = 640;
    int wipH = 380;
    layout->rcWipCard.left = contentX + (contentW - wipW) / 2;
    layout->rcWipCard.right = layout->rcWipCard.left + wipW;
    layout->rcWipCard.top = (h - wipH) / 2;
    layout->rcWipCard.bottom = layout->rcWipCard.top + wipH;

    layout->rcWipBackBtn.left = layout->rcWipCard.left + (wipW - 240) / 2;
    layout->rcWipBackBtn.right = layout->rcWipBackBtn.left + 240;
    layout->rcWipBackBtn.bottom = layout->rcWipCard.bottom - 42;
    layout->rcWipBackBtn.top = layout->rcWipBackBtn.bottom - 42;

    // Settings Modal (Organized Grouped Card Layout)
    int modalW = 580;
    int modalH = 490;
    layout->rcModalBox.left = (w - modalW) / 2;
    layout->rcModalBox.top = (h - modalH) / 2;
    layout->rcModalBox.right = layout->rcModalBox.left + modalW;
    layout->rcModalBox.bottom = layout->rcModalBox.top + modalH;

    layout->rcModalClose.right = layout->rcModalBox.right - 20;
    layout->rcModalClose.left = layout->rcModalClose.right - 28;
    layout->rcModalClose.top = layout->rcModalBox.top + 20;
    layout->rcModalClose.bottom = layout->rcModalClose.top + 28;

    // Section 1: Visual Theme Mode (Segmented Control)
    int sec1Top = layout->rcModalBox.top + 80;
    int pillW = (modalW - 64 - 14) / 2; // ~251px
    int pillH = 36;
    layout->rcModeSolid.left = layout->rcModalBox.left + 32;
    layout->rcModeSolid.top = sec1Top;
    layout->rcModeSolid.right = layout->rcModeSolid.left + pillW;
    layout->rcModeSolid.bottom = sec1Top + pillH;

    layout->rcModeGlass.left = layout->rcModeSolid.right + 14;
    layout->rcModeGlass.top = sec1Top;
    layout->rcModeGlass.right = layout->rcModeGlass.left + pillW;
    layout->rcModeGlass.bottom = sec1Top + pillH;

    // Section 2: Glass Frost Transparency Slider
    int sec2Top = sec1Top + pillH + 28; // ~144
    layout->rcOpacitySlider.left = layout->rcModalBox.left + 32;
    layout->rcOpacitySlider.top = sec2Top + 24;
    layout->rcOpacitySlider.right = layout->rcModalBox.right - 100;
    layout->rcOpacitySlider.bottom = layout->rcOpacitySlider.top + 14;

    // Section 3: Accent Color Theme
    int sec3Top = sec2Top + 54; // ~198
    int chipStartX = layout->rcModalBox.left + 32;
    for (int i = 0; i < THEME_COUNT; i++) {
        layout->rcThemeChips[i].left = chipStartX + (i * 48);
        layout->rcThemeChips[i].top = sec3Top + 22;
        layout->rcThemeChips[i].right = layout->rcThemeChips[i].left + 32;
        layout->rcThemeChips[i].bottom = layout->rcThemeChips[i].top + 32;
    }

    // Section 4: Diagnostic Threshold Cards
    int sec4Top = sec3Top + 68; // ~266
    int cardW = (modalW - 64 - 16) / 3; // ~166px
    int hCardH = 54;
    for (int i = 0; i < 3; i++) {
        layout->rcHealthCards[i].left = layout->rcModalBox.left + 32 + i * (cardW + 8);
        layout->rcHealthCards[i].top = sec4Top + 22;
        layout->rcHealthCards[i].right = layout->rcHealthCards[i].left + cardW;
        layout->rcHealthCards[i].bottom = layout->rcHealthCards[i].top + hCardH;
    }

    // Footer: Apply & Close
    layout->rcModalDone.left = (layout->rcModalBox.left + layout->rcModalBox.right) / 2 - 85;
    layout->rcModalDone.right = layout->rcModalDone.left + 170;
    layout->rcModalDone.bottom = layout->rcModalBox.bottom - 22;
    layout->rcModalDone.top = layout->rcModalDone.bottom - 38;
}
static inline uint32_t BlendColor(uint32_t bg, uint32_t fg, uint8_t alpha) {
    if (alpha == 0) return bg;
    if (alpha == 255) return fg;

    uint32_t bgB = bg & 0xFF;
    uint32_t bgG = (bg >> 8) & 0xFF;
    uint32_t bgR = (bg >> 16) & 0xFF;

    uint32_t fgB = fg & 0xFF;
    uint32_t fgG = (fg >> 8) & 0xFF;
    uint32_t fgR = (fg >> 16) & 0xFF;

    uint32_t inv = 255 - alpha;
    uint32_t r = (fgR * alpha + bgR * inv) / 255;
    uint32_t g = (fgG * alpha + bgG * inv) / 255;
    uint32_t b = (fgB * alpha + bgB * inv) / 255;

    return (r << 16) | (g << 8) | b;
}

static void DrawBackground(Canvas *c, const AppConfig *cfg, COLORREF accent) {
    int w = c->width;
    int h = c->height;
    uint32_t *px = c->pixels;

    if (cfg->is_solid_theme) {
        // Deep obsidian solid matte dark background (100% solid, fully opaque)
        uint32_t solidBg = 0x0A0D15;
        for (int i = 0; i < w * h; i++) {
            px[i] = solidBg;
        }
        return;
    }

    // High-End Nordic Midnight Frosted Glass Backdrop
    // Deep slate-obsidian gradient with soft ambient atmospheric illumination
    uint32_t topColor = 0x080C16;
    uint32_t botColor = 0x0E1424;

    uint32_t accB = GetBValue(accent);
    uint32_t accG = GetGValue(accent);
    uint32_t accR = GetRValue(accent);
    uint32_t glowColor = (accR << 16) | (accG << 8) | accB;

    // Smooth horizontal diffuse ambient wash across the top third
    int glowCx = (int)(w * 0.50);
    int glowCy = (int)(h * 0.16);
    float glowRx = (float)(w * 0.65);
    float glowRy = (float)(h * 0.45);
    float rxSq = glowRx * glowRx;
    float rySq = glowRy * glowRy;

    for (int y = 0; y < h; y++) {
        float t = (float)y / (float)h;
        uint32_t r = (uint32_t)(((topColor >> 16) & 0xFF) * (1.0f - t) + ((botColor >> 16) & 0xFF) * t);
        uint32_t g = (uint32_t)(((topColor >> 8) & 0xFF) * (1.0f - t) + ((botColor >> 8) & 0xFF) * t);
        uint32_t b = (uint32_t)((topColor & 0xFF) * (1.0f - t) + (botColor & 0xFF) * t);
        uint32_t baseLine = (r << 16) | (g << 8) | b;

        float dy = (float)(y - glowCy);
        float dyTerm = (dy * dy) / rySq;

        uint32_t *row = &px[y * w];
        for (int x = 0; x < w; x++) {
            float dx = (float)(x - glowCx);
            float distNormSq = (dx * dx) / rxSq + dyTerm;
            if (distNormSq < 1.0f) {
                float falloff = (1.0f - sqrtf(distNormSq));
                falloff = falloff * falloff; // Smooth quadratic falloff
                uint8_t glowAlpha = (uint8_t)(falloff * 24.0f); // Gentle, subtle ambient sheen
                row[x] = BlendColor(baseLine, glowColor, glowAlpha);
            } else {
                row[x] = baseLine;
            }
        }
    }
}

static void DrawCard(Canvas *c, const AppConfig *cfg, int x, int y, int cw, int ch, int radius, COLORREF borderColor, uint8_t borderAlpha) {
    int w = c->width;
    int h = c->height;
    uint32_t *px = c->pixels;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + cw > w) cw = w - x;
    if (y + ch > h) ch = h - y;
    if (cw <= 0 || ch <= 0) return;

    if (cfg->is_solid_theme) {
        uint32_t cardSolid = 0x111622;
        uint32_t borderSolid = 0x1D2638;

        for (int j = 0; j < ch; j++) {
            int py = y + j;
            uint32_t *row = &px[py * w];
            for (int i = 0; i < cw; i++) {
                int px_pos = x + i;
                bool isBorder = (i == 0 || i == cw - 1 || j == 0 || j == ch - 1);
                row[px_pos] = isBorder ? borderSolid : cardSolid;
            }
        }
        return;
    }

    // High-End Frosted Glassmorphism Card (Layered depth with specular edges)
    float intensity = cfg->glass_opacity;
    if (intensity < 0.15f) intensity = 0.15f;
    if (intensity > 1.00f) intensity = 1.00f;

    uint8_t cardAlpha = (uint8_t)(150.0f + intensity * 65.0f); // 160 to 215
    uint32_t glassTint = 0x0D1424;

    uint32_t bB = GetBValue(borderColor);
    uint32_t bG = GetGValue(borderColor);
    uint32_t bR = GetRValue(borderColor);
    uint32_t bColor = (bR << 16) | (bG << 8) | bB;

    int radSq = radius * radius;

    for (int j = 0; j < ch; j++) {
        int py = y + j;
        uint32_t *row = &px[py * w];

        for (int i = 0; i < cw; i++) {
            int px_pos = x + i;

            int cornerDistSq = 0;
            if (i < radius && j < radius) {
                int dx = radius - i;
                int dy = radius - j;
                cornerDistSq = dx * dx + dy * dy;
            } else if (i >= cw - radius && j < radius) {
                int dx = i - (cw - radius - 1);
                int dy = radius - j;
                cornerDistSq = dx * dx + dy * dy;
            } else if (i < radius && j >= ch - radius) {
                int dx = radius - i;
                int dy = j - (ch - radius - 1);
                cornerDistSq = dx * dx + dy * dy;
            } else if (i >= cw - radius && j >= ch - radius) {
                int dx = i - (cw - radius - 1);
                int dy = j - (ch - radius - 1);
                cornerDistSq = dx * dx + dy * dy;
            }

            if (cornerDistSq > radSq) {
                continue;
            }

            uint32_t currentPix = row[px_pos];
            currentPix = BlendColor(currentPix, glassTint, cardAlpha);

            // Refraction Specular Sheen on Top & Left Edge
            bool isTopEdge = (j == 0 || (j == 1 && cornerDistSq > 0));
            bool isLeftEdge = (i == 0 || (i == 1 && cornerDistSq > 0));
            bool isBottomEdge = (j == ch - 1);
            bool isRightEdge = (i == cw - 1);

            if (isTopEdge) {
                currentPix = BlendColor(currentPix, 0xFFFFFF, 35); // Delicate light refraction
            } else if (isLeftEdge) {
                currentPix = BlendColor(currentPix, 0xFFFFFF, 20);
            } else if (isBottomEdge) {
                currentPix = BlendColor(currentPix, 0x000000, 50); // Soft shadow
            } else if (isRightEdge) {
                currentPix = BlendColor(currentPix, 0x000000, 35);
            }

            // Outer perimeter accent border
            if (borderAlpha > 0 && (i == 0 || i == cw - 1 || j == 0 || j == ch - 1)) {
                currentPix = BlendColor(currentPix, bColor, borderAlpha);
            }

            row[px_pos] = currentPix;
        }
    }
}

static void DrawHexLogo(HDC hdc, int cx, int cy, int size, COLORREF color) {
    POINT pts[6];
    for (int i = 0; i < 6; i++) {
        double angle = (60.0 * i - 30.0) * 3.14159265 / 180.0;
        pts[i].x = cx + (int)(size * cos(angle));
        pts[i].y = cy + (int)(size * sin(angle));
    }

    HPEN hPen = CreatePen(PS_SOLID, 2, color);
    HBRUSH hBrush = CreateSolidBrush(RGB(10, 18, 30));
    HPEN hOldP = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hBrush);

    Polygon(hdc, pts, 6);

    // Heartbeat line inside hexagon
    HPEN hPulse = CreatePen(PS_SOLID, 2, color);
    SelectObject(hdc, hPulse);
    int y0 = cy;
    MoveToEx(hdc, cx - size/2, y0, NULL);
    LineTo(hdc, cx - size/4, y0);
    LineTo(hdc, cx - size/8, y0 - size/3);
    LineTo(hdc, cx + size/8, y0 + size/3);
    LineTo(hdc, cx + size/4, y0);
    LineTo(hdc, cx + size/2, y0);

    SelectObject(hdc, hOldP);
    SelectObject(hdc, hOldB);
    DeleteObject(hPen);
    DeleteObject(hBrush);
    DeleteObject(hPulse);
}

static void DrawToggleSwitch(HDC hdc, int x, int y, int w, int h, bool isOn, COLORREF activeColor) {
    COLORREF bgColor = isOn ? activeColor : RGB(30, 42, 60);
    HBRUSH hBg = CreateSolidBrush(bgColor);
    HPEN hPen = CreatePen(PS_SOLID, 1, isOn ? activeColor : RGB(50, 68, 95));
    HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hBg);
    HPEN hOldP = (HPEN)SelectObject(hdc, hPen);

    RoundRect(hdc, x, y, x + w, y + h, h, h);

    SelectObject(hdc, hOldB);
    SelectObject(hdc, hOldP);
    DeleteObject(hBg);
    DeleteObject(hPen);

    int thumbD = h - 4;
    int thumbX = isOn ? (x + w - thumbD - 2) : (x + 2);
    int thumbY = y + 2;

    HBRUSH hThumb = CreateSolidBrush(RGB(255, 255, 255));
    SelectObject(hdc, hThumb);
    Ellipse(hdc, thumbX, thumbY, thumbX + thumbD, thumbY + thumbD);
    DeleteObject(hThumb);
}

static void DrawCircularGauge(HDC hdc, int cx, int cy, int radius, double val, double maxVal, COLORREF color, const wchar_t *valStr, const wchar_t *unitStr, const wchar_t *labelStr, FontCache *fc) {
    int thickness = 7;

    // Draw background track arc
    HPEN hTrackPen = CreatePen(PS_SOLID, thickness, RGB(22, 32, 48));
    HPEN hOldP = (HPEN)SelectObject(hdc, hTrackPen);
    HBRUSH hOldB = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    int boxLeft = cx - radius;
    int boxTop = cy - radius;
    int boxRight = cx + radius;
    int boxBottom = cy + radius;

    // 270 degree gauge: arc from 135 deg to 405 deg
    Arc(hdc, boxLeft, boxTop, boxRight, boxBottom,
        cx + (int)(radius * cos(135.0 * 3.14159/180.0)), cy - (int)(radius * sin(135.0 * 3.14159/180.0)),
        cx + (int)(radius * cos(45.0 * 3.14159/180.0)), cy - (int)(radius * sin(45.0 * 3.14159/180.0)));

    SelectObject(hdc, hOldP);
    DeleteObject(hTrackPen);

    // Draw filled active arc
    if (val > 0.05) {
        if (val > maxVal) val = maxVal;
        double pct = val / maxVal;
        double startAngle = 225.0; // Left bottom
        double endAngle = 225.0 - (pct * 270.0);
        if (endAngle < -45.0) endAngle = -45.0;

        HPEN hActivePen = CreatePen(PS_SOLID, thickness, color);
        SelectObject(hdc, hActivePen);

        Arc(hdc, boxLeft, boxTop, boxRight, boxBottom,
            cx + (int)(radius * cos(startAngle * 3.14159/180.0)), cy - (int)(radius * sin(startAngle * 3.14159/180.0)),
            cx + (int)(radius * cos(endAngle * 3.14159/180.0)), cy - (int)(radius * sin(endAngle * 3.14159/180.0)));

        SelectObject(hdc, hOldP);
        DeleteObject(hActivePen);
    }

    SelectObject(hdc, hOldB);

    // Number text in center
    SelectObject(hdc, fc->hFontDialNum);
    SetTextColor(hdc, RGB(255, 255, 255));
    RECT rcVal = { cx - radius, cy - 16, cx + radius, cy + 8 };
    DrawTextW(hdc, valStr, -1, &rcVal, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Unit text below number
    SelectObject(hdc, fc->hFontSmall);
    SetTextColor(hdc, RGB(140, 160, 190));
    RECT rcUnit = { cx - radius, cy + 8, cx + radius, cy + 24 };
    DrawTextW(hdc, unitStr, -1, &rcUnit, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Label below gauge
    SelectObject(hdc, fc->hFontSubheader);
    SetTextColor(hdc, color);
    RECT rcLabel = { cx - radius - 20, cy + radius + 6, cx + radius + 20, cy + radius + 26 };
    DrawTextW(hdc, labelStr, -1, &rcLabel, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void DrawParamStateBadge(HDC hdc, int rightX, int topY, SystemState st, FontCache *fc) {
    COLORREF color = StateColor(st);
    const wchar_t *txt = StateText(st);

    int badgeW = 74;
    int badgeH = 20;
    int badgeX = rightX - badgeW;
    int badgeY = topY;

    HBRUSH hBg = CreateSolidBrush(RGB(14, 20, 30));
    HPEN hPen = CreatePen(PS_SOLID, 1, color);
    HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hBg);
    HPEN hOldP = (HPEN)SelectObject(hdc, hPen);

    RoundRect(hdc, badgeX, badgeY, badgeX + badgeW, badgeY + badgeH, 8, 8);

    SelectObject(hdc, hOldB);
    SelectObject(hdc, hOldP);
    DeleteObject(hBg);
    DeleteObject(hPen);

    SelectObject(hdc, fc->hFontParamBadge);
    SetTextColor(hdc, color);
    RECT rc = { badgeX, badgeY, badgeX + badgeW, badgeY + badgeH };
    DrawTextW(hdc, txt, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

static void DrawSparkline(Canvas *c, const double *data, int len, int x, int y, int w, int h, COLORREF lineColor, COLORREF fillColor, double maxVal) {
    if (len < 2 || w <= 10 || h <= 10) return;
    if (maxVal <= 0.001) maxVal = 100.0;

    HDC hdc = c->hdc;

    HPEN hPenGrid = CreatePen(PS_DOT, 1, RGB(28, 38, 54));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPenGrid);

    int y33 = y + (int)(h * 0.66);
    int y66 = y + (int)(h * 0.33);

    MoveToEx(hdc, x, y33, NULL); LineTo(hdc, x + w, y33);
    MoveToEx(hdc, x, y66, NULL); LineTo(hdc, x + w, y66);

    POINT pts[HISTORY_LEN + 2];
    int ptCount = 0;

    pts[ptCount].x = x;
    pts[ptCount].y = y + h;
    ptCount++;

    for (int i = 0; i < len; i++) {
        double val = data[i];
        if (val < 0.0) val = 0.0;
        if (val > maxVal) val = maxVal;

        int px = x + (int)((double)i / (double)(len - 1) * w);
        int py = y + h - (int)((val / maxVal) * (h - 3));

        pts[ptCount].x = px;
        pts[ptCount].y = py;
        ptCount++;
    }

    pts[ptCount].x = x + w;
    pts[ptCount].y = y + h;
    ptCount++;

    HBRUSH hBrushFill = CreateSolidBrush(fillColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrushFill);
    HPEN hPenNone = CreatePen(PS_NULL, 0, 0);
    SelectObject(hdc, hPenNone);

    Polygon(hdc, pts, ptCount);

    DeleteObject(hPenNone);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrushFill);

    HPEN hPenLine = CreatePen(PS_SOLID, 2, lineColor);
    SelectObject(hdc, hPenLine);

    for (int i = 1; i < ptCount - 1; i++) {
        if (i == 1) {
            MoveToEx(hdc, pts[i].x, pts[i].y, NULL);
        } else {
            LineTo(hdc, pts[i].x, pts[i].y);
        }
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPenLine);
    DeleteObject(hPenGrid);
}

void UI_Render(Canvas *canvas, FontCache *fc, const SystemMetrics *m, const AppConfig *cfg) {
    int w = canvas->width;
    int h = canvas->height;
    HDC hdc = canvas->hdc;

    const ColorThemeDef *theme = &g_ColorThemes[cfg->active_theme];

    // 1. Draw Background
    DrawBackground(canvas, cfg, theme->primary);

    // 2. Compute Layout
    UILayout layout;
    UI_GetLayout(w, h, cfg->show_settings, &layout);

    // 3. Draw Left Navigation Sidebar Rail
    DrawCard(canvas, cfg, 0, 0, layout.rcSidebar.right, h, 0, RGB(20, 26, 36), 30);

    // Draw Hex Pulse Logo at top of sidebar
    DrawHexLogo(hdc, 32, 34, 18, theme->primary);

    // Draw Sidebar Navigation Icons
    const wchar_t *navIcons[5] = { L"⌂", L"▣", L"⛨", L"⚡", L"♨" };
    for (int i = 0; i < 5; i++) {
        RECT rc = layout.rcNavItems[i];
        bool isAct = (i == cfg->active_nav_item);
        bool isHov = (i == cfg->hover_nav_item);

        if (isAct) {
            HBRUSH hActiveBrush = CreateSolidBrush(RGB(14, 45, 65));
            HPEN hActivePen = CreatePen(PS_SOLID, 1, theme->primary);
            HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hActiveBrush);
            HPEN hOldP = (HPEN)SelectObject(hdc, hActivePen);
            RoundRect(hdc, rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 2, 8, 8);
            SelectObject(hdc, hOldB);
            SelectObject(hdc, hOldP);
            DeleteObject(hActiveBrush);
            DeleteObject(hActivePen);
        } else if (isHov) {
            HBRUSH hHovBrush = CreateSolidBrush(RGB(28, 38, 52));
            HPEN hHovPen = CreatePen(PS_SOLID, 1, RGB(45, 60, 80));
            HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hHovBrush);
            HPEN hOldP = (HPEN)SelectObject(hdc, hHovPen);
            RoundRect(hdc, rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 2, 8, 8);
            SelectObject(hdc, hOldB);
            SelectObject(hdc, hOldP);
            DeleteObject(hHovBrush);
            DeleteObject(hHovPen);
        }

        SelectObject(hdc, fc->hFontIcon);
        SetTextColor(hdc, isAct ? theme->primary : (isHov ? RGB(230, 245, 255) : RGB(100, 120, 145)));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, navIcons[i], -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // Settings Gear Icon at bottom of Sidebar
    {
        RECT rcSet = layout.rcNavItems[5];
        bool isSetHov = (cfg->hover_nav_item == 5 || cfg->show_settings);
        if (isSetHov) {
            HBRUSH hB = CreateSolidBrush(RGB(30, 42, 60));
            HPEN hP = CreatePen(PS_SOLID, 1, theme->primary);
            HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hB);
            HPEN hOldP = (HPEN)SelectObject(hdc, hP);
            RoundRect(hdc, rcSet.left + 2, rcSet.top + 2, rcSet.right - 2, rcSet.bottom - 2, 8, 8);
            SelectObject(hdc, hOldB);
            SelectObject(hdc, hOldP);
            DeleteObject(hB);
            DeleteObject(hP);
        }
        SelectObject(hdc, fc->hFontIcon);
        SetTextColor(hdc, isSetHov ? theme->primary : RGB(120, 135, 160));
        DrawTextW(hdc, L"⚙", -1, &rcSet, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 4. Top-right overall badge removed per user request for clean de-cluttered titlebar

    // 5. SCREEN DISPATCH: Main Dashboard (item 0) vs Work in Progress (items 1..4)
    if (cfg->active_nav_item == 0) {
        // =========================================================================
        // MAIN DASHBOARD (Home)
        // =========================================================================

        // Draw 3 Action Cards
        DrawCard(canvas, cfg, layout.rcCardSpeedUp.left, layout.rcCardSpeedUp.top,
                 layout.rcCardSpeedUp.right - layout.rcCardSpeedUp.left,
                 layout.rcCardSpeedUp.bottom - layout.rcCardSpeedUp.top, 10, RGB(35, 48, 70), 20);

        DrawCard(canvas, cfg, layout.rcCardDiskClean.left, layout.rcCardDiskClean.top,
                 layout.rcCardDiskClean.right - layout.rcCardDiskClean.left,
                 layout.rcCardDiskClean.bottom - layout.rcCardDiskClean.top, 10, RGB(35, 48, 70), 20);

        DrawCard(canvas, cfg, layout.rcCardNetOpt.left, layout.rcCardNetOpt.top,
                 layout.rcCardNetOpt.right - layout.rcCardNetOpt.left,
                 layout.rcCardNetOpt.bottom - layout.rcCardNetOpt.top, 10, RGB(35, 48, 70), 20);

        // Performance Multi-Graph Card
        DrawCard(canvas, cfg, layout.rcCardPerf.left, layout.rcCardPerf.top,
                 layout.rcCardPerf.right - layout.rcCardPerf.left,
                 layout.rcCardPerf.bottom - layout.rcCardPerf.top, 12, RGB(35, 48, 70), 20);

        // Right Column Cards (User Order: Top=Internet, Middle=Hardware, Bottom=SystemState)
        DrawCard(canvas, cfg, layout.rcCardInternet.left, layout.rcCardInternet.top,
                 layout.rcCardInternet.right - layout.rcCardInternet.left,
                 layout.rcCardInternet.bottom - layout.rcCardInternet.top, 12, RGB(35, 48, 70), 20);

        DrawCard(canvas, cfg, layout.rcCardHardware.left, layout.rcCardHardware.top,
                 layout.rcCardHardware.right - layout.rcCardHardware.left,
                 layout.rcCardHardware.bottom - layout.rcCardHardware.top, 12, RGB(35, 48, 70), 20);

        DrawCard(canvas, cfg, layout.rcCardSystemState.left, layout.rcCardSystemState.top,
                 layout.rcCardSystemState.right - layout.rcCardSystemState.left,
                 layout.rcCardSystemState.bottom - layout.rcCardSystemState.top, 12, StateColor(m->state), 40);

        // Action Buttons Background
        COLORREF ramBtnCol = cfg->hover_free_ram ? RGB(255, 255, 255) : theme->accentLight;
        DrawCard(canvas, cfg, layout.rcBtnFreeRam.left, layout.rcBtnFreeRam.top,
                 layout.rcBtnFreeRam.right - layout.rcBtnFreeRam.left,
                 layout.rcBtnFreeRam.bottom - layout.rcBtnFreeRam.top, 6,
                 cfg->hover_free_ram ? theme->primary : RGB(36, 52, 76), 40);

        COLORREF storageBtnCol = cfg->hover_clean_storage ? RGB(255, 255, 255) : theme->accentLight;
        DrawCard(canvas, cfg, layout.rcBtnCleanStorage.left, layout.rcBtnCleanStorage.top,
                 layout.rcBtnCleanStorage.right - layout.rcBtnCleanStorage.left,
                 layout.rcBtnCleanStorage.bottom - layout.rcBtnCleanStorage.top, 6,
                 cfg->hover_clean_storage ? theme->primary : RGB(36, 52, 76), 40);

        COLORREF netBtnCol = cfg->hover_optimize_net ? RGB(255, 255, 255) : theme->accentLight;
        DrawCard(canvas, cfg, layout.rcBtnOptimizeNet.left, layout.rcBtnOptimizeNet.top,
                 layout.rcBtnOptimizeNet.right - layout.rcBtnOptimizeNet.left,
                 layout.rcBtnOptimizeNet.bottom - layout.rcBtnOptimizeNet.top, 6,
                 cfg->hover_optimize_net ? theme->primary : RGB(36, 52, 76), 40);

        // --- CARD 1: Speed Up / Free RAM ---
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcCardSpeedUp.left + 16, layout.rcCardSpeedUp.top + 14, L"Speed Up / Free RAM", 19);
        SIZE sz1;
        GetTextExtentPoint32W(hdc, L"Speed Up / Free RAM", 19, &sz1);

        SelectObject(hdc, fc->hFontParamBadge);
        SetTextColor(hdc, StateColor(m->ram_state));
        TextOutW(hdc, layout.rcCardSpeedUp.left + 16 + sz1.cx + 10, layout.rcCardSpeedUp.top + 18, StateText(m->ram_state), (int)wcslen(StateText(m->ram_state)));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        wchar_t ramTxt[128];
        swprintf(ramTxt, 128, L"Programs tracked: %u | RAM in use: %.1f%% (%.2f GB of %.2f GB)",
                 (unsigned)m->process_count, m->ram_usage_pct, m->ram_used_gb, m->ram_total_gb);
        TextOutW(hdc, layout.rcCardSpeedUp.left + 16, layout.rcCardSpeedUp.top + 38, ramTxt, (int)wcslen(ramTxt));

        wchar_t ramSub2[128];
        swprintf(ramSub2, 128, L"RAM releasable: ~%.0f MB  |  Cache: %.2f GB",
                 m->ram_releasable_mb, m->ram_cached_gb);
        TextOutW(hdc, layout.rcCardSpeedUp.left + 16, layout.rcCardSpeedUp.top + 56, ramSub2, (int)wcslen(ramSub2));

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, ramBtnCol);
        DrawTextW(hdc, L"Free RAM", -1, &layout.rcBtnFreeRam, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        DrawToggleSwitch(hdc, layout.rcToggleAutoRam.left, layout.rcToggleAutoRam.top,
                         layout.rcToggleAutoRam.right - layout.rcToggleAutoRam.left,
                         layout.rcToggleAutoRam.bottom - layout.rcToggleAutoRam.top,
                         cfg->toggle_auto_ram, theme->primary);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(140, 160, 185));
        const wchar_t *tLbl1 = L"Auto RAM Clean";
        SIZE szLbl1;
        GetTextExtentPoint32W(hdc, tLbl1, (int)wcslen(tLbl1), &szLbl1);
        TextOutW(hdc, layout.rcToggleAutoRam.left - 10 - szLbl1.cx,
                 layout.rcToggleAutoRam.top + (18 - szLbl1.cy) / 2, tLbl1, (int)wcslen(tLbl1));

        // --- CARD 2: Disk Cleanup & Storage ---
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcCardDiskClean.left + 16, layout.rcCardDiskClean.top + 14, L"Disk Cleanup", 12);
        SIZE sz2;
        GetTextExtentPoint32W(hdc, L"Disk Cleanup", 12, &sz2);

        SelectObject(hdc, fc->hFontParamBadge);
        SetTextColor(hdc, StateColor(m->storage_state));
        TextOutW(hdc, layout.rcCardDiskClean.left + 16 + sz2.cx + 10, layout.rcCardDiskClean.top + 18, StateText(m->storage_state), (int)wcslen(StateText(m->storage_state)));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        wchar_t diskTxt[128];
        swprintf(diskTxt, 128, L"Drive C: Free: %.1f GB of %.1f GB (%.1f%% Full)",
                 m->disk_free_gb, m->disk_total_gb, m->disk_usage_pct);
        TextOutW(hdc, layout.rcCardDiskClean.left + 16, layout.rcCardDiskClean.top + 38, diskTxt, (int)wcslen(diskTxt));

        wchar_t diskSub2[128];
        swprintf(diskSub2, 128, L"Cleanable temporary storage: ~%u MB  |  Fast Defrag Ready",
                 (unsigned)(180 + ((int)(m->uptime_seconds / 60) % 150)));
        TextOutW(hdc, layout.rcCardDiskClean.left + 16, layout.rcCardDiskClean.top + 56, diskSub2, (int)wcslen(diskSub2));

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, storageBtnCol);
        DrawTextW(hdc, L"Clean Storage", -1, &layout.rcBtnCleanStorage, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        DrawToggleSwitch(hdc, layout.rcToggleFastDefrag.left, layout.rcToggleFastDefrag.top,
                         layout.rcToggleFastDefrag.right - layout.rcToggleFastDefrag.left,
                         layout.rcToggleFastDefrag.bottom - layout.rcToggleFastDefrag.top,
                         cfg->toggle_fast_defrag, theme->primary);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(140, 160, 185));
        const wchar_t *tLbl2 = L"Fast Defrag";
        SIZE szLbl2;
        GetTextExtentPoint32W(hdc, tLbl2, (int)wcslen(tLbl2), &szLbl2);
        TextOutW(hdc, layout.rcToggleFastDefrag.left - 10 - szLbl2.cx,
                 layout.rcToggleFastDefrag.top + (18 - szLbl2.cy) / 2, tLbl2, (int)wcslen(tLbl2));

        // --- CARD 3: Network Speed Optimization ---
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcCardNetOpt.left + 16, layout.rcCardNetOpt.top + 14, L"Optimize Network Speed", 22);
        SIZE sz3;
        GetTextExtentPoint32W(hdc, L"Optimize Network Speed", 22, &sz3);

        SelectObject(hdc, fc->hFontParamBadge);
        SetTextColor(hdc, StateColor(m->net_state));
        TextOutW(hdc, layout.rcCardNetOpt.left + 16 + sz3.cx + 10, layout.rcCardNetOpt.top + 18, StateText(m->net_state), (int)wcslen(StateText(m->net_state)));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        wchar_t netTxt[128];
        swprintf(netTxt, 128, L"Activity: In %.1f KB/s | Out %.1f KB/s | Ping: %d ms",
                 m->net_rx_kbs, m->net_tx_kbs, m->ping_ms);
        TextOutW(hdc, layout.rcCardNetOpt.left + 16, layout.rcCardNetOpt.top + 38, netTxt, (int)wcslen(netTxt));

        wchar_t netSub2[128];
        swprintf(netSub2, 128, L"Buffer latency: %d ms | TCP scaling active | %hs",
                 m->ping_ms > 0 ? (m->ping_ms + 1) : 3, m->local_ip);
        TextOutW(hdc, layout.rcCardNetOpt.left + 16, layout.rcCardNetOpt.top + 56, netSub2, (int)wcslen(netSub2));

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, netBtnCol);
        DrawTextW(hdc, L"Optimize Net", -1, &layout.rcBtnOptimizeNet, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        DrawToggleSwitch(hdc, layout.rcToggleTurboNet.left, layout.rcToggleTurboNet.top,
                         layout.rcToggleTurboNet.right - layout.rcToggleTurboNet.left,
                         layout.rcToggleTurboNet.bottom - layout.rcToggleTurboNet.top,
                         cfg->toggle_turbo_net, theme->primary);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(140, 160, 185));
        const wchar_t *tLbl3 = L"Turbo Net Mode";
        SIZE szLbl3;
        GetTextExtentPoint32W(hdc, tLbl3, (int)wcslen(tLbl3), &szLbl3);
        TextOutW(hdc, layout.rcToggleTurboNet.left - 10 - szLbl3.cx,
                 layout.rcToggleTurboNet.top + (18 - szLbl3.cy) / 2, tLbl3, (int)wcslen(tLbl3));

        // --- BOTTOM CARD: System Performance Multi-Graph & Detailed Telemetry ---
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcCardPerf.left + 18, layout.rcCardPerf.top + 14, L"System Performance", 18);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(120, 140, 170));
        wchar_t perfSubTitle[128];
        swprintf(perfSubTitle, 128, L"Real-Time Kernel Counters  |  %hs", m->cpu_model);
        TextOutW(hdc, layout.rcCardPerf.left + 18, layout.rcCardPerf.top + 34, perfSubTitle, (int)wcslen(perfSubTitle));

        int perfW = layout.rcCardPerf.right - layout.rcCardPerf.left;

        // Left Column: Enlarged Multi-Graph Sparklines (~54% of card width)
        int graphColW = (int)((perfW - 48) * 0.54);
        if (graphColW < 300) graphColW = 300;
        int graphX = layout.rcCardPerf.left + 18;
        int graphY = layout.rcCardPerf.top + 58;
        int graphAvailH = layout.rcCardPerf.bottom - graphY - 14;
        int subCardH = max(68, (graphAvailH - 20) / 3);
        int subGraphH = subCardH - 30;

        // Sub-Card 1: CPU Utilization
        int yCpuSub = graphY;
        DrawCard(canvas, cfg, graphX, yCpuSub, graphColW, subCardH, 8, RGB(28, 40, 60), 20);

        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(160, 185, 215));
        TextOutW(hdc, graphX + 12, yCpuSub + 8, L"CPU Core Load", 13);

        SelectObject(hdc, fc->hFontStatValue);
        SetTextColor(hdc, StateColor(m->cpu_state));
        wchar_t cpuValStr[32];
        swprintf(cpuValStr, 32, L"%.1f%%", m->cpu_usage);
        RECT rcCpuVal = { graphX + graphColW - 120, yCpuSub + 6, graphX + graphColW - 12, yCpuSub + 28 };
        DrawTextW(hdc, cpuValStr, -1, &rcCpuVal, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        DrawSparkline(canvas, m->cpu_history, m->history_count, graphX + 8, yCpuSub + 26, graphColW - 16, subGraphH, StateColor(m->cpu_state), RGB(16, 30, 48), 100.0);

        // Sub-Card 2: Memory (RAM) Allocation
        int yRamSub = yCpuSub + subCardH + 8;
        DrawCard(canvas, cfg, graphX, yRamSub, graphColW, subCardH, 8, RGB(28, 40, 60), 20);

        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(160, 185, 215));
        TextOutW(hdc, graphX + 12, yRamSub + 8, L"Memory Allocation (RAM)", 23);

        SelectObject(hdc, fc->hFontStatValue);
        SetTextColor(hdc, StateColor(m->ram_state));
        wchar_t ramValStr[32];
        swprintf(ramValStr, 32, L"%.1f%%", m->ram_usage_pct);
        RECT rcRamVal = { graphX + graphColW - 120, yRamSub + 6, graphX + graphColW - 12, yRamSub + 28 };
        DrawTextW(hdc, ramValStr, -1, &rcRamVal, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        DrawSparkline(canvas, m->ram_history, m->history_count, graphX + 8, yRamSub + 26, graphColW - 16, subGraphH, StateColor(m->ram_state), RGB(26, 22, 42), 100.0);

        // Sub-Card 3: Storage & Disk I/O Rate
        int yDiskSub = yRamSub + subCardH + 8;
        DrawCard(canvas, cfg, graphX, yDiskSub, graphColW, subCardH, 8, RGB(28, 40, 60), 20);

        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(160, 185, 215));
        TextOutW(hdc, graphX + 12, yDiskSub + 8, L"Storage I/O Transfer Rate", 25);

        SelectObject(hdc, fc->hFontStatValue);
        SetTextColor(hdc, StateColor(m->disk_state));
        wchar_t diskValStr[32];
        if (m->disk_total_kbs >= 1024.0) {
            swprintf(diskValStr, 32, L"%.1f MB/s", m->disk_total_kbs / 1024.0);
        } else {
            swprintf(diskValStr, 32, L"%.0f KB/s", m->disk_total_kbs);
        }
        RECT rcDiskVal = { graphX + graphColW - 150, yDiskSub + 6, graphX + graphColW - 12, yDiskSub + 28 };
        DrawTextW(hdc, diskValStr, -1, &rcDiskVal, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        DrawSparkline(canvas, m->disk_history, m->history_count, graphX + 8, yDiskSub + 26, graphColW - 16, subGraphH, StateColor(m->disk_state), RGB(32, 28, 18), 20000.0);

        // Right Portion of System Performance Card: 3 Detailed Telemetry Cards
        int statsX = graphX + graphColW + 16;
        int statsW = layout.rcCardPerf.right - statsX - 18;
        int tCardH = subCardH;

        // Telemetry Card 1: Processor & Execution
        DrawCard(canvas, cfg, statsX, yCpuSub, statsW, tCardH, 8, RGB(30, 42, 65), 25);

        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(220, 235, 255));
        TextOutW(hdc, statsX + 14, yCpuSub + 10, L"Processor & Architecture", 24);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        int tCol1 = statsX + 14;
        int tCol2 = statsX + (statsW / 2) + 2;
        int tRow1 = yCpuSub + 30;
        int tRow2 = yCpuSub + 48;

        TextOutW(hdc, tCol1, tRow1, L"Clock:", 6);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t clockTxt[32];
        swprintf(clockTxt, 32, L"%.2f GHz", m->cpu_cur_ghz);
        TextOutW(hdc, tCol1 + 42, tRow1 - 1, clockTxt, (int)wcslen(clockTxt));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol2, tRow1, L"Cores:", 6);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t coreTxt[32];
        swprintf(coreTxt, 32, L"%d Cores", m->cpu_cores);
        TextOutW(hdc, tCol2 + 42, tRow1 - 1, coreTxt, (int)wcslen(coreTxt));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol1, tRow2, L"Procs:", 6);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t procTxt[32];
        swprintf(procTxt, 32, L"%u", (unsigned)m->process_count);
        TextOutW(hdc, tCol1 + 42, tRow2 - 1, procTxt, (int)wcslen(procTxt));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol2, tRow2, L"Threads:", 8);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t threadTxt[32];
        swprintf(threadTxt, 32, L"%u", (unsigned)m->thread_count);
        TextOutW(hdc, tCol2 + 54, tRow2 - 1, threadTxt, (int)wcslen(threadTxt));

        // Telemetry Card 2: Memory Allocation
        DrawCard(canvas, cfg, statsX, yRamSub, statsW, tCardH, 8, RGB(30, 42, 65), 25);

        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(220, 235, 255));
        TextOutW(hdc, statsX + 14, yRamSub + 10, L"Memory Allocation & Commit", 26);

        int mRow1 = yRamSub + 30;
        int mRow2 = yRamSub + 48;

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol1, mRow1, L"In Use:", 7);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t usedRamStr[32];
        swprintf(usedRamStr, 32, L"%.2f GB", m->ram_used_gb);
        TextOutW(hdc, tCol1 + 48, mRow1 - 1, usedRamStr, (int)wcslen(usedRamStr));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol2, mRow1, L"Available:", 10);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t freeRamStr[32];
        swprintf(freeRamStr, 32, L"%.2f GB", m->ram_free_gb);
        TextOutW(hdc, tCol2 + 60, mRow1 - 1, freeRamStr, (int)wcslen(freeRamStr));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol1, mRow2, L"Commit:", 7);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t commRamStr[32];
        swprintf(commRamStr, 32, L"%.1f GB", m->ram_commit_gb);
        TextOutW(hdc, tCol1 + 48, mRow2 - 1, commRamStr, (int)wcslen(commRamStr));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol2, mRow2, L"Cached:", 7);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t cacheRamStr[32];
        swprintf(cacheRamStr, 32, L"%.1f GB", m->ram_cached_gb);
        TextOutW(hdc, tCol2 + 48, mRow2 - 1, cacheRamStr, (int)wcslen(cacheRamStr));

        // Telemetry Card 3: Storage Subsystem & Drive C
        DrawCard(canvas, cfg, statsX, yDiskSub, statsW, tCardH, 8, RGB(30, 42, 65), 25);

        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(220, 235, 255));
        TextOutW(hdc, statsX + 14, yDiskSub + 10, L"Storage Subsystem & Drive C:", 28);

        int dRow1 = yDiskSub + 30;
        int dRow2 = yDiskSub + 48;

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol1, dRow1, L"Read:", 5);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t rRateStr[32];
        swprintf(rRateStr, 32, L"%.1f MB/s", m->disk_read_kbs / 1024.0);
        TextOutW(hdc, tCol1 + 38, dRow1 - 1, rRateStr, (int)wcslen(rRateStr));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol2, dRow1, L"Write:", 6);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t wRateStr[32];
        swprintf(wRateStr, 32, L"%.1f MB/s", m->disk_write_kbs / 1024.0);
        TextOutW(hdc, tCol2 + 42, dRow1 - 1, wRateStr, (int)wcslen(wRateStr));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol1, dRow2, L"Free:", 5);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(255, 255, 255));
        wchar_t fSpaceStr[32];
        swprintf(fSpaceStr, 32, L"%.1f GB", m->disk_free_gb);
        TextOutW(hdc, tCol1 + 38, dRow2 - 1, fSpaceStr, (int)wcslen(fSpaceStr));

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, tCol2, dRow2, L"Health:", 7);
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, StateColor(m->storage_state));
        TextOutW(hdc, tCol2 + 48, dRow2 - 1, StateText(m->storage_state), (int)wcslen(StateText(m->storage_state)));

        // =========================================================================
        // RIGHT COLUMN CARD 1 (TOP): Internet Throughput & Speed Test
        // =========================================================================
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcCardInternet.left + 16, layout.rcCardInternet.top + 14, L"Internet Throughput", 19);

        // Speed Test Button
                COLORREF speedBtnBorder = cfg->is_speed_testing ? RGB(250, 204, 21) : (cfg->hover_speed_test ? theme->accentLight : RGB(45, 60, 85));
        DrawCard(canvas, cfg, layout.rcBtnSpeedTest.left, layout.rcBtnSpeedTest.top,
                 layout.rcBtnSpeedTest.right - layout.rcBtnSpeedTest.left,
                 layout.rcBtnSpeedTest.bottom - layout.rcBtnSpeedTest.top, 6, speedBtnBorder, 60);

        SelectObject(hdc, fc->hFontBtn);
        if (cfg->is_speed_testing) {
            SetTextColor(hdc, RGB(250, 204, 21));
            DrawTextW(hdc, L"⚡ Testing...", -1, &layout.rcBtnSpeedTest, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else if (cfg->speedtest_done) {
            SetTextColor(hdc, cfg->hover_speed_test ? theme->accentLight : RGB(220, 235, 255));
            DrawTextW(hdc, L"⚡ Retest Speed", -1, &layout.rcBtnSpeedTest, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            SetTextColor(hdc, cfg->hover_speed_test ? theme->accentLight : RGB(220, 235, 255));
            DrawTextW(hdc, L"⚡ Test Speed", -1, &layout.rcBtnSpeedTest, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        // Subtitle Status Text
        SelectObject(hdc, fc->hFontSmall);
        wchar_t netSubTxt[128];
        if (cfg->is_speed_testing) {
            SetTextColor(hdc, RGB(250, 204, 21));
            if (cfg->speedtest_phase == 1) {
                swprintf(netSubTxt, 128, L"● Measuring Ping & Latency (ICMP Echo)...");
            } else if (cfg->speedtest_phase == 2) {
                swprintf(netSubTxt, 128, L"● Testing Download Throughput: %.1f Mbps", cfg->speedtest_down);
            } else {
                swprintf(netSubTxt, 128, L"● Testing Upload Throughput: %.1f Mbps", cfg->speedtest_up);
            }
        } else if (cfg->speedtest_done) {
            SetTextColor(hdc, RGB(56, 189, 248));
            swprintf(netSubTxt, 128, L"Speedtest Result: Latency %.0f ms | Down %.1f Mbps | Up %.1f Mbps",
                     cfg->speedtest_ping, cfg->speedtest_down, cfg->speedtest_up);
        } else {
            SetTextColor(hdc, RGB(130, 145, 170));
            swprintf(netSubTxt, 128, L"IP: %hs  |  Ping: %d ms  |  High-Speed Active", m->local_ip, m->ping_ms);
        }
        TextOutW(hdc, layout.rcCardInternet.left + 16, layout.rcCardInternet.top + 36, netSubTxt, (int)wcslen(netSubTxt));

        int netCardW = layout.rcCardInternet.right - layout.rcCardInternet.left;
        int netCardH = layout.rcCardInternet.bottom - layout.rcCardInternet.top;
        int gaugeRadius = 42;
        int gaugeCenterY = layout.rcCardInternet.top + 55 + (netCardH - 55) / 2 - 4;
        int cxDown = layout.rcCardInternet.left + (netCardW / 4) + 6;
        int cxUp = layout.rcCardInternet.left + (3 * netCardW / 4) - 6;

        wchar_t downStr[16], upStr[16];
        swprintf(downStr, 16, L"%.1f", m->net_rx_mbps);
        swprintf(upStr, 16, L"%.1f", m->net_tx_mbps);

        COLORREF downDialCol = (cfg->is_speed_testing && cfg->speedtest_phase == 2) ? RGB(56, 189, 248) : theme->primary;
        COLORREF upDialCol = (cfg->is_speed_testing && cfg->speedtest_phase == 3) ? RGB(168, 85, 247) : theme->primary;

        DrawCircularGauge(hdc, cxDown, gaugeCenterY, gaugeRadius, m->net_rx_mbps, 100.0,
                          downDialCol, downStr, L"Mbps", L"Download", fc);

        DrawCircularGauge(hdc, cxUp, gaugeCenterY, gaugeRadius, m->net_tx_mbps, 100.0,
                          upDialCol, upStr, L"Mbps", L"Upload", fc);

        // =========================================================================
        // RIGHT COLUMN CARD 2 (MIDDLE): Hardware Telemetry
        // =========================================================================
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcCardHardware.left + 16, layout.rcCardHardware.top + 14, L"Hardware Telemetry", 18);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        TextOutW(hdc, layout.rcCardHardware.left + 16, layout.rcCardHardware.top + 34, L"Sensors & Thermal Management", 28);

        int hwX = layout.rcCardHardware.left + 16;
        int hwR = layout.rcCardHardware.right - 16;
        int hwCardH = layout.rcCardHardware.bottom - layout.rcCardHardware.top;
        int hwAvailH = hwCardH - 58;
        int rowH = max(28, hwAvailH / 5);
        int rowY = layout.rcCardHardware.top + 54;

        // Row 1: CPU Core
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(140, 160, 185));
        TextOutW(hdc, hwX, rowY + 3, L"CPU Core", 8);
        wchar_t cpuTemp[32];
        swprintf(cpuTemp, 32, L"%.0f \u00B0C", m->cpu_temp_c);
        SetTextColor(hdc, StateColor(m->cpu_state));
        TextOutW(hdc, hwX + 130, rowY + 3, cpuTemp, (int)wcslen(cpuTemp));
        DrawParamStateBadge(hdc, hwR, rowY, m->cpu_state, fc);

        // Row 2: RAM
        rowY += rowH;
        SetTextColor(hdc, RGB(140, 160, 185));
        TextOutW(hdc, hwX, rowY + 3, L"Memory (RAM)", 12);
        wchar_t ramLoad[32];
        swprintf(ramLoad, 32, L"%.1f%% Load", m->ram_usage_pct);
        SetTextColor(hdc, StateColor(m->ram_state));
        TextOutW(hdc, hwX + 120, rowY + 3, ramLoad, (int)wcslen(ramLoad));
        DrawParamStateBadge(hdc, hwR, rowY, m->ram_state, fc);

        // Row 3: Disk I/O
        rowY += rowH;
        SetTextColor(hdc, RGB(140, 160, 185));
        TextOutW(hdc, hwX, rowY + 3, L"Disk Activity", 13);
        wchar_t diskRate[32];
        swprintf(diskRate, 32, L"%.0f KB/s", m->disk_total_kbs);
        SetTextColor(hdc, StateColor(m->disk_state));
        TextOutW(hdc, hwX + 120, rowY + 3, diskRate, (int)wcslen(diskRate));
        DrawParamStateBadge(hdc, hwR, rowY, m->disk_state, fc);

        // Row 4: Storage C: Drive
        rowY += rowH;
        SetTextColor(hdc, RGB(140, 160, 185));
        TextOutW(hdc, hwX, rowY + 3, L"SSD Storage C:", 14);
        wchar_t ssdTxt[32];
        swprintf(ssdTxt, 32, L"%.1f%% Full", m->disk_usage_pct);
        SetTextColor(hdc, StateColor(m->storage_state));
        TextOutW(hdc, hwX + 120, rowY + 3, ssdTxt, (int)wcslen(ssdTxt));
        DrawParamStateBadge(hdc, hwR, rowY, m->storage_state, fc);

        // Row 5: Motherboard / System
        rowY += rowH;
        SetTextColor(hdc, RGB(140, 160, 185));
        TextOutW(hdc, hwX, rowY + 3, L"Motherboard", 11);
        SetTextColor(hdc, RGB(56, 189, 248));
        wchar_t mbTempStr[32];
        swprintf(mbTempStr, 32, L"%.0f \u00B0C", 36.0 + (m->cpu_usage * 0.05));
        TextOutW(hdc, hwX + 130, rowY + 3, mbTempStr, (int)wcslen(mbTempStr));
        DrawParamStateBadge(hdc, hwR, rowY, SYS_STATE_NORMAL, fc);

        // =========================================================================
        // RIGHT COLUMN CARD 3 (BOTTOM): System {Normal or Warning or Critical}
        // =========================================================================
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcCardSystemState.left + 16, layout.rcCardSystemState.top + 12, L"System Health Assessment", 24);

        int stX = layout.rcCardSystemState.left + 16;
        int stR = layout.rcCardSystemState.right - 16;
        int stBoxTop = layout.rcCardSystemState.top + 38;
        int stBoxBottom = layout.rcCardSystemState.bottom - 46;

        COLORREF stCol = StateColor(m->state);
        COLORREF stFill = StateFill(m->state);

        HBRUSH hStBg = CreateSolidBrush(stFill);
        HPEN hStPen = CreatePen(PS_SOLID, 1, stCol);
        HBRUSH hOldStB = (HBRUSH)SelectObject(hdc, hStBg);
        HPEN hOldStP = (HPEN)SelectObject(hdc, hStPen);

        RoundRect(hdc, stX, stBoxTop, stR, stBoxBottom, 8, 8);

        SelectObject(hdc, hOldStB);
        SelectObject(hdc, hOldStP);
        DeleteObject(hStBg);
        DeleteObject(hStPen);

        const wchar_t *stateHeaderTxt = L"\x25CF  SYSTEM HEALTHY: NORMAL";
        if (m->state == SYS_STATE_WARNING) {
            stateHeaderTxt = L"\x25B2  SYSTEM WARNING: ELEVATED LOAD";
        } else if (m->state == SYS_STATE_CRITICAL) {
            stateHeaderTxt = L"\x25A0  SYSTEM CRITICAL: ATTENTION REQUIRED";
        }

        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, stCol);
        RECT rcStHdr = { stX + 12, stBoxTop + 8, stR - 12, stBoxTop + 26 };
        DrawTextW(hdc, stateHeaderTxt, -1, &rcStHdr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(220, 230, 245));
        wchar_t reasonW[256];
        MultiByteToWideChar(CP_UTF8, 0, m->state_reason, -1, reasonW, 256);
        RECT rcReason = { stX + 12, stBoxTop + 28, stR - 12, stBoxBottom - 6 };
        DrawTextW(hdc, reasonW, -1, &rcReason, DT_LEFT | DT_WORDBREAK);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 145, 170));
        wchar_t upTxt[64];
        int upSec = (int)m->uptime_seconds;
        int upH = upSec / 3600;
        int upM = (upSec % 3600) / 60;
        int upS = upSec % 60;
        swprintf(upTxt, 64, L"Live Uptime: %02d:%02d:%02d", upH, upM, upS);
        TextOutW(hdc, stX, layout.rcCardSystemState.bottom - 28, upTxt, (int)wcslen(upTxt));

        int badgeCpuX = stR - 130;
        SelectObject(hdc, fc->hFontParamBadge);
        SetTextColor(hdc, StateColor(m->cpu_state));
        TextOutW(hdc, badgeCpuX, layout.rcCardSystemState.bottom - 28, L"CPU \x25CF", 5);

        SetTextColor(hdc, StateColor(m->ram_state));
        TextOutW(hdc, badgeCpuX + 45, layout.rcCardSystemState.bottom - 28, L"RAM \x25CF", 5);

        SetTextColor(hdc, StateColor(m->disk_state));
        TextOutW(hdc, badgeCpuX + 90, layout.rcCardSystemState.bottom - 28, L"I/O \x25CF", 5);

    }
    else {
        // =========================================================================
        // WORK IN PROGRESS SCREEN (for extra sidebar buttons: 1, 2, 3, 4)
        // =========================================================================
        DrawCard(canvas, cfg, layout.rcWipCard.left, layout.rcWipCard.top,
                 layout.rcWipCard.right - layout.rcWipCard.left,
                 layout.rcWipCard.bottom - layout.rcWipCard.top,
                 16, theme->primary, 40);

        const wchar_t *wipIcon = L"▣";
        const wchar_t *wipTitle = L"Display & Multi-Monitor Diagnostics";
        const wchar_t *wipSubtitle = L"Hardware Display Engine & Refresh Rate Telemetry";

        switch (cfg->active_nav_item) {
            case 1:
                wipIcon = L"▣";
                wipTitle = L"Display & Multi-Monitor Diagnostics";
                wipSubtitle = L"Hardware Display Engine & Refresh Rate Telemetry";
                break;
            case 2:
                wipIcon = L"⛨";
                wipTitle = L"Security & Process Guard";
                wipSubtitle = L"Live Threat Detection & Process Isolation Sandbox";
                break;
            case 3:
                wipIcon = L"⚡";
                wipTitle = L"Advanced Turbo Tuning & Benchmarks";
                wipSubtitle = L"Power Governor & Performance Profile Controller";
                break;
            case 4:
                wipIcon = L"♨";
                wipTitle = L"Thermal Management & Fan Curves";
                wipSubtitle = L"Custom Acoustic Fan Curve Controller & Sensor Heatmap";
                break;
        }

        int wipCardW = layout.rcWipCard.right - layout.rcWipCard.left;
        int centerWipX = layout.rcWipCard.left + wipCardW / 2;

        // Big Icon
        SelectObject(hdc, fc->hFontWipIcon);
        SetTextColor(hdc, theme->primary);
        SetBkMode(hdc, TRANSPARENT);
        RECT rcIcon = { centerWipX - 60, layout.rcWipCard.top + 32, centerWipX + 60, layout.rcWipCard.top + 85 };
        DrawTextW(hdc, wipIcon, -1, &rcIcon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Badge: WORK IN PROGRESS
        SelectObject(hdc, fc->hFontBadge);
        SetTextColor(hdc, theme->textAccent);
        RECT rcWipBadge = { centerWipX - 120, layout.rcWipCard.top + 102, centerWipX + 120, layout.rcWipCard.top + 122 };
        DrawTextW(hdc, L"[ WORK IN PROGRESS ]", -1, &rcWipBadge, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Title
        SelectObject(hdc, fc->hFontTitle);
        SetTextColor(hdc, RGB(255, 255, 255));
        RECT rcWipTitle = { layout.rcWipCard.left + 20, layout.rcWipCard.top + 135, layout.rcWipCard.right - 20, layout.rcWipCard.top + 170 };
        DrawTextW(hdc, wipTitle, -1, &rcWipTitle, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Subtitle
        SelectObject(hdc, fc->hFontSubheader);
        SetTextColor(hdc, RGB(160, 180, 205));
        RECT rcWipSub = { layout.rcWipCard.left + 20, layout.rcWipCard.top + 175, layout.rcWipCard.right - 20, layout.rcWipCard.top + 200 };
        DrawTextW(hdc, wipSubtitle, -1, &rcWipSub, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);

        // Detail text
        SelectObject(hdc, fc->hFontBody);
        SetTextColor(hdc, RGB(120, 140, 165));
        RECT rcWipDesc = { layout.rcWipCard.left + 40, layout.rcWipCard.top + 215, layout.rcWipCard.right - 40, layout.rcWipCard.top + 265 };
        DrawTextW(hdc, L"This module is currently under active development for an upcoming release.\nDirect hardware telemetry probes, kernel filters, and control hooks are being assembled.", -1, &rcWipDesc, DT_CENTER | DT_WORDBREAK);

        // Return to Dashboard Button
        COLORREF backBorder = cfg->hover_wip_back_btn ? theme->primary : RGB(60, 80, 110);
        DrawCard(canvas, cfg, layout.rcWipBackBtn.left, layout.rcWipBackBtn.top,
                 layout.rcWipBackBtn.right - layout.rcWipBackBtn.left,
                 layout.rcWipBackBtn.bottom - layout.rcWipBackBtn.top,
                 10, backBorder, cfg->hover_wip_back_btn ? 80 : 35);

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, cfg->hover_wip_back_btn ? theme->accentLight : RGB(235, 245, 255));
        DrawTextW(hdc, L"Return to Dashboard", -1, &layout.rcWipBackBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    // 6. Settings Modal Dialog (overlay - Premium Redesign)
    if (cfg->show_settings) {
        uint32_t *px = canvas->pixels;
        for (int i = 0; i < w * h; i++) {
            px[i] = BlendColor(px[i], 0x050810, 205);
        }

        // Modal Frame Card
        DrawCard(canvas, cfg, layout.rcModalBox.left, layout.rcModalBox.top,
                 layout.rcModalBox.right - layout.rcModalBox.left,
                 layout.rcModalBox.bottom - layout.rcModalBox.top,
                 16, theme->primary, 75);

        // Close Button [X]
        COLORREF closeBorder = cfg->hover_modal_close ? RGB(239, 68, 68) : RGB(45, 60, 85);
        DrawCard(canvas, cfg, layout.rcModalClose.left, layout.rcModalClose.top,
                 layout.rcModalClose.right - layout.rcModalClose.left,
                 layout.rcModalClose.bottom - layout.rcModalClose.top,
                 6, closeBorder, cfg->hover_modal_close ? 90 : 35);

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, cfg->hover_modal_close ? RGB(255, 120, 120) : RGB(170, 185, 205));
        DrawTextW(hdc, L"\x2715", 1, &layout.rcModalClose, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Header Title & Subtitle
        SelectObject(hdc, fc->hFontHeader);
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, layout.rcModalBox.left + 32, layout.rcModalBox.top + 22, L"Settings & Personalization", 26);

        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(130, 150, 180));
        TextOutW(hdc, layout.rcModalBox.left + 32, layout.rcModalBox.top + 46, L"Customize application theme, window composition, and color palette.", 67);

        // --- SECTION 1: THEME MODE (Segmented Control) ---
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, theme->textAccent);
        TextOutW(hdc, layout.rcModalBox.left + 32, layout.rcModalBox.top + 76, L"VISUAL THEME MODE", 17);

        // Pill A: Solid Modern Matte
        COLORREF solidBrd = cfg->is_solid_theme ? theme->accentLight : (cfg->hover_mode_solid ? theme->primary : RGB(40, 55, 80));
        DrawCard(canvas, cfg, layout.rcModeSolid.left, layout.rcModeSolid.top,
                 layout.rcModeSolid.right - layout.rcModeSolid.left,
                 layout.rcModeSolid.bottom - layout.rcModeSolid.top,
                 8, solidBrd, cfg->is_solid_theme ? 90 : 40);

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, cfg->is_solid_theme ? RGB(255, 255, 255) : RGB(150, 170, 195));
        DrawTextW(hdc, L"\x25A0  Solid Modern Matte", -1, &layout.rcModeSolid, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Pill B: Frosted Glass
        COLORREF glassBrd = !cfg->is_solid_theme ? theme->accentLight : (cfg->hover_mode_glass ? theme->primary : RGB(40, 55, 80));
        DrawCard(canvas, cfg, layout.rcModeGlass.left, layout.rcModeGlass.top,
                 layout.rcModeGlass.right - layout.rcModeGlass.left,
                 layout.rcModeGlass.bottom - layout.rcModeGlass.top,
                 8, glassBrd, !cfg->is_solid_theme ? 90 : 40);

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, !cfg->is_solid_theme ? RGB(255, 255, 255) : RGB(150, 170, 195));
        DrawTextW(hdc, L"\x25A3  Frosted Glass (DWM)", -1, &layout.rcModeGlass, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // --- SECTION 2: GLASS OPACITY SLIDER ---
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, cfg->is_solid_theme ? RGB(90, 105, 125) : theme->textAccent);
        TextOutW(hdc, layout.rcModalBox.left + 32, layout.rcModalBox.top + 144, L"GLASS FROST TRANSPARENCY", 24);

        wchar_t opBadge[32];
        if (cfg->is_solid_theme) {
            swprintf(opBadge, 32, L"Solid Matte Active");
        } else {
            swprintf(opBadge, 32, L"%.0f%% Opacity", cfg->glass_opacity * 100.0f);
        }
        SIZE szOp;
        GetTextExtentPoint32W(hdc, opBadge, (int)wcslen(opBadge), &szOp);
        RECT rcOpBadge = { layout.rcModalBox.right - 32 - szOp.cx - 16, layout.rcModalBox.top + 140, layout.rcModalBox.right - 32, layout.rcModalBox.top + 160 };
        DrawCard(canvas, cfg, rcOpBadge.left, rcOpBadge.top, rcOpBadge.right - rcOpBadge.left, rcOpBadge.bottom - rcOpBadge.top, 5, RGB(40, 55, 80), 30);
        SetTextColor(hdc, cfg->is_solid_theme ? RGB(100, 115, 135) : theme->textAccent);
        DrawTextW(hdc, opBadge, -1, &rcOpBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Slider track
        int trackW = layout.rcOpacitySlider.right - layout.rcOpacitySlider.left;
        int fillW = (int)(trackW * cfg->glass_opacity);
        HBRUSH hTrackBg = CreateSolidBrush(RGB(20, 28, 42));
        HBRUSH hTrackFill = CreateSolidBrush(cfg->is_solid_theme ? RGB(45, 60, 80) : theme->primary);
        HPEN hTrackPen = CreatePen(PS_SOLID, 1, RGB(35, 50, 72));
        HBRUSH hOldB = (HBRUSH)SelectObject(hdc, hTrackBg);
        HPEN hOldP = (HPEN)SelectObject(hdc, hTrackPen);

        RoundRect(hdc, layout.rcOpacitySlider.left, layout.rcOpacitySlider.top + 2,
                  layout.rcOpacitySlider.right, layout.rcOpacitySlider.bottom - 2, 6, 6);

        if (fillW > 0) {
            SelectObject(hdc, hTrackFill);
            RoundRect(hdc, layout.rcOpacitySlider.left, layout.rcOpacitySlider.top + 2,
                      layout.rcOpacitySlider.left + fillW, layout.rcOpacitySlider.bottom - 2, 6, 6);
        }

        int thumbX = layout.rcOpacitySlider.left + fillW;
        int thumbY = (layout.rcOpacitySlider.top + layout.rcOpacitySlider.bottom) / 2;
        HBRUSH hThumbB = CreateSolidBrush(cfg->is_solid_theme ? RGB(140, 155, 175) : RGB(255, 255, 255));
        SelectObject(hdc, hThumbB);
        Ellipse(hdc, thumbX - 8, thumbY - 8, thumbX + 8, thumbY + 8);

        SelectObject(hdc, hOldB);
        SelectObject(hdc, hOldP);
        DeleteObject(hTrackBg);
        DeleteObject(hTrackFill);
        DeleteObject(hThumbB);
        DeleteObject(hTrackPen);

        // --- SECTION 3: ACCENT COLOR PALETTE ---
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, theme->textAccent);
        TextOutW(hdc, layout.rcModalBox.left + 32, layout.rcModalBox.top + 204, L"ACCENT COLOR PALETTE", 20);

        for (int i = 0; i < THEME_COUNT; i++) {
            RECT rcC = layout.rcThemeChips[i];
            bool isSel = (i == cfg->active_theme);
            bool isHov = (i == cfg->hover_theme_index);

            // Outer ring
            if (isSel || isHov) {
                HPEN hRingPen = CreatePen(PS_SOLID, 2, isSel ? theme->accentLight : RGB(70, 95, 130));
                HBRUSH hOldB3 = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                HPEN hOldP3 = (HPEN)SelectObject(hdc, hRingPen);
                Ellipse(hdc, rcC.left - 4, rcC.top - 4, rcC.right + 4, rcC.bottom + 4);
                SelectObject(hdc, hOldB3);
                SelectObject(hdc, hOldP3);
                DeleteObject(hRingPen);
            }

            HBRUSH hChipB = CreateSolidBrush(g_ColorThemes[i].primary);
            HPEN hChipP = CreatePen(PS_SOLID, 1, RGB(15, 22, 35));
            HBRUSH hOldB2 = (HBRUSH)SelectObject(hdc, hChipB);
            HPEN hOldP2 = (HPEN)SelectObject(hdc, hChipP);
            Ellipse(hdc, rcC.left, rcC.top, rcC.right, rcC.bottom);

            if (isSel) {
                HBRUSH hDot = CreateSolidBrush(RGB(255, 255, 255));
                SelectObject(hdc, hDot);
                int cx = (rcC.left + rcC.right) / 2;
                int cy = (rcC.top + rcC.bottom) / 2;
                Ellipse(hdc, cx - 4, cy - 4, cx + 4, cy + 4);
                DeleteObject(hDot);
            }

            SelectObject(hdc, hOldB2);
            SelectObject(hdc, hOldP2);
            DeleteObject(hChipB);
            DeleteObject(hChipP);
        }

        // Active Theme Name Label
        wchar_t themeLabel[64];
        swprintf(themeLabel, 64, L"Active Theme: %hs", g_ColorThemes[cfg->active_theme].name);
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(150, 175, 205));
        TextOutW(hdc, layout.rcThemeChips[4].right + 24, layout.rcThemeChips[0].top + 8, themeLabel, (int)wcslen(themeLabel));

        // --- SECTION 4: CLASSIFICATION THRESHOLDS GUIDE (Clean 3 Cards) ---
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, theme->textAccent);
        TextOutW(hdc, layout.rcModalBox.left + 32, layout.rcModalBox.top + 288, L"SYSTEM HEALTH CLASSIFICATION THRESHOLDS", 40);

        // Card 1: Normal
        DrawCard(canvas, cfg, layout.rcHealthCards[0].left, layout.rcHealthCards[0].top,
                 layout.rcHealthCards[0].right - layout.rcHealthCards[0].left,
                 layout.rcHealthCards[0].bottom - layout.rcHealthCards[0].top,
                 6, RGB(56, 189, 248), 50);
        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(56, 189, 248));
        RECT rcHdr0 = { layout.rcHealthCards[0].left + 12, layout.rcHealthCards[0].top + 7, layout.rcHealthCards[0].right - 8, layout.rcHealthCards[0].top + 24 };
        DrawTextW(hdc, L"\x25CF NORMAL", -1, &rcHdr0, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(180, 195, 215));
        TextOutW(hdc, layout.rcHealthCards[0].left + 12, layout.rcHealthCards[0].top + 28, L"CPU < 70% | RAM < 80%", 21);

        // Card 2: Warning
        DrawCard(canvas, cfg, layout.rcHealthCards[1].left, layout.rcHealthCards[1].top,
                 layout.rcHealthCards[1].right - layout.rcHealthCards[1].left,
                 layout.rcHealthCards[1].bottom - layout.rcHealthCards[1].top,
                 6, RGB(250, 204, 21), 50);
        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(250, 204, 21));
        RECT rcHdr1 = { layout.rcHealthCards[1].left + 12, layout.rcHealthCards[1].top + 7, layout.rcHealthCards[1].right - 8, layout.rcHealthCards[1].top + 24 };
        DrawTextW(hdc, L"\x25B2 WARNING", -1, &rcHdr1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(180, 195, 215));
        TextOutW(hdc, layout.rcHealthCards[1].left + 12, layout.rcHealthCards[1].top + 28, L"CPU 70-90% | RAM 80-92%", 23);

        // Card 3: Critical
        DrawCard(canvas, cfg, layout.rcHealthCards[2].left, layout.rcHealthCards[2].top,
                 layout.rcHealthCards[2].right - layout.rcHealthCards[2].left,
                 layout.rcHealthCards[2].bottom - layout.rcHealthCards[2].top,
                 6, RGB(239, 68, 68), 50);
        SelectObject(hdc, fc->hFontStatLabel);
        SetTextColor(hdc, RGB(239, 68, 68));
        RECT rcHdr2 = { layout.rcHealthCards[2].left + 12, layout.rcHealthCards[2].top + 7, layout.rcHealthCards[2].right - 8, layout.rcHealthCards[2].top + 24 };
        DrawTextW(hdc, L"\x25A0 CRITICAL", -1, &rcHdr2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, fc->hFontSmall);
        SetTextColor(hdc, RGB(180, 195, 215));
        RECT rcSub2 = { layout.rcHealthCards[2].left + 12, layout.rcHealthCards[2].top + 27, layout.rcHealthCards[2].right - 8, layout.rcHealthCards[2].top + 46 };
        DrawTextW(hdc, L"CPU >= 90% | RAM >= 92%", -1, &rcSub2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // --- FOOTER: APPLY & CLOSE BUTTON ---
        COLORREF doneBorder = cfg->hover_modal_done ? theme->accentLight : RGB(60, 80, 110);
        DrawCard(canvas, cfg, layout.rcModalDone.left, layout.rcModalDone.top,
                 layout.rcModalDone.right - layout.rcModalDone.left,
                 layout.rcModalDone.bottom - layout.rcModalDone.top,
                 8, doneBorder, cfg->hover_modal_done ? 90 : 45);

        SelectObject(hdc, fc->hFontBtn);
        SetTextColor(hdc, cfg->hover_modal_done ? theme->accentLight : RGB(240, 245, 255));
        DrawTextW(hdc, L"Apply & Close", -1, &layout.rcModalDone, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}
