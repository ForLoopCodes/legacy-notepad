#include "menu.h"
#include "core/globals.h"
#include "resource.h"
#include "lang/lang.h"

void UpdateMenuStrings()
{
    HMENU hMenu = GetMenu(g_hwndMain);
    if (!hMenu)
        return;

    const auto &lang = GetLangStrings();

    ModifyMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 0)), lang.menuFile.c_str());
    ModifyMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 1)), lang.menuEdit.c_str());
    ModifyMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 2)), lang.menuFormat.c_str());
    ModifyMenuW(hMenu, 3, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 3)), lang.menuView.c_str());
    ModifyMenuW(hMenu, 4, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 4)), lang.menuLanguage.c_str());
    ModifyMenuW(hMenu, 5, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(GetSubMenu(hMenu, 5)), lang.menuHelp.c_str());

    HMENU hFileMenu = GetSubMenu(hMenu, 0);
    if (hFileMenu)
    {
        ModifyMenuW(hFileMenu, 0, MF_BYPOSITION | MF_STRING, IDM_FILE_NEW, lang.menuNew.c_str());
        ModifyMenuW(hFileMenu, 1, MF_BYPOSITION | MF_STRING, IDM_FILE_OPEN, lang.menuOpen.c_str());
        ModifyMenuW(hFileMenu, 2, MF_BYPOSITION | MF_STRING, IDM_FILE_SAVE, lang.menuSave.c_str());
        ModifyMenuW(hFileMenu, 3, MF_BYPOSITION | MF_STRING, IDM_FILE_SAVEAS, lang.menuSaveAs.c_str());
        ModifyMenuW(hFileMenu, 5, MF_BYPOSITION | MF_STRING, IDM_FILE_PRINT, lang.menuPrint.c_str());
        ModifyMenuW(hFileMenu, 6, MF_BYPOSITION | MF_STRING, IDM_FILE_PAGESETUP, lang.menuPageSetup.c_str());
        ModifyMenuW(hFileMenu, 8, MF_BYPOSITION | MF_STRING, IDM_FILE_EXIT, lang.menuExit.c_str());
    }

    HMENU hEditMenu = GetSubMenu(hMenu, 1);
    if (hEditMenu)
    {
        ModifyMenuW(hEditMenu, 0, MF_BYPOSITION | MF_STRING, IDM_EDIT_UNDO, lang.menuUndo.c_str());
        ModifyMenuW(hEditMenu, 1, MF_BYPOSITION | MF_STRING, IDM_EDIT_REDO, lang.menuRedo.c_str());
        ModifyMenuW(hEditMenu, 3, MF_BYPOSITION | MF_STRING, IDM_EDIT_CUT, lang.menuCut.c_str());
        ModifyMenuW(hEditMenu, 4, MF_BYPOSITION | MF_STRING, IDM_EDIT_COPY, lang.menuCopy.c_str());
        ModifyMenuW(hEditMenu, 5, MF_BYPOSITION | MF_STRING, IDM_EDIT_PASTE, lang.menuPaste.c_str());
        ModifyMenuW(hEditMenu, 6, MF_BYPOSITION | MF_STRING, IDM_EDIT_DELETE, lang.menuDelete.c_str());
        ModifyMenuW(hEditMenu, 8, MF_BYPOSITION | MF_STRING, IDM_EDIT_FIND, lang.menuFind.c_str());
        ModifyMenuW(hEditMenu, 9, MF_BYPOSITION | MF_STRING, IDM_EDIT_FINDNEXT, lang.menuFindNext.c_str());
        ModifyMenuW(hEditMenu, 10, MF_BYPOSITION | MF_STRING, IDM_EDIT_FINDPREV, lang.menuFindPrev.c_str());
        ModifyMenuW(hEditMenu, 11, MF_BYPOSITION | MF_STRING, IDM_EDIT_REPLACE, lang.menuReplace.c_str());
        ModifyMenuW(hEditMenu, 12, MF_BYPOSITION | MF_STRING, IDM_EDIT_GOTO, lang.menuGoTo.c_str());
        ModifyMenuW(hEditMenu, 14, MF_BYPOSITION | MF_STRING, IDM_EDIT_SELECTALL, lang.menuSelectAll.c_str());
        ModifyMenuW(hEditMenu, 15, MF_BYPOSITION | MF_STRING, IDM_EDIT_TIMEDATE, lang.menuTimeDate.c_str());
    }

    HMENU hFormatMenu = GetSubMenu(hMenu, 2);
    if (hFormatMenu)
    {
        ModifyMenuW(hFormatMenu, 0, MF_BYPOSITION | MF_STRING, IDM_FORMAT_WORDWRAP, lang.menuWordWrap.c_str());
        ModifyMenuW(hFormatMenu, 1, MF_BYPOSITION | MF_STRING, IDM_FORMAT_FONT, lang.menuFont.c_str());
    }

    HMENU hViewMenu = GetSubMenu(hMenu, 3);
    if (hViewMenu)
    {
        ModifyMenuW(hViewMenu, 0, MF_BYPOSITION | MF_STRING, IDM_VIEW_ZOOMIN, lang.menuZoomIn.c_str());
        ModifyMenuW(hViewMenu, 1, MF_BYPOSITION | MF_STRING, IDM_VIEW_ZOOMOUT, lang.menuZoomOut.c_str());
        ModifyMenuW(hViewMenu, 2, MF_BYPOSITION | MF_STRING, IDM_VIEW_ZOOMDEFAULT, lang.menuZoomDefault.c_str());
        ModifyMenuW(hViewMenu, 4, MF_BYPOSITION | MF_STRING, IDM_VIEW_STATUSBAR, lang.menuStatusBar.c_str());
        ModifyMenuW(hViewMenu, 5, MF_BYPOSITION | MF_STRING, IDM_VIEW_DARKMODE, lang.menuDarkMode.c_str());

        HMENU hBgMenu = GetSubMenu(hViewMenu, 7);
        if (hBgMenu)
        {
            ModifyMenuW(hViewMenu, 7, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hBgMenu), lang.menuBackground.c_str());
            ModifyMenuW(hBgMenu, 0, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_SELECT, lang.menuBgSelect.c_str());
            ModifyMenuW(hBgMenu, 1, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_CLEAR, lang.menuBgClear.c_str());
            ModifyMenuW(hBgMenu, 2, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_OPACITY, lang.menuBgOpacity.c_str());

            HMENU hPosMenu = GetSubMenu(hBgMenu, 4);
            if (hPosMenu)
            {
                ModifyMenuW(hBgMenu, 4, MF_BYPOSITION | MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hPosMenu), lang.menuBgPosition.c_str());
                ModifyMenuW(hPosMenu, 0, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_TOPLEFT, lang.menuBgPosTopLeft.c_str());
                ModifyMenuW(hPosMenu, 1, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_TOPCENTER, lang.menuBgPosTopCenter.c_str());
                ModifyMenuW(hPosMenu, 2, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_TOPRIGHT, lang.menuBgPosTopRight.c_str());
                ModifyMenuW(hPosMenu, 4, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_CENTERLEFT, lang.menuBgPosCenterLeft.c_str());
                ModifyMenuW(hPosMenu, 5, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_CENTER, lang.menuBgPosCenter.c_str());
                ModifyMenuW(hPosMenu, 6, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_CENTERRIGHT, lang.menuBgPosCenterRight.c_str());
                ModifyMenuW(hPosMenu, 8, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_BOTTOMLEFT, lang.menuBgPosBottomLeft.c_str());
                ModifyMenuW(hPosMenu, 9, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_BOTTOMCENTER, lang.menuBgPosBottomCenter.c_str());
                ModifyMenuW(hPosMenu, 10, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_BOTTOMRIGHT, lang.menuBgPosBottomRight.c_str());
                ModifyMenuW(hPosMenu, 12, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_TILE, lang.menuBgPosTile.c_str());
                ModifyMenuW(hPosMenu, 13, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_STRETCH, lang.menuBgPosStretch.c_str());
                ModifyMenuW(hPosMenu, 14, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_FIT, lang.menuBgPosFit.c_str());
                ModifyMenuW(hPosMenu, 15, MF_BYPOSITION | MF_STRING, IDM_VIEW_BG_POS_FILL, lang.menuBgPosFill.c_str());
            }
        }

        ModifyMenuW(hViewMenu, 9, MF_BYPOSITION | MF_STRING, IDM_VIEW_TRANSPARENCY, lang.menuTransparency.c_str());
        ModifyMenuW(hViewMenu, 10, MF_BYPOSITION | MF_STRING, IDM_VIEW_ALWAYSONTOP, lang.menuAlwaysOnTop.c_str());
    }

    HMENU hLangMenu = GetSubMenu(hMenu, 4);
    if (hLangMenu)
    {
        ModifyMenuW(hLangMenu, 0, MF_BYPOSITION | MF_STRING, IDM_VIEW_LANG_EN, lang.menuLangEnglish.c_str());
        ModifyMenuW(hLangMenu, 1, MF_BYPOSITION | MF_STRING, IDM_VIEW_LANG_JA, lang.menuLangJapanese.c_str());
    }

    HMENU hHelpMenu = GetSubMenu(hMenu, 5);
    if (hHelpMenu)
    {
        ModifyMenuW(hHelpMenu, 0, MF_BYPOSITION | MF_STRING, IDM_HELP_CHECKUPDATES, lang.menuCheckUpdates.c_str());
        ModifyMenuW(hHelpMenu, 1, MF_BYPOSITION | MF_STRING, IDM_HELP_ABOUT, lang.menuAbout.c_str());
    }

    DrawMenuBar(g_hwndMain);
}

void UpdateLanguageMenu()
{
    HMENU hMenu = GetMenu(g_hwndMain);
    if (!hMenu)
        return;

    HMENU hLangMenu = GetSubMenu(hMenu, 4);
    if (!hLangMenu)
        return;

    LangID currentLang = GetCurrentLanguage();
    CheckMenuItem(hLangMenu, IDM_VIEW_LANG_EN, (currentLang == LangID::EN) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hLangMenu, IDM_VIEW_LANG_JA, (currentLang == LangID::JA) ? MF_CHECKED : MF_UNCHECKED);
}
