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
  Handles font name and font size storage and retrieval.
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

        RegCloseKey(hKey);
    }
}
