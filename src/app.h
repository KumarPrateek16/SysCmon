#ifndef APPH_H
#define APPH_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#define APP_TITLE "SysCmon Pro - System Monitoring & Optimization Suite"
#define APP_VERSION "2.3.0"
#define HISTORY_LEN 60

// Health State Classification
typedef enum {
    SYS_STATE_NORMAL = 0,
    SYS_STATE_WARNING = 1,
    SYS_STATE_CRITICAL = 2
} SystemState;

// Color Themes
typedef enum {
    THEME_CYAN = 0,      // Electric Cyan / Neon Blue
    THEME_EMERALD,       // Matrix Emerald Green
    THEME_AMETHYST,      // Royal Purple / Amethyst
    THEME_AMBER,         // Sunset Amber / Golden Fire
    THEME_FROST,         // Arctic Diamond / Frost Silver
    THEME_COUNT
} ColorThemeId;

typedef struct {
    const char *name;
    COLORREF primary;
    COLORREF accentLight;
    COLORREF accentDark;
    COLORREF glow;
    COLORREF textAccent;
} ColorThemeDef;

// System Metrics Snapshot
typedef struct {
    SystemState state;
    char state_label[32];
    char state_reason[256];

    // Individual parameter health states
    SystemState cpu_state;
    SystemState ram_state;
    SystemState disk_state;
    SystemState storage_state;
    SystemState net_state;

    // CPU
    double cpu_usage;          // 0 - 100%
    int cpu_cores;
    char cpu_model[128];
    double cpu_temp_c;         // Estimated core temp
    double cpu_history[HISTORY_LEN];

    // RAM
    double ram_usage_pct;      // 0 - 100%
    double ram_total_gb;
    double ram_used_gb;
    double ram_free_gb;
    double ram_history[HISTORY_LEN];

    // Disk & Storage
    double disk_read_kbs;
    double disk_write_kbs;
    double disk_total_kbs;
    double disk_free_gb;
    double disk_total_gb;
    double disk_usage_pct;
    double disk_history[HISTORY_LEN];

    // Network & Internet
    double net_rx_kbs;
    double net_tx_kbs;
    double net_rx_mbps;
    double net_tx_mbps;
    int ping_ms;
    char local_ip[32];

    // Advanced & Dynamic OS Kernel Telemetry
    DWORD cpu_mhz;             // Base CPU clock in MHz
    double cpu_cur_ghz;        // Dynamic current CPU clock in GHz
    DWORD process_count;       // Real dynamic running process count
    DWORD thread_count;        // Real dynamic active kernel thread count
    DWORD handle_count;        // Real dynamic open handle count
    double ram_commit_gb;      // Real dynamic committed memory (GB)
    double ram_commit_limit_gb;// Real dynamic commit limit (GB)
    double ram_cached_gb;      // Real dynamic system cache (GB)
    double ram_releasable_mb;  // Dynamic calculated releasable memory

    DWORD uptime_seconds;
    int history_count;
} SystemMetrics;

// Application Configuration and UI State
typedef struct {
    int is_solid_theme;        // 1 = Solid Matte (DEFAULT), 0 = Glassmorphism
    float glass_opacity;       // 0.20f - 0.95f
    ColorThemeId active_theme;
    int window_width;
    int window_height;

    // Sidebar navigation
    int active_nav_item;       // 0 = Home Dashboard, 1 = Display, 2 = Security, 3 = Speed, 4 = Flame
    bool show_settings;

    // Toggle switch states
    bool toggle_auto_ram;
    bool toggle_fast_defrag;
    bool toggle_turbo_net;

    // Hover states
    bool hover_free_ram;
    bool hover_clean_storage;
    bool hover_optimize_net;
    bool hover_settings_btn;
    bool hover_speed_test;
    bool hover_mode_solid;
    bool hover_mode_glass;
    int hover_nav_item;        // -1 if none
    bool hover_wip_back_btn;
    bool hover_modal_close;
    bool hover_modal_done;
    bool hover_solid_toggle;
    bool hover_opacity_slider;
    int hover_theme_index;

    bool dragging_opacity;

    // Interactive Speed Test State
    bool is_speed_testing;
    int speedtest_phase;          // 0=idle, 1=ping/latency, 2=download, 3=upload, 4=completed
    DWORD speedtest_start_tick;
    double speedtest_progress;     // 0.0 to 1.0
    double speedtest_ping;
    double speedtest_down;
    double speedtest_up;
    bool speedtest_done;
} AppConfig;

#define ID_BTN_FREE_RAM         1001
#define ID_BTN_CLEAN_STORAGE    1002
#define ID_BTN_OPTIMIZE_NET     1003
#define ID_BTN_SETTINGS         1004
#define ID_BTN_SPEED_TEST       1005
#define ID_TIMER_UPDATE         2001
#define ID_TIMER_SPEEDTEST      2002

extern const ColorThemeDef g_ColorThemes[THEME_COUNT];

#endif // APPH_H
