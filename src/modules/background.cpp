/*
   ▄████████  ▄██████▄     ▄████████  ▄█        ▄██████▄   ▄██████▄     ▄███████▄
  ███    ███ ███    ███   ███    ███ ███       ███    ███ ███    ███   ███    ███
  ███    █▀  ███    ███   ███    ███ ███       ███    ███ ███    ███   ███    ███
 ▄███▄▄▄     ███    ███  ▄███▄▄▄▄██▀ ███       ███    ███ ███    ███   ███    ███
▀▀███▀▀▀     ███    ███ ▀▀███▀▀▀▀▀   ███       ███    ███ ███    ███ ▀█████████▀
  ███        ███    ███ ▀███████████ ███       ███    ███ ███    ███   ███
  ███        ███    ███   ███    ███ ███▌    ▄ ███    ███ ███    ███   ███
  ███         ▀██████▀    ███    ███ █████▄▄██  ▀██████▀   ▀██████▀   ▄████▀
                          ███    ███ ▀

  Background image rendering with GDI+ support and multiple positioning modes.
  Supports tile, stretch, fit, fill, and nine anchor point positions.
*/

#include "background.h"
#include "core/globals.h"
#include "theme.h"
#include "resource.h"
#include <commdlg.h>
#include <algorithm>

void LoadBackgroundImage(const std::wstring &path)
{
    if (g_bgImage)
    {
        delete g_bgImage;
        g_bgImage = nullptr;
    }
    g_bgImage = Gdiplus::Image::FromFile(path.c_str());
    if (g_bgImage && g_bgImage->GetLastStatus() != Gdiplus::Ok)
    {
        delete g_bgImage;
        g_bgImage = nullptr;
    }
    g_state.background.imagePath = path;
    g_state.background.enabled = (g_bgImage != nullptr);
    InvalidateRect(g_hwndEditor, nullptr, TRUE);
}

void PaintBackground(HDC hdc, const RECT &rc)
{
    if (!g_state.background.enabled || !g_bgImage)
        return;
    Gdiplus::Graphics graphics(hdc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    int imgW = g_bgImage->GetWidth();
    int imgH = g_bgImage->GetHeight();
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;
    Gdiplus::ImageAttributes imgAttr;
    float opacity = g_state.background.opacity / 255.0f;
    Gdiplus::ColorMatrix colorMatrix = {{
        {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, opacity, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    }};
    imgAttr.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
    auto drawImage = [&](int x, int y, int w, int h)
    {
        graphics.DrawImage(g_bgImage, Gdiplus::Rect(x, y, w, h), 0, 0, imgW, imgH, Gdiplus::UnitPixel, &imgAttr);
    };
    switch (g_state.background.position)
    {
    case BgPosition::TopLeft:
        drawImage(0, 0, imgW, imgH);
        break;
    case BgPosition::TopCenter:
        drawImage((winW - imgW) / 2, 0, imgW, imgH);
        break;
    case BgPosition::TopRight:
        drawImage(winW - imgW, 0, imgW, imgH);
        break;
    case BgPosition::CenterLeft:
        drawImage(0, (winH - imgH) / 2, imgW, imgH);
        break;
    case BgPosition::Center:
        drawImage((winW - imgW) / 2, (winH - imgH) / 2, imgW, imgH);
        break;
    case BgPosition::CenterRight:
        drawImage(winW - imgW, (winH - imgH) / 2, imgW, imgH);
        break;
    case BgPosition::BottomLeft:
        drawImage(0, winH - imgH, imgW, imgH);
        break;
    case BgPosition::BottomCenter:
        drawImage((winW - imgW) / 2, winH - imgH, imgW, imgH);
        break;
    case BgPosition::BottomRight:
        drawImage(winW - imgW, winH - imgH, imgW, imgH);
        break;
    case BgPosition::Tile:
        for (int y = 0; y < winH; y += imgH)
            for (int x = 0; x < winW; x += imgW)
                drawImage(x, y, imgW, imgH);
        break;
    case BgPosition::Stretch:
        drawImage(0, 0, winW, winH);
        break;
    case BgPosition::Fit:
    {
        float scale = (std::min)(static_cast<float>(winW) / imgW, static_cast<float>(winH) / imgH);
        int newW = static_cast<int>(imgW * scale);
        int newH = static_cast<int>(imgH * scale);
        drawImage((winW - newW) / 2, (winH - newH) / 2, newW, newH);
        break;
    }
    case BgPosition::Fill:
    {
        float scale = (std::max)(static_cast<float>(winW) / imgW, static_cast<float>(winH) / imgH);
        int newW = static_cast<int>(imgW * scale);
        int newH = static_cast<int>(imgH * scale);
        drawImage((winW - newW) / 2, (winH - newH) / 2, newW, newH);
        break;
    }
    }
}

void UpdateBackgroundBitmap(HWND hwnd)
{
    if (!g_state.background.enabled || !g_bgImage)
    {
        if (g_bgBitmap)
        {
            DeleteObject(g_bgBitmap);
            g_bgBitmap = nullptr;
        }
        return;
    }
    RECT rc;
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0)
        return;
    if (g_bgBitmap && g_bgBitmapW == w && g_bgBitmapH == h)
        return;
    if (g_bgBitmap)
    {
        DeleteObject(g_bgBitmap);
        g_bgBitmap = nullptr;
    }
    HDC hdcScreen = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    g_bgBitmap = CreateCompatibleBitmap(hdcScreen, w, h);
    g_bgBitmapW = w;
    g_bgBitmapH = h;
    HBITMAP hOldBmp = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, g_bgBitmap));
    COLORREF bgColor = IsDarkMode() ? RGB(30, 30, 30) : GetSysColor(COLOR_WINDOW);
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdcMem, &rc, hBrush);
    DeleteObject(hBrush);
    PaintBackground(hdcMem, rc);
    SelectObject(hdcMem, hOldBmp);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcScreen);
}

void SetBackgroundPosition(BgPosition pos)
{
    g_state.background.position = pos;
    HMENU hMenu = GetMenu(g_hwndMain);
    HMENU hViewMenu = GetSubMenu(hMenu, 3);
    HMENU hBgMenu = GetSubMenu(hViewMenu, 6);
    HMENU hPosMenu = GetSubMenu(hBgMenu, 4);
    for (int i = 0; i < 13; ++i)
        CheckMenuItem(hPosMenu, i, MF_BYPOSITION | MF_UNCHECKED);
    int idx = 0;
    switch (pos)
    {
    case BgPosition::TopLeft:
        idx = 0;
        break;
    case BgPosition::TopCenter:
        idx = 1;
        break;
    case BgPosition::TopRight:
        idx = 2;
        break;
    case BgPosition::CenterLeft:
        idx = 4;
        break;
    case BgPosition::Center:
        idx = 5;
        break;
    case BgPosition::CenterRight:
        idx = 6;
        break;
    case BgPosition::BottomLeft:
        idx = 8;
        break;
    case BgPosition::BottomCenter:
        idx = 9;
        break;
    case BgPosition::BottomRight:
        idx = 10;
        break;
    case BgPosition::Tile:
        idx = 12;
        break;
    case BgPosition::Stretch:
        idx = 13;
        break;
    case BgPosition::Fit:
        idx = 14;
        break;
    case BgPosition::Fill:
        idx = 15;
        break;
    }
    CheckMenuItem(hPosMenu, idx, MF_BYPOSITION | MF_CHECKED);
    if (g_bgBitmap)
    {
        DeleteObject(g_bgBitmap);
        g_bgBitmap = nullptr;
    }
    InvalidateRect(g_hwndEditor, nullptr, TRUE);
}

void ViewSelectBackground()
{
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.lpstrFilter = L"Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.gif)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn))
        LoadBackgroundImage(path);
}

void ViewClearBackground()
{
    if (g_bgImage)
    {
        delete g_bgImage;
        g_bgImage = nullptr;
    }
    if (g_bgBitmap)
    {
        DeleteObject(g_bgBitmap);
        g_bgBitmap = nullptr;
    }
    g_state.background.enabled = false;
    g_state.background.imagePath.clear();
    InvalidateRect(g_hwndEditor, nullptr, TRUE);
}

static INT_PTR CALLBACK OpacityDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM)
{
    static HWND hEdit = nullptr;
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", L"Opacity (0-100%):", WS_CHILD | WS_VISIBLE, 10, 15, 110, 20, hDlg, nullptr, nullptr, nullptr);
        hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_NUMBER, 125, 12, 60, 22, hDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 55, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 135, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        int pct = g_state.background.opacity * 100 / 255;
        wchar_t buf[32];
        wsprintfW(buf, L"%d", pct);
        SetWindowTextW(hEdit, buf);
        SendMessageW(hEdit, EM_SETSEL, 0, -1);
        SetFocus(hEdit);
        return FALSE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t buf[32];
            GetWindowTextW(hEdit, buf, 32);
            int val = _wtoi(buf);
            val = (val < 0) ? 0 : (val > 100) ? 100
                                              : val;
            g_state.background.opacity = static_cast<BYTE>(val * 255 / 100);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

void ViewBackgroundOpacity()
{
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", L"Background Opacity",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 300, 300, 270, 120,
                                g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!hDlg)
        return;
    SetWindowLongPtrW(hDlg, DWLP_DLGPROC, reinterpret_cast<LONG_PTR>(OpacityDlgProc));
    OpacityDlgProc(hDlg, WM_INITDIALOG, 0, 0);
    EnableWindow(g_hwndMain, FALSE);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (!IsWindow(hDlg))
            break;
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN)
        {
            SendMessageW(hDlg, WM_COMMAND, IDOK, 0);
            break;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
        {
            SendMessageW(hDlg, WM_COMMAND, IDCANCEL, 0);
            break;
        }
        if (!IsDialogMessageW(hDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!IsWindow(hDlg))
            break;
    }
    EnableWindow(g_hwndMain, TRUE);
    if (IsWindow(hDlg))
        DestroyWindow(hDlg);
    if (g_bgBitmap)
    {
        DeleteObject(g_bgBitmap);
        g_bgBitmap = nullptr;
    }
    InvalidateRect(g_hwndEditor, nullptr, TRUE);
    SetForegroundWindow(g_hwndMain);
}
