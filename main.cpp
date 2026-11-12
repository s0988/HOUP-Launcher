
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

static const wchar_t* APP_NAME = L"HOUP Launcher";
static const wchar_t* INI_FILE = L"HOUP.ini";

enum Page { HOME_PAGE, SETTINGS_PAGE };
Page g_page = HOME_PAGE;
HWND g_hwnd = nullptr;
ULONG_PTR g_gdiplusToken = 0;
Image* g_bg = nullptr;
Image* g_logo = nullptr;
std::wstring g_mtaPath;
std::wstring g_status = L"СТАТУС: ГОТОВО ДО ЗАПУСКУ";
int g_progress = 0;
bool g_loading = false;
POINT g_mouse{ -1, -1 };

RECT R(int l, int t, int r, int b) { return RECT{ l,t,r,b }; }
bool In(const RECT& r, POINT p) { return p.x >= r.left && p.x <= r.right && p.y >= r.top && p.y <= r.bottom; }

RECT HomeTab() { return R(680, 28, 820, 76); }
RECT SettingsTab() { return R(830, 28, 1010, 76); }
RECT MinButton() { return R(1050, 28, 1095, 76); }
RECT CloseButton() { return R(1100, 28, 1145, 76); }
RECT PlayButton() { return R(64, 515, 330, 580); }
RECT BrowseButton() { return R(825, 290, 1080, 342); }
RECT SaveButton() { return R(64, 385, 330, 438); }

std::wstring AppDir() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring s(path);
    size_t p = s.find_last_of(L"\\/");
    return p == std::wstring::npos ? L"." : s.substr(0, p);
}
std::wstring Asset(const wchar_t* name) { return AppDir() + L"\\assets\\" + name; }
std::wstring IniPath() { return AppDir() + L"\\" + INI_FILE; }

void LoadSettings() {
    wchar_t buf[2048]{};
    GetPrivateProfileStringW(L"Game", L"MTAPath", L"", buf, 2048, IniPath().c_str());
    g_mtaPath = buf;
}
void SaveSettings() {
    WritePrivateProfileStringW(L"Game", L"MTAPath", g_mtaPath.c_str(), IniPath().c_str());
}

void DrawTextG(Graphics& g, const std::wstring& text, float x, float y, float w, float h,
               float size, Color color, bool bold = false, StringAlignment align = StringAlignmentNear) {
    FontFamily family(L"Segoe UI");
    Font font(&family, size, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    SolidBrush brush(color);
    StringFormat fmt;
    fmt.SetAlignment(align);
    fmt.SetLineAlignment(StringAlignmentCenter);
    fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    RectF box(x, y, w, h);
    g.DrawString(text.c_str(), -1, &font, box, &fmt, &brush);
}
void Rounded(Graphics& g, RectF r, float radius, Color fill, Color border) {
    GraphicsPath path;
    float d = radius * 2;
    path.AddArc(r.X, r.Y, d, d, 180, 90);
    path.AddArc(r.GetRight()-d, r.Y, d, d, 270, 90);
    path.AddArc(r.GetRight()-d, r.GetBottom()-d, d, d, 0, 90);
    path.AddArc(r.X, r.GetBottom()-d, d, d, 90, 90);
    path.CloseFigure();
    SolidBrush b(fill);
    Pen p(border, 1.2f);
    g.FillPath(&b, &path);
    g.DrawPath(&p, &path);
}
bool Hover(const RECT& r) { return In(r, g_mouse); }

void DrawButton(Graphics& g, const RECT& r, const wchar_t* label, bool primary = false) {
    bool hot = Hover(r);
    Color fill = primary ? (hot ? Color(255,205,230,250) : Color(255,239,246,252))
                         : (hot ? Color(185,100,145,180) : Color(145,15,22,31));
    Color edge = hot ? Color(255,225,243,255) : Color(115,205,225,245);
    Rounded(g, RectF((REAL)r.left, (REAL)r.top, (REAL)(r.right-r.left), (REAL)(r.bottom-r.top)), 10, fill, edge);
    DrawTextG(g, label, (REAL)r.left, (REAL)r.top, (REAL)(r.right-r.left), (REAL)(r.bottom-r.top),
              primary ? 18 : 13, primary ? Color(255,8,16,24) : Color(255,238,246,255), true, StringAlignmentCenter);
}

void Paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client; GetClientRect(hwnd, &client);
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    int w = client.right, h = client.bottom;
    if (g_bg && g_bg->GetLastStatus() == Ok) {
        UINT iw = g_bg->GetWidth(), ih = g_bg->GetHeight();
        REAL scale = std::max((REAL)w/iw, (REAL)h/ih);
        REAL dw = iw*scale, dh = ih*scale;
        g.DrawImage(g_bg, RectF((w-dw)/2, (h-dh)/2, dw, dh));
    } else {
        SolidBrush bg(Color(255,8,12,18)); g.FillRectangle(&bg, 0, 0, w, h);
    }
    SolidBrush shade(Color(205,4,7,12));
    g.FillRectangle(&shade, 0, 0, w, h);

    // Header and HOUP logo
    if (g_logo && g_logo->GetLastStatus() == Ok) {
        UINT iw = g_logo->GetWidth(), ih = g_logo->GetHeight();
        REAL scale = std::min(225.0f/iw, 76.0f/ih);
        REAL dw = iw*scale, dh = ih*scale;
        g.DrawImage(g_logo, RectF(35, 18, dw, dh));
    } else {
        DrawTextG(g, L"HOUP", 35, 18, 200, 70, 42, Color(255,255,255,255), true);
    }

    Pen separator(Color(75,220,238,255), 1);
    g.DrawLine(&separator, 35, 105, w-35, 105);
    DrawButton(g, HomeTab(), L"ГОЛОВНА");
    DrawButton(g, SettingsTab(), L"НАЛАШТУВАННЯ");
    DrawButton(g, MinButton(), L"—");
    DrawButton(g, CloseButton(), L"×");

    if (g_page == HOME_PAGE) {
        DrawTextG(g, L"ЛАСКАВО ПРОСИМО ДО", 65, 175, 520, 28, 12, Color(255,165,199,228), true);
        DrawTextG(g, L"HOUP ROLEPLAY", 62, 208, 650, 72, 48, Color(255,255,255,255), true);
        DrawTextG(g, L"Твоя історія починається тут.", 65, 285, 560, 35, 18, Color(255,198,211,225));
        DrawTextG(g, g_loading ? L"ПІДГОТОВКА ДО ЗАПУСКУ" : L"ГОТОВНІСТЬ ДО ЗАПУСКУ",
                  65, 420, 480, 25, 11, Color(255,170,199,225), true);
        Rounded(g, RectF(65, 452, 500, 12), 6, Color(180,0,0,0), Color(100,210,230,245));
        if (g_progress > 0) {
            Rounded(g, RectF(66, 453, 498.0f*g_progress/100.0f, 10), 5,
                    Color(255,220,239,255), Color(0,220,239,255));
        }
        DrawButton(g, PlayButton(), g_loading ? L"ЗАПУСК..." : L"ГРАТИ   →", true);

        Rounded(g, RectF(700, 165, 420, 410), 16, Color(220,7,12,19), Color(110,210,230,245));
        DrawTextG(g, L"HOUP  •  ОНОВЛЕННЯ", 730, 190, 350, 28, 11, Color(255,166,203,234), true);
        DrawTextG(g, L"НОВИНИ ПРОЄКТУ", 730, 228, 350, 42, 23, Color(255,255,255,255), true);
        DrawTextG(g, L"Вітаємо у HOUP RolePlay!\n\nСлідкуй за оновленнями, анонсами та важливими повідомленнями проєкту. Незабаром тут з’являться новини сервера.",
                  730, 285, 350, 205, 15, Color(255,200,211,224));
        DrawTextG(g, L"ОФІЦІЙНІ НОВИНИ", 730, 515, 350, 25, 10, Color(255,140,165,190), true);
    } else {
        DrawTextG(g, L"НАЛАШТУВАННЯ", 65, 165, 650, 65, 42, Color(255,255,255,255), true);
        DrawTextG(g, L"Вкажи шлях до виконуваного файлу Multi Theft Auto.", 68, 235, 850, 32, 16, Color(255,190,207,224));
        Rounded(g, RectF(65, 285, 1015, 195), 14, Color(220,7,12,19), Color(110,210,230,245));
        DrawTextG(g, L"ШЛЯХ ДО MTA:SA", 92, 307, 400, 26, 11, Color(255,165,199,228), true);
        Rounded(g, RectF(92, 345, 710, 47), 8, Color(255,5,8,12), Color(130,195,220,245));
        DrawTextG(g, g_mtaPath.empty() ? L"Оберіть файл Multi Theft Auto.exe" : g_mtaPath,
                  108, 348, 680, 40, 13, Color(255,226,237,247));
        DrawButton(g, BrowseButton(), L"ОБРАТИ ФАЙЛ");
        DrawButton(g, SaveButton(), L"ЗБЕРЕГТИ", true);
    }

    DrawTextG(g, g_status, 38, h-43, 650, 22, 10, Color(255,174,199,222), true);
    DrawTextG(g, L"HOUP  •  MTA:SA", w-250, h-43, 210, 22, 10, Color(255,135,158,181), true, StringAlignmentFar);
    EndPaint(hwnd, &ps);
}

void BrowseForMta(HWND hwnd) {
    wchar_t file[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Виконувані файли (*.exe)\0*.exe\0Усі файли (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Оберіть Multi Theft Auto.exe";
    if (GetOpenFileNameW(&ofn)) {
        g_mtaPath = file;
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}
void StartGame(HWND hwnd) {
    if (g_mtaPath.empty() || GetFileAttributesW(g_mtaPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(hwnd, L"Спочатку вкажи правильний шлях до Multi Theft Auto.exe у налаштуваннях.",
                    APP_NAME, MB_ICONWARNING | MB_OK);
        g_page = SETTINGS_PAGE; InvalidateRect(hwnd, nullptr, FALSE); return;
    }
    SaveSettings();
    g_progress = 0; g_loading = true;
    g_status = L"СТАТУС: ПІДГОТОВКА ДО ЗАПУСКУ...";
    SetTimer(hwnd, 1, 35, nullptr);
    InvalidateRect(hwnd, nullptr, FALSE);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        LoadSettings();
        SetTimer(hwnd, 2, 30, nullptr);
        return 0;
    case WM_MOUSEMOVE:
        g_mouse = POINT{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN: {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        if (In(CloseButton(), p)) { DestroyWindow(hwnd); return 0; }
        if (In(MinButton(), p)) { ShowWindow(hwnd, SW_MINIMIZE); return 0; }
        if (In(HomeTab(), p)) { g_page = HOME_PAGE; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
        if (In(SettingsTab(), p)) { g_page = SETTINGS_PAGE; InvalidateRect(hwnd, nullptr, FALSE); return 0; }
        if (g_page == HOME_PAGE && In(PlayButton(), p) && !g_loading) { StartGame(hwnd); return 0; }
        if (g_page == SETTINGS_PAGE && In(BrowseButton(), p)) { BrowseForMta(hwnd); return 0; }
        if (g_page == SETTINGS_PAGE && In(SaveButton(), p)) {
            SaveSettings(); g_status = L"СТАТУС: ШЛЯХ ЗБЕРЕЖЕНО"; InvalidateRect(hwnd, nullptr, FALSE); return 0;
        }
        // Drag frameless window by header empty area
        if (p.y < 105 && p.x < 660) { ReleaseCapture(); SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); }
        return 0;
    }
    case WM_TIMER:
        if (wp == 1 && g_loading) {
            g_progress = std::min(100, g_progress + 5);
            if (g_progress >= 100) {
                KillTimer(hwnd, 1); g_loading = false;
                HINSTANCE result = ShellExecuteW(hwnd, L"open", g_mtaPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                if ((INT_PTR)result > 32) g_status = L"СТАТУС: MTA ЗАПУЩЕНО";
                else { g_status = L"СТАТУС: ПОМИЛКА ЗАПУСКУ"; MessageBoxW(hwnd, L"Не вдалося запустити MTA.", APP_NAME, MB_ICONERROR); }
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DESTROY: KillTimer(hwnd, 1); KillTimer(hwnd, 2); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) return 1;

    std::wstring bg = Asset(L"background.png");
    std::wstring logo = Asset(L"houp_logo.png");
    g_bg = Image::FromFile(bg.c_str());
    g_logo = Image::FromFile(logo.c_str());

    const wchar_t CLASS_NAME[] = L"HOUP_MTA_LAUNCHER_WINDOW";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, APP_NAME, WS_POPUP | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1180, 720, nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }

    delete g_bg; delete g_logo;
    GdiplusShutdown(g_gdiplusToken);
    return (int)msg.wParam;
}
