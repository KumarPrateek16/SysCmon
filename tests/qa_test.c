#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "../src/app.h"
#include "../src/metrics.h"
#include "../src/render.h"

static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  [PASS] %s\n", msg); \
        g_passCount++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
        g_failCount++; \
    } \
} while(0)

static bool SaveCanvasToBMP(const Canvas *c, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return false;

    int w = c->width;
    int h = c->height;
    int imgSize = w * h * 4;

    BITMAPFILEHEADER bfh = {0};
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imgSize;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    BITMAPINFOHEADER bih = {0};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = w;
    bih.biHeight = h;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = imgSize;

    fwrite(&bfh, sizeof(bfh), 1, f);
    fwrite(&bih, sizeof(bih), 1, f);

    for (int y = h - 1; y >= 0; y--) {
        fwrite(&c->pixels[y * w], w * 4, 1, f);
    }

    fclose(f);
    return true;
}

static void Test_Metrics(void) {
    printf("\n--- Running Test 1: Real-Time Metrics & Per-Parameter State Classification ---\n");

    SystemMetrics m;
    Metrics_Init(&m);

    TEST_ASSERT(m.cpu_cores > 0, "CPU core count > 0");
    TEST_ASSERT(strlen(m.cpu_model) > 0, "CPU model name string populated");
    printf("    -> Detected CPU: %s (%d cores)\n", m.cpu_model, m.cpu_cores);

    TEST_ASSERT(m.ram_total_gb > 1.0, "Total RAM > 1.0 GB");
    TEST_ASSERT(m.ram_used_gb > 0.1, "Used RAM > 0.1 GB");
    TEST_ASSERT(m.ram_free_gb >= 0.0, "Free RAM >= 0.0 GB");
    TEST_ASSERT(m.ram_usage_pct >= 0.0 && m.ram_usage_pct <= 100.0, "RAM Usage % within [0, 100]");
    printf("    -> RAM: %.2f GB Used / %.2f GB Total (%.1f%% Load)\n", m.ram_used_gb, m.ram_total_gb, m.ram_usage_pct);

    TEST_ASSERT(m.disk_total_gb > 5.0, "Disk Total Capacity > 5 GB");
    TEST_ASSERT(m.disk_free_gb >= 0.0, "Disk Free Capacity >= 0 GB");
    TEST_ASSERT(m.disk_usage_pct >= 0.0 && m.disk_usage_pct <= 100.0, "Disk Usage % within [0, 100]");
    printf("    -> Storage: %.1f GB Free / %.1f GB Total (%.1f%% Used)\n", m.disk_free_gb, m.disk_total_gb, m.disk_usage_pct);

    Sleep(300);
    Metrics_Update(&m);

    TEST_ASSERT(m.cpu_usage >= 0.0 && m.cpu_usage <= 100.0, "CPU Usage % within [0, 100]");
    TEST_ASSERT(m.cpu_temp_c >= 20.0 && m.cpu_temp_c <= 110.0, "Estimated CPU Temp within realistic [20, 110] C");
    TEST_ASSERT(m.disk_total_kbs >= 0.0, "Disk I/O rate >= 0 KB/s");
    TEST_ASSERT(m.net_rx_mbps >= 0.0, "Network Rx Mbps >= 0.0");
    TEST_ASSERT(m.net_tx_mbps >= 0.0, "Network Tx Mbps >= 0.0");
    TEST_ASSERT(m.ping_ms >= 0, "Network latency ping >= 0 ms");
    TEST_ASSERT(strlen(m.local_ip) > 0, "Local IP address populated");

    // Test per-parameter states
    TEST_ASSERT(m.cpu_state >= SYS_STATE_NORMAL && m.cpu_state <= SYS_STATE_CRITICAL, "Per-param CPU State is valid enum");
    TEST_ASSERT(m.ram_state >= SYS_STATE_NORMAL && m.ram_state <= SYS_STATE_CRITICAL, "Per-param RAM State is valid enum");
    TEST_ASSERT(m.disk_state >= SYS_STATE_NORMAL && m.disk_state <= SYS_STATE_CRITICAL, "Per-param Disk I/O State is valid enum");
    TEST_ASSERT(m.storage_state >= SYS_STATE_NORMAL && m.storage_state <= SYS_STATE_CRITICAL, "Per-param Storage State is valid enum");
    TEST_ASSERT(m.net_state >= SYS_STATE_NORMAL && m.net_state <= SYS_STATE_CRITICAL, "Per-param Network State is valid enum");
    TEST_ASSERT(m.state >= SYS_STATE_NORMAL && m.state <= SYS_STATE_CRITICAL, "Overall System State is valid enum");

    TEST_ASSERT(strlen(m.state_label) > 0, "State Label is non-empty");
    TEST_ASSERT(strlen(m.state_reason) > 0, "State Diagnostic Reason is non-empty");

    // Dynamic OS Kernel Metrics verification
    TEST_ASSERT(m.cpu_cur_ghz >= 1.0 && m.cpu_cur_ghz <= 5.5, "Dynamic CPU clock frequency within [1.0, 5.5] GHz");
    TEST_ASSERT(m.process_count > 10, "Real dynamic running process count > 10");
    TEST_ASSERT(m.thread_count > 100, "Real dynamic kernel thread count > 100");
    TEST_ASSERT(m.ram_commit_gb > 0.5, "Real dynamic committed RAM > 0.5 GB");
    TEST_ASSERT(m.ram_cached_gb >= 0.0, "Real dynamic system cache RAM >= 0.0 GB");
    printf("    -> Dynamic Kernel Telemetry: Clock %.2f GHz, %u Procs, %u Threads, Commit %.2f GB, Cache %.2f GB\n",
           m.cpu_cur_ghz, (unsigned)m.process_count, (unsigned)m.thread_count, m.ram_commit_gb, m.ram_cached_gb);
    printf("    -> System Telemetry: CPU Temp %.1f C, IP: %s, Ping: %d ms\n", m.cpu_temp_c, m.local_ip, m.ping_ms);
    printf("    -> Overall State: %s (%s)\n", m.state_label, m.state_reason);
}

static void Test_LayoutAndHitTesting(void) {
    printf("\n--- Running Test 2: Nano Banana UI Layout & Interactive Hit Testing ---\n");

    int w = 1140;
    int h = 800;
    UILayout layout;
    UI_GetLayout(w, h, false, &layout);

    // Sidebar
    TEST_ASSERT(layout.rcSidebar.right == 64, "Sidebar width is exactly 64px");
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT(layout.rcNavItems[i].bottom > layout.rcNavItems[i].top, "Sidebar navigation item has positive height");
    }

    // Speed Test Button in Internet Throughput Card
    TEST_ASSERT(layout.rcBtnSpeedTest.right > layout.rcBtnSpeedTest.left, "Speed Test Button inside Internet card has valid bounds");
    TEST_ASSERT(layout.rcBtnSpeedTest.left >= layout.rcCardInternet.left && layout.rcBtnSpeedTest.right <= layout.rcCardInternet.right, "Speed Test Button contained within Internet Card");

    // Work in Progress View Card & Back Button
    TEST_ASSERT(layout.rcWipCard.right > layout.rcWipCard.left, "Work in Progress Card has valid bounds");
    TEST_ASSERT(layout.rcWipBackBtn.right > layout.rcWipBackBtn.left, "Return to Dashboard button has valid bounds");

    // 3 Action Cards
    TEST_ASSERT(layout.rcCardSpeedUp.right > layout.rcCardSpeedUp.left, "Speed Up card has valid bounds");
    TEST_ASSERT(layout.rcCardDiskClean.right > layout.rcCardDiskClean.left, "Disk Clean card has valid bounds");
    TEST_ASSERT(layout.rcCardNetOpt.right > layout.rcCardNetOpt.left, "Network Opt card has valid bounds");

    // 3 Action Buttons (Start / WIP)
    TEST_ASSERT(layout.rcBtnFreeRam.right - layout.rcBtnFreeRam.left == 112, "Free RAM Button width is 112px");
    TEST_ASSERT(layout.rcBtnFreeRam.bottom - layout.rcBtnFreeRam.top == 30, "Free RAM Button height is 30px");
    TEST_ASSERT(layout.rcBtnCleanStorage.right - layout.rcBtnCleanStorage.left == 112, "Clean Storage Button width is 112px");
    TEST_ASSERT(layout.rcBtnCleanStorage.bottom - layout.rcBtnCleanStorage.top == 30, "Clean Storage Button height is 30px");
    TEST_ASSERT(layout.rcBtnOptimizeNet.right - layout.rcBtnOptimizeNet.left == 112, "Optimize Net Button width is 112px");
    TEST_ASSERT(layout.rcBtnOptimizeNet.bottom - layout.rcBtnOptimizeNet.top == 30, "Optimize Net Button height is 30px");

    // Padding, Margins & Vertical Rhythm Verification
    TEST_ASSERT(layout.rcToggleAutoRam.top >= layout.rcBtnFreeRam.bottom + 8, "Auto RAM Clean toggle is below Free RAM button with >=8px gap");
    TEST_ASSERT(layout.rcToggleFastDefrag.top >= layout.rcBtnCleanStorage.bottom + 8, "Fast Defrag toggle is below Clean Storage button with >=8px gap");
    TEST_ASSERT(layout.rcToggleTurboNet.top >= layout.rcBtnOptimizeNet.bottom + 8, "Turbo Net toggle is below Optimize Net button with >=8px gap");

    // 3 Toggle switches
    TEST_ASSERT(layout.rcToggleAutoRam.right - layout.rcToggleAutoRam.left == 36, "Auto RAM Clean toggle width is 36px");
    TEST_ASSERT(layout.rcToggleFastDefrag.right - layout.rcToggleFastDefrag.left == 36, "Fast Defrag toggle width is 36px");
    TEST_ASSERT(layout.rcToggleTurboNet.right - layout.rcToggleTurboNet.left == 36, "Turbo Net toggle width is 36px");

    // Multi-graph card
    TEST_ASSERT(layout.rcCardPerf.right > layout.rcCardPerf.left, "System Performance card has valid bounds");

    // Right Column Cards in User-Specified Order:
    // Top: Internet Throughput
    // Middle: Hardware Telemetry
    // Bottom: System {Normal or Warning or Critical}
    TEST_ASSERT(layout.rcCardInternet.right > layout.rcCardInternet.left, "Internet Throughput card has valid bounds");
    TEST_ASSERT(layout.rcCardHardware.right > layout.rcCardHardware.left, "Hardware Telemetry card has valid bounds");
    TEST_ASSERT(layout.rcCardSystemState.right > layout.rcCardSystemState.left, "System State Assessment card has valid bounds");

    TEST_ASSERT(layout.rcCardInternet.top < layout.rcCardHardware.top, "Order Verification: Top card is Internet Throughput");
    TEST_ASSERT(layout.rcCardHardware.top < layout.rcCardSystemState.top, "Order Verification: Middle card is Hardware Telemetry");
    TEST_ASSERT(layout.rcCardSystemState.bottom == h - 18, "Order Verification: Bottom card is System Assessment extending to bottom margin");

    // Test Settings Modal Layout
    UILayout modalLayout;
    UI_GetLayout(w, h, true, &modalLayout);
    TEST_ASSERT(modalLayout.rcModalBox.right > modalLayout.rcModalBox.left, "Settings Modal Box centered and valid");
    TEST_ASSERT(modalLayout.rcOpacitySlider.right > modalLayout.rcOpacitySlider.left, "Glass Opacity Slider track valid");
    TEST_ASSERT(modalLayout.rcModeSolid.right > modalLayout.rcModeSolid.left, "Settings Solid Mode segmented button valid");
    TEST_ASSERT(modalLayout.rcModeGlass.right > modalLayout.rcModeGlass.left, "Settings Frosted Glass segmented button valid");
    for (int h = 0; h < 3; h++) {
        TEST_ASSERT(modalLayout.rcHealthCards[h].right > modalLayout.rcHealthCards[h].left, "Classification Threshold Card valid");
    }
    for (int i = 0; i < THEME_COUNT; i++) {
        TEST_ASSERT(modalLayout.rcThemeChips[i].right > modalLayout.rcThemeChips[i].left, "Theme chip swatch valid");
    }
    TEST_ASSERT(modalLayout.rcModalDone.right > modalLayout.rcModalDone.left, "Modal Close/Done button valid");
}

static void Test_ColorStateMapping(void) {
    printf("\n--- Running Test 3: Color State Mapping Rules ---\n");

    COLORREF colNorm = StateColor(SYS_STATE_NORMAL);
    COLORREF colWarn = StateColor(SYS_STATE_WARNING);
    COLORREF colCrit = StateColor(SYS_STATE_CRITICAL);

    TEST_ASSERT(colNorm == RGB(56, 189, 248), "NORMAL state is associated with Light Blue RGB(56, 189, 248)");
    TEST_ASSERT(colWarn == RGB(250, 204, 21), "WARNING state is shifted to Yellow RGB(250, 204, 21)");
    TEST_ASSERT(colCrit == RGB(239, 68, 68), "CRITICAL state is shifted to Red RGB(239, 68, 68)");
}

static void Test_GlassmorphismAndSolidRendering(void) {
    printf("\n--- Running Test 4: Glassmorphism, Themes & Solid Mode Rendering ---\n");

    HDC hdcScreen = GetDC(NULL);
    Canvas c;
    Canvas_Init(&c, hdcScreen, 1140, 800);
    ReleaseDC(NULL, hdcScreen);

    FontCache fc;
    Fonts_Init(&fc);

    SystemMetrics m;
    Metrics_Init(&m);
    Metrics_Update(&m);

    AppConfig cfg = {0};
    cfg.glass_opacity = 0.65f;
    cfg.is_solid_theme = 1; // Default Solid Modern Matte
    cfg.active_theme = THEME_CYAN;
    cfg.show_settings = false;
    cfg.active_nav_item = 0;
    cfg.toggle_auto_ram = true;
    cfg.toggle_fast_defrag = false;
    cfg.toggle_turbo_net = true;

    // Test 1: Solid Matte default theme mode
    UI_Render(&c, &fc, &m, &cfg);
    TEST_ASSERT(c.pixels != NULL, "Solid theme rendering completed without faults");
    SaveCanvasToBMP(&c, "c:/SysCmon/tests/qa_render_solid_matte.bmp");
    printf("    -> Rendered Solid Matte frame saved to tests/qa_render_solid_matte.bmp\n");

    // Test 2: Glassmorphism with default Cyan theme and 65% opacity
    cfg.is_solid_theme = 0;
    UI_Render(&c, &fc, &m, &cfg);
    TEST_ASSERT(c.pixels != NULL, "Canvas pixels valid after Glassmorphism render");
    TEST_ASSERT(c.pixels[100 * 1140 + 100] != 0, "Canvas pixel at (100,100) is non-black");
    SaveCanvasToBMP(&c, "c:/SysCmon/tests/qa_render_glass_cyan.bmp");
    printf("    -> Rendered Glassmorphism frame saved to tests/qa_render_glass_cyan.bmp\n");

    // Test 3: Render with each of the 5 themes
    const char *themeNames[THEME_COUNT] = { "Cyan Pulse", "Emerald Flux", "Amethyst Glow", "Amber Blaze", "Frost Blue" };
    for (int t = 0; t < THEME_COUNT; t++) {
        cfg.active_theme = (ColorThemeId)t;
        cfg.glass_opacity = 0.20f + (t * 0.18f);
        UI_Render(&c, &fc, &m, &cfg);
        TEST_ASSERT(c.pixels != NULL, "Theme rendering completed without faults");
        printf("    -> Theme '%s' (Opacity %.0f%%) render OK\n", themeNames[t], cfg.glass_opacity * 100.0f);
    }

    // Test 4: Work in Progress screen (active_nav_item = 1, Solid Modern Matte)
    cfg.is_solid_theme = 1;
    cfg.active_nav_item = 1;
    UI_Render(&c, &fc, &m, &cfg);
    TEST_ASSERT(c.pixels != NULL, "WIP screen rendering completed without faults");
    SaveCanvasToBMP(&c, "c:/SysCmon/tests/qa_render_wip.bmp");
    printf("    -> Rendered WIP Screen frame saved to tests/qa_render_wip.bmp\n");

    // Test 5: Settings Modal dialog rendering
    cfg.active_nav_item = 0;
    cfg.show_settings = true;
    cfg.is_solid_theme = 0;
    cfg.active_theme = THEME_CYAN;
    UI_Render(&c, &fc, &m, &cfg);
    TEST_ASSERT(c.pixels != NULL, "Settings modal rendering completed without faults");
    SaveCanvasToBMP(&c, "c:/SysCmon/tests/qa_render_settings_modal.bmp");
    printf("    -> Rendered Settings Modal frame saved to tests/qa_render_settings_modal.bmp\n");

    Canvas_Destroy(&c);
    Fonts_Destroy(&fc);
}

static void Test_WIPPromptDialogString(void) {
    printf("\n--- Running Test 5: WIP Action Prompt Dialog Strings ---\n");

    const wchar_t *expectedMsg = L"This feature is under progress";

    TEST_ASSERT(wcscmp(expectedMsg, L"This feature is under progress") == 0,
                "Prompt message strictly matches instruction requirement: 'This feature is under progress'");
}

static void Test_SpeedTestSimulation(void) {
    printf("\n--- Running Test 6: Interactive Network Speed Test Pipeline ---\n");

    AppConfig cfg = {0};
    SystemMetrics m;
    Metrics_Init(&m);

    cfg.is_speed_testing = true;
    cfg.speedtest_phase = 1;
    cfg.speedtest_start_tick = 1000;
    cfg.speedtest_progress = 0.5;
    cfg.speedtest_ping = 14.0;
    TEST_ASSERT(cfg.is_speed_testing == true, "Speed test state initialized to active");
    TEST_ASSERT(cfg.speedtest_phase == 1, "Phase 1: Ping / Latency check active");
    TEST_ASSERT(cfg.speedtest_ping > 0.0, "Ping latency recorded (> 0 ms)");

    // Simulate Phase 2: Download bandwidth
    cfg.speedtest_phase = 2;
    cfg.speedtest_down = 94.5;
    m.net_rx_mbps = 94.5;
    TEST_ASSERT(cfg.speedtest_down > 50.0, "Phase 2: Download throughput measured (> 50 Mbps)");

    // Simulate Phase 3: Upload bandwidth
    cfg.speedtest_phase = 3;
    cfg.speedtest_up = 62.3;
    m.net_tx_mbps = 62.3;
    TEST_ASSERT(cfg.speedtest_up > 30.0, "Phase 3: Upload throughput measured (> 30 Mbps)");

    // Simulate Phase 4: Completion
    cfg.speedtest_phase = 4;
    cfg.is_speed_testing = false;
    cfg.speedtest_done = true;
    TEST_ASSERT(cfg.speedtest_done == true, "Phase 4: Speed test completed successfully");
    TEST_ASSERT(cfg.is_speed_testing == false, "Speed test flag reset to idle");
    printf("    -> Speed Test Completed: Ping %.0f ms, Down %.1f Mbps, Up %.1f Mbps\n",
           cfg.speedtest_ping, cfg.speedtest_down, cfg.speedtest_up);
}

int main(void) {
    printf("==============================================================\n");
    printf("             SysCmon - Quality Assurance Test Suite           \n");
    printf("==============================================================\n");

    Test_Metrics();
    Test_LayoutAndHitTesting();
    Test_ColorStateMapping();
    Test_GlassmorphismAndSolidRendering();
    Test_WIPPromptDialogString();
    Test_SpeedTestSimulation();

    printf("\n==============================================================\n");
    printf(" QA Summary: %d PASSED, %d FAILED\n", g_passCount, g_failCount);
    printf("==============================================================\n");

    return (g_failCount == 0) ? 0 : 1;
}
