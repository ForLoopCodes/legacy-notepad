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

  Menu command handlers for File, Edit, Format, and View menu operations.
  Bridges user actions to core functionality modules.
*/

#include "commands.h"
#include "core/globals.h"
#include "editor.h"
#include "file.h"
#include "ui.h"
#include "settings.h"
#include "resource.h"
#include "lang/lang.h"
#include <commdlg.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <vector>

bool ConfirmDiscard()
{
    if (!g_state.modified)
        return true;
    if (g_state.filePath.empty())
    {
        std::wstring text = GetEditorText();
        if (text.empty())
            return true;
    }
    const auto &lang = GetLangStrings();
    std::wstring filename = g_state.filePath.empty() ? lang.untitled : PathFindFileNameW(g_state.filePath.c_str());
    std::wstring msg;
    msg.reserve(lang.msgSaveChanges.size() + filename.size() + 2);
    msg = lang.msgSaveChanges;
    msg += filename;
    msg += L"?";
    int result = MessageBoxW(g_hwndMain, msg.c_str(), lang.appName.c_str(), MB_YESNOCANCEL | MB_ICONWARNING);
    if (result == IDYES)
    {
        FileSave();
        return true;
    }
    return result == IDNO;
}

void FileNew()
{
    if (!ConfirmDiscard())
        return;
    SetEditorText(L"");
    g_state.filePath.clear();
    g_state.modified = false;
    g_state.encoding = Encoding::UTF8;
    g_state.lineEnding = LineEnding::CRLF;
    UpdateTitle();
    UpdateStatus();
}

void FileOpen()
{
    if (!ConfirmDiscard())
        return;
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.lpstrFilter = L"Text Documents (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING;
    if (GetOpenFileNameW(&ofn))
        LoadFile(path);
}

void FileSave()
{
    if (g_state.filePath.empty())
        FileSaveAs();
    else
        SaveToPath(g_state.filePath);
}

void FileSaveAs()
{
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.lpstrFilter = L"Text Documents (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_ENABLESIZING;
    if (GetSaveFileNameW(&ofn))
        SaveToPath(path);
}

void FilePrint()
{
    std::wstring text = GetEditorText();
    PRINTDLGW pd = {};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = g_hwndMain;
    pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;
    if (!PrintDlgW(&pd))
        return;
    HDC hDC = pd.hDC;
    DOCINFOW di = {};
    di.cbSize = sizeof(di);
    const auto &lang = GetLangStrings();
    std::wstring docName = g_state.filePath.empty() ? lang.untitled : PathFindFileNameW(g_state.filePath.c_str());
    di.lpszDocName = docName.c_str();
    if (StartDocW(hDC, &di) > 0)
    {
        int pageWidth = GetDeviceCaps(hDC, HORZRES);
        int pageHeight = GetDeviceCaps(hDC, VERTRES);
        int marginX = pageWidth / 10, marginY = pageHeight / 10;
        int printWidth = pageWidth - 2 * marginX;
        int printHeight = pageHeight - 2 * marginY;
        HFONT hPrintFont = CreateFontW(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                                       0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                                       FIXED_PITCH | FF_MODERN, g_state.fontName.c_str());
        HFONT hOldFont = reinterpret_cast<HFONT>(SelectObject(hDC, hPrintFont));
        TEXTMETRICW tm;
        GetTextMetricsW(hDC, &tm);
        int lineHeight = tm.tmHeight + tm.tmExternalLeading;
        int linesPerPage = printHeight / lineHeight;
        std::vector<std::wstring> lines;
        std::wstring line;
        for (size_t i = 0; i <= text.size(); ++i)
        {
            if (i == text.size() || text[i] == L'\n' || text[i] == L'\r')
            {
                lines.push_back(line);
                line.clear();
                if (i < text.size() && text[i] == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n')
                    ++i;
            }
            else
                line += text[i];
        }
        int totalLines = static_cast<int>(lines.size());
        int lineIndex = 0;
        while (lineIndex < totalLines)
        {
            StartPage(hDC);
            int y = marginY;
            for (int i = 0; i < linesPerPage && lineIndex < totalLines; ++i, ++lineIndex)
            {
                RECT rc = {marginX, y, marginX + printWidth, y + lineHeight};
                DrawTextW(hDC, lines[lineIndex].c_str(), -1, &rc, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
                y += lineHeight;
            }
            EndPage(hDC);
        }
        SelectObject(hDC, hOldFont);
        DeleteObject(hPrintFont);
        EndDoc(hDC);
    }
    DeleteDC(hDC);
}

void FilePageSetup()
{
    g_pageSetup.lStructSize = sizeof(g_pageSetup);
    g_pageSetup.hwndOwner = g_hwndMain;
    g_pageSetup.Flags = PSD_MARGINS | PSD_INHUNDREDTHSOFMILLIMETERS;
    PageSetupDlgW(&g_pageSetup);
}

void EditUndo() { SendMessageW(g_hwndEditor, EM_UNDO, 0, 0); }
void EditRedo() { SendMessageW(g_hwndEditor, EM_UNDO, 0, 0); }
void EditCut() { SendMessageW(g_hwndEditor, WM_CUT, 0, 0); }
void EditCopy() { SendMessageW(g_hwndEditor, WM_COPY, 0, 0); }
void EditPaste() { SendMessageW(g_hwndEditor, WM_PASTE, 0, 0); }
void EditDelete() { SendMessageW(g_hwndEditor, WM_CLEAR, 0, 0); }
void EditSelectAll() { SendMessageW(g_hwndEditor, EM_SETSEL, 0, -1); }

void EditTimeDate()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[64];
    wsprintfW(buf, L"%02d:%02d %s %02d/%02d/%04d",
              st.wHour % 12 == 0 ? 12 : st.wHour % 12, st.wMinute,
              st.wHour >= 12 ? L"PM" : L"AM", st.wMonth, st.wDay, st.wYear);
    SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(buf));
}

void FormatWordWrap()
{
    g_state.wordWrap = !g_state.wordWrap;
    CheckMenuItem(GetMenu(g_hwndMain), IDM_FORMAT_WORDWRAP, g_state.wordWrap ? MF_CHECKED : MF_UNCHECKED);
    ApplyWordWrap();
}

void ViewZoomIn()
{
    static const int levels[] = {25, 50, 75, 100, 125, 150, 175, 200, 250, 300, 350, 400, 450, 500};
    for (int l : levels)
    {
        if (l > g_state.zoomLevel)
        {
            g_state.zoomLevel = l;
            ApplyZoom();
            UpdateStatus();
            return;
        }
    }
}

void ViewZoomOut()
{
    static const int levels[] = {25, 50, 75, 100, 125, 150, 175, 200, 250, 300, 350, 400, 450, 500};
    for (int i = 13; i >= 0; --i)
    {
        if (levels[i] < g_state.zoomLevel)
        {
            g_state.zoomLevel = levels[i];
            ApplyZoom();
            UpdateStatus();
            return;
        }
    }
}

void ViewZoomDefault()
{
    g_state.zoomLevel = ZOOM_DEFAULT;
    ApplyZoom();
    UpdateStatus();
}

void ViewStatusBar()
{
    g_state.showStatusBar = !g_state.showStatusBar;
    CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_STATUSBAR, g_state.showStatusBar ? MF_CHECKED : MF_UNCHECKED);
    ResizeControls();
    UpdateStatus();
}

void ViewChangeIcon()
{
    const auto &lang = GetLangStrings();
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwndMain;
    ofn.lpstrFilter = L"Icon Files (*.ico)\0*.ico\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn))
    {
        HICON hNewIcon = static_cast<HICON>(LoadImageW(nullptr, path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE));
        if (hNewIcon)
        {
            if (g_hCustomIcon && g_hCustomIcon != hNewIcon)
                DestroyIcon(g_hCustomIcon);
            g_hCustomIcon = hNewIcon;
            g_state.customIconPath = path;
            SendMessageW(g_hwndMain, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hNewIcon));
            SendMessageW(g_hwndMain, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hNewIcon));
        }
        else
            MessageBoxW(g_hwndMain, L"Failed to load icon file.", lang.appName.c_str(), MB_ICONERROR);
    }
}

void ViewResetIcon()
{
    if (g_hCustomIcon)
    {
        DestroyIcon(g_hCustomIcon);
        g_hCustomIcon = nullptr;
    }
    g_state.customIconPath.clear();
    HICON hDefaultIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_NOTEPAD));
    SendMessageW(g_hwndMain, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hDefaultIcon));
    SendMessageW(g_hwndMain, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hDefaultIcon));
}

void HelpCheckUpdates()
{
    ShellExecuteW(nullptr, L"open", L"https://github.com/ForLoopCodes/legacy-notepad/releases/latest", nullptr, nullptr, SW_SHOWNORMAL);
}

void ViewAlwaysOnTop()
{
    g_state.alwaysOnTop = !g_state.alwaysOnTop;
    CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_ALWAYSONTOP, g_state.alwaysOnTop ? MF_CHECKED : MF_UNCHECKED);
    SetWindowPos(g_hwndMain, g_state.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SaveFontSettings();
}
