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

  Global variable definitions for the notepad application storing runtime state.
  Initializes all shared resources like window handles, GDI objects, and app state.
*/

#include "globals.h"

HWND g_hwndMain = nullptr;
HWND g_hwndEditor = nullptr;
HWND g_hwndStatus = nullptr;
HWND g_hwndFindDlg = nullptr;
HACCEL g_hAccel = nullptr;
AppState g_state;
WNDPROC g_origEditorProc = nullptr;
WNDPROC g_origStatusProc = nullptr;
ULONG_PTR g_gdiplusToken = 0;
Gdiplus::Image *g_bgImage = nullptr;
HBITMAP g_bgBitmap = nullptr;
int g_bgBitmapW = 0;
int g_bgBitmapH = 0;
HBRUSH g_hbrStatusDark = nullptr;
HBRUSH g_hbrMenuDark = nullptr;
PAGESETUPDLGW g_pageSetup = {};
std::wstring g_statusTexts[4];
HICON g_hCustomIcon = nullptr;
