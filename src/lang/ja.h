#pragma once

#include "lang.h"

inline LangStrings g_langJA = {
    // App
    L"メモ帳",
    L"無題",

    // Menu - File
    L"ファイル(&F)",
    L"新規(&N)\tCtrl+N",
    L"開く(&O)...\tCtrl+O",
    L"上書き保存(&S)\tCtrl+S",
    L"名前を付けて保存(&A)...\tCtrl+Shift+S",
    L"印刷(&P)...\tCtrl+P",
    L"ページ設定(&U)...",
    L"終了(&X)",
    L"最近使ったファイル",

    // Menu - Edit
    L"編集(&E)",
    L"元に戻す(&U)\tCtrl+Z",
    L"やり直し(&R)\tCtrl+Y",
    L"切り取り(&T)\tCtrl+X",
    L"コピー(&C)\tCtrl+C",
    L"貼り付け(&P)\tCtrl+V",
    L"削除(&L)\tDel",
    L"検索(&F)...\tCtrl+F",
    L"次を検索(&N)\tF3",
    L"前を検索(&V)\tShift+F3",
    L"置換(&H)...\tCtrl+H",
    L"ジャンプ(&G)...\tCtrl+G",
    L"すべて選択(&A)\tCtrl+A",
    L"日時(&D)\tF5",

    // Menu - Format
    L"書式(&O)",
    L"折り返し(&W)",
    L"フォント(&F)...",

    // Menu - View
    L"表示(&V)",
    L"拡大(&I)\tCtrl+Plus",
    L"縮小(&O)\tCtrl+Minus",
    L"既定のサイズに戻す(&R)\tCtrl+0",
    L"ステータスバー(&S)",
    L"ダークモード(&D)",
    L"背景(&B)",
    L"画像を選択(&S)...",
    L"背景をクリア(&C)",
    L"不透明度(&O)...",
    L"位置(&P)",
    L"左上",
    L"上中央",
    L"右上",
    L"中央左",
    L"中央",
    L"中央右",
    L"左下",
    L"下中央",
    L"右下",
    L"並べて表示",
    L"拡大",
    L"サイズに合わせる",
    L"フィル",
    L"ウィンドウの透明度(&T)...",
    L"常に最前面に表示(&T)",

    // Menu - Help
    L"ヘルプ(&H)",
    L"メモ帳について(&A)",

    // Menu - Language
    L"言語(&L)",
    L"英語(&E)",
    L"日本語(&J)",

    // Dialogs
    L"検索",
    L"検索と置換",
    L"行へジャンプ",
    L"ウィンドウの透明度",
    L"検索:",
    L"置換:",
    L"次を検索",
    L"置換",
    L"すべて置換",
    L"閉じる",
    L"行番号:",
    L"OK",
    L"キャンセル",
    L"不透明度 (10-100%):",

    // Messages
    L"「",
    L"次のファイルに変更を保存しますか: ",
    L"ファイルを開けません。",
    L"ファイルを保存できません。",
    L"エラー",
    L"Legacy Notepad v1.1.1\n\n高速で軽量なテキストエディタ。\n\nC++ Win32 API で構築。\n",//\nModify by 0x2o.net",

    // Status bar
    L" 行 ",
    L", 列 ",

    // Encoding names
    L"UTF-8",
    L"UTF-8 (BOM付き)",
    L"UTF-16 LE",
    L"UTF-16 BE",
    L"ANSI",

    // Line ending names
    L"Windows (CRLF)",
    L"Unix (LF)",
    L"Macintosh (CR)"
};
