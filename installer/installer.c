#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <shlobj.h>
#include <objbase.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdbool.h>

#define ID_BTN_INSTALL    201
#define ID_BTN_CANCEL     202
#define ID_CHK_DESKTOP    203
#define ID_CHK_STARTMENU  204
#define ID_CHK_LAUNCH     205

static HWND g_hBtnInstall;
static HWND g_hBtnCancel;
static HWND g_hChkDesktop;
static HWND g_hChkStartMenu;
static HWND g_hChkLaunch;
static HWND g_hProgress;
static HFONT g_hFontTitle;
static HFONT g_hFontBody;

// Create Windows shortcut via COM
static bool CreateShortcut(LPCWSTR targetPath, LPCWSTR linkPath, LPCWSTR desc) {
    IShellLinkW* psl = NULL;
    HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (LPVOID*)&psl);
    if (SUCCEEDED(hr)) {
        IShellLinkW_SetPath(psl, targetPath);
        IShellLinkW_SetDescription(psl, desc);

        IPersistFile* ppf = NULL;
        hr = IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            hr = IPersistFile_Save(ppf, linkPath, TRUE);
            IPersistFile_Release(ppf);
        }
        IShellLinkW_Release(psl);
        return SUCCEEDED(hr);
    }
    return false;
}

// Perform Installation
static void DoInstallation(HWND hwnd) {
    EnableWindow(g_hBtnInstall, FALSE);
    EnableWindow(g_hBtnCancel, FALSE);
    ShowWindow(g_hProgress, SW_SHOW);

    SendMessageW(g_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessageW(g_hProgress, PBM_SETPOS, 20, 0);

    // Target directory: %LocalAppData%\SysCmon
    wchar_t localAppData[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        wcscpy(localAppData, L"C:\\SysCmon");
    }

    wchar_t destDir[MAX_PATH];
    swprintf(destDir, MAX_PATH, L"%ls\\SysCmon", localAppData);
    CreateDirectoryW(destDir, NULL);

    SendMessageW(g_hProgress, PBM_SETPOS, 40, 0);

    // Source SysCmon.exe path
    wchar_t currentExe[MAX_PATH];
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);
    wchar_t *lastSlash = wcsrchr(currentExe, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';

    wchar_t srcSysCmon[MAX_PATH];
    swprintf(srcSysCmon, MAX_PATH, L"%lsSysCmon.exe", currentExe);

    // If running from installer folder or parent
    if (GetFileAttributesW(srcSysCmon) == INVALID_FILE_ATTRIBUTES) {
        swprintf(srcSysCmon, MAX_PATH, L"%ls..\\SysCmon.exe", currentExe);
    }
    if (GetFileAttributesW(srcSysCmon) == INVALID_FILE_ATTRIBUTES) {
        swprintf(srcSysCmon, MAX_PATH, L"c:\\SysCmon\\SysCmon.exe");
    }

    wchar_t destSysCmon[MAX_PATH];
    swprintf(destSysCmon, MAX_PATH, L"%ls\\SysCmon.exe", destDir);

    CopyFileW(srcSysCmon, destSysCmon, FALSE);

    wchar_t srcRunBat[MAX_PATH];
    wchar_t destRunBat[MAX_PATH];
    swprintf(srcRunBat, MAX_PATH, L"c:\\SysCmon\\run.bat");
    swprintf(destRunBat, MAX_PATH, L"%ls\\run.bat", destDir);
    CopyFileW(srcRunBat, destRunBat, FALSE);

    SendMessageW(g_hProgress, PBM_SETPOS, 70, 0);

    // Desktop Shortcut
    if (SendMessageW(g_hChkDesktop, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wchar_t desktopPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
            wchar_t linkPath[MAX_PATH];
            swprintf(linkPath, MAX_PATH, L"%ls\\SysCmon.lnk", desktopPath);
            CreateShortcut(destSysCmon, linkPath, L"SysCmon System Performance Monitor");
        }
    }

    // Start Menu Shortcut
    if (SendMessageW(g_hChkStartMenu, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wchar_t programsPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, programsPath))) {
            wchar_t startDir[MAX_PATH];
            swprintf(startDir, MAX_PATH, L"%ls\\SysCmon", programsPath);
            CreateDirectoryW(startDir, NULL);
            wchar_t linkPath[MAX_PATH];
            swprintf(linkPath, MAX_PATH, L"%ls\\SysCmon.lnk", startDir);
            CreateShortcut(destSysCmon, linkPath, L"SysCmon System Performance Monitor");
        }
    }

    SendMessageW(g_hProgress, PBM_SETPOS, 100, 0);

    // Launch application if requested
    if (SendMessageW(g_hChkLaunch, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        ShellExecuteW(NULL, L"open", destSysCmon, NULL, destDir, SW_SHOWNORMAL);
    }

    MessageBoxW(hwnd, L"SysCmon has been successfully installed on your computer!", L"Installation Complete", MB_OK | MB_ICONINFORMATION);
    DestroyWindow(hwnd);
}

// Installer Window Procedure
LRESULT CALLBACK InstallerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));

            g_hFontTitle = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontBody = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            // Checkboxes
            g_hChkDesktop = CreateWindowW(L"BUTTON", L"Create Desktop Shortcut",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                45, 145, 300, 24, hwnd, (HMENU)ID_CHK_DESKTOP, NULL, NULL);
            SendMessageW(g_hChkDesktop, BM_SETCHECK, BST_CHECKED, 0);
            SendMessageW(g_hChkDesktop, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

            g_hChkStartMenu = CreateWindowW(L"BUTTON", L"Create Start Menu Program Folder",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                45, 180, 300, 24, hwnd, (HMENU)ID_CHK_STARTMENU, NULL, NULL);
            SendMessageW(g_hChkStartMenu, BM_SETCHECK, BST_CHECKED, 0);
            SendMessageW(g_hChkStartMenu, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

            g_hChkLaunch = CreateWindowW(L"BUTTON", L"Launch SysCmon immediately after installation",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                45, 215, 360, 24, hwnd, (HMENU)ID_CHK_LAUNCH, NULL, NULL);
            SendMessageW(g_hChkLaunch, BM_SETCHECK, BST_CHECKED, 0);
            SendMessageW(g_hChkLaunch, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

            // Progress Bar (hidden initially)
            g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                WS_CHILD | PBS_SMOOTH,
                45, 255, 430, 16, hwnd, NULL, NULL, NULL);

            // Buttons
            g_hBtnInstall = CreateWindowW(L"BUTTON", L"Install Now",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                245, 290, 115, 36, hwnd, (HMENU)ID_BTN_INSTALL, NULL, NULL);
            SendMessageW(g_hBtnInstall, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

            g_hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                370, 290, 105, 36, hwnd, (HMENU)ID_BTN_CANCEL, NULL, NULL);
            SendMessageW(g_hBtnCancel, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            HBRUSH hBg = CreateSolidBrush(RGB(14, 19, 28));
            FillRect(hdc, &rc, hBg);
            DeleteObject(hBg);

            RECT rcTop = { 0, 0, rc.right, 80 };
            HBRUSH hTopBg = CreateSolidBrush(RGB(20, 28, 42));
            FillRect(hdc, &rcTop, hTopBg);
            DeleteObject(hTopBg);

            HPEN hLine = CreatePen(PS_SOLID, 1, RGB(0, 229, 255));
            HPEN hOldP = (HPEN)SelectObject(hdc, hLine);
            MoveToEx(hdc, 0, 80, NULL);
            LineTo(hdc, rc.right, 80);
            SelectObject(hdc, hOldP);
            DeleteObject(hLine);

            SetBkMode(hdc, TRANSPARENT);

            SelectObject(hdc, g_hFontTitle);
            SetTextColor(hdc, RGB(255, 255, 255));
            TextOutW(hdc, 35, 16, L"SysCmon Setup Wizard", 20);

            SelectObject(hdc, g_hFontBody);
            SetTextColor(hdc, RGB(0, 229, 255));
            TextOutW(hdc, 35, 46, L"System Performance Monitor & Optimization Suite v1.2.0", 54);

            SetTextColor(hdc, RGB(210, 220, 235));
            TextOutW(hdc, 45, 105, L"Select installation options and click 'Install Now' to proceed:", 62);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            if (id == ID_BTN_INSTALL) {
                DoInstallation(hwnd);
                return 0;
            }
            if (id == ID_BTN_CANCEL) {
                DestroyWindow(hwnd);
                return 0;
            }
            break;
        }

        case WM_DESTROY: {
            if (g_hFontTitle) DeleteObject(g_hFontTitle);
            if (g_hFontBody) DeleteObject(g_hFontBody);
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev;
    (void)lpCmd;

    CoInitialize(NULL);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"SysCmonInstallerClass";
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = InstallerWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    int winW = 530;
    int winH = 385;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"SysCmon Setup",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}
