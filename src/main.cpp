#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>

#include <gdiplus.h>

#include "resource.h"

#define APP_NAME L"Notepad"
#define ZOOM_MIN 25
#define ZOOM_MAX 500
#define ZOOM_DEFAULT 100
#define MAX_RECENT_FILES 10

enum class Encoding
{
  UTF8,
  UTF8BOM,
  UTF16LE,
  UTF16BE,
  ANSI
};
enum class LineEnding
{
  CRLF,
  LF,
  CR
};
enum class BgPosition
{
  TopLeft,
  TopCenter,
  TopRight,
  CenterLeft,
  Center,
  CenterRight,
  BottomLeft,
  BottomCenter,
  BottomRight,
  Tile,
  Stretch,
  Fit,
  Fill
};

struct BackgroundSettings
{
  bool enabled = false;
  std::wstring imagePath;
  BgPosition position = BgPosition::Center;
  BYTE opacity = 128;
};

struct AppState
{
  std::wstring filePath;
  bool modified = false;
  Encoding encoding = Encoding::UTF8;
  LineEnding lineEnding = LineEnding::CRLF;
  std::wstring findText;
  std::wstring replaceText;
  bool wordWrap = false;
  int zoomLevel = ZOOM_DEFAULT;
  bool showStatusBar = true;
  int fontSize = 16;
  std::wstring fontName = L"Consolas";
  BYTE windowOpacity = 255;
  bool closing = false;
  HFONT hFont = nullptr;
  std::deque<std::wstring> recentFiles;
  BackgroundSettings background;
};

HWND g_hwndMain = nullptr;
HWND g_hwndEditor = nullptr;
HWND g_hwndStatus = nullptr;
HWND g_hwndFindDlg = nullptr;
HACCEL g_hAccel = nullptr;
AppState g_state;
WNDPROC g_origEditorProc = nullptr;
ULONG_PTR g_gdiplusToken = 0;
Gdiplus::Image *g_bgImage = nullptr;
HBITMAP g_bgBitmap = nullptr;
int g_bgBitmapW = 0, g_bgBitmapH = 0;

void UpdateTitle();
void UpdateStatus();
void ResizeControls();
void ApplyFont();
void ApplyZoom();
bool ConfirmDiscard();
void FileNew();
void FileOpen();
void FileSave();
void FileSaveAs();
void FilePrint();
void FilePageSetup();
void EditUndo();
void EditRedo();
void EditCut();
void EditCopy();
void EditPaste();
void EditDelete();
void EditFind();
void EditFindNext();
void EditFindPrev();
void EditReplace();
void EditGoto();
void EditSelectAll();
void EditTimeDate();
void FormatWordWrap();
void FormatFont();
void ViewZoomIn();
void ViewZoomOut();
void ViewZoomDefault();
void ViewStatusBar();
void ViewTransparency();
void ViewSelectBackground();
void ViewClearBackground();
void ViewBackgroundOpacity();
void ViewBackgroundPosition();
void HelpAbout();
void AddRecentFile(const std::wstring &path);
void UpdateRecentFilesMenu();
void LoadFile(const std::wstring &path);
void LoadBackgroundImage(const std::wstring &path);
void PaintBackground(HDC hdc, const RECT &rc);
void DeleteWordBackward();
void DeleteWordForward();

PAGESETUPDLGW g_pageSetup = {sizeof(g_pageSetup)};

const wchar_t *GetEncodingName(Encoding e)
{
  switch (e)
  {
  case Encoding::UTF8:
    return L"UTF-8";
  case Encoding::UTF8BOM:
    return L"UTF-8 with BOM";
  case Encoding::UTF16LE:
    return L"UTF-16 LE";
  case Encoding::UTF16BE:
    return L"UTF-16 BE";
  case Encoding::ANSI:
    return L"ANSI";
  }
  return L"";
}

const wchar_t *GetLineEndingName(LineEnding le)
{
  switch (le)
  {
  case LineEnding::CRLF:
    return L"Windows (CRLF)";
  case LineEnding::LF:
    return L"Unix (LF)";
  case LineEnding::CR:
    return L"Macintosh (CR)";
  }
  return L"";
}

std::pair<Encoding, LineEnding> DetectEncoding(const std::vector<BYTE> &data)
{
  Encoding enc = Encoding::UTF8;
  if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
  {
    enc = Encoding::UTF8BOM;
  }
  else if (data.size() >= 2 && data[0] == 0xFF && data[1] == 0xFE)
  {
    enc = Encoding::UTF16LE;
  }
  else if (data.size() >= 2 && data[0] == 0xFE && data[1] == 0xFF)
  {
    enc = Encoding::UTF16BE;
  }
  else
  {
    int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                     reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size()), nullptr, 0);
    if (result == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
    {
      enc = Encoding::ANSI;
    }
  }
  LineEnding le = LineEnding::CRLF;
  for (size_t i = 0; i < data.size(); ++i)
  {
    if (data[i] == '\r')
    {
      if (i + 1 < data.size() && data[i + 1] == '\n')
      {
        le = LineEnding::CRLF;
        break;
      }
      le = LineEnding::CR;
      break;
    }
    if (data[i] == '\n')
    {
      le = LineEnding::LF;
      break;
    }
  }
  return {enc, le};
}

std::wstring DecodeText(const std::vector<BYTE> &data, Encoding enc)
{
  size_t skip = 0;
  UINT codepage = CP_UTF8;
  switch (enc)
  {
  case Encoding::UTF8BOM:
    skip = 3;
    break;
  case Encoding::UTF16LE:
  {
    skip = 2;
    const wchar_t *wptr = reinterpret_cast<const wchar_t *>(data.data() + skip);
    size_t wlen = (data.size() - skip) / 2;
    return std::wstring(wptr, wlen);
  }
  case Encoding::UTF16BE:
  {
    skip = 2;
    std::wstring result;
    result.reserve((data.size() - skip) / 2);
    for (size_t i = skip; i + 1 < data.size(); i += 2)
    {
      wchar_t c = static_cast<wchar_t>((data[i] << 8) | data[i + 1]);
      result += c;
    }
    return result;
  }
  case Encoding::ANSI:
    codepage = CP_ACP;
    break;
  default:
    break;
  }
  const char *ptr = reinterpret_cast<const char *>(data.data() + skip);
  int len = static_cast<int>(data.size() - skip);
  if (len <= 0)
    return L"";
  int wlen = MultiByteToWideChar(codepage, 0, ptr, len, nullptr, 0);
  if (wlen <= 0)
    return L"";
  std::wstring result(wlen, 0);
  MultiByteToWideChar(codepage, 0, ptr, len, &result[0], wlen);
  return result;
}

std::vector<BYTE> EncodeText(const std::wstring &text, Encoding enc, LineEnding le)
{
  std::wstring converted;
  converted.reserve(text.size() * 2);
  for (size_t i = 0; i < text.size(); ++i)
  {
    wchar_t c = text[i];
    if (c == L'\r' || c == L'\n')
    {
      if (c == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n')
        ++i;
      switch (le)
      {
      case LineEnding::CRLF:
        converted += L"\r\n";
        break;
      case LineEnding::LF:
        converted += L'\n';
        break;
      case LineEnding::CR:
        converted += L'\r';
        break;
      }
    }
    else
    {
      converted += c;
    }
  }
  std::vector<BYTE> result;
  switch (enc)
  {
  case Encoding::UTF8BOM:
    result.push_back(0xEF);
    result.push_back(0xBB);
    result.push_back(0xBF);
    [[fallthrough]];
  case Encoding::UTF8:
  {
    int len = WideCharToMultiByte(CP_UTF8, 0, converted.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len > 1)
    {
      size_t offset = result.size();
      result.resize(offset + len - 1);
      WideCharToMultiByte(CP_UTF8, 0, converted.c_str(), -1,
                          reinterpret_cast<char *>(result.data() + offset), len, nullptr, nullptr);
    }
    break;
  }
  case Encoding::UTF16LE:
    result.push_back(0xFF);
    result.push_back(0xFE);
    for (wchar_t c : converted)
    {
      result.push_back(static_cast<BYTE>(c & 0xFF));
      result.push_back(static_cast<BYTE>((c >> 8) & 0xFF));
    }
    break;
  case Encoding::UTF16BE:
    result.push_back(0xFE);
    result.push_back(0xFF);
    for (wchar_t c : converted)
    {
      result.push_back(static_cast<BYTE>((c >> 8) & 0xFF));
      result.push_back(static_cast<BYTE>(c & 0xFF));
    }
    break;
  case Encoding::ANSI:
  {
    int len = WideCharToMultiByte(CP_ACP, 0, converted.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len > 1)
    {
      result.resize(len - 1);
      WideCharToMultiByte(CP_ACP, 0, converted.c_str(), -1,
                          reinterpret_cast<char *>(result.data()), len, nullptr, nullptr);
    }
    break;
  }
  }
  return result;
}

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
  std::wstring text = GetEditorText();
  DWORD start = 0, end = 0;
  SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
  int pos = static_cast<int>(start);
  int line = 1, col = 1;
  for (int i = 0; i < pos && i < static_cast<int>(text.size()); ++i)
  {
    if (text[i] == L'\n')
    {
      ++line;
      col = 1;
    }
    else if (text[i] != L'\r')
    {
      ++col;
    }
  }
  return {line, col};
}

void UpdateTitle()
{
  std::wstring filename = g_state.filePath.empty() ? L"Untitled" : PathFindFileNameW(g_state.filePath.c_str());
  std::wstring title = (g_state.modified ? L"*" : L"") + filename + L" - " + APP_NAME;
  SetWindowTextW(g_hwndMain, title.c_str());
}

void UpdateStatus()
{
  if (!g_state.showStatusBar)
  {
    ShowWindow(g_hwndStatus, SW_HIDE);
    return;
  }
  ShowWindow(g_hwndStatus, SW_SHOW);
  auto [line, col] = GetCursorPos();
  wchar_t buf[256];
  wsprintfW(buf, L" Ln %d, Col %d ", line, col);
  SendMessageW(g_hwndStatus, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(buf));
  SendMessageW(g_hwndStatus, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(GetEncodingName(g_state.encoding)));
  SendMessageW(g_hwndStatus, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(GetLineEndingName(g_state.lineEnding)));
  wsprintfW(buf, L" %d%% ", g_state.zoomLevel);
  SendMessageW(g_hwndStatus, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(buf));
}

void SetupStatusBarParts()
{
  RECT rc;
  GetClientRect(g_hwndMain, &rc);
  int w = rc.right;
  int parts[] = {w - 240, w - 180, w - 80, -1};
  SendMessageW(g_hwndStatus, SB_SETPARTS, 4, reinterpret_cast<LPARAM>(parts));
}

void ResizeControls()
{
  RECT rc;
  GetClientRect(g_hwndMain, &rc);
  int statusH = g_state.showStatusBar ? 22 : 0;
  MoveWindow(g_hwndEditor, 0, 0, rc.right, rc.bottom - statusH, TRUE);
  SendMessageW(g_hwndStatus, WM_SIZE, 0, 0);
  SetupStatusBarParts();
}

void ApplyFont()
{
  if (g_state.hFont)
  {
    DeleteObject(g_state.hFont);
    g_state.hFont = nullptr;
  }
  int size = g_state.fontSize * g_state.zoomLevel / 100;
  if (size < 8)
    size = 8;
  if (size > 500)
    size = 500;
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
  UpdateStatus();
}

void ApplyWordWrap()
{
  DWORD style = GetWindowLongW(g_hwndEditor, GWL_STYLE);
  if (g_state.wordWrap)
  {
    style &= ~WS_HSCROLL;
    style &= ~ES_AUTOHSCROLL;
  }
  else
  {
    style |= WS_HSCROLL;
    style |= ES_AUTOHSCROLL;
  }
  std::wstring text = GetEditorText();
  DWORD start = 0, end = 0;
  SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
  DestroyWindow(g_hwndEditor);
  style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN | ES_NOHIDESEL;
  if (!g_state.wordWrap)
  {
    style |= WS_HSCROLL | ES_AUTOHSCROLL;
  }
  g_hwndEditor = CreateWindowExW(0, L"EDIT", nullptr, style,
                                 0, 0, 100, 100, g_hwndMain, reinterpret_cast<HMENU>(IDC_EDITOR), GetModuleHandleW(nullptr), nullptr);
  ApplyFont();
  SetEditorText(text);
  SendMessageW(g_hwndEditor, EM_SETSEL, start, end);
  ResizeControls();
  SetFocus(g_hwndEditor);
}

bool ConfirmDiscard()
{
  if (!g_state.modified)
    return true;
  std::wstring filename = g_state.filePath.empty() ? L"Untitled" : PathFindFileNameW(g_state.filePath.c_str());
  std::wstring msg = L"Do you want to save changes to " + filename + L"?";
  int result = MessageBoxW(g_hwndMain, msg.c_str(), APP_NAME, MB_YESNOCANCEL | MB_ICONWARNING);
  if (result == IDYES)
  {
    FileSave();
    return true;
  }
  return result == IDNO;
}

void LoadFile(const std::wstring &path)
{
  HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    MessageBoxW(g_hwndMain, L"Cannot open file.", L"Error", MB_ICONERROR);
    return;
  }
  DWORD size = GetFileSize(hFile, nullptr);
  std::vector<BYTE> data(size);
  DWORD read = 0;
  ReadFile(hFile, data.data(), size, &read, nullptr);
  CloseHandle(hFile);
  auto [enc, le] = DetectEncoding(data);
  std::wstring text = DecodeText(data, enc);
  SetEditorText(text);
  g_state.filePath = path;
  g_state.encoding = enc;
  g_state.lineEnding = le;
  g_state.modified = false;
  UpdateTitle();
  UpdateStatus();
  AddRecentFile(path);
}

void AddRecentFile(const std::wstring &path)
{
  auto it = std::find(g_state.recentFiles.begin(), g_state.recentFiles.end(), path);
  if (it != g_state.recentFiles.end())
  {
    g_state.recentFiles.erase(it);
  }
  g_state.recentFiles.push_front(path);
  while (g_state.recentFiles.size() > MAX_RECENT_FILES)
  {
    g_state.recentFiles.pop_back();
  }
  UpdateRecentFilesMenu();
}

void UpdateRecentFilesMenu()
{
  HMENU hMenu = GetMenu(g_hwndMain);
  HMENU hFileMenu = GetSubMenu(hMenu, 0);
  static HMENU hRecentMenu = nullptr;
  if (hRecentMenu)
  {
    int count = GetMenuItemCount(hFileMenu);
    for (int i = 0; i < count; ++i)
    {
      if (GetSubMenu(hFileMenu, i) == hRecentMenu)
      {
        RemoveMenu(hFileMenu, static_cast<UINT>(i), MF_BYPOSITION);
        break;
      }
    }
    DestroyMenu(hRecentMenu);
    hRecentMenu = nullptr;
  }
  if (g_state.recentFiles.empty())
    return;
  hRecentMenu = CreatePopupMenu();
  int id = IDM_FILE_RECENT_BASE;
  for (const auto &file : g_state.recentFiles)
  {
    std::wstring display = PathFindFileNameW(file.c_str());
    AppendMenuW(hRecentMenu, MF_STRING, id++, display.c_str());
  }
  InsertMenuW(hFileMenu, 5, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hRecentMenu), L"Recent Files");
}

void SaveToPath(const std::wstring &path)
{
  std::wstring text = GetEditorText();
  std::vector<BYTE> data = EncodeText(text, g_state.encoding, g_state.lineEnding);
  HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    MessageBoxW(g_hwndMain, L"Cannot save file.", L"Error", MB_ICONERROR);
    return;
  }
  DWORD written = 0;
  WriteFile(hFile, data.data(), static_cast<DWORD>(data.size()), &written, nullptr);
  CloseHandle(hFile);
  g_state.filePath = path;
  g_state.modified = false;
  UpdateTitle();
  AddRecentFile(path);
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
  OPENFILENAMEW ofn = {sizeof(ofn)};
  ofn.hwndOwner = g_hwndMain;
  ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  if (GetOpenFileNameW(&ofn))
  {
    LoadFile(path);
  }
}

void FileSave()
{
  if (g_state.filePath.empty())
  {
    FileSaveAs();
  }
  else
  {
    SaveToPath(g_state.filePath);
  }
}

void FileSaveAs()
{
  wchar_t path[MAX_PATH] = {0};
  OPENFILENAMEW ofn = {sizeof(ofn)};
  ofn.hwndOwner = g_hwndMain;
  ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.lpstrDefExt = L"txt";
  ofn.Flags = OFN_OVERWRITEPROMPT;
  if (GetSaveFileNameW(&ofn))
  {
    SaveToPath(path);
  }
}

void FilePrint()
{
  std::wstring text = GetEditorText();
  PRINTDLGW pd = {sizeof(pd)};
  pd.hwndOwner = g_hwndMain;
  pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;
  if (!PrintDlgW(&pd))
    return;
  HDC hDC = pd.hDC;
  DOCINFOW di = {sizeof(di)};
  std::wstring docName = g_state.filePath.empty() ? L"Untitled" : PathFindFileNameW(g_state.filePath.c_str());
  di.lpszDocName = docName.c_str();
  if (StartDocW(hDC, &di) > 0)
  {
    int pageWidth = GetDeviceCaps(hDC, HORZRES);
    int pageHeight = GetDeviceCaps(hDC, VERTRES);
    int marginX = pageWidth / 10;
    int marginY = pageHeight / 10;
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
      {
        line += text[i];
      }
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
  g_pageSetup.hwndOwner = g_hwndMain;
  g_pageSetup.Flags = PSD_MARGINS | PSD_INHUNDREDTHSOFMILLIMETERS;
  PageSetupDlgW(&g_pageSetup);
}

void EditUndo()
{
  SendMessageW(g_hwndEditor, EM_UNDO, 0, 0);
}

void EditRedo()
{
  SendMessageW(g_hwndEditor, EM_UNDO, 0, 0);
}

void EditCut()
{
  SendMessageW(g_hwndEditor, WM_CUT, 0, 0);
}

void EditCopy()
{
  SendMessageW(g_hwndEditor, WM_COPY, 0, 0);
}

void EditPaste()
{
  SendMessageW(g_hwndEditor, WM_PASTE, 0, 0);
}

void EditDelete()
{
  SendMessageW(g_hwndEditor, WM_CLEAR, 0, 0);
}

void DoFind(bool forward)
{
  if (g_state.findText.empty())
    return;
  std::wstring text = GetEditorText();
  DWORD start = 0, end = 0;
  SendMessageW(g_hwndEditor, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
  std::transform(text.begin(), text.end(), text.begin(), towlower);
  std::wstring findLower = g_state.findText;
  std::transform(findLower.begin(), findLower.end(), findLower.begin(), towlower);
  size_t pos = std::string::npos;
  if (forward)
  {
    pos = text.find(findLower, end);
    if (pos == std::string::npos)
      pos = text.find(findLower);
  }
  else
  {
    if (start > 0)
      pos = text.rfind(findLower, start - 1);
    if (pos == std::string::npos)
      pos = text.rfind(findLower);
  }
  if (pos != std::string::npos)
  {
    SendMessageW(g_hwndEditor, EM_SETSEL, pos, pos + g_state.findText.size());
    SendMessageW(g_hwndEditor, EM_SCROLLCARET, 0, 0);
  }
  else
  {
    MessageBoxW(g_hwndMain, (L"Cannot find \"" + g_state.findText + L"\"").c_str(), APP_NAME, MB_ICONINFORMATION);
  }
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
        {
          SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(g_state.replaceText.c_str()));
        }
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
      size_t pos = 0;
      std::wstring newText;
      size_t lastPos = 0;
      std::wstring lower = text;
      std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
      while ((pos = lower.find(findLower, lastPos)) != std::string::npos)
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
    {
      SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }
    SetWindowLongPtrW(g_hwndFindDlg, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FindDlgProc));
    InvalidateRect(g_hwndFindDlg, nullptr, FALSE);
    UpdateWindow(g_hwndFindDlg);
  }
}

void EditFindNext()
{
  if (!g_state.findText.empty())
  {
    DoFind(true);
  }
}

void EditFindPrev()
{
  if (!g_state.findText.empty())
  {
    DoFind(false);
  }
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
    {
      SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }
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
        switch (msg) {
            case WM_INITDIALOG:
                SetWindowTextW(hDlg, L"Go To Line");
                hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | ES_NUMBER, 80, 15, 120, 22, hDlg, reinterpret_cast<HMENU>(1001), nullptr, nullptr);
                CreateWindowExW(0, L"STATIC", L"Line number:", WS_CHILD | WS_VISIBLE, 10, 18, 70, 20, hDlg, nullptr, nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 60, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 140, 50, 70, 26, hDlg, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
                {
                    HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                    for (HWND h = GetWindow(hDlg, GW_CHILD); h; h = GetWindow(h, GW_HWNDNEXT)) {
                        SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
                    }
                }
                SetFocus(hEdit);
                return FALSE;
            case WM_COMMAND:
                if (LOWORD(wParam) == IDOK) {
                    wchar_t buf[32];
                    GetWindowTextW(hEdit, buf, 32);
                    int line = _wtoi(buf);
                    if (line > 0) {
                        std::wstring text = GetEditorText();
                        int current = 1;
                        size_t pos = 0;
                        for (size_t i = 0; i < text.size() && current < line; ++i) {
                            if (text[i] == L'\n') {
                                ++current;
                                pos = i + 1;
                            }
                        }
                        if (current < line) pos = text.size();
                        SendMessageW(g_hwndEditor, EM_SETSEL, pos, pos);
                        SendMessageW(g_hwndEditor, EM_SCROLLCARET, 0, 0);
                        SetFocus(g_hwndEditor);
                    }
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                } else if (LOWORD(wParam) == IDCANCEL) {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
                break;
            case WM_CLOSE:
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
        }
        return FALSE; });
}

void EditSelectAll()
{
  SendMessageW(g_hwndEditor, EM_SETSEL, 0, -1);
}

void EditTimeDate()
{
  SYSTEMTIME st;
  GetLocalTime(&st);
  wchar_t buf[64];
  wsprintfW(buf, L"%02d:%02d %s %02d/%02d/%04d",
            st.wHour % 12 == 0 ? 12 : st.wHour % 12,
            st.wMinute,
            st.wHour >= 12 ? L"PM" : L"AM",
            st.wMonth, st.wDay, st.wYear);
  SendMessageW(g_hwndEditor, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(buf));
}

void FormatWordWrap()
{
  g_state.wordWrap = !g_state.wordWrap;
  CheckMenuItem(GetMenu(g_hwndMain), IDM_FORMAT_WORDWRAP, g_state.wordWrap ? MF_CHECKED : MF_UNCHECKED);
  ApplyWordWrap();
}

void FormatFont()
{
  LOGFONTW lf = {0};
  if (g_state.hFont)
  {
    GetObjectW(g_state.hFont, sizeof(LOGFONTW), &lf);
  }
  else
  {
    HDC hdc = GetDC(g_hwndMain);
    lf.lfHeight = -MulDiv(g_state.fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(g_hwndMain, hdc);
    wcscpy_s(lf.lfFaceName, g_state.fontName.c_str());
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  }
  CHOOSEFONTW cf = {sizeof(cf)};
  cf.hwndOwner = g_hwndMain;
  cf.lpLogFont = &lf;
  cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST;
  if (ChooseFontW(&cf))
  {
    g_state.fontName = lf.lfFaceName;
    HDC hdc2 = GetDC(g_hwndMain);
    g_state.fontSize = MulDiv(-lf.lfHeight, 72, GetDeviceCaps(hdc2, LOGPIXELSY));
    ReleaseDC(g_hwndMain, hdc2);
    ApplyFont();
  }
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
      return;
    }
  }
}

void ViewZoomDefault()
{
  g_state.zoomLevel = ZOOM_DEFAULT;
  ApplyZoom();
}

void ViewStatusBar()
{
  g_state.showStatusBar = !g_state.showStatusBar;
  CheckMenuItem(GetMenu(g_hwndMain), IDM_VIEW_STATUSBAR, g_state.showStatusBar ? MF_CHECKED : MF_UNCHECKED);
  ResizeControls();
  UpdateStatus();
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
    {
      SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
      if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
        break;
      if (msg.hwnd == hOk && msg.message == WM_LBUTTONUP)
      {
        GetWindowTextW(hEdit, buf, 32);
        int val = _wtoi(buf);
        if (val < 10)
          val = 10;
        if (val > 100)
          val = 100;
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
  MessageBoxW(g_hwndMain, L"Legacy Notepad v1.0.0\n\nA fast, lightweight text editor.\n\nBuilt with C++ and Win32 API.", L"About Notepad", MB_ICONINFORMATION);
}

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

void ViewSelectBackground()
{
  wchar_t path[MAX_PATH] = {0};
  OPENFILENAMEW ofn = {sizeof(ofn)};
  ofn.hwndOwner = g_hwndMain;
  ofn.lpstrFilter = L"Image Files (*.png;*.jpg;*.jpeg;*.bmp;*.gif)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All Files (*.*)\0*.*\0";
  ofn.lpstrFile = path;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  if (GetOpenFileNameW(&ofn))
  {
    LoadBackgroundImage(path);
  }
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
    {
      SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }
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
      if (val < 0)
        val = 0;
      if (val > 100)
        val = 100;
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

void SetBackgroundPosition(BgPosition pos)
{
  g_state.background.position = pos;
  HMENU hMenu = GetMenu(g_hwndMain);
  HMENU hViewMenu = GetSubMenu(hMenu, 3);
  HMENU hBgMenu = GetSubMenu(hViewMenu, 6);
  HMENU hPosMenu = GetSubMenu(hBgMenu, 4);
  for (int i = 0; i < 13; ++i)
  {
    CheckMenuItem(hPosMenu, i, MF_BYPOSITION | MF_UNCHECKED);
  }
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
  Gdiplus::ColorMatrix colorMatrix = {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, opacity, 0.0f,
      0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
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
    {
      for (int x = 0; x < winW; x += imgW)
      {
        drawImage(x, y, imgW, imgH);
      }
    }
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
  HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
  FillRect(hdcMem, &rc, hBrush);
  DeleteObject(hBrush);
  PaintBackground(hdcMem, rc);
  SelectObject(hdcMem, hOldBmp);
  DeleteDC(hdcMem);
  ReleaseDC(hwnd, hdcScreen);
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
    if (g_state.background.enabled && g_bgImage)
    {
      if (g_bgBitmap)
      {
        DeleteObject(g_bgBitmap);
        g_bgBitmap = nullptr;
      }
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
    if (g_state.background.enabled && g_bgImage)
    {
      if (wParam == VK_BACK || wParam == VK_DELETE)
      {
        LRESULT result = CallWindowProcW(g_origEditorProc, hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, nullptr, TRUE);
        return result;
      }
    }
    break;
  }
  return CallWindowProcW(g_origEditorProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
  case WM_CREATE:
  {
    DragAcceptFiles(hwnd, TRUE);
    LoadLibraryW(L"Msftedit.dll");
    g_hwndEditor = CreateWindowExW(0, MSFTEDIT_CLASS, nullptr,
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
                                   0, 0, 100, 100, hwnd, reinterpret_cast<HMENU>(IDC_EDITOR), GetModuleHandleW(nullptr), nullptr);
    g_origEditorProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_hwndEditor, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditorSubclassProc)));
    g_hwndStatus = CreateWindowExW(0, STATUSCLASSNAMEW, nullptr,
                                   WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_STATUSBAR), GetModuleHandleW(nullptr), nullptr);
    SendMessageW(g_hwndEditor, EM_SETLIMITTEXT, 0, 0);
    SendMessageW(g_hwndEditor, EM_SETEVENTMASK, 0, ENM_CHANGE);
    ApplyFont();
    SetupStatusBarParts();
    UpdateTitle();
    UpdateStatus();
    SetFocus(g_hwndEditor);
    return 0;
  }
  case WM_DROPFILES:
  {
    HDROP hDrop = reinterpret_cast<HDROP>(wParam);
    wchar_t path[MAX_PATH];
    if (DragQueryFileW(hDrop, 0, path, MAX_PATH))
    {
      if (ConfirmDiscard())
      {
        LoadFile(path);
      }
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
      HDC hdc = reinterpret_cast<HDC>(wParam);
      SetBkMode(hdc, TRANSPARENT);
      return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
    }
    break;
  case WM_COMMAND:
  {
    WORD cmd = LOWORD(wParam);
    if (cmd >= IDM_FILE_RECENT_BASE && cmd < IDM_FILE_RECENT_BASE + MAX_RECENT_FILES)
    {
      int idx = cmd - IDM_FILE_RECENT_BASE;
      if (idx < static_cast<int>(g_state.recentFiles.size()))
      {
        if (ConfirmDiscard())
        {
          LoadFile(g_state.recentFiles[idx]);
        }
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
    case IDM_HELP_ABOUT:
      HelpAbout();
      break;
    }
    return 0;
  }
  case WM_NOTIFY:
  {
    NMHDR *pnmh = reinterpret_cast<NMHDR *>(lParam);
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
    {
      DestroyWindow(hwnd);
    }
    else
    {
      g_state.closing = false;
    }
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
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
  INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES};
  InitCommonControlsEx(&icc);
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
  ShowWindow(g_hwndMain, nCmdShow);
  UpdateWindow(g_hwndMain);
  if (lpCmdLine && lpCmdLine[0])
  {
    std::wstring path = lpCmdLine;
    if (path.front() == L'"' && path.back() == L'"')
    {
      path = path.substr(1, path.size() - 2);
    }
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