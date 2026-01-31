#include "lang.h"
#include "en.h"
#include "ja.h"
#include <windows.h>

static LangID g_currentLang = LangID::EN;
static const LangStrings *g_currentStrings = &g_langEN;

LangID LoadLanguageSetting()
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\LegacyNotepad", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD value = 0, size = sizeof(value);
        if (RegQueryValueExW(hKey, L"Language", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return static_cast<LangID>(value);
        }
        RegCloseKey(hKey);
    }
    return LangID::EN;
}

void SaveLanguageSetting()
{
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\LegacyNotepad", 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS)
    {
        DWORD value = static_cast<DWORD>(g_currentLang);
        RegSetValueExW(hKey, L"Language", 0, REG_DWORD, reinterpret_cast<const BYTE *>(&value), sizeof(value));
        RegCloseKey(hKey);
    }
}

void InitLanguage()
{
    LangID savedLang = LoadLanguageSetting();
    SetLanguage(savedLang);
}

void SetLanguage(LangID lang)
{
    g_currentLang = lang;
    
    switch (lang)
    {
    case LangID::JA:
        g_currentStrings = &g_langJA;
        break;
    case LangID::EN:
    default:
        g_currentStrings = &g_langEN;
        break;
    }
    
    SaveLanguageSetting();
}

LangID GetCurrentLanguage()
{
    return g_currentLang;
}

const LangStrings &GetLangStrings()
{
    return *g_currentStrings;
}

const std::wstring &GetString([[maybe_unused]] const std::wstring &key)
{
    static std::wstring empty;
    return empty;
}

