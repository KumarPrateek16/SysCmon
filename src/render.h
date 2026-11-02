#ifndef RENDER_H
#define RENDER_H

#include "app.h"

// 32-bit ARGB Canvas
typedef struct {
    int width;
    int height;
    BITMAPINFO bmi;
    HBITMAP hBitmap;
    uint32_t *pixels;
    HDC hdc;
    HBITMAP hOldBitmap;
} Canvas;

// Fonts Cache
typedef struct {
    HFONT hFontTitle;
    HFONT hFontHeader;
    HFONT hFontSubheader;
    HFONT hFontBody;
    HFONT hFontSmall;
    HFONT hFontBigNum;
    HFONT hFontDialNum;
    HFONT hFontBadge;
    HFONT hFontBtn;
    HFONT hFontParamBadge;
    HFONT hFontIcon;
    HFONT hFontWipIcon;
    HFONT hFontStatValue;
    HFONT hFontStatLabel;
} FontCache;

// UI Layout Rectangles
typedef struct {
    // Sidebar
    RECT rcSidebar;
    RECT rcNavItems[6]; // 0=Home, 1=Display, 2=Security, 3=Speed, 4=Flame, 5=Settings

    // Action Cards & Buttons (Home Dashboard)
    RECT rcCardSpeedUp;
    RECT rcBtnFreeRam;
    RECT rcToggleAutoRam;

    RECT rcCardDiskClean;
    RECT rcBtnCleanStorage;
    RECT rcToggleFastDefrag;

    RECT rcCardNetOpt;
    RECT rcBtnOptimizeNet;
    RECT rcToggleTurboNet;

    // Bottom Performance Graph Card
    RECT rcCardPerf;

    // Right Column Cards (User Order: Top=Internet, Middle=Hardware, Bottom=SystemState)
    RECT rcCardInternet;
    RECT rcBtnSpeedTest;
    RECT rcCardHardware;
    RECT rcCardSystemState;

    // Work In Progress Screen (when active_nav_item != 0)
    RECT rcWipCard;
    RECT rcWipBackBtn;

    // Settings Modal
    RECT rcModalBox;
    RECT rcModalClose;
    RECT rcModeSolid;              // Segmented Control: Solid Modern Matte
    RECT rcModeGlass;              // Segmented Control: Frosted Glass
    RECT rcOpacitySlider;
    RECT rcThemeChips[THEME_COUNT];
    RECT rcHealthCards[3];         // Clean cards for Normal, Warning, Critical
    RECT rcModalDone;
} UILayout;

void Canvas_Init(Canvas *c, HDC hdcScreen, int width, int height);
void Canvas_Resize(Canvas *c, HDC hdcScreen, int width, int height);
void Canvas_Destroy(Canvas *c);

void Fonts_Init(FontCache *fc);
void Fonts_Destroy(FontCache *fc);

void UI_GetLayout(int winWidth, int winHeight, bool showSettings, UILayout *layout);
void UI_Render(Canvas *canvas, FontCache *fc, const SystemMetrics *m, const AppConfig *cfg);

COLORREF StateColor(SystemState st);
COLORREF StateGlow(SystemState st);
COLORREF StateFill(SystemState st);
const wchar_t* StateText(SystemState st);

#endif // RENDER_H
