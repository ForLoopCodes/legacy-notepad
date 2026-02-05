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

  Settings management for persisting user preferences via Windows Registry.
  Handles font settings, always on top, window size and position storage and retrieval.
*/

#include "settings.h"
#include "core/globals.h"
#include "core/types.h"
#include <windows.h>

#define SETTINGS_KEY L"Software\\LegacyNotepad"
#define FONT_NAME_VALUE L"FontName"
#define FONT_SIZE_VALUE L"FontSize"
#define FONT_WEIGHT_VALUE L"FontWeight"
#define FONT_ITALIC_VALUE L"FontItalic"
#define FONT_UNDERLINE_VALUE L"FontUnderline"
#define ALWAYS_ON_TOP_VALUE L"AlwaysOnTop"
#define WINDOW_X_VALUE L"WindowX"
#define WINDOW_Y_VALUE L"WindowY"
#define WINDOW_WIDTH_VALUE L"WindowWidth"
#define WINDOW_HEIGHT_VALUE L"WindowHeight"
#define MIN_FONT_SIZE 8
#define MAX_FONT_SIZE 72

void LoadFontSettings()
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SETTINGS_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        wchar_t fontName[LF_FACESIZE] = {0};
        DWORD size = sizeof(fontName);
        if (RegQueryValueExW(hKey, FONT_NAME_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(fontName), &size) == ERROR_SUCCESS)
        {
            g_state.fontName = fontName;
        }

        DWORD fontSize = 0;
        size = sizeof(fontSize);
        if (RegQueryValueExW(hKey, FONT_SIZE_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&fontSize), &size) == ERROR_SUCCESS)
        {
            if (fontSize >= MIN_FONT_SIZE && fontSize <= MAX_FONT_SIZE)
            {
                g_state.fontSize = static_cast<int>(fontSize);
            }
        }

        DWORD weight = FW_NORMAL;
        size = sizeof(weight);
        if (RegQueryValueExW(hKey, FONT_WEIGHT_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&weight), &size) == ERROR_SUCCESS)
        {
            g_state.fontWeight = static_cast<int>(weight);
        }

        DWORD italic = 0;
        size = sizeof(italic);
        if (RegQueryValueExW(hKey, FONT_ITALIC_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&italic), &size) == ERROR_SUCCESS)
        {
            g_state.fontItalic = (italic != 0);
        }

        DWORD underline = 0;
        size = sizeof(underline);
        if (RegQueryValueExW(hKey, FONT_UNDERLINE_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&underline), &size) == ERROR_SUCCESS)
        {
            g_state.fontUnderline = (underline != 0);
        }

        DWORD top = 0;
        size = sizeof(top);
        if (RegQueryValueExW(hKey, ALWAYS_ON_TOP_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&top), &size) == ERROR_SUCCESS)
        {
            g_state.alwaysOnTop = (top != 0);
        }

        RegCloseKey(hKey);
    }
}

void SaveFontSettings()
{
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, SETTINGS_KEY, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, FONT_NAME_VALUE, 0, REG_SZ,
                       reinterpret_cast<const BYTE *>(g_state.fontName.c_str()),
                       static_cast<DWORD>((g_state.fontName.length() + 1) * sizeof(wchar_t)));

        DWORD fontSize = static_cast<DWORD>(g_state.fontSize);
        RegSetValueExW(hKey, FONT_SIZE_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&fontSize),
                       sizeof(fontSize));

        DWORD weight = static_cast<DWORD>(g_state.fontWeight);
        RegSetValueExW(hKey, FONT_WEIGHT_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&weight),
                       sizeof(weight));

        DWORD italic = g_state.fontItalic ? 1 : 0;
        RegSetValueExW(hKey, FONT_ITALIC_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&italic),
                       sizeof(italic));

        DWORD underline = g_state.fontUnderline ? 1 : 0;
        RegSetValueExW(hKey, FONT_UNDERLINE_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&underline),
                       sizeof(underline));

        DWORD top = g_state.alwaysOnTop ? 1 : 0;
        RegSetValueExW(hKey, ALWAYS_ON_TOP_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&top),
                       sizeof(top));

        RegCloseKey(hKey);
    }
}

void LoadWindowSettings()
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SETTINGS_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD x = 0;
        DWORD size = sizeof(x);
        if (RegQueryValueExW(hKey, WINDOW_X_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&x), &size) == ERROR_SUCCESS)
        {
            // Reinterpret DWORD as signed int to correctly handle negative coordinates
            g_state.windowX = *reinterpret_cast<int*>(&x);
        }

        DWORD y = 0;
        size = sizeof(y);
        if (RegQueryValueExW(hKey, WINDOW_Y_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&y), &size) == ERROR_SUCCESS)
        {
            // Reinterpret DWORD as signed int to correctly handle negative coordinates
            g_state.windowY = *reinterpret_cast<int*>(&y);
        }

        DWORD width = 0;
        size = sizeof(width);
        if (RegQueryValueExW(hKey, WINDOW_WIDTH_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&width), &size) == ERROR_SUCCESS)
        {
            if (width > 0)
            {
                g_state.windowWidth = static_cast<int>(width);
            }
        }

        DWORD height = 0;
        size = sizeof(height);
        if (RegQueryValueExW(hKey, WINDOW_HEIGHT_VALUE, nullptr, nullptr, reinterpret_cast<LPBYTE>(&height), &size) == ERROR_SUCCESS)
        {
            if (height > 0)
            {
                g_state.windowHeight = static_cast<int>(height);
            }
        }

        RegCloseKey(hKey);
    }

    // Validate that the window position is visible on at least one monitor
    RECT rc = {g_state.windowX, g_state.windowY, g_state.windowX + g_state.windowWidth, g_state.windowY + g_state.windowHeight};
    HMONITOR hMonitor = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
    if (!hMonitor)
    {
        // Window would be off-screen, reset to default
        g_state.windowX = CW_USEDEFAULT;
        g_state.windowY = CW_USEDEFAULT;
    }
}

void SaveWindowSettings()
{
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, SETTINGS_KEY, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
    {
        // Store coordinates as REG_DWORD, preserving bit pattern for negative values
        RegSetValueExW(hKey, WINDOW_X_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&g_state.windowX),
                       sizeof(g_state.windowX));

        RegSetValueExW(hKey, WINDOW_Y_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&g_state.windowY),
                       sizeof(g_state.windowY));

        RegSetValueExW(hKey, WINDOW_WIDTH_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&g_state.windowWidth),
                       sizeof(g_state.windowWidth));

        RegSetValueExW(hKey, WINDOW_HEIGHT_VALUE, 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&g_state.windowHeight),
                       sizeof(g_state.windowHeight));

        RegCloseKey(hKey);
    }
}
