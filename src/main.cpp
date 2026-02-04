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

  Main entry point and window procedure for Legacy Notepad text editor application.
  Coordinates all modules and handles Windows message loop and command dispatching.
*/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <gdiplus.h>

#include "resource.h"
#include "core/types.h"
#include "core/globals.h"
#include "modules/theme.h"
#include "modules/editor.h"
#include "modules/file.h"
#include "modules/ui.h"
#include "modules/background.h"
#include "modules/dialog.h"
#include "modules/commands.h"
#include "modules/settings.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g_hwndMain = hwnd;
        DragAcceptFiles(hwnd, TRUE);
        LoadLibraryW(L"Msftedit.dll");
        g_hwndEditor = CreateWindowExW(0, MSFTEDIT_CLASS, nullptr,
                                       WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
                                       0, 0, 100, 100, hwnd, reinterpret_cast<HMENU>(IDC_EDITOR), GetModuleHandleW(nullptr), nullptr);
        g_origEditorProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_hwndEditor, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditorSubclassProc)));
        g_hwndStatus = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
                                       WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_STATUSBAR), GetModuleHandleW(nullptr), nullptr);
        g_origStatusProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_hwndStatus, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(StatusSubclassProc)));
        SendMessageW(g_hwndEditor, EM_SETLIMITTEXT, 0, 0);
        SendMessageW(g_hwndEditor, EM_SETEVENTMASK, 0, ENM_CHANGE);
        ApplyFont();
        SetupStatusBarParts();
        UpdateTitle();
        UpdateStatus();
        ApplyTheme();
        SetFocus(g_hwndEditor);
        return 0;
    }
    case WM_UAHDRAWMENU:
    {
        if (IsDarkMode())
        {
            UAHMENU *pUDM = reinterpret_cast<UAHMENU *>(lParam);
            MENUBARINFO mbi = {sizeof(mbi)};
            if (GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi))
            {
                RECT rcWindow;
                GetWindowRect(hwnd, &rcWindow);
                RECT rcMenuBar = mbi.rcBar;
                OffsetRect(&rcMenuBar, -rcWindow.left, -rcWindow.top);
                FillRect(pUDM->hdc, &rcMenuBar, g_hbrMenuDark ? g_hbrMenuDark : reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            }
            return TRUE;
        }
        break;
    }
    case WM_UAHDRAWMENUITEM:
    {
        if (IsDarkMode())
        {
            UAHDRAWMENUITEM *pUDMI = reinterpret_cast<UAHDRAWMENUITEM *>(lParam);
            wchar_t szText[256] = {};
            MENUITEMINFOW mii = {sizeof(mii)};
            mii.fMask = MIIM_STRING;
            mii.dwTypeData = szText;
            mii.cch = 255;
            GetMenuItemInfoW(pUDMI->um.hMenu, pUDMI->umi.iPosition, TRUE, &mii);
            COLORREF bgColor = RGB(45, 45, 45);
            COLORREF textColor = RGB(255, 255, 255);
            if ((pUDMI->dis.itemState & ODS_HOTLIGHT) || (pUDMI->dis.itemState & ODS_SELECTED))
                bgColor = RGB(65, 65, 65);
            HBRUSH hbr = CreateSolidBrush(bgColor);
            FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, hbr);
            DeleteObject(hbr);
            NONCLIENTMETRICSW ncm = {sizeof(ncm)};
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            HFONT hFont = CreateFontIndirectW(&ncm.lfMenuFont);
            HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(pUDMI->um.hdc, hFont));
            SetBkMode(pUDMI->um.hdc, TRANSPARENT);
            SetTextColor(pUDMI->um.hdc, textColor);
            RECT rcText = pUDMI->dis.rcItem;
            DrawTextW(pUDMI->um.hdc, szText, -1, &rcText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            SelectObject(pUDMI->um.hdc, hOldFont);
            DeleteObject(hFont);
            return TRUE;
        }
        break;
    }
    case WM_NCPAINT:
    case WM_NCACTIVATE:
    {
        LRESULT result = DefWindowProcW(hwnd, msg, wParam, lParam);
        if (IsDarkMode())
        {
            HDC hdc = GetWindowDC(hwnd);
            MENUBARINFO mbi = {sizeof(mbi)};
            if (GetMenuBarInfo(hwnd, OBJID_MENU, 0, &mbi))
            {
                RECT rcWindow;
                GetWindowRect(hwnd, &rcWindow);
                RECT rcMenuBar = mbi.rcBar;
                OffsetRect(&rcMenuBar, -rcWindow.left, -rcWindow.top);
                rcMenuBar.bottom += 2;
                FillRect(hdc, &rcMenuBar, g_hbrMenuDark ? g_hbrMenuDark : reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
                HMENU hMenu = GetMenu(hwnd);
                int itemCount = GetMenuItemCount(hMenu);
                NONCLIENTMETRICSW ncm = {sizeof(ncm)};
                SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
                HFONT hFont = CreateFontIndirectW(&ncm.lfMenuFont);
                HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(hdc, hFont));
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 255, 255));
                for (int i = 0; i < itemCount; i++)
                {
                    RECT rcItem;
                    if (GetMenuBarInfo(hwnd, OBJID_MENU, i + 1, &mbi))
                    {
                        rcItem = mbi.rcBar;
                        OffsetRect(&rcItem, -rcWindow.left, -rcWindow.top);
                        wchar_t szText[256] = {};
                        MENUITEMINFOW mii = {sizeof(mii)};
                        mii.fMask = MIIM_STRING;
                        mii.dwTypeData = szText;
                        mii.cch = 255;
                        GetMenuItemInfoW(hMenu, i, TRUE, &mii);
                        DrawTextW(hdc, szText, -1, &rcItem, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
                    }
                }
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
            }
            ReleaseDC(hwnd, hdc);
        }
        return result;
    }
    case WM_SETTINGCHANGE:
    {
        if (lParam && wcscmp(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0)
            ApplyTheme();
        return 0;
    }
    case WM_DROPFILES:
    {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        wchar_t path[MAX_PATH];
        if (DragQueryFileW(hDrop, 0, path, MAX_PATH))
        {
            if (ConfirmDiscard())
                LoadFile(path);
        }
        DragFinish(hDrop);
        return 0;
    }
    case WM_SIZE:
        ResizeControls();
        UpdateStatus();
        return 0;
    case WM_SETFOCUS:
        SetFocus(g_hwndEditor);
        return 0;
    case WM_CTLCOLOREDIT:
        if (g_state.background.enabled && g_bgImage && reinterpret_cast<HWND>(lParam) == g_hwndEditor)
        {
            SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
        }
        break;
    case WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) == g_hwndStatus && IsDarkMode())
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, RGB(45, 45, 45));
            return reinterpret_cast<LRESULT>(g_hbrStatusDark ? g_hbrStatusDark : GetStockObject(BLACK_BRUSH));
        }
        break;
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (pDIS->hwndItem == g_hwndStatus && IsDarkMode())
        {
            HBRUSH hbr = g_hbrStatusDark ? g_hbrStatusDark : CreateSolidBrush(RGB(45, 45, 45));
            FillRect(pDIS->hDC, &pDIS->rcItem, hbr);
            if (!g_hbrStatusDark)
                DeleteObject(hbr);
            SetBkMode(pDIS->hDC, TRANSPARENT);
            SetTextColor(pDIS->hDC, RGB(255, 255, 255));
            int part = static_cast<int>(pDIS->itemID);
            if (part >= 0 && part < 4)
            {
                RECT rc = pDIS->rcItem;
                rc.left += 4;
                DrawTextW(pDIS->hDC, g_statusTexts[part].c_str(), -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
            }
            return TRUE;
        }
        break;
    }
    case WM_COMMAND:
    {
        WORD cmd = LOWORD(wParam);
        if (cmd >= IDM_FILE_RECENT_BASE && cmd < IDM_FILE_RECENT_BASE + MAX_RECENT_FILES)
        {
            int idx = cmd - IDM_FILE_RECENT_BASE;
            if (idx < static_cast<int>(g_state.recentFiles.size()))
            {
                if (ConfirmDiscard())
                    LoadFile(g_state.recentFiles[idx]);
            }
            return 0;
        }
        switch (cmd)
        {
        case IDM_FILE_NEW:
            FileNew();
            break;
        case IDM_FILE_OPEN:
            FileOpen();
            break;
        case IDM_FILE_SAVE:
            FileSave();
            break;
        case IDM_FILE_SAVEAS:
            FileSaveAs();
            break;
        case IDM_FILE_PRINT:
            FilePrint();
            break;
        case IDM_FILE_PAGESETUP:
            FilePageSetup();
            break;
        case IDM_FILE_EXIT:
            if (ConfirmDiscard())
                DestroyWindow(hwnd);
            break;
        case IDM_EDIT_UNDO:
            EditUndo();
            break;
        case IDM_EDIT_REDO:
            EditRedo();
            break;
        case IDM_EDIT_CUT:
            EditCut();
            break;
        case IDM_EDIT_COPY:
            EditCopy();
            break;
        case IDM_EDIT_PASTE:
            EditPaste();
            break;
        case IDM_EDIT_DELETE:
            EditDelete();
            break;
        case IDM_EDIT_FIND:
            EditFind();
            break;
        case IDM_EDIT_FINDNEXT:
            EditFindNext();
            break;
        case IDM_EDIT_FINDPREV:
            EditFindPrev();
            break;
        case IDM_EDIT_REPLACE:
            EditReplace();
            break;
        case IDM_EDIT_GOTO:
            EditGoto();
            break;
        case IDM_EDIT_SELECTALL:
            EditSelectAll();
            break;
        case IDM_EDIT_TIMEDATE:
            EditTimeDate();
            break;
        case IDM_FORMAT_WORDWRAP:
            FormatWordWrap();
            break;
        case IDM_FORMAT_FONT:
            FormatFont();
            break;
        case IDM_VIEW_ZOOMIN:
            ViewZoomIn();
            break;
        case IDM_VIEW_ZOOMOUT:
            ViewZoomOut();
            break;
        case IDM_VIEW_ZOOMDEFAULT:
            ViewZoomDefault();
            break;
        case IDM_VIEW_STATUSBAR:
            ViewStatusBar();
            break;
        case IDM_VIEW_DARKMODE:
            ToggleDarkMode();
            break;
        case IDM_VIEW_TRANSPARENCY:
            ViewTransparency();
            break;
        case IDM_VIEW_BG_SELECT:
            ViewSelectBackground();
            break;
        case IDM_VIEW_BG_CLEAR:
            ViewClearBackground();
            break;
        case IDM_VIEW_BG_OPACITY:
            ViewBackgroundOpacity();
            break;
        case IDM_VIEW_BG_POS_TOPLEFT:
            SetBackgroundPosition(BgPosition::TopLeft);
            break;
        case IDM_VIEW_BG_POS_TOPCENTER:
            SetBackgroundPosition(BgPosition::TopCenter);
            break;
        case IDM_VIEW_BG_POS_TOPRIGHT:
            SetBackgroundPosition(BgPosition::TopRight);
            break;
        case IDM_VIEW_BG_POS_CENTERLEFT:
            SetBackgroundPosition(BgPosition::CenterLeft);
            break;
        case IDM_VIEW_BG_POS_CENTER:
            SetBackgroundPosition(BgPosition::Center);
            break;
        case IDM_VIEW_BG_POS_CENTERRIGHT:
            SetBackgroundPosition(BgPosition::CenterRight);
            break;
        case IDM_VIEW_BG_POS_BOTTOMLEFT:
            SetBackgroundPosition(BgPosition::BottomLeft);
            break;
        case IDM_VIEW_BG_POS_BOTTOMCENTER:
            SetBackgroundPosition(BgPosition::BottomCenter);
            break;
        case IDM_VIEW_BG_POS_BOTTOMRIGHT:
            SetBackgroundPosition(BgPosition::BottomRight);
            break;
        case IDM_VIEW_BG_POS_TILE:
            SetBackgroundPosition(BgPosition::Tile);
            break;
        case IDM_VIEW_BG_POS_STRETCH:
            SetBackgroundPosition(BgPosition::Stretch);
            break;
        case IDM_VIEW_BG_POS_FIT:
            SetBackgroundPosition(BgPosition::Fit);
            break;
        case IDM_VIEW_BG_POS_FILL:
            SetBackgroundPosition(BgPosition::Fill);
            break;
        case IDM_VIEW_ICON_CHANGE:
            ViewChangeIcon();
            break;
        case IDM_VIEW_ICON_RESET:
            ViewResetIcon();
            break;
        case IDM_HELP_ABOUT:
            HelpAbout();
            break;
        }
        return 0;
    }
    case WM_NOTIFY:
    {
        NMHDR *pnmh = reinterpret_cast<NMHDR *>(lParam);
        if (pnmh->hwndFrom == g_hwndStatus && pnmh->code == NM_CUSTOMDRAW)
        {
            if (IsDarkMode())
            {
                LPNMCUSTOMDRAW lpnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
                if (lpnmcd->dwDrawStage == CDDS_PREPAINT)
                    return CDRF_NOTIFYITEMDRAW;
                if (lpnmcd->dwDrawStage == CDDS_ITEMPREPAINT)
                {
                    HBRUSH hbr = g_hbrStatusDark ? g_hbrStatusDark : CreateSolidBrush(RGB(45, 45, 45));
                    FillRect(lpnmcd->hdc, &lpnmcd->rc, hbr);
                    if (!g_hbrStatusDark && hbr)
                        DeleteObject(hbr);
                    SetBkMode(lpnmcd->hdc, TRANSPARENT);
                    SetBkColor(lpnmcd->hdc, RGB(45, 45, 45));
                    SetTextColor(lpnmcd->hdc, RGB(255, 255, 255));
                    wchar_t buf[256] = {};
                    int part = static_cast<int>(lpnmcd->dwItemSpec);
                    SendMessageW(g_hwndStatus, SB_GETTEXTW, part, reinterpret_cast<LPARAM>(buf));
                    RECT rc = lpnmcd->rc;
                    rc.left += 6;
                    DrawTextW(lpnmcd->hdc, buf, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);
                    return CDRF_SKIPDEFAULT;
                }
            }
        }
        if (pnmh->hwndFrom == g_hwndEditor && pnmh->code == EN_CHANGE)
        {
            g_state.modified = true;
            UpdateTitle();
            UpdateStatus();
        }
        return 0;
    }
    case WM_CLOSE:
        if (g_state.closing)
            return 0;
        g_state.closing = true;
        if (ConfirmDiscard())
            DestroyWindow(hwnd);
        else
            g_state.closing = false;
        return 0;
    case WM_DESTROY:
        if (g_state.hFont)
        {
            DeleteObject(g_state.hFont);
            g_state.hFont = nullptr;
        }
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
        if (g_hCustomIcon)
        {
            DestroyIcon(g_hCustomIcon);
            g_hCustomIcon = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    case WM_MOUSEWHEEL:
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0)
                ViewZoomIn();
            else
                ViewZoomOut();
            return 0;
        }
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxtheme)
    {
        BOOL useDark = IsDarkMode();
        typedef void(WINAPI * fnAllowDarkModeForApp)(BOOL allow);
        fnAllowDarkModeForApp allowDarkModeForApp = (fnAllowDarkModeForApp)(void *)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132));
        if (allowDarkModeForApp)
            allowDarkModeForApp(useDark);
        typedef PreferredAppMode(WINAPI * fnSetPreferredAppMode)(PreferredAppMode appMode);
        fnSetPreferredAppMode setPreferredAppMode = (fnSetPreferredAppMode)(void *)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        if (setPreferredAppMode)
            setPreferredAppMode(useDark ? ForceDark : ForceLight);
        typedef void(WINAPI * fnRefreshImmersiveColorPolicyState)();
        fnRefreshImmersiveColorPolicyState refreshPolicy = (fnRefreshImmersiveColorPolicyState)(void *)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));
        if (refreshPolicy)
            refreshPolicy();
        typedef void(WINAPI * fnFlushMenuThemes)();
        fnFlushMenuThemes flushMenuThemes = (fnFlushMenuThemes)(void *)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136));
        if (flushMenuThemes)
            flushMenuThemes();
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icc);

    LoadFontSettings();

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_NOTEPAD));
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
    wc.lpszClassName = L"NotepadClass";
    wc.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_NOTEPAD));
    RegisterClassExW(&wc);

    g_hwndMain = CreateWindowExW(0, L"NotepadClass", L"Untitled - Notepad",
                                 WS_OVERLAPPEDWINDOW | WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                                 nullptr, nullptr, hInstance, nullptr);
    g_hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_ACCEL));
    SetTitleBarDark(g_hwndMain, IsDarkMode());
    ShowWindow(g_hwndMain, nCmdShow);
    UpdateWindow(g_hwndMain);

    if (lpCmdLine && lpCmdLine[0])
    {
        std::wstring path = lpCmdLine;
        if (path.front() == L'"' && path.back() == L'"')
            path = path.substr(1, path.size() - 2);
        LoadFile(path);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        if (g_hwndFindDlg && IsDialogMessageW(g_hwndFindDlg, &msg))
            continue;
        if (!TranslateAcceleratorW(g_hwndMain, g_hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    Gdiplus::GdiplusShutdown(g_gdiplusToken);
    return static_cast<int>(msg.wParam);
}
