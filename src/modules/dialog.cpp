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
#include "theme.h"
#include "settings.h"
#include "lang/lang.h"
#include <commdlg.h>
#include <richedit.h>
#include <uxtheme.h>
#include <algorithm>
#include <cwctype>

static HWND g_transparencyEdit = nullptr;

static HBRUSH GetDialogBackgroundBrush()
{
    static HBRUSH brush = CreateSolidBrush(RGB(32, 32, 32));
    return brush;
}

static HBRUSH GetDialogEditBrush()
{
    static HBRUSH brush = CreateSolidBrush(RGB(45, 45, 45));
    return brush;
}

static bool DrawDarkDialogButton(const DRAWITEMSTRUCT *dis)
{
    if (!IsDarkMode() || !dis || dis->CtlType != ODT_BUTTON)
        return false;

    COLORREF bgColor = (dis->itemState & ODS_SELECTED) ? RGB(70, 70, 70) : RGB(56, 56, 56);
    COLORREF borderColor = RGB(95, 95, 95);
    COLORREF textColor = (dis->itemState & ODS_DISABLED) ? RGB(140, 140, 140) : RGB(240, 240, 240);

    HBRUSH bgBrush = CreateSolidBrush(bgColor);
    FillRect(dis->hDC, &dis->rcItem, bgBrush);
    DeleteObject(bgBrush);

    HBRUSH borderBrush = CreateSolidBrush(borderColor);
    FrameRect(dis->hDC, &dis->rcItem, borderBrush);
    DeleteObject(borderBrush);

    wchar_t text[128] = {};
    GetWindowTextW(dis->hwndItem, text, 128);
    RECT textRect = dis->rcItem;
    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, textColor);
    DrawTextW(dis->hDC, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if (dis->itemState & ODS_FOCUS)
    {
        RECT focusRect = dis->rcItem;
        InflateRect(&focusRect, -4, -4);
        DrawFocusRect(dis->hDC, &focusRect);
    }
    return true;
}

static void MakeDialogButtonsOwnerDraw(HWND hDlg)
{
    if (!IsDarkMode())
        return;
    wchar_t cls[16] = {};
    for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
    {
        if (GetClassNameW(h, cls, 16) > 0 && lstrcmpiW(cls, L"Button") == 0)
        {
            LONG_PTR style = GetWindowLongPtrW(h, GWL_STYLE);
            SetWindowLongPtrW(h, GWL_STYLE, (style & ~BS_TYPEMASK) | BS_OWNERDRAW);
            InvalidateRect(h, nullptr, TRUE);
        }
    }
}

static void ApplyDialogDarkMode(HWND hDlg)
{
    if (!IsDarkMode())
        return;
    SetTitleBarDark(hDlg, TRUE);
    SetWindowTheme(hDlg, L"DarkMode_Explorer", nullptr);
    for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
        SetWindowTheme(h, L"DarkMode_Explorer", nullptr);
    MakeDialogButtonsOwnerDraw(hDlg);
}

static INT_PTR HandleDialogDarkColors(UINT msg, WPARAM wParam)
{
    if (!IsDarkMode())
        return 0;
    HDC hdc = reinterpret_cast<HDC>(wParam);
    switch (msg)
    {
    case WM_CTLCOLOREDIT:
        SetTextColor(hdc, RGB(240, 240, 240));
        SetBkColor(hdc, RGB(45, 45, 45));
        return reinterpret_cast<INT_PTR>(GetDialogEditBrush());
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
        SetTextColor(hdc, RGB(240, 240, 240));
        SetBkColor(hdc, RGB(32, 32, 32));
        return reinterpret_cast<INT_PTR>(GetDialogBackgroundBrush());
    }
    return 0;
}

static INT_PTR CALLBACK TransparencyDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        ApplyDialogDarkMode(hDlg);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t buf[32] = {};
            GetWindowTextW(g_transparencyEdit, buf, 32);
            int val = _wtoi(buf);
            val = (val < 10) ? 10 : (val > 100) ? 100
                                                : val;
            g_state.windowOpacity = static_cast<BYTE>(val * 255 / 100);
            SetWindowLongW(g_hwndMain, GWL_EXSTYLE, GetWindowLongW(g_hwndMain, GWL_EXSTYLE) | WS_EX_LAYERED);
            SetLayeredWindowAttributes(g_hwndMain, 0, g_state.windowOpacity, LWA_ALPHA);
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    {
        INT_PTR colorResult = HandleDialogDarkColors(msg, wParam);
        if (colorResult)
            return colorResult;
        break;
    }
    case WM_DRAWITEM:
        if (DrawDarkDialogButton(reinterpret_cast<const DRAWITEMSTRUCT *>(lParam)))
            return TRUE;
        break;
    case WM_CLOSE:
        EndDialog(hDlg, IDCANCEL);
        return TRUE;
    }
    return FALSE;
}

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
    {
        const auto &lang = GetLangStrings();
        MessageBoxW(g_hwndMain, (lang.msgCannotFind + g_state.findText + L"\"").c_str(), lang.appName.c_str(), MB_ICONINFORMATION);
    }
}

INT_PTR CALLBACK FindDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        ApplyDialogDarkMode(hDlg);
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
                std::wstring sel(static_cast<size_t>(end - start) + 1, L'\0');
                LRESULT copied = SendMessageW(g_hwndEditor, EM_GETSELTEXT, 0, reinterpret_cast<LPARAM>(sel.data()));
                if (copied >= 0)
                    sel.resize(static_cast<size_t>(copied));
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
            size_t matchCount = 0;
            while ((pos = lower.find(findLower, lastPos)) != std::wstring::npos)
            {
                newText += text.substr(lastPos, pos - lastPos);
                newText += g_state.replaceText;
                lastPos = pos + g_state.findText.size();
                ++matchCount;
            }
            if (matchCount == 0)
            {
                const auto &lang = GetLangStrings();
                MessageBoxW(g_hwndMain, (lang.msgCannotFind + g_state.findText + L"\"").c_str(), lang.appName.c_str(), MB_ICONINFORMATION);
                return TRUE;
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
        HBRUSH hBrush = IsDarkMode() ? GetDialogBackgroundBrush() : CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        FillRect(hdc, &ps.rcPaint, hBrush);
        if (!IsDarkMode())
            DeleteObject(hBrush);
        EndPaint(hDlg, &ps);
        return FALSE;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    {
        INT_PTR colorResult = HandleDialogDarkColors(msg, wParam);
        if (colorResult)
            return colorResult;
        return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_BTNFACE));
    }
    case WM_DRAWITEM:
        if (DrawDarkDialogButton(reinterpret_cast<const DRAWITEMSTRUCT *>(lParam)))
            return TRUE;
        break;
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
    const auto &lang = GetLangStrings();
    g_hwndFindDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", lang.dialogFind.c_str(),
                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 420, 120,
                                    g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (g_hwndFindDlg)
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", lang.dialogFindLabel.c_str(), WS_CHILD | WS_VISIBLE, 10, 12, 45, 16, g_hwndFindDlg, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_state.findText.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 60, 10, 230, 20, g_hwndFindDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogFindNext.c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 300, 10, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogClose.c_str(), WS_CHILD | WS_VISIBLE, 300, 38, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(2), nullptr, nullptr);
        for (HWND h = GetWindow(g_hwndFindDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SetWindowLongPtrW(g_hwndFindDlg, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FindDlgProc));
        ApplyDialogDarkMode(g_hwndFindDlg);
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
    const auto &lang = GetLangStrings();
    g_hwndFindDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", lang.dialogFindReplace.c_str(),
                                    WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 420, 175,
                                    g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (g_hwndFindDlg)
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", lang.dialogFindLabel.c_str(), WS_CHILD | WS_VISIBLE, 10, 12, 45, 16, g_hwndFindDlg, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_state.findText.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 60, 10, 230, 20, g_hwndFindDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        CreateWindowExW(0, L"STATIC", lang.dialogReplaceLabel.c_str(), WS_CHILD | WS_VISIBLE, 10, 40, 50, 16, g_hwndFindDlg, nullptr, nullptr, nullptr);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_state.replaceText.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 60, 38, 230, 20, g_hwndFindDlg, reinterpret_cast<HMENU>(1002), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogFindNext.c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 300, 10, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogReplace.c_str(), WS_CHILD | WS_VISIBLE, 300, 38, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(3), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogReplaceAll.c_str(), WS_CHILD | WS_VISIBLE, 300, 66, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(4), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogClose.c_str(), WS_CHILD | WS_VISIBLE, 300, 94, 100, 22, g_hwndFindDlg, reinterpret_cast<HMENU>(2), nullptr, nullptr);
        for (HWND h = GetWindow(g_hwndFindDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SetWindowLongPtrW(g_hwndFindDlg, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FindDlgProc));
        ApplyDialogDarkMode(g_hwndFindDlg);
        InvalidateRect(g_hwndFindDlg, nullptr, FALSE);
        UpdateWindow(g_hwndFindDlg);
    }
}

INT_PTR CALLBACK GotoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        ApplyDialogDarkMode(hDlg);
        return TRUE;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    {
        INT_PTR colorResult = HandleDialogDarkColors(msg, wParam);
        if (colorResult)
            return colorResult;
        break;
    }
    case WM_DRAWITEM:
        if (DrawDarkDialogButton(reinterpret_cast<const DRAWITEMSTRUCT *>(lParam)))
            return TRUE;
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t buf[32];
            GetWindowTextW(GetDlgItem(hDlg, 1001), buf, 32);
            int line = _wtoi(buf);
            if (line > 0)
            {
                LRESULT charIndex = SendMessageW(g_hwndEditor, EM_LINEINDEX, static_cast<WPARAM>(line) - 1, 0);
                if (charIndex != -1)
                {
                    SendMessageW(g_hwndEditor, EM_SETSEL, charIndex, charIndex);
                    SendMessageW(g_hwndEditor, EM_SCROLLCARET, 0, 0);
                    SetFocus(g_hwndEditor);
                    DestroyWindow(hDlg);
                }
                else
                {
                    const auto &lang = GetLangStrings();
                    MessageBoxW(hDlg, L"The line number is beyond the total number of lines.", (lang.appName + L" - " + lang.dialogGoTo).c_str(), MB_OK | MB_ICONWARNING);
                }
            }
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            DestroyWindow(hDlg);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;
    }
    return DefDlgProcW(hDlg, msg, wParam, lParam);
}

void EditGoto()
{
    const auto &lang = GetLangStrings();
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", lang.dialogGoTo.c_str(),
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 100, 100, 250, 140,
                                g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (hDlg)
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", lang.dialogLineNumber.c_str(), WS_CHILD | WS_VISIBLE, 15, 15, 100, 16, hDlg, nullptr, nullptr, nullptr);

        DWORD start = 0;
        SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), 0);
        int curLine = (int)SendMessageW(g_hwndEditor, EM_EXLINEFROMCHAR, 0, start) + 1;
        wchar_t buf[32];
        wsprintfW(buf, L"%d", curLine);

        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_AUTOHSCROLL, 15, 35, 210, 22, hDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        SendMessageW(hEdit, EM_SETSEL, 0, -1);

        CreateWindowExW(0, L"BUTTON", lang.dialogOK.c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 60, 70, 80, 25, hDlg, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogCancel.c_str(), WS_CHILD | WS_VISIBLE, 145, 70, 80, 25, hDlg, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);

        for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

        SetWindowLongPtrW(hDlg, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(GotoDlgProc));
        ApplyDialogDarkMode(hDlg);
        SetFocus(hEdit);
    }
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
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_BOTH;
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
    const auto &lang = GetLangStrings();
    int pct = g_state.windowOpacity * 100 / 255;
    wchar_t buf[32];
    wsprintfW(buf, L"%d", pct);
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, L"#32770", lang.dialogTransparency.c_str(),
                                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 300, 300, 280, 110,
                                g_hwndMain, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (hDlg)
    {
        HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        CreateWindowExW(0, L"STATIC", lang.dialogOpacityLabel.c_str(), WS_CHILD | WS_VISIBLE, 10, 18, 110, 20, hDlg, nullptr, nullptr, nullptr);
        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", buf, WS_CHILD | WS_VISIBLE | ES_NUMBER, 125, 15, 60, 22, hDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogOK.c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 50, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
        CreateWindowExW(0, L"BUTTON", lang.dialogCancel.c_str(), WS_CHILD | WS_VISIBLE, 130, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
        for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT))
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        g_transparencyEdit = hEdit;
        SetWindowLongPtrW(hDlg, DWLP_DLGPROC, reinterpret_cast<LONG_PTR>(TransparencyDlgProc));
        TransparencyDlgProc(hDlg, WM_INITDIALOG, 0, 0);
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0))
        {
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
        if (IsWindow(hDlg))
            DestroyWindow(hDlg);
        g_transparencyEdit = nullptr;
    }
}

void HelpAbout()
{
    const auto &lang = GetLangStrings();
    MessageBoxW(g_hwndMain, lang.msgAbout.c_str(), lang.appName.c_str(), MB_ICONINFORMATION);
}
