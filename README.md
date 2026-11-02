# SysCmon - System Performance & Optimization Suite

[![Platform](https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011-0078D6?logo=windows)](https://microsoft.com/windows)
[![Language](https://img.shields.io/badge/Language-Pure%20C%20(Win32%20API)-A8B9CC?logo=c)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Graphics](https://img.shields.io/badge/Graphics-GDI%20%2B%20DWM%20Acrylic-00B294)](https://learn.microsoft.com/en-us/windows/win32/dwm/dwm-overview)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![QA Tests](https://img.shields.io/badge/QA%20Tests-96%2F96%20Passed%20(100%25)-brightgreen)](tests/qa_test.c)

A high-performance, native Windows desktop system telemetry monitor and optimization utility written in pure C. Built directly upon the Win32 API, GDI, and the Desktop Window Manager (DWM) Acrylic composition engine, **SysCmon** delivers real-time hardware metrics without the memory bloat of Electron or heavyweight GUI frameworks.

---


## ✨ Key Features

- **Pure Native Win32 / C**: Instant launch, zero runtime dependencies, low CPU overhead (< 0.2%), and minimal RAM footprint (~8 MB).
- **Dual Visual Engines**:
  - **Solid Modern Matte**: Default out-of-the-box dark mode with high-contrast slate surfaces.
  - **High-End Frosted Glassmorphism**: Window-level DWM Acrylic blur backdrop composition (`WS_EX_LAYERED` + `DWMSBT_TRANSIENTWINDOW`) with specular top/left card refraction and 100% sharp, readable text.
- **Dynamic Real-Time Telemetry**:
  - **Kernel Metrics via `psapi`**: Real-time process count, kernel thread count, handle count, committed virtual memory, and system file cache.
  - **Hardware Gauges**: CPU utilization, frequency scaling (GHz), estimated core temperatures (°C), and multi-tier sparkline wave graphs.
  - **Storage & Disk I/O**: Real-time read/write throughput rates (MB/s), Drive C: capacity, and free space.
  - **Internet & Network**: Live upload/download bandwidth dials, ping latency (ms), and local IP reporting.
- **Interactive Network Speed Test**:
  - Multi-phase 25fps animated pipeline: ICMP Echo ping measurement, download bandwidth ramp-up, and upload bandwidth ramp-up.
- **Automated Health State Classification**:
  - Real-time parameter evaluation into **NORMAL** (Light Blue `#38BDF8`), **WARNING** (Yellow `#FACC15`), and **CRITICAL** (Red `#EF4444`) with dynamic diagnostic reasoning.
- **Settings & Personalization Modal**:
  - Visual theme mode segmented toggle, glass frost opacity slider, 5 accent color palettes (Cyan Pulse, Emerald Flux, Amethyst Glow, Amber Blaze, Frost Blue), and threshold guide cards.
- **Navigation & Screen Dispatch**:
  - 64px compact sidebar rail with Home dashboard, modular WIP view screens, and settings gear.

---

## 📁 Repository Structure

```
SysCmon/
├── assets/
│   └── SysCmon.ico             # Application icon resource
├── installer/
│   └── installer.c             # Self-extracting Windows installer source
├── src/
│   ├── app.h                   # Core structures, state enums, and themes
│   ├── main.c                  # Win32 window lifecycle, message loop, timers
│   ├── manifest.xml            # Windows Common Controls v6 & DPI manifest
│   ├── metrics.c               # Kernel performance counters & hardware polling
│   ├── metrics.h               # Telemetry query interfaces
│   ├── render.c                # Double-buffered GDI & Acrylic rendering engine
│   ├── render.h                # Layout geometry & rendering interfaces
│   ├── resource.h              # Resource identifiers
│   └── resource.rc             # Icon, manifest, and version resource script
├── tests/
│   └── qa_test.c               # Automated headless regression test suite (96 checks)
├── build.bat                   # Compilation batch script (MinGW GCC)
├── run.bat                     # Launch script
├── LICENSE                     # MIT License
├── .gitignore                  # Git ignore rules
└── README.md                   # Project documentation
```

---

## 🛠️ Build & Installation

### Prerequisites
- **Windows 10 / 11** (64-bit)
- **MinGW-w64 GCC** (with `windres`) installed and available in `PATH`.

### Compile from Source
To compile `SysCmon.exe` and the standalone installer `SysCmon_Setup.exe`:

```cmd
build.bat
```

Or manually with GCC:
```cmd
windres -I"src" -I"assets" "src\resource.rc" -O coff -o "src\resource.o"
gcc -O3 -mwindows -Wall src\main.c src\metrics.c src\render.c src\resource.o -o SysCmon.exe -luser32 -lgdi32 -lmsimg32 -ldwmapi -lcomctl32 -liphlpapi -lpsapi -lm
```

### Launch
Run the launcher script or binary directly:
```cmd
run.bat
```

---

## 🧪 Quality Assurance Test Suite

SysCmon includes a headless automated test suite that verifies telemetry sampling, UI geometry, and rendering without launching an interactive GUI window:

```cmd
gcc -O2 -Wall tests\qa_test.c src\metrics.c src\render.c -o tests\qa_test.exe -lgdi32 -ldwmapi -liphlpapi -lpsapi -lm
tests\qa_test.exe
```

Test Results: **96 PASSED, 0 FAILED (100% Pass Rate)**.

---

## 📄 License
This project is licensed under the [MIT License](LICENSE).
