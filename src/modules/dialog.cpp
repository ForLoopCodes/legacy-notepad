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

  Dialog box implementations for find, replace, goto, font selection, and more.
  Provides modeless and modal dialog creation with proper event handling.
*/

#include "dialog.h"
#include "core/globals.h"
#include "editor.h"
#include "ui.h"
#include "settings.h"
#include <commdlg.h>
#include <algorithm>
#include <cwctype>

void DoFind(bool forward)
{
    if (g_state.findText.empty())
        return;
    std::wstring text = GetEditorText();
    DWORD start = 0, end = 0;
    SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    std::wstring textLower = text;
    std::transform(textLower.begin(), textLower.end(), textLower.begin(), towlower);
    std::wstring findLower = g_state.findText;
    std::transform(findLower.begin(), findLower.end(), findLower.begin(), towlower);
    size_t pos = std::wstring::npos;
    if (forward)
    {
        pos = textLower.find(findLower, end);
        if (pos == std::wstring::npos)
            pos = textLower.find(findLower);
    }
    else
    {
        if (start > 0)
            pos = textLower.rfind(findLower, start - 1);
        if (pos == std::wstring::npos)
            pos = textLower.rfind(findLower);
    }
    if (pos != std::wstring::npos)
    {
        SendMessageW(g_hwndEditor, EM_SETSEL, pos, pos + g_state.findText.size());
        SendMessageW(g_hwndEditor, EM_SCROLLCARET, 0, 0);
    }
    else
        MessageBoxW(g_hwndMain, (L"Cannot find \"" + g_state.findText + L"\"").c_str(), APP_NAME, MB_ICONINFORMATION);
}

INT_PTR CALLBACK FindDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowTextW(GetDlgItem(hDlg, 1001), g_state.findText.c_str());
        if (GetDlgItem(hDlg, 1002))
            SetWindowTextW(GetDlgItem(hDlg, 1002), g_state.replaceText.c_str());
        InvalidateRect(hDlg, nullptr, FALSE);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1:
        {
            wchar_t buf[256] = {0};
            GetWindowTextW(GetDlgItem(hDlg, 1001), buf, 256);
            g_state.findText = buf;
            DoFind(true);
            return TRUE;
        }
        case 2:
            DestroyWindow(hDlg);
            g_hwndFindDlg = nullptr;
            SetFocus(g_hwndEditor);
            return TRUE;
        case 3:
        {
            wchar_t buf[256] = {0};
            GetWindowTextW(GetDlgItem(hDlg, 1001), buf, 256);
            g_state.findText = buf;
            GetWindowTextW(GetDlgItem(hDlg, 1002), buf, 256);
            g_state.replaceText = buf;
            if (g_state.findText.empty())
                return TRUE;
            DWORD start = 0, end = 0;
            SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
            if (start != end)
            {
                std::wstring text = GetEditorText();
                std::wstring sel = text.substr(start, end - start);
                std::transform(sel.begin(), sel.end(), sel.begin(), towlower);
                std::wstring findLower = g_state.findText;
                std::transform(findLower.begin(), findLower.end(), findLower.begin(), towlower);
                if (sel == findLower)
                    SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(g_state.replaceText.c_str()));
            }
            DoFind(true);
            return TRUE;
        }
        case 4:
        {
            wchar_t buf[256] = {0};
            GetWindowTextW(GetDlgItem(hDlg, 1001), buf, 256);
            g_state.findText = buf;
            GetWindowTextW(GetDlgItem(hDlg, 1002), buf, 256);
            g_state.replaceText = buf;
            if (g_state.findText.empty())
                return TRUE;
            std::wstring text = GetEditorText();
            std::wstring findLower = g_state.findText;
            std::transform(findLower.begin(), findLower.end(), findLower.begin(), towlower);
            std::wstring lower = text;
            std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
            std::wstring newText;
            size_t lastPos = 0, pos = 0;
            while ((pos = lower.find(findLower, lastPos)) != std::wstring::npos)
            {
                newText += text.substr(lastPos, pos - lastPos);
                newText += g_state.replaceText;
                lastPos = pos + g_state.findText.size();
            }
            newText += text.substr(lastPos);
            if (newText != text)
            {
                SetEditorText(newText);
                g_state.modified = true;
                UpdateTitle();
            }
            return TRUE;
        }
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);
        EndPaint(hDlg, &ps);
        return FALSE;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
        return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
    case WM_CLOSE:
        DestroyWindow(hDlg);
        g_hwndFindDlg = nullptr;
        SetFocus(g_hwndEditor);
        return TRUE;
    case WM_DESTROY:
        g_hwndFindDlg = nullptr;
        return TRUE;
    }
    return DefDlgProcW(hDlg, msg, wParam, lParam);
}

void EditFind()
{
    if (g_hwndFindDlg)
    {
        SetFocus(g_hwndFindDlg);
        return;
    }
    g_hwndFindDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", L"Find",
                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 420, 120,
                                    g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (g_hwndFindDlg)
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", L"Find:", WS_CHILD | WS_VISIBLE, 10, 12, 45, 16, g_hwndFindDlg, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_state.findText.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 60, 10, 230, 20, g_hwndFindDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Find Next", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 300, 10, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE, 300, 38, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(2), nullptr, nullptr);
        for (HWND h = GetWindow(g_hwndFindDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SetWindowLongPtrW(g_hwndFindDlg, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FindDlgProc));
        InvalidateRect(g_hwndFindDlg, nullptr, FALSE);
        UpdateWindow(g_hwndFindDlg);
    }
}

void EditFindNext()
{
    if (!g_state.findText.empty())
        DoFind(true);
}

void EditFindPrev()
{
    if (!g_state.findText.empty())
        DoFind(false);
}

void EditReplace()
{
    if (g_hwndFindDlg)
    {
        SetFocus(g_hwndFindDlg);
        return;
    }
    g_hwndFindDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", L"Find and Replace",
                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 420, 175,
                                    g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (g_hwndFindDlg)
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", L"Find:", WS_CHILD | WS_VISIBLE, 10, 12, 45, 16, g_hwndFindDlg, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_state.findText.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 60, 10, 230, 20, g_hwndFindDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        CreateWindowExW(0, L"STATIC", L"Replace:", WS_CHILD | WS_VISIBLE, 10, 40, 50, 16, g_hwndFindDlg, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_state.replaceText.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 60, 38, 230, 20, g_hwndFindDlg, reinterpret_cast<HMENU>(1002), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Find Next", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 300, 10, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Replace", WS_CHILD | WS_VISIBLE, 300, 38, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(3), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Replace All", WS_CHILD | WS_VISIBLE, 300, 66, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(4), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE, 300, 94, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(2), nullptr, nullptr);
        for (HWND h = GetWindow(g_hwndFindDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SetWindowLongPtrW(g_hwndFindDlg, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FindDlgProc));
        InvalidateRect(g_hwndFindDlg, nullptr, FALSE);
        UpdateWindow(g_hwndFindDlg);
    }
}

void EditGoto()
{
    (void)DialogBoxW(nullptr, nullptr, g_hwndMain, [](HWND hDlg, UINT msg, WPARAM wParam, LPARAM) -> INT_PTR
                     {
        static HWND hEdit = nullptr;
        switch (msg)
        {
        case WM_INITDIALOG:
            SetWindowTextW(hDlg, L"Go To Line");
            hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | ES_NUMBER, 80, 15, 120, 22, hDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
            CreateWindowExW(0, L"STATIC", L"Line number:", WS_CHILD | WS_VISIBLE, 10, 18, 70, 20, hDlg, nullptr, nullptr, nullptr);
            CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 60, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 140, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
            {
                HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
                    SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
            }
            SetFocus(hEdit);
            return FALSE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                wchar_t buf[32];
                GetWindowTextW(hEdit, buf, 32);
                int line = _wtoi(buf);
                if (line > 0)
                {
                    std::wstring text = GetEditorText();
                    int current = 1;
                    size_t pos = 0;
                    for (size_t i = 0; i < text.size() && current < line; ++i)
                        if (text[i] == L'\n') { ++current; pos = i + 1; }
                    if (current < line) pos = text.size();
                    SendMessageW(g_hwndEditor, EM_SETSEL, pos, pos);
                    SendMessageW(g_hwndEditor, EM_SCROLLCARET, 0, 0);
                    SetFocus(g_hwndEditor);
                }
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL) { EndDialog(hDlg, IDCANCEL); return TRUE; }
            break;
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        return FALSE; });
}

void FormatFont()
{
    LOGFONTW lf{};
    if (g_state.hFont)
        GetObjectW(g_state.hFont, sizeof(LOGFONTW), &lf);
    else
    {
        HDC hdc = GetDC(g_hwndMain);
        lf.lfHeight = -MulDiv(g_state.fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(g_hwndMain, hdc);
        wcscpy_s(lf.lfFaceName, g_state.fontName.c_str());
        lf.lfWeight = g_state.fontWeight;
        lf.lfItalic = g_state.fontItalic ? TRUE : FALSE;
        lf.lfUnderline = g_state.fontUnderline ? TRUE : FALSE;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = CLEARTYPE_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    }
    CHOOSEFONTW cf{};
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = g_hwndMain;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST;
    if (ChooseFontW(&cf))
    {
        g_state.fontName = lf.lfFaceName;
        g_state.fontWeight = lf.lfWeight;
        g_state.fontItalic = (lf.lfItalic != 0);
        g_state.fontUnderline = (lf.lfUnderline != 0);
        HDC hdc2 = GetDC(g_hwndMain);
        g_state.fontSize = MulDiv(-lf.lfHeight, 72, GetDeviceCaps(hdc2, LOGPIXELSY));
        ReleaseDC(g_hwndMain, hdc2);
        ApplyFont();
        SaveFontSettings();
    }
}

void ViewTransparency()
{
    int pct = g_state.windowOpacity * 100 / 255;
    wchar_t buf[32];
    wsprintfW(buf, L"%d", pct);
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", L"Window Transparency",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 300, 300, 280, 110,
                                g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (hDlg)
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", L"Opacity (10-100%):", WS_CHILD | WS_VISIBLE, 10, 18, 110, 20, hDlg, nullptr, nullptr, nullptr);
        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE | ES_NUMBER, 125, 15, 60, 22, hDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        HWND hOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 50, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        HWND hCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 130, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0))
        {
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
                break;
            if (msg.hwnd == hOk && msg.message == WM_LBUTTONUP)
            {
                GetWindowTextW(hEdit, buf, 32);
                int val = _wtoi(buf);
                val = (val < 10) ? 10 : (val > 100) ? 100
                                                    : val;
                g_state.windowOpacity = static_cast<BYTE>(val * 255 / 100);
                SetWindowLongW(g_hwndMain, GWL_EXSTYLE, GetWindowLongW(g_hwndMain, GWL_EXSTYLE) | WS_EX_LAYERED);
                SetLayeredWindowAttributes(g_hwndMain, 0, g_state.windowOpacity, LWA_ALPHA);
                break;
            }
            if (msg.hwnd == hCancel && msg.message == WM_LBUTTONUP)
                break;
            if (!IsDialogMessageW(hDlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        DestroyWindow(hDlg);
    }
}

void HelpAbout()
{
    MessageBoxW(g_hwndMain, L"Legacy Notepad v1.1.0\n\nA fast, lightweight text editor.\n\nBuilt with C++ and Win32 API.", L"About Notepad", MB_ICONINFORMATION);
}
