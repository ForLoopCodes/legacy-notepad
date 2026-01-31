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

  Editor control functions for text manipulation, font rendering, and zoom control.
  Handles RichEdit control subclassing, word wrap, and cursor position tracking.
*/

#include "editor.h"
#include "core/types.h"
#include "core/globals.h"
#include "theme.h"
#include "background.h"
#include "resource.h"

std::wstring GetEditorText()
{
    int len = GetWindowTextLengthW(g_hwndEditor);
    if (len <= 0)
        return L"";
    std::wstring text(len + 1, 0);
    GetWindowTextW(g_hwndEditor, &text[0], len + 1);
    text.resize(len);
    return text;
}

void SetEditorText(const std::wstring &text)
{
    SetWindowTextW(g_hwndEditor, text.c_str());
}

std::pair<int, int> GetCursorPos()
{
    DWORD start = 0, end = 0;
    SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    int pos = static_cast<int>(start);
    int line = static_cast<int>(SendMessageW(g_hwndEditor, EM_LINEFROMCHAR, pos, 0)) + 1;
    int lineStart = static_cast<int>(SendMessageW(g_hwndEditor, EM_LINEINDEX, line - 1, 0));
    int col = pos - lineStart + 1;
    return {line, col};
}

void ApplyFont()
{
    if (g_state.hFont)
    {
        DeleteObject(g_state.hFont);
        g_state.hFont = nullptr;
    }
    int size = g_state.fontSize * g_state.zoomLevel / 100;
    size = (size < 8) ? 8 : (size > 500) ? 500
                                         : size;
    HDC hdc = GetDC(g_hwndMain);
    int height = -MulDiv(size, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(g_hwndMain, hdc);
    g_state.hFont = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                FIXED_PITCH | FF_MODERN, g_state.fontName.c_str());
    SendMessageW(g_hwndEditor, WM_SETFONT, reinterpret_cast<WPARAM>(g_state.hFont), TRUE);
}

void ApplyZoom()
{
    ApplyFont();
}

void ApplyWordWrap()
{
    std::wstring text = GetEditorText();
    DWORD start = 0, end = 0;
    SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    DestroyWindow(g_hwndEditor);
    DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL;
    if (!g_state.wordWrap)
        style |= WS_HSCROLL | ES_AUTOHSCROLL;
    g_hwndEditor = CreateWindowExW(0, L"EDIT", nullptr, style,
                                   0, 0, 100, 100, g_hwndMain, reinterpret_cast<HMENU>(IDC_EDITOR), GetModuleHandleW(nullptr), nullptr);
    ApplyFont();
    SetEditorText(text);
    SendMessageW(g_hwndEditor, EM_SETSEL, start, end);
    RECT rc;
    GetClientRect(g_hwndMain, &rc);
    int statusH = 0;
    if (g_state.showStatusBar)
    {
        RECT rs;
        GetWindowRect(g_hwndStatus, &rs);
        statusH = rs.bottom - rs.top;
    }
    MoveWindow(g_hwndEditor, 0, 0, rc.right, rc.bottom - statusH, TRUE);
    SetFocus(g_hwndEditor);
}

void DeleteWordBackward()
{
    DWORD start = 0, end = 0;
    SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    if (start != end)
    {
        SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L""));
        return;
    }
    if (start == 0)
        return;
    std::wstring text = GetEditorText();
    size_t pos = start;
    while (pos > 0 && iswspace(text[pos - 1]))
        --pos;
    while (pos > 0 && !iswspace(text[pos - 1]))
        --pos;
    SendMessageW(g_hwndEditor, EM_SETSEL, pos, start);
    SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L""));
}

void DeleteWordForward()
{
    DWORD start = 0, end = 0;
    SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    if (start != end)
    {
        SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L""));
        return;
    }
    std::wstring text = GetEditorText();
    size_t len = text.size();
    size_t pos = start;
    while (pos < len && !iswspace(text[pos]))
        ++pos;
    while (pos < len && iswspace(text[pos]))
        ++pos;
    SendMessageW(g_hwndEditor, EM_SETSEL, start, pos);
    SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L""));
}

LRESULT CALLBACK EditorSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        if (g_state.background.enabled && g_bgImage)
        {
            UpdateBackgroundBitmap(hwnd);
            if (g_bgBitmap)
            {
                HDC hdc = reinterpret_cast<HDC>(wParam);
                RECT rc;
                GetClientRect(hwnd, &rc);
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hOldBmp = reinterpret_cast<HBITMAP>(SelectObject(hdcMem, g_bgBitmap));
                BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);
                SelectObject(hdcMem, hOldBmp);
                DeleteDC(hdcMem);
                return 1;
            }
        }
        break;
    case WM_SIZE:
        if (g_state.background.enabled && g_bgImage && g_bgBitmap)
        {
            DeleteObject(g_bgBitmap);
            g_bgBitmap = nullptr;
        }
        break;
    case WM_CHAR:
        if (wParam == 127)
        {
            DeleteWordBackward();
            return 0;
        }
        if (g_state.background.enabled && g_bgImage)
        {
            LRESULT result = CallWindowProcW(g_origEditorProc, hwnd, msg, wParam, lParam);
            InvalidateRect(hwnd, nullptr, TRUE);
            return result;
        }
        break;
    case WM_KEYDOWN:
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            if (wParam == VK_BACK)
            {
                DeleteWordBackward();
                return 0;
            }
            if (wParam == VK_DELETE)
            {
                DeleteWordForward();
                return 0;
            }
        }
        if (g_state.background.enabled && g_bgImage && (wParam == VK_BACK || wParam == VK_DELETE))
        {
            LRESULT result = CallWindowProcW(g_origEditorProc, hwnd, msg, wParam, lParam);
            InvalidateRect(hwnd, nullptr, TRUE);
            return result;
        }
        break;
    }
    return CallWindowProcW(g_origEditorProc, hwnd, msg, wParam, lParam);
}
