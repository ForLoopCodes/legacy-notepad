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

  File I/O operations with multi-encoding support (UTF-8, UTF-16, ANSI).
  Handles BOM detection, line ending conversion, and recent files tracking.
*/
#include "file.h"
#include "core/globals.h"
#include "editor.h"
#include "ui.h"
#include "resource.h"
#include "lang/lang.h"
#include <shlwapi.h>
#include <algorithm>

const wchar_t *GetEncodingName(Encoding e)
{
    const auto &lang = GetLangStrings();
    switch (e)
    {
    case Encoding::UTF8:
        return lang.encodingUTF8.c_str();
    case Encoding::UTF8BOM:
        return lang.encodingUTF8BOM.c_str();
    case Encoding::UTF16LE:
        return lang.encodingUTF16LE.c_str();
    case Encoding::UTF16BE:
        return lang.encodingUTF16BE.c_str();
    case Encoding::ANSI:
        return lang.encodingANSI.c_str();
    }
    return L"";
}

const wchar_t *GetLineEndingName(LineEnding le)
{
    const auto &lang = GetLangStrings();
    switch (le)
    {
    case LineEnding::CRLF:
        return lang.lineEndingCRLF.c_str();
    case LineEnding::LF:
        return lang.lineEndingLF.c_str();
    case LineEnding::CR:
        return lang.lineEndingCR.c_str();
    }
    return L"";
}

std::pair<Encoding, LineEnding> DetectEncoding(const std::vector<BYTE> &data)
{
    Encoding enc = Encoding::UTF8;
    if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        enc = Encoding::UTF8BOM;
    else if (data.size() >= 2 && data[0] == 0xFF && data[1] == 0xFE)
        enc = Encoding::UTF16LE;
    else if (data.size() >= 2 && data[0] == 0xFE && data[1] == 0xFF)
        enc = Encoding::UTF16BE;
    else
    {
        int result = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                         reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size()), nullptr, 0);
        if (result == 0 && GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
            enc = Encoding::ANSI;
    }
    LineEnding le = LineEnding::CRLF;
    for (size_t i = 0; i < data.size(); ++i)
    {
        if (data[i] == '\r')
        {
            le = (i + 1 < data.size() && data[i + 1] == '\n') ? LineEnding::CRLF : LineEnding::CR;
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
        return std::wstring(wptr, (data.size() - skip) / 2);
    }
    case Encoding::UTF16BE:
    {
        skip = 2;
        std::wstring result;
        result.reserve((data.size() - skip) / 2);
        for (size_t i = skip; i + 1 < data.size(); i += 2)
            result += static_cast<wchar_t>((data[i] << 8) | data[i + 1]);
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
    converted.reserve(text.size() + text.size() / 10);
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
            converted += c;
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
        result.reserve(result.size() + converted.size() * 2);
        for (wchar_t c : converted)
        {
            result.push_back(static_cast<BYTE>(c & 0xFF));
            result.push_back(static_cast<BYTE>((c >> 8) & 0xFF));
        }
        break;
    case Encoding::UTF16BE:
        result.push_back(0xFE);
        result.push_back(0xFF);
        result.reserve(result.size() + converted.size() * 2);
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

void LoadFile(const std::wstring &path)
{
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        const auto &lang = GetLangStrings();
        MessageBoxW(g_hwndMain, lang.msgCannotOpenFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
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

void SaveToPath(const std::wstring &path)
{
    std::wstring text = GetEditorText();
    std::vector<BYTE> data = EncodeText(text, g_state.encoding, g_state.lineEnding);
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        const auto &lang = GetLangStrings();
        MessageBoxW(g_hwndMain, lang.msgCannotSaveFile.c_str(), lang.msgError.c_str(), MB_ICONERROR);
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

void AddRecentFile(const std::wstring &path)
{
    auto it = std::find(g_state.recentFiles.begin(), g_state.recentFiles.end(), path);
    if (it != g_state.recentFiles.end())
        g_state.recentFiles.erase(it);
    g_state.recentFiles.push_front(path);
    while (g_state.recentFiles.size() > MAX_RECENT_FILES)
        g_state.recentFiles.pop_back();
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
    const auto &lang = GetLangStrings();
    InsertMenuW(hFileMenu, 5, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(hRecentMenu), lang.menuRecentFiles.c_str());
}
