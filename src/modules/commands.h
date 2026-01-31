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

#pragma once
#include <windows.h>

extern PAGESETUPDLGW g_pageSetup;

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
void EditSelectAll();
void EditTimeDate();
void FormatWordWrap();
void ViewZoomIn();
void ViewZoomOut();
void ViewZoomDefault();
void ViewStatusBar();
void HelpCheckUpdates();
