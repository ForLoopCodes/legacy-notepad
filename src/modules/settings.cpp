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
        
        RegCloseKey(hKey);
    }
}

void SaveFontSettings()
{
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, SETTINGS_KEY, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, FONT_NAME_VALUE, 0, REG_SZ, 
                       reinterpret_cast<const BYTE*>(g_state.fontName.c_str()), 
                       static_cast<DWORD>((g_state.fontName.length() + 1) * sizeof(wchar_t)));
        
        DWORD fontSize = static_cast<DWORD>(g_state.fontSize);
        RegSetValueExW(hKey, FONT_SIZE_VALUE, 0, REG_DWORD, 
                       reinterpret_cast<const BYTE*>(&fontSize), 
                       sizeof(fontSize));
        
        RegCloseKey(hKey);
    }
}
