// RisohEditor.cpp --- RisohEditor
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Another free Win32 resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#include "RisohEditor.hpp"

#pragma comment(lib, "msimg32.lib")

//////////////////////////////////////////////////////////////////////////////
// constants

#ifndef INVALID_FILE_ATTRIBUTES
    #define INVALID_FILE_ATTRIBUTES     ((DWORD)-1)
#endif

#ifndef RT_MANIFEST
    #define RT_MANIFEST 24
#endif

#define TV_WIDTH        250     // default m_hTreeView width
#define BV_WIDTH        160     // default m_hBmpView width
#define BE_HEIGHT       90      // default m_hBinEdit height
#define CX_STATUS_PART  80      // status bar part width

#define MYWM_POSTSEARCH (WM_USER + 200)

MString GetLanguageStatement(WORD langid, BOOL bOldStyle);

static const DWORD s_nMaxCaptions = 10;
static const UINT s_nBackupMaxCount = 5;

//////////////////////////////////////////////////////////////////////////////

void GetStyleSelect(HWND hLst, std::vector<BYTE>& sel)
{
    for (size_t i = 0; i < sel.size(); ++i)
    {
        sel[i] = (ListBox_GetSel(hLst, (DWORD)i) > 0);
    }
}

void GetStyleSelect(std::vector<BYTE>& sel,
                    const ConstantsDB::TableType& table, DWORD dwValue)
{
    sel.resize(table.size());
    for (size_t i = 0; i < table.size(); ++i)
    {
        if ((dwValue & table[i].mask) == table[i].value)
            sel[i] = TRUE;
        else
            sel[i] = FALSE;
    }
}

DWORD AnalyseStyleDiff(
    DWORD dwValue, ConstantsDB::TableType& table,
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel)
{
    assert(old_sel.size() == new_sel.size());
    for (size_t i = 0; i < old_sel.size(); ++i)
    {
        if (old_sel[i] && !new_sel[i])
        {
            dwValue &= ~table[i].mask;
        }
        else if (!old_sel[i] && new_sel[i])
        {
            dwValue &= ~table[i].mask;
            dwValue |= table[i].value;
        }
    }
    return dwValue;
}

void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table)
{
    ListBox_ResetContent(hLst);

    if (table.size())
    {
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            ListBox_AddString(hLst, it->name.c_str());
        }
    }
}

static int CALLBACK
EnumFontFamProc(
    ENUMLOGFONT *lpelf,
    NEWTEXTMETRIC *lpntm,
    int FontType,
    LPARAM lParam)
{
    HWND hCmb = HWND(lParam);
    if (lpelf->elfLogFont.lfFaceName[0] != TEXT('@'))
        ComboBox_AddString(hCmb, lpelf->elfLogFont.lfFaceName);
    return TRUE;
}

void InitFontComboBox(HWND hCmb)
{
    HDC hDC = CreateCompatibleDC(NULL);
    EnumFontFamilies(hDC, NULL, (FONTENUMPROC)EnumFontFamProc, (LPARAM)hCmb);
    DeleteDC(hDC);
}

typedef struct CharSetInfo
{
    BYTE CharSet;
    LPCTSTR name;
} CharSetInfo;

static const CharSetInfo s_charset_entries[] =
{
    { ANSI_CHARSET, TEXT("ANSI_CHARSET") },
    { DEFAULT_CHARSET, TEXT("DEFAULT_CHARSET") },
    { SYMBOL_CHARSET, TEXT("SYMBOL_CHARSET") },
    { SHIFTJIS_CHARSET, TEXT("SHIFTJIS_CHARSET") },
    { HANGEUL_CHARSET, TEXT("HANGEUL_CHARSET") },
    { HANGUL_CHARSET, TEXT("HANGUL_CHARSET") },
    { GB2312_CHARSET, TEXT("GB2312_CHARSET") },
    { CHINESEBIG5_CHARSET, TEXT("CHINESEBIG5_CHARSET") },
    { OEM_CHARSET, TEXT("OEM_CHARSET") },
    { JOHAB_CHARSET, TEXT("JOHAB_CHARSET") },
    { HEBREW_CHARSET, TEXT("HEBREW_CHARSET") },
    { ARABIC_CHARSET, TEXT("ARABIC_CHARSET") },
    { GREEK_CHARSET, TEXT("GREEK_CHARSET") },
    { TURKISH_CHARSET, TEXT("TURKISH_CHARSET") },
    { VIETNAMESE_CHARSET, TEXT("VIETNAMESE_CHARSET") },
    { THAI_CHARSET, TEXT("THAI_CHARSET") },
    { EASTEUROPE_CHARSET, TEXT("EASTEUROPE_CHARSET") },
    { RUSSIAN_CHARSET, TEXT("RUSSIAN_CHARSET") },
    { MAC_CHARSET, TEXT("MAC_CHARSET") },
    { BALTIC_CHARSET, TEXT("BALTIC_CHARSET") },
};

void InitCharSetComboBox(HWND hCmb, BYTE CharSet)
{
    ComboBox_ResetContent(hCmb);

    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        ComboBox_AddString(hCmb, s_charset_entries[i].name);
    }
    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        ComboBox_SetItemData(hCmb, i, s_charset_entries[i].CharSet);
    }
    ComboBox_SetCurSel(hCmb, 1);
    for (UINT i = 0; i < _countof(s_charset_entries); ++i)
    {
        if (s_charset_entries[i].CharSet == CharSet)
        {
            ComboBox_SetCurSel(hCmb, i);
            break;
        }
    }
}

BYTE GetCharSetFromComboBox(HWND hCmb)
{
    INT i = ComboBox_GetCurSel(hCmb);
    if (i == CB_ERR)
        return DEFAULT_CHARSET;
    if (i < _countof(s_charset_entries))
        return s_charset_entries[i].CharSet;
    return DEFAULT_CHARSET;
}

void InitCaptionComboBox(HWND hCmb, RisohSettings& settings, LPCTSTR pszCaption)
{
    ComboBox_ResetContent(hCmb);
    for (size_t i = 0; i < settings.captions.size(); ++i)
    {
        ComboBox_AddString(hCmb, settings.captions[i].c_str());
    }
    ComboBox_SetText(hCmb, pszCaption);
}

void InitClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszClass)
{
    ComboBox_ResetContent(hCmb);

    ConstantsDB::TableType table;
    table = db.GetTable(TEXT("CONTROL.CLASSES"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == pszClass)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

void InitWndClassComboBox(HWND hCmb, ConstantsDB& db, LPCTSTR pszWndClass)
{
    ComboBox_ResetContent(hCmb);

    ConstantsDB::TableType table;
    table = db.GetTable(TEXT("CONTROL.CLASSES"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        if (it->value > 2)
            continue;

        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == pszWndClass)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

void InitCtrlIDComboBox(HWND hCmb, ConstantsDB& db)
{
    ConstantsDB::TableType table;
    table = db.GetTable(TEXT("CTRLID"));

    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        ComboBox_AddString(hCmb, it->name.c_str());
    }

    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_CONTROL].name;
    if (prefix.size())
    {
        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            ComboBox_AddString(hCmb, it->name.c_str());
        }
    }
    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    prefix = table[IDTYPE_COMMAND].name;
    if (prefix.size())
    {
        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            ComboBox_AddString(hCmb, it->name.c_str());
        }
    }
    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    prefix = table[IDTYPE_NEWCOMMAND].name;
    if (prefix.size())
    {
        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            ComboBox_AddString(hCmb, it->name.c_str());
        }
    }
}

void ReplaceResTypeString(MString& str, bool bRevert = false)
{
    if (bRevert)
    {
        if (str == L"Icon.ID")
            str = L"RT_GROUP_ICON";
        else if (str == L"Cursor.ID")
            str = L"RT_GROUP_CURSOR";
        else if (str == L"Accel.ID")
            str = L"RT_ACCELERATOR";
        else if (str == L"AniCursor.ID")
            str = L"RT_ANICURSOR";
        else if (str == L"AniIcon.ID")
            str = L"RT_ANIICON";
        else if (str == L"Dialog.ID")
            str = L"RT_DIALOG";
        else if (str == L"Menu.ID")
            str = L"RT_MENU";
        else if (str == L"Bitmap.ID")
            str = L"RT_BITMAP";
        else if (str == L"RCData.ID")
            str = L"RT_RCDATA";
    }
    else
    {
        if (str == L"RT_GROUP_CURSOR")
            str = L"Cursor.ID";
        else if (str == L"RT_GROUP_ICON")
            str = L"Icon.ID";
        else if (str == L"RT_ACCELERATOR")
            str = L"Accel.ID";
        else if (str == L"RT_ANICURSOR")
            str = L"AniCursor.ID";
        else if (str == L"RT_ANIICON")
            str = L"AniIcon.ID";
        else if (str == L"RT_DIALOG")
            str = L"Dialog.ID";
        else if (str == L"RT_MENU")
            str = L"Menu.ID";
        else if (str == L"RT_BITMAP")
            str = L"Bitmap.ID";
        else if (str == L"RT_RCDATA")
            str = L"RCData.ID";
    }
}

MString
GetEntityIDText(ResEntries& entries, RisohSettings& settings,
                ConstantsDB& db,
                const MString& name, INT nIDTYPE_, BOOL bFlag)
{
    MIdOrString type;
    switch (nIDTYPE_)
    {
    case IDTYPE_CURSOR:     type = RT_GROUP_CURSOR; break;
    case IDTYPE_BITMAP:     type = RT_BITMAP; break;
    case IDTYPE_MENU:       type = RT_MENU; break;
    case IDTYPE_DIALOG:     type = RT_DIALOG; break;
    case IDTYPE_ACCEL:      type = RT_ACCELERATOR; break;
    case IDTYPE_ICON:       type = RT_GROUP_ICON; break;
    case IDTYPE_ANICURSOR:  type = RT_ANICURSOR; break;
    case IDTYPE_ANIICON:    type = RT_ANIICON; break;
    case IDTYPE_HTML:       type = RT_HTML; break;
    case IDTYPE_RESOURCE:   type.clear(); break;
    case IDTYPE_RCDATA:     type = RT_RCDATA; break;
    default:
        return L"";
    }

    WORD wName = WORD(db.GetResIDValue(name));
    MIdOrString name_or_id(wName);
    if (wName == 0)
    {
        id_map_type::iterator it;
        MStringA strA = MTextToAnsi(CP_ACP, name).c_str();
        it = settings.id_map.find(strA);
        if (it != settings.id_map.end())
        {
            MStringA strA = it->second;
            if (strA[0] == 'L')
                strA = strA.substr(1);
            mstr_unquote(strA);
            CharUpperA(&strA[0]);
            name_or_id.m_str = MAnsiToWide(CP_ACP, strA).c_str();
        }
    }

    ResEntries found;
    Res_Search(found, entries, type, name_or_id, 0xFFFF);

    if (nIDTYPE_ == IDTYPE_RESOURCE)
    {
        for (size_t i = found.size(); i > 0; )
        {
            --i;

            BOOL bErase = FALSE;
            MIdOrString type = found[i].type;
            if (type == RT_GROUP_CURSOR || type == RT_BITMAP ||
                type == RT_DIALOG || type == RT_GROUP_ICON ||
                type == RT_RCDATA)
            {
                bErase = TRUE;
            }
            if (!Res_IsEntityType(type))
            {
                bErase = TRUE;
            }

            if (bErase)
            {
                found.erase(found.begin() + i);
            }
        }
    }

    MString ret;
    if (found.size())
    {
        for (size_t i = 0; i < found.size(); ++i)
        {
            MString res_name;
            if (found[i].type.is_int())
            {
                res_name = db.GetName(L"RESOURCE", found[i].type.m_id);
                if (res_name.empty())
                {
                    res_name = mstr_dec(found[i].type.m_id);
                }
                ReplaceResTypeString(res_name, false);
            }
            else
            {
                res_name = found[i].type.str();
            }
            if (res_name.size())
            {
                if (ret.find(L"[" + res_name + L"]") == MString::npos)
                {
                    ret += L"[";
                    ret += res_name;
                    ret += L"]";
                }
            }
        }
    }
    else if (bFlag)
    {
        for (size_t i = 0; i < found.size(); ++i)
        {
            MString res_name = db.GetName(L"RESOURCE", type.m_id);
            if (res_name.empty())
            {
                res_name = mstr_dec(type.m_id);
            }
            if (res_name.size())
            {
                if (ret.find(L"[" + res_name + L"]") == MString::npos)
                {
                    ret += L"[";
                    ret += res_name;
                    ret += L"]";
                }
            }
        }
    }
    mstr_replace_all(ret, L"][", L"/");
    mstr_replace_all(ret, L"[", L"");
    mstr_replace_all(ret, L"]", L"");
    return ret;
}

void InitResNameComboBox(HWND hCmb, ConstantsDB& db, MIdOrString id, INT nIDTYPE_)
{
    SetWindowTextW(hCmb, id.c_str());

    if (!db.AreMacroIDShown())
        return;

    INT k = -1;
    MStringW prefix;
    ConstantsDB::TableType table;
    if (nIDTYPE_ != IDTYPE_UNKNOWN)
    {
        table = db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[nIDTYPE_].name;
        if (prefix.empty())
            return;

        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            INT i = ComboBox_AddString(hCmb, it->name.c_str());
            if (it->value == id.m_id)
            {
                k = i;
                ComboBox_SetCurSel(hCmb, i);
                SetWindowTextW(hCmb, it->name.c_str());
            }
        }
    }

    if (k == -1 &&
        nIDTYPE_ != IDTYPE_RESOURCE && db.IsEntityIDType(nIDTYPE_))
    {
        table = db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[IDTYPE_RESOURCE].name;
        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            INT i = ComboBox_AddString(hCmb, it->name.c_str());
            if (it->value == id.m_id)
            {
                ComboBox_SetCurSel(hCmb, i);
                SetWindowTextW(hCmb, it->name.c_str());
            }
        }
    }
}

void InitResNameComboBox(HWND hCmb, ConstantsDB& db, MIdOrString id, INT nIDTYPE_1, INT nIDTYPE_2)
{
    SetWindowTextW(hCmb, id.c_str());

    if (!db.AreMacroIDShown())
        return;

    INT k = -1;
    MStringW prefix;
    ConstantsDB::TableType table;
    if (nIDTYPE_1 != IDTYPE_UNKNOWN)
    {
        table = db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[nIDTYPE_1].name;
        if (prefix.empty())
            return;

        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            INT i = ComboBox_AddString(hCmb, it->name.c_str());
            if (it->value == id.m_id)
            {
                k = i;
                ComboBox_SetCurSel(hCmb, i);
                SetWindowTextW(hCmb, it->name.c_str());
            }
        }
    }
    if (nIDTYPE_2 != IDTYPE_UNKNOWN)
    {
        table = db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[nIDTYPE_2].name;
        if (prefix.empty())
            return;

        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            INT i = ComboBox_AddString(hCmb, it->name.c_str());
            if (it->value == id.m_id)
            {
                k = i;
                ComboBox_SetCurSel(hCmb, i);
                SetWindowTextW(hCmb, it->name.c_str());
            }
        }
    }

    if (k == -1 &&
        nIDTYPE_1 != IDTYPE_RESOURCE && db.IsEntityIDType(nIDTYPE_1))
    {
        table = db.GetTable(L"RESOURCE.ID.PREFIX");
        prefix = table[IDTYPE_RESOURCE].name;
        table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
        ConstantsDB::TableType::iterator it, end = table.end();
        for (it = table.begin(); it != end; ++it)
        {
            INT i = ComboBox_AddString(hCmb, it->name.c_str());
            if (it->value == id.m_id)
            {
                ComboBox_SetCurSel(hCmb, i);
                SetWindowTextW(hCmb, it->name.c_str());
            }
        }
    }
}

BOOL CheckCommand(ConstantsDB& db, MString strCommand)
{
    mstr_trim(strCommand);
    if (('0' <= strCommand[0] && strCommand[0] <= '9') ||
        strCommand[0] == '-' || strCommand[0] == '+')
    {
        return TRUE;
    }
    return db.HasResID(strCommand);
}

void InitStringComboBox(HWND hCmb, ConstantsDB& db, MString strString)
{
    SetWindowText(hCmb, strString.c_str());

    if (!db.AreMacroIDShown())
        return;

    ConstantsDB::TableType table;
    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_STRING].name;
    if (prefix.empty())
        return;

    table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == strString)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }

    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    prefix = table[IDTYPE_PROMPT].name;
    table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
    end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == strString)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

void InitMessageComboBox(HWND hCmb, ConstantsDB& db, MString strString)
{
    SetWindowText(hCmb, strString.c_str());

    if (!db.AreMacroIDShown())
        return;

    ConstantsDB::TableType table;
    table = db.GetTable(L"RESOURCE.ID.PREFIX");
    MStringW prefix = table[IDTYPE_MESSAGE].name;
    if (prefix.empty())
        return;

    table = db.GetTableByPrefix(L"RESOURCE.ID", prefix);
    ConstantsDB::TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        INT i = ComboBox_AddString(hCmb, it->name.c_str());
        if (it->name == strString)
        {
            ComboBox_SetCurSel(hCmb, i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// languages

struct LangEntry
{
    WORD LangID;
    MStringW str;

    bool operator<(const LangEntry& ent) const
    {
        return str < ent.str;
    }
};
std::vector<LangEntry> g_Langs;

//////////////////////////////////////////////////////////////////////////////
// useful global functions

BOOL GetPathOfShortcutDx(HWND hwnd, LPCWSTR pszLnkFile, LPWSTR pszPath)
{
    BOOL                bRes = FALSE;
    WIN32_FIND_DATAW    find;
    IShellLinkW*        pShellLink;
    IPersistFile*       pPersistFile;
    HRESULT             hRes;

    pszPath[0] = 0;
    hRes = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IShellLinkW, (void **)&pShellLink);
    if (SUCCEEDED(hRes))
    {
        hRes = pShellLink->QueryInterface(IID_IPersistFile,
                                          (void **)&pPersistFile);
        if (SUCCEEDED(hRes))
        {
            hRes = pPersistFile->Load(pszLnkFile, STGM_READ);
            if (SUCCEEDED(hRes))
            {
                pShellLink->Resolve(hwnd, SLR_NO_UI | SLR_UPDATE);

                hRes = pShellLink->GetPath(pszPath, MAX_PATH, &find, 0);
                if (SUCCEEDED(hRes) && 0 != pszPath[0])
                {
                    bRes = TRUE;
                }
            }
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    return bRes;
}

BOOL DumpBinaryFileDx(const WCHAR *filename, LPCVOID pv, DWORD size)
{
    using namespace std;
    FILE *fp;
    _wfopen_s(&fp, filename, L"wb");
    int n = (int)fwrite(pv, size, 1, fp);
    fclose(fp);
    return n == 1;
}

LPWSTR GetTempFileNameDx(LPCWSTR pszPrefix3Chars)
{
    static WCHAR TempFile[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    GetTempPathW(_countof(szPath), szPath);
    GetTempFileNameW(szPath, L"KRE", 0, TempFile);
    return TempFile;
}

//////////////////////////////////////////////////////////////////////////////
// specialized tool bar

TBBUTTON g_buttons0[] =
{
    { -1, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEST },
};

TBBUTTON g_buttons1[] =
{
    { -1, ID_TEXTEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEXTEDIT },
    { -1, 0, TBSTATE_ENABLED, BTNS_SEP | BTNS_AUTOSIZE, {0}, 0, 0 },
    { -1, ID_TEST, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEST },
};

TBBUTTON g_buttons2[] =
{
    { -1, ID_COMPILE, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_COMPILE },
    { -1, ID_CANCELEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_CANCELEDIT },
};

TBBUTTON g_buttons3[] =
{
    { -1, ID_ADDICON, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_ADDICON },
    { -1, ID_ADDCURSOR, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_ADDCURSOR },
    { -1, ID_ADDDIALOG, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_ADDDIALOG },
    { -1, ID_ADDMENU, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_ADDMENU },
    { -1, ID_ADDVERINFO, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_ADDVERINFO },
};

TBBUTTON g_buttons4[] =
{
    { -1, ID_GUIEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_GUIEDIT },
};

TBBUTTON g_buttons5[] =
{
    { -1, ID_TEXTEDIT, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, IDS_TEXTEDIT },
};

VOID ToolBar_StoreStrings(HWND hwnd, INT nCount, TBBUTTON *pButtons)
{
    for (INT i = 0; i < nCount; ++i)
    {
        if (pButtons[i].idCommand == 0 || (pButtons[i].fsStyle & BTNS_SEP))
            continue;

        INT_PTR id = pButtons[i].iString;
        LPWSTR psz = LoadStringDx(INT(id));
        id = SendMessageW(hwnd, TB_ADDSTRING, 0, (LPARAM)psz);
        pButtons[i].iString = id;
    }
}

//////////////////////////////////////////////////////////////////////////////

void ReplaceFullWithHalf(LPWSTR pszText)
{
    MStringW strFullWidth = LoadStringDx(IDS_FULLWIDTH);
    MStringW strHalfWidth = LoadStringDx(IDS_HALFWIDTH);

    for (DWORD i = 0; pszText[i]; ++i)
    {
        size_t k = strFullWidth.find(pszText[i]);
        if (k != MStringW::npos)
        {
            pszText[i] = strHalfWidth[k];
        }
    }
}

void ReplaceFullWithHalf(MStringW& strText)
{
    ReplaceFullWithHalf(&strText[0]);
}

void InitLangComboBox(HWND hCmb3, LANGID langid)
{
    for (size_t i = 0; i < g_Langs.size(); ++i)
    {
        WCHAR sz[MAX_PATH];
        StringCchPrintfW(sz, _countof(sz), L"%s (%u)", g_Langs[i].str.c_str(), g_Langs[i].LangID);
        INT k = ComboBox_AddString(hCmb3, sz);
        if (langid == g_Langs[i].LangID)
        {
            ComboBox_SetCurSel(hCmb3, k);
        }
    }
}

void InitLangListView(HWND hLst1, LPCTSTR pszText)
{
    ListView_DeleteAllItems(hLst1);

    WCHAR sz1[64], sz2[64];
    LV_ITEM item;
    INT iItem = 0;
    for (size_t i = 0; i < g_Langs.size(); ++i)
    {
        StringCchPrintfW(sz1, _countof(sz1), L"%s", g_Langs[i].str.c_str());
        StringCchPrintfW(sz2, _countof(sz2), L"%u", g_Langs[i].LangID);

        if (pszText)
        {
            MString str = sz1;
            if (str.find(pszText) == MString::npos)
            {
                str = sz2;
                if (str.find(pszText) == MString::npos)
                    continue;
            }
        }

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 0;
        item.pszText = sz1;
        ListView_InsertItem(hLst1, &item);

        ZeroMemory(&item, sizeof(item));
        item.iItem = iItem;
        item.mask = LVIF_TEXT;
        item.iSubItem = 1;
        item.pszText = sz2;
        ListView_SetItem(hLst1, &item);

        ++iItem;
    }
}

BOOL CheckTypeComboBox(HWND hCmb1, MIdOrString& type)
{
    WCHAR szType[MAX_PATH];
    GetWindowTextW(hCmb1, szType, _countof(szType));
    ReplaceFullWithHalf(szType);
    MStringW str = szType;
    mstr_trim(str);
    lstrcpynW(szType, str.c_str(), _countof(szType));

    if (szType[0] == 0)
    {
        ComboBox_SetEditSel(hCmb1, 0, -1);
        SetFocus(hCmb1);
        MessageBoxW(GetParent(hCmb1), LoadStringDx(IDS_ENTERTYPE),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (mchr_is_digit(szType[0]) || szType[0] == L'-' || szType[0] == L'+')
    {
        type = WORD(mstr_parse_int(szType));
    }
    else
    {
        MStringW str = szType;
        size_t i = str.rfind(L'('); // ')'
        if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
        {
            type = WORD(mstr_parse_int(&str[i + 1]));
        }
        else
        {
            type = szType;
        }
    }

    return TRUE;
}

BOOL CheckNameComboBox(ConstantsDB& db, HWND hCmb2, MIdOrString& name)
{
    WCHAR szName[MAX_PATH];
    GetWindowTextW(hCmb2, szName, _countof(szName));
    MStringW str = szName;
    ReplaceFullWithHalf(str);
    mstr_trim(str);
    lstrcpynW(szName, str.c_str(), _countof(szName));
    if (szName[0] == 0)
    {
        ComboBox_SetEditSel(hCmb2, 0, -1);
        SetFocus(hCmb2);
        MessageBoxW(GetParent(hCmb2), LoadStringDx(IDS_ENTERNAME),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (mchr_is_digit(szName[0]) || szName[0] == L'-' || szName[0] == L'+')
    {
        name = WORD(mstr_parse_int(szName));
    }
    else
    {
        if (db.HasResID(szName))
            name = (WORD)db.GetResIDValue(szName);
        else
            name = szName;
    }
    return TRUE;
}

BOOL CheckLangComboBox(HWND hCmb3, WORD& lang)
{
    WCHAR szLang[MAX_PATH];
    GetWindowTextW(hCmb3, szLang, _countof(szLang));
    MStringW str = szLang;
    mstr_trim(str);
    lstrcpynW(szLang, str.c_str(), _countof(szLang));

    if (szLang[0] == 0)
    {
        ComboBox_SetEditSel(hCmb3, 0, -1);
        SetFocus(hCmb3);
        MessageBoxW(GetParent(hCmb3), LoadStringDx(IDS_ENTERLANG),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    else if (mchr_is_digit(szLang[0]) || szLang[0] == L'-' || szLang[0] == L'+')
    {
        lang = WORD(mstr_parse_int(szLang));
    }
    else
    {
        INT i = ComboBox_GetCurSel(hCmb3);
        if (i == CB_ERR || i >= INT(g_Langs.size()))
        {
            i = ComboBox_FindStringExact(hCmb3, -1, szLang);
            if (i == CB_ERR || i >= INT(g_Langs.size()))
            {
                ComboBox_SetEditSel(hCmb3, 0, -1);
                SetFocus(hCmb3);
                MessageBoxW(GetParent(hCmb3), LoadStringDx(IDS_ENTERLANG),
                            NULL, MB_ICONERROR);
                return FALSE;
            }
        }
        lang = g_Langs[i].LangID;
    }

    return TRUE;
}

BOOL Edt1_CheckFile(HWND hEdt1, MStringW& file)
{
    WCHAR szFile[MAX_PATH];
    GetWindowTextW(hEdt1, szFile, _countof(szFile));
    MStringW str = szFile;
    mstr_trim(str);
    lstrcpynW(szFile, str.c_str(), _countof(szFile));
    if (::GetFileAttributesW(szFile) == INVALID_FILE_ATTRIBUTES)
    {
        Edit_SetSel(hEdt1, 0, -1);
        SetFocus(hEdt1);
        MessageBoxW(GetParent(hEdt1), LoadStringDx(IDS_FILENOTFOUND),
                    NULL, MB_ICONERROR);
        return FALSE;
    }
    file = szFile;
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

MStringW DumpDataAsString(const std::vector<BYTE>& data)
{
    MStringW ret;
    WCHAR sz[64];
    DWORD addr, size = DWORD(data.size());

    if (data.empty())
    {
        return ret;
    }

    ret.reserve(data.size() * 3);   // for speed

    ret +=
        L"+ADDRESS  +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F  0123456789ABCDEF\r\n"
        L"--------  -----------------------  -----------------------  ----------------\r\n";

    for (addr = 0; ; ++addr)
    {
        if ((addr & 0xF) == 0)
        {
            StringCchPrintfW(sz, _countof(sz), L"%08lX  ", addr);
            ret += sz;

            bool flag = false;
            for (DWORD i = 0; i < 16; ++i)
            {
                if (i == 8)
                    ret += L' ';
                DWORD offset = addr + i;
                if (offset < size)
                {
                    StringCchPrintfW(sz, _countof(sz), L"%02X ", data[offset]);
                    ret += sz;
                }
                else
                {
                    ret += L"   ";
                    flag = true;
                }
            }

            ret += L' ';

            for (DWORD i = 0; i < 16; ++i)
            {
                DWORD offset = addr + i;
                if (offset < size)
                {
                    if (data[offset] == 0)
                        ret += L' ';
                    else if (data[offset] < 0x20 || data[offset] > 0x7F)
                        ret += L'.';
                    else
                        ret += WCHAR(data[offset]);
                }
                else
                {
                    ret += L' ';
                    flag = true;
                }
            }

            ret += L"\r\n";

            if (flag)
                break;
        }
    }

    return ret;
}

MStringW GetKeyID(ConstantsDB& db, UINT wId)
{
    if (!db.AreMacroIDShown())
        return mstr_dec_short((SHORT)wId);

    return db.GetNameOfResID(IDTYPE_COMMAND, IDTYPE_NEWCOMMAND, wId);
}

void Cmb1_InitVirtualKeys(HWND hCmb1, ConstantsDB& db)
{
    ComboBox_ResetContent(hCmb1);

    typedef ConstantsDB::TableType TableType;
    TableType table;
    table = db.GetTable(L"VIRTUALKEYS");

    TableType::iterator it, end = table.end();
    for (it = table.begin(); it != end; ++it)
    {
        ComboBox_AddString(hCmb1, it->name.c_str());
    }
}

BOOL Cmb1_CheckKey(HWND hwnd, HWND hCmb1, BOOL bVirtKey, MStringW& str)
{
    if (bVirtKey)
    {
        INT i = ComboBox_FindStringExact(hCmb1, -1, str.c_str());
        if (i == CB_ERR)
        {
            BOOL bOK;
            i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
            if (!bOK)
            {
                return FALSE;
            }
            str = mstr_dec(i);
        }
    }
    else
    {
        BOOL bOK;
        INT i = GetDlgItemInt(hwnd, cmb1, &bOK, TRUE);
        if (!bOK)
        {
            LPCWSTR pch = str.c_str();
            MStringW str2;
            if (!guts_quote(str2, pch) || str2.size() != 1)
            {
                return FALSE;
            }
            str = mstr_quote(str2);
        }
        else
        {
            str = mstr_dec(i);
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// STRING_ENTRY

BOOL StrDlg_GetEntry(HWND hwnd, STRING_ENTRY& entry, ConstantsDB& db)
{
    MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
    ReplaceFullWithHalf(str);
    mstr_trim(str);
    if (('0' <= str[0] && str[0] <= '9') || str[0] == '-' || str[0] == '+')
    {
        LONG n = mstr_parse_int(str.c_str());
        str = mstr_dec_word(WORD(n));
    }
    else if (!db.HasResID(str))
    {
        return FALSE;
    }
    lstrcpynW(entry.StringID, str.c_str(), _countof(entry.StringID));

    str = MWindowBase::GetDlgItemText(hwnd, edt1);
    mstr_trim(str);
    if (str[0] == L'"')
    {
        mstr_unquote(str);
    }
    lstrcpynW(entry.StringValue, str.c_str(), _countof(entry.StringValue));
    return TRUE;
}

void StrDlg_SetEntry(HWND hwnd, STRING_ENTRY& entry, ConstantsDB& db)
{
    SetDlgItemTextW(hwnd, cmb1, entry.StringID);

    MStringW str = entry.StringValue;
    str = mstr_quote(str);

    SetDlgItemTextW(hwnd, edt1, str.c_str());
}

//////////////////////////////////////////////////////////////////////////////
// MESSAGE_ENTRY

BOOL MsgDlg_GetEntry(HWND hwnd, MESSAGE_ENTRY& entry, ConstantsDB& db)
{
    MString str = MWindowBase::GetDlgItemText(hwnd, cmb1);
    ReplaceFullWithHalf(str);
    mstr_trim(str);
    if (('0' <= str[0] && str[0] <= '9') || str[0] == '-' || str[0] == '+')
    {
        LONG n = mstr_parse_int(str.c_str());
        str = mstr_hex(n);
    }
    else if (!db.HasResID(str))
    {
        return FALSE;
    }
    lstrcpynW(entry.MessageID, str.c_str(), _countof(entry.MessageID));

    str = MWindowBase::GetDlgItemText(hwnd, edt1);
    mstr_trim(str);
    if (str[0] == L'"')
    {
        mstr_unquote(str);
    }
    lstrcpynW(entry.MessageValue, str.c_str(), _countof(entry.MessageValue));
    return TRUE;
}

void MsgDlg_SetEntry(HWND hwnd, MESSAGE_ENTRY& entry, ConstantsDB& db)
{
    SetDlgItemTextW(hwnd, cmb1, entry.MessageID);

    MStringW str = entry.MessageValue;
    str = mstr_quote(str);

    SetDlgItemTextW(hwnd, edt1, str.c_str());
}

//////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK
EnumLocalesProc(LPWSTR lpLocaleString)
{
    LangEntry entry;
    LCID lcid = mstr_parse_int(lpLocaleString, false, 16);
    entry.LangID = LANGIDFROMLCID(lcid);

    WCHAR sz[MAX_PATH] = L"";
    if (lcid == 0)
        return TRUE;
    if (!GetLocaleInfoW(lcid, LOCALE_SLANGUAGE, sz, _countof(sz)))
        return TRUE;

    entry.str = sz;
    g_Langs.push_back(entry);
    return TRUE;
}

MStringW
Res_GetLangName(WORD lang)
{
    WCHAR sz[64], szLoc[64];
    LCID lcid = MAKELCID(lang, SORT_DEFAULT);
    if (lcid == 0)
    {
        StringCchPrintfW(sz, _countof(sz), L"%s (0)", LoadStringDx(IDS_NEUTRAL));
    }
    else
    {
        GetLocaleInfo(lcid, LOCALE_SLANGUAGE, szLoc, 64);
        StringCchPrintfW(sz, _countof(sz), L"%s (%u)", szLoc, lang);
    }
    return MStringW(sz);
}

//////////////////////////////////////////////////////////////////////////////
// MMainWnd

class MMainWnd : public MWindowBase
{
protected:
    INT         m_argc;         // number of command line parameters
    TCHAR **    m_targv;        // command line parameters
    BOOL        m_bLoading;

    // handles
    HINSTANCE   m_hInst;        // the instance handle
    HICON       m_hIcon;        // the icon handle
    HICON       m_hIconSm;      // the small icon handle
    HACCEL      m_hAccel;       // the accelerator handle
    HIMAGELIST  m_hImageList;   // the image list for m_hTreeView
    HICON       m_hFileIcon, m_hFolderIcon;
    HFONT       m_hSrcFont, m_hBinFont;
    HWND        m_hTreeView, m_hToolBar, m_hStatusBar;
    HWND        m_hFindReplaceDlg;

    // path strings
    WCHAR       m_szDataFolder[MAX_PATH];
    WCHAR       m_szConstantsFile[MAX_PATH];
    WCHAR       m_szCppExe[MAX_PATH];
    WCHAR       m_szWindresExe[MAX_PATH];
    WCHAR       m_szUpxExe[MAX_PATH];
    WCHAR       m_szMcdxExe[MAX_PATH];
    WCHAR       m_szRealFile[MAX_PATH];
    WCHAR       m_szNominalFile[MAX_PATH];
    WCHAR       m_szResourceH[MAX_PATH];
    WCHAR       m_szUpxTempFile[MAX_PATH];
    BOOL        m_bUpxCompressed;

    // selection
    MIdOrString     m_type;
    MIdOrString     m_name;
    WORD            m_lang;

    // classes
    RisohSettings   m_settings;
    ConstantsDB     m_db;
    MRadWindow      m_rad_window;
    MEditCtrl       m_hBinEdit, m_hSrcEdit;
    MBmpView        m_hBmpView;
    MSplitterWnd    m_splitter1, m_splitter2, m_splitter3;
    MIDListDlg      m_id_list_dlg;
    ResEntries      m_entries;
    ITEM_SEARCH     m_search;

    // find/replace
    FINDREPLACE     m_fr;
    TCHAR           m_szFindWhat[80];
    TCHAR           m_szReplaceWith[80];

    MString GetLanguageStatement(WORD langid)
    {
        return ::GetLanguageStatement(langid, TRUE) + L"\r\n";
    }

public:
    MMainWnd(int argc, TCHAR **targv, HINSTANCE hInst) :
        m_argc(argc), m_targv(targv), m_bLoading(FALSE),
        m_hInst(hInst), m_hIcon(NULL), m_hIconSm(NULL), m_hAccel(NULL),
        m_hImageList(NULL), m_hFileIcon(NULL), m_hFolderIcon(NULL),
        m_hSrcFont(NULL), m_hBinFont(NULL),
        m_hTreeView(NULL), m_hToolBar(NULL), m_hStatusBar(NULL),
        m_hFindReplaceDlg(NULL), m_rad_window(m_entries, m_db, m_settings),
        m_id_list_dlg(m_entries, m_db, m_settings),
        m_search(m_settings, m_db, m_entries)
    {
        m_szDataFolder[0] = 0;
        m_szConstantsFile[0] = 0;
        m_szCppExe[0] = 0;
        m_szWindresExe[0] = 0;
        m_szUpxExe[0] = 0;
        m_szMcdxExe[0] = 0;
        m_szRealFile[0] = 0;
        m_szNominalFile[0] = 0;
        m_szResourceH[0] = 0;
        m_szUpxTempFile[0] = 0;

        m_bUpxCompressed = FALSE;

        m_lang = 0xFFFF;

        ZeroMemory(&m_fr, sizeof(m_fr));
        m_fr.lStructSize = sizeof(m_fr);
        m_fr.Flags = FR_HIDEWHOLEWORD | FR_DOWN;

        m_szFindWhat[0] = 0;
        m_fr.lpstrFindWhat = m_szFindWhat;
        m_fr.wFindWhatLen = _countof(m_szFindWhat);

        m_szReplaceWith[0] = 0;
        m_fr.lpstrReplaceWith = m_szReplaceWith;
        m_fr.wReplaceWithLen = _countof(m_szReplaceWith);
    }

    // settings
    void SetDefaultSettings(HWND hwnd);
    BOOL LoadSettings(HWND hwnd);
    BOOL SaveSettings(HWND hwnd);
    void UpdatePrefixDB(HWND hwnd);

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        MWindowBase::ModifyWndClassDx(wcx);
        wcx.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
        wcx.hIcon = m_hIcon;
        wcx.hIconSm = m_hIconSm;
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's RisohEditor");
    }

    BOOL StartDx();
    INT_PTR RunDx();
    void DoEvents();
    void DoMsg(MSG& msg);

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    //////////////////////////////////////////////////////////////////////

    // status bar
    void ChangeStatusText(INT nID)
    {
        ChangeStatusText(LoadStringDx(nID));
    }
    void ChangeStatusText(LPCTSTR pszText)
    {
        SendMessage(m_hStatusBar, SB_SETTEXT, 0, (LPARAM)pszText);
    }

    // utilities
    BOOL CheckDataFolder(VOID);
    INT CheckData(VOID);
    BOOL SetFilePath(HWND hwnd, LPCWSTR pszRealFile, LPCWSTR pszNominal = NULL);

    void UpdateMenu();
    void SelectTV(HWND hwnd, LPARAM lParam, BOOL DoubleClick);
    BOOL IsEditableEntry(HWND hwnd, LPARAM lParam);
    BOOL CompileIfNecessary(HWND hwnd, BOOL bReopen = FALSE);
    BOOL ReCompileOnSelChange(HWND hwnd, BOOL bReopen = FALSE);
    void SelectString(HWND hwnd);
    void SelectMessage(HWND hwnd);
    BOOL CreateOurToolBar(HWND hwndParent);
    void UpdateOurToolBar(INT iType);

    // ID list
    void OnIDList(HWND hwnd);
    void OnIdAssoc(HWND hwnd);
    void OnPredefMacros(HWND hwnd);
    void OnEditLabel(HWND hwnd);
    void OnSetPaths(HWND hwnd);
    void OnUseOldStyleLangStmt(HWND hwnd);
    void OnShowLangs(HWND hwnd);
    void OnShowHideToolBar(HWND hwnd);

    // show/hide
    void ShowIDList(HWND hwnd, BOOL bShow = TRUE);
    void ShowMovie(BOOL bShow = TRUE);
    void ShowBmpView(BOOL bShow = TRUE);
    void ShowStatusBar(BOOL bShow = TRUE);
    void ShowBinEdit(BOOL bShow = TRUE, BOOL bShowError = FALSE);

    // preview
    VOID HidePreview(HWND hwnd);
    BOOL Preview(HWND hwnd, const ResEntry& entry);

    // actions
    BOOL DoLoadResH(HWND hwnd, LPCTSTR pszFile);
    void DoLoadLangInfo(VOID);
    BOOL DoLoadFile(HWND hwnd, LPCWSTR pszFileName, DWORD nFilterIndex = 0, BOOL bForceDecompress = FALSE);
    BOOL DoImport(HWND hwnd, LPCWSTR ResFile, ResEntries& entries);
    BOOL DoLoadRC(HWND hwnd, LPCWSTR szRCFile, ResEntries& entries);
    BOOL DoLoadMsgTables(HWND hwnd, LPCWSTR szRCFile, ResEntries& entries, MStringA& strOutput);
    BOOL DoExtract(const ResEntry& entry, BOOL bExporting);
    BOOL DoExtractIcon(LPCWSTR pszFileName, const ResEntry& entry);
    BOOL DoExtractCursor(LPCWSTR pszFileName, const ResEntry& entry);
    BOOL DoExtractRes(HWND hwnd, LPCWSTR pszFileName, const ResEntries& entries);
    BOOL DoExtractBin(LPCWSTR pszFileName, const ResEntry& entry);
    BOOL DoExport(LPCWSTR pszFileName);
    void DoIDStat(UINT anValues[5]);
    BOOL DoBackupFile(LPCWSTR pszFileName, UINT nCount = 0);
    BOOL DoBackupFolder(LPCWSTR pszFileName, UINT nCount = 0);
    BOOL DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH);
    BOOL DoWriteRCLang(MFile& file, ResToText& res2text, WORD lang);
    BOOL DoWriteResH(LPCWSTR pszRCFile, LPCWSTR pszFileName);
    BOOL DoSaveResAs(HWND hwnd, LPCWSTR pszExeFile);
    BOOL DoSaveAs(HWND hwnd, LPCWSTR pszExeFile);
    BOOL DoSaveExeAs(HWND hwnd, LPCWSTR pszExeFile);
    BOOL DoCopyGroupIcon(ResEntry& entry, const MIdOrString& name);
    BOOL DoCopyGroupCursor(ResEntry& entry, const MIdOrString& name);
    BOOL DoUpxTest(LPCWSTR pszUpx, LPCWSTR pszFile);
    BOOL DoUpxExtract(LPCWSTR pszUpx, LPCWSTR pszFile);
    BOOL DoUpxCompress(LPCWSTR pszUpx, LPCWSTR pszExeFile);
    void DoRenameEntry(ResEntry entry, const MIdOrString& old_name, const MIdOrString& new_name);
    void DoRelangEntry(ResEntry entry, WORD old_lang, WORD new_lang);
    void DoRefresh(HWND hwnd, BOOL bRefreshAll = FALSE);
    void DoRefreshIDList(HWND hwnd);

    HTREEITEM GetLastItem(HTREEITEM hItem);
    HTREEITEM GetLastLeaf(HTREEITEM hItem);

    void ReCreateFonts(HWND hwnd);
    void ReSetPaths(HWND hwnd);

    BOOL DoItemSearch(HTREEITEM hItem, ITEM_SEARCH& search);

protected:
    // parsing resource IDs
    BOOL CareWindresResult(HWND hwnd, ResEntries& entries, MStringA& msg);
    BOOL CompileParts(HWND hwnd, const MStringW& strWide, BOOL bReopen = FALSE);
    BOOL CompileMessageTable(HWND hwnd, const MStringW& strWide);
    BOOL CheckResourceH(HWND hwnd, LPCTSTR pszPath);
    BOOL ParseResH(HWND hwnd, LPCTSTR pszFile, const char *psz, DWORD len);
    BOOL ParseMacros(HWND hwnd, LPCTSTR pszFile, std::vector<MStringA>& macros, MStringA& str);
    BOOL UnloadResourceH(HWND hwnd, BOOL bRefresh = TRUE);
    MStringW GetMacroDump();
    MStringW GetIncludesDump();
    void ReadResHLines(FILE *fp, std::vector<std::string>& lines);
    void UpdateResHLines(std::vector<std::string>& lines);

    void JoinLinesByBackslash(std::vector<std::string>& lines);
    void DeleteIncludeGuard(std::vector<std::string>& lines);
    void AddAdditionalMacroLines(std::vector<std::string>& lines);
    void DeleteSpecificMacroLines(std::vector<std::string>& lines);
    void AddApStudioBlock(std::vector<std::string>& lines);
    void DeleteApStudioBlock(std::vector<std::string>& lines);
    void AddHeadComment(std::vector<std::string>& lines);
    void DeleteHeadComment(std::vector<std::string>& lines);

    // preview
    void PreviewIcon(HWND hwnd, const ResEntry& entry);
    void PreviewCursor(HWND hwnd, const ResEntry& entry);
    void PreviewGroupIcon(HWND hwnd, const ResEntry& entry);
    void PreviewGroupCursor(HWND hwnd, const ResEntry& entry);
    void PreviewBitmap(HWND hwnd, const ResEntry& entry);
    void PreviewImage(HWND hwnd, const ResEntry& entry);
    void PreviewWAVE(HWND hwnd, const ResEntry& entry);
    void PreviewAVI(HWND hwnd, const ResEntry& entry);
    void PreviewAccel(HWND hwnd, const ResEntry& entry);
    void PreviewMessage(HWND hwnd, const ResEntry& entry);
    void PreviewString(HWND hwnd, const ResEntry& entry);
    void PreviewHtml(HWND hwnd, const ResEntry& entry);
    void PreviewMenu(HWND hwnd, const ResEntry& entry);
    void PreviewVersion(HWND hwnd, const ResEntry& entry);
    void PreviewDialog(HWND hwnd, const ResEntry& entry);
    void PreviewAniIcon(HWND hwnd, const ResEntry& entry, BOOL bIcon);
    void PreviewStringTable(HWND hwnd, const ResEntry& entry);
    void PreviewMessageTable(HWND hwnd, const ResEntry& entry);
    void PreviewRCData(HWND hwnd, const ResEntry& entry);
    void PreviewDlgInit(HWND hwnd, const ResEntry& entry);
    void PreviewUnknown(HWND hwnd, const ResEntry& entry);

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
    void OnSysColorChange(HWND hwnd);
    void OnPlay(HWND hwnd);
    void OnDropFiles(HWND hwnd, HDROP hdrop);
    void OnMove(HWND hwnd, int x, int y);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    void OnInitMenu(HWND hwnd, HMENU hMenu);
    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr);
    void OnDestroy(HWND hwnd);

    void OnCancelEdit(HWND hwnd);
    void OnCompile(HWND hwnd);
    void OnGuiEdit(HWND hwnd);
    void OnEdit(HWND hwnd);
    void OnUpdateDlgRes(HWND hwnd);
    void OnCopyAsNewName(HWND hwnd);
    void OnCopyAsNewLang(HWND hwnd);
    void OnItemSearch(HWND hwnd);
    void OnItemSearchBang(HWND hwnd, MItemSearchDlg *pDialog);

    LRESULT OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnMoveSizeReport(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnClearStatus(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnReopenRad(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnPostSearch(HWND hwnd, WPARAM wParam, LPARAM lParam);
    LRESULT OnIDJumpBang(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void OnIDJumpBang2(HWND hwnd, const MString& name, MString& strType);

    void OnAddBitmap(HWND hwnd);
    void OnAddCursor(HWND hwnd);
    void OnAddDialog(HWND hwnd);
    void OnAddIcon(HWND hwnd);
    void OnAddMenu(HWND hwnd);
    void OnAddRes(HWND hwnd);
    void OnAddVerInfo(HWND hwnd);
    void OnAddManifest(HWND hwnd);
    void OnAddStringTable(HWND hwnd);
    void OnAddMessageTable(HWND hwnd);
    void OnAddHtml(HWND hwnd);
    void OnAddAccel(HWND hwnd);
    void OnDeleteRes(HWND hwnd);
    void OnExtractBin(HWND hwnd);
    void OnExtractBitmap(HWND hwnd);
    void OnExtractCursor(HWND hwnd);
    void OnExtractIcon(HWND hwnd);
    void OnReplaceBin(HWND hwnd);
    void OnReplaceBitmap(HWND hwnd);
    void OnReplaceCursor(HWND hwnd);
    void OnReplaceIcon(HWND hwnd);
    void OnUpdateResHBang(HWND hwnd);

    void OnNew(HWND hwnd);
    void OnOpen(HWND hwnd);
    void OnSaveAs(HWND hwnd);
    void OnImport(HWND hwnd);
    void OnLoadResH(HWND hwnd);
    void OnLoadResHBang(HWND hwnd);
    void OnLoadWCLib(HWND hwnd);
    void OnExport(HWND hwnd);
    void OnFonts(HWND hwnd);
    void OnAbout(HWND hwnd);
    void OnConfig(HWND hwnd);
    void OnOpenReadMe(HWND hwnd);
    void OnOpenReadMeJp(HWND hwnd);
    void OnOpenLicense(HWND hwnd);
    void OnOpenHyojunka(HWND hwnd);
    void OnDebugTreeNode(HWND hwnd);
    void OnAdviceResH(HWND hwnd);
    void OnUnloadResH(HWND hwnd);
    void OnHideIDMacros(HWND hwnd);
    void OnUseIDC_STATIC(HWND hwnd);
    void OnTest(HWND hwnd);

    // find/replace
    void OnFind(HWND hwnd);
    BOOL OnFindNext(HWND hwnd);
    BOOL OnFindPrev(HWND hwnd);
    BOOL OnReplaceNext(HWND hwnd);
    BOOL OnReplacePrev(HWND hwnd);
    BOOL OnReplace(HWND hwnd);
    BOOL OnReplaceAll(HWND hwnd);
    LRESULT OnFindMsg(HWND hwnd, WPARAM wParam, LPARAM lParam);
};

//////////////////////////////////////////////////////////////////////////////
// MMainWnd out-of-line functions

void MMainWnd::OnSysColorChange(HWND hwnd)
{
    m_splitter1.SendMessageDx(WM_SYSCOLORCHANGE);
    m_splitter2.SendMessageDx(WM_SYSCOLORCHANGE);
    m_splitter3.SendMessageDx(WM_SYSCOLORCHANGE);
    m_rad_window.SendMessageDx(WM_SYSCOLORCHANGE);
}

LRESULT MMainWnd::OnCompileCheck(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (!CompileIfNecessary(hwnd, TRUE))
    {
        return FALSE;
    }
    return FALSE;
}

LRESULT MMainWnd::OnReopenRad(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    OnGuiEdit(hwnd);
    return 0;
}

LRESULT MMainWnd::OnMoveSizeReport(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT x = (SHORT)LOWORD(wParam);
    INT y = (SHORT)HIWORD(wParam);
    INT cx = (SHORT)LOWORD(lParam);
    INT cy = (SHORT)HIWORD(lParam);

    ChangeStatusText(LoadStringPrintfDx(IDS_COORD, x, y, cx, cy));
    return 0;
}

LRESULT MMainWnd::OnClearStatus(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    ChangeStatusText(TEXT(""));
    return 0;
}

void MMainWnd::OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
    {
        SetFocus(m_hTreeView);
    }
    FORWARD_WM_ACTIVATE(hwnd, state, hwndActDeact, fMinimized, CallWindowProcDx);
}

void MMainWnd::UpdateMenu()
{
    HMENU hMenu = GetMenu(m_hwnd);
    HMENU hFileMenu = GetSubMenu(hMenu, 0);
    HMENU hMruMenu = GetSubMenu(hFileMenu, GetMenuItemCount(hFileMenu) - 3);
    assert(hMruMenu);
    while (DeleteMenu(hMruMenu, 0, MF_BYPOSITION))
        ;

    INT i = 0;
    TCHAR szText[MAX_PATH * 2];
    static const TCHAR szPrefix[] = TEXT("123456789ABCDEF0");
    mru_type::iterator it, end = m_settings.vecRecentlyUsed.end();
    for (it = m_settings.vecRecentlyUsed.begin(); it != end; ++it)
    {
#if 1
        LPCTSTR pch = _tcsrchr(it->c_str(), TEXT('\\'));
        if (pch == NULL)
            pch = _tcsrchr(it->c_str(), TEXT('/'));
        if (pch == NULL)
            pch = it->c_str();
        else
            ++pch;
        StringCchPrintf(szText, _countof(szText), TEXT("&%c  %s"), szPrefix[i], pch);
        InsertMenu(hMruMenu, i, MF_BYPOSITION | MF_STRING, ID_MRUFILE0 + i, szText);
#else
        InsertMenu(hMruMenu, i, MF_BYPOSITION | MF_STRING, ID_MRUFILE0 + i, it->c_str());
#endif
        ++i;
    }

    if (m_settings.vecRecentlyUsed.empty())
    {
        InsertMenu(hMruMenu, 0, MF_BYPOSITION | MF_STRING | MF_GRAYED, -1, LoadStringDx(IDS_NONE));
    }
}

void MMainWnd::OnExtractBin(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) == I_NONE)
        return;

    UINT i = LOWORD(lParam);

    WCHAR szFile[MAX_PATH] = L"";

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    if (HIWORD(lParam) == I_STRING || HIWORD(lParam) == I_MESSAGE)
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
    if (HIWORD(lParam) == I_LANG)
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESBINFILTER));
    else
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTRES);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
        OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"res";
    if (GetSaveFileNameW(&ofn))
    {
        if (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"res") == 0)
        {
            ResEntries selection;
            INT count = TV_GetSelection(m_hTreeView, selection, m_entries);
            if (count && !DoExtractRes(hwnd, ofn.lpstrFile, selection))
            {
                ErrorBoxDx(IDS_CANNOTSAVE);
            }
        }
        else
        {
            if (!DoExtractBin(ofn.lpstrFile, m_entries[i]))
            {
                ErrorBoxDx(IDS_CANNOTSAVE);
            }
        }
    }
}

void MMainWnd::OnExtractIcon(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    ResEntry& entry = m_entries[i];

    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_ICOFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTICO);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
        OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (entry.type == RT_ANIICON)
    {
        ofn.nFilterIndex = 2;
        ofn.lpstrDefExt = L"ani";
    }
    else
    {
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"ico";
    }
    if (GetSaveFileNameW(&ofn))
    {
        if (!DoExtractIcon(ofn.lpstrFile, m_entries[i]))
        {
            ErrorBoxDx(IDS_CANTEXTRACTICO);
        }
    }
}

void MMainWnd::OnExtractCursor(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    ResEntry& entry = m_entries[i];

    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_CURFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTCUR);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
        OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (entry.type == RT_ANICURSOR)
    {
        ofn.nFilterIndex = 2;
        ofn.lpstrDefExt = L"ani";
    }
    else
    {
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"cur";
    }
    if (GetSaveFileNameW(&ofn))
    {
        if (!DoExtractCursor(ofn.lpstrFile, m_entries[i]))
        {
            ErrorBoxDx(IDS_CANTEXTRACTCUR);
        }
    }
}

void MMainWnd::OnExtractBitmap(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);

    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_BMPFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = LoadStringDx(IDS_EXTRACTBMP);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
        OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"bmp";
    if (GetSaveFileNameW(&ofn))
    {
        BOOL PNG;
        PNG = (lstrcmpiW(&ofn.lpstrFile[ofn.nFileExtension], L"png") == 0);
        if (!PackedDIB_Extract(ofn.lpstrFile, &m_entries[i][0], m_entries[i].size(), PNG))
        {
            ErrorBoxDx(IDS_CANTEXTRACTBMP);
        }
    }
}

void MMainWnd::OnReplaceBin(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    MReplaceBinDlg dialog(m_entries, m_entries[i], m_db);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAbout(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

#if 1
    MVersionInfoDlg dialog;
    dialog.DialogBoxDx(hwnd);
#else
    MSGBOXPARAMSW params;

    ZeroMemory(&params, sizeof(params));
    params.cbSize = sizeof(params);
    params.hwndOwner = hwnd;
    params.hInstance = m_hInst;
    params.lpszText = LoadStringDx(IDS_VERSIONINFO);
    params.lpszCaption = LoadStringDx(IDS_APPNAME);
    params.dwStyle = MB_OK | MB_USERICON;
    params.lpszIcon = MAKEINTRESOURCEW(IDI_MAIN);
    params.dwLanguageId = LANG_USER_DEFAULT;

    MWindowBase::HookCenterMsgBoxDx(TRUE);
    MessageBoxIndirectW(&params);
    MWindowBase::HookCenterMsgBoxDx(FALSE);
#endif
}

void MMainWnd::OnFonts(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    MFontsDlg dialog(m_settings);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DeleteObject(m_hBinFont);
        m_hBinFont = dialog.DetachBinFont();
        SetWindowFont(m_hBinEdit, m_hBinFont, TRUE);

        DeleteObject(m_hSrcFont);
        m_hSrcFont = dialog.DetachSrcFont();
        SetWindowFont(m_hSrcEdit, m_hSrcFont, TRUE);
    }
}

void MMainWnd::OnExport(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    MExportOptionsDlg dialog(m_settings);
    if (dialog.DialogBoxDx(hwnd) != IDOK)
        return;

    WCHAR file[MAX_PATH] = TEXT("");

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RCFILTER));
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);
    ofn.lpstrTitle = LoadStringDx(IDS_EXPORT);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
        OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"rc";
    if (GetSaveFileNameW(&ofn))
    {
        if (!DoExport(file))
        {
            ErrorBoxDx(IDS_CANTEXPORT);
        }
    }
}

typedef std::set<HMODULE> wclib_t;

wclib_t& wclib()
{
    static wclib_t s_wclib;
    return s_wclib;
}

BOOL IsThereWndClass(const WCHAR *pszName)
{
    if (!pszName || pszName[0] == 0)
        return FALSE;   // failure

    WNDCLASSEX cls;
    if (GetClassInfoEx(NULL, pszName, &cls) ||
        GetClassInfoEx(GetModuleHandle(NULL), pszName, &cls))
    {
        return TRUE;
    }

    // in the window class libraries?
    wclib_t::iterator it, end = wclib().end();
    for (it = wclib().begin(); it != end; ++it)
    {
        if (GetClassInfoEx(*it, pszName, &cls))
            return TRUE;
    }

    // CLSID?
    if (pszName[0] == L'{' &&
        pszName[9] == L'-' && pszName[14] == L'-' &&
        pszName[19] == L'-' && pszName[24] == L'-' &&
        pszName[37] == L'}')
    {
        return TRUE;
    }

    // ATL OLE controls?
    if (std::wstring(pszName).find(L"AtlAxWin") == 0)
        return TRUE;

    return FALSE;   // failure
}

void FreeWCLib()
{
    wclib_t::iterator it, end = wclib().end();
    for (it = wclib().begin(); it != end; ++it)
    {
        FreeLibrary(*it);
    }
    wclib().clear();
}

void MMainWnd::OnLoadWCLib(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    WCHAR file[MAX_PATH] = TEXT("");

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_DLLFILTER));
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);
    ofn.lpstrTitle = LoadStringDx(IDS_LOADWCLIB);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
        OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"dll";
    if (GetOpenFileNameW(&ofn))
    {
        HMODULE hMod = LoadLibraryW(file);
        if (hMod)
        {
            wclib().insert(hMod);
        }
        else
        {
            ErrorBoxDx(IDS_CANNOTLOAD);
        }
    }
}

void MMainWnd::OnImport(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    WCHAR file[MAX_PATH] = TEXT("");

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_RESFILTER));
    ofn.lpstrFile = file;
    ofn.nMaxFile = _countof(file);
    ofn.lpstrTitle = LoadStringDx(IDS_IMPORTRES);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
        OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"res";
    if (GetOpenFileNameW(&ofn))
    {
        ResEntries entries;
        if (DoImport(hwnd, file, entries))
        {
            BOOL bOverwrite = TRUE;
            if (Res_Intersect(m_entries, entries))
            {
                INT nID = MsgBoxDx(IDS_EXISTSOVERWRITE,
                                   MB_ICONINFORMATION | MB_YESNOCANCEL);
                switch (nID)
                {
                case IDYES:
                    break;
                case IDNO:
                    bOverwrite = FALSE;
                    break;
                case IDCANCEL:
                    return;
                }
            }

            size_t i, count = entries.size();
            for (i = 0; i < count; ++i)
            {
                Res_AddEntry(m_entries, entries[i], bOverwrite);
            }

            TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        }
        else
        {
            ErrorBoxDx(IDS_CANNOTIMPORT);
        }
    }
}

void MMainWnd::OnOpen(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    WCHAR szFile[MAX_PATH];
    lstrcpynW(szFile, m_szNominalFile, _countof(szFile));

    if (GetFileAttributesW(szFile) == INVALID_FILE_ATTRIBUTES)
        szFile[0] = 0;

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESRCFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_OPEN);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
        OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"exe";
    if (GetOpenFileNameW(&ofn))
    {
        DoLoadFile(hwnd, szFile, ofn.nFilterIndex);
    }
}

void MMainWnd::OnNew(HWND hwnd)
{
    HidePreview(hwnd);
    OnUnloadResH(hwnd);
    SetFilePath(hwnd, NULL, NULL);
    m_entries.clear();
    TV_RefreshInfo(m_hTreeView, m_db, m_entries);
}

void MMainWnd::OnSaveAs(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    WCHAR szFile[MAX_PATH];

    lstrcpynW(szFile, m_szNominalFile, _countof(szFile));

    if (GetFileAttributesW(szFile) == INVALID_FILE_ATTRIBUTES)
        szFile[0] = 0;

    LPWSTR pch = wcsrchr(szFile, L'.');
    if (lstrcmpiW(pch, L".rc") == 0)
        szFile[0] = 0;

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_EXERESFILTER));
    if (GetFileAttributesW(m_szRealFile) == INVALID_FILE_ATTRIBUTES)
    {
        ofn.nFilterIndex = 2;
        ofn.lpstrDefExt = L"res";
    }
    else
    {
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"exe";
    }
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_SAVEAS);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
        OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileNameW(&ofn))
    {
        if (ofn.nFilterIndex == 2)
        {
            if (!DoSaveResAs(hwnd, szFile))
            {
                ErrorBoxDx(IDS_CANNOTSAVE);
            }
        }
        else
        {
            if (!DoSaveAs(hwnd, szFile))
            {
                ErrorBoxDx(IDS_CANNOTSAVE);
            }
        }
    }
}

void MMainWnd::OnUpdateDlgRes(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    ResEntry& entry = m_entries[i];
    if (entry.type != RT_DIALOG)
    {
        return;
    }

    DialogRes& dialog_res = m_rad_window.m_dialog_res;

    MByteStreamEx stream;
    dialog_res.SaveToStream(stream);
    entry.data = stream.data();

    MString str = GetLanguageStatement(entry.lang);
    str += dialog_res.Dump(entry.name);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    str = DumpDataAsString(entry.data);
    SetWindowTextW(m_hBinEdit, str.c_str());

    dialog_res.m_dlginit.EraseInvalid();
    if (dialog_res.m_dlginit.empty())
    {
        Res_DeleteEntries(m_entries, RT_DLGINIT, entry.name, entry.lang);
    }
    else
    {
        MByteStreamEx stream;
        if (dialog_res.m_dlginit.SaveToStream(stream))
        {
            Res_AddEntry(m_entries, RT_DLGINIT, entry.name, entry.lang,
                         stream.data(), TRUE);
        }
    }
}

BOOL MMainWnd::DoCopyGroupIcon(ResEntry& entry, const MIdOrString& name)
{
    ICONDIR dir;
    if (entry.size() < sizeof(dir))
    {
        assert(0);
        return FALSE;
    }

    memcpy(&dir, &entry[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_ICON || dir.idCount == 0)
    {
        assert(0);
        return FALSE;
    }

    if (entry.size() < sizeof(dir) + dir.idCount * sizeof(GRPICONDIRENTRY))
    {
        assert(0);
        return FALSE;
    }

    const GRPICONDIRENTRY *pEntries = (const GRPICONDIRENTRY *)&entry[sizeof(dir)];

    LONG cx = 0, cy = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find2(m_entries, RT_ICON, pEntries[i].nID, entry.lang, FALSE);
        if (k == -1)
        {
            return FALSE;
        }
        ResEntry icon_entry = m_entries[k];

        UINT nLastID = Res_GetLastIconID(m_entries);
        UINT nNextID = nLastID + 1;

        icon_entry.name = WORD(nNextID);

        Res_AddEntry(m_entries, icon_entry, TRUE);
    }

    entry.name = name;
    return Res_AddEntry(m_entries, entry, TRUE);
}

BOOL MMainWnd::DoCopyGroupCursor(ResEntry& entry, const MIdOrString& name)
{
    ICONDIR dir;
    if (entry.size() < sizeof(dir))
    {
        assert(0);
        return FALSE;
    }

    memcpy(&dir, &entry[0], sizeof(dir));

    if (dir.idReserved != 0 || dir.idType != RES_CURSOR || dir.idCount == 0)
    {
        assert(0);
        return FALSE;
    }

    if (entry.size() < sizeof(dir) + dir.idCount * sizeof(GRPCURSORDIRENTRY))
    {
        assert(0);
        return FALSE;
    }

    const GRPCURSORDIRENTRY *pEntries = (const GRPCURSORDIRENTRY *)&entry[sizeof(dir)];

    LONG cx = 0, cy = 0;
    for (WORD i = 0; i < dir.idCount; ++i)
    {
        INT k = Res_Find2(m_entries, RT_CURSOR, pEntries[i].nID, entry.lang, FALSE);
        if (k == -1)
        {
            return FALSE;
        }
        ResEntry cursor_entry = m_entries[k];

        UINT nLastID = Res_GetLastCursorID(m_entries);
        UINT nNextID = nLastID + 1;

        cursor_entry.name = WORD(nNextID);

        Res_AddEntry(m_entries, cursor_entry, TRUE);
    }

    entry.name = name;
    return Res_AddEntry(m_entries, entry, TRUE);
}

HTREEITEM MMainWnd::GetLastItem(HTREEITEM hItem)
{
    HTREEITEM hNext = hItem;
    do
    {
        hItem = hNext;
        hNext = TreeView_GetNextSibling(m_hTreeView, hItem);
    } while (hNext);
    return hItem;
}

HTREEITEM MMainWnd::GetLastLeaf(HTREEITEM hItem)
{
    HTREEITEM hNext, hChild;
    for (;;)
    {
        hNext = GetLastItem(hItem);
        if (!hNext)
            break;

        hChild = TreeView_GetChild(m_hTreeView, hNext);
        if (!hChild)
            break;

        hItem = hChild;
    }
    return hItem;
}

void MMainWnd::ReCreateFonts(HWND hwnd)
{
    if (m_hBinFont)
    {
        DeleteObject(m_hBinFont);
        m_hBinFont = NULL;
    }
    if (m_hSrcFont)
    {
        DeleteObject(m_hSrcFont);
        m_hSrcFont = NULL;
    }

    LOGFONTW lfBin, lfSrc;
    ZeroMemory(&lfBin, sizeof(lfBin));
    ZeroMemory(&lfSrc, sizeof(lfSrc));

    StringCchCopy(lfBin.lfFaceName, _countof(lfBin.lfFaceName), m_settings.strBinFont.c_str());
    StringCchCopy(lfSrc.lfFaceName, _countof(lfSrc.lfFaceName), m_settings.strSrcFont.c_str());

    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        lfBin.lfHeight = -MulDiv(m_settings.nBinFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        lfSrc.lfHeight = -MulDiv(m_settings.nSrcFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        DeleteDC(hDC);
    }

    m_hBinFont = CreateFontIndirectW(&lfBin);
    assert(m_hBinFont);

    m_hSrcFont = ::CreateFontIndirectW(&lfSrc);
    assert(m_hSrcFont);

    SetWindowFont(m_hBinEdit, m_hBinFont, TRUE);
    SetWindowFont(m_hSrcEdit, m_hSrcFont, TRUE);
}

BOOL MMainWnd::DoItemSearch(HTREEITEM hItem, ITEM_SEARCH& search)
{
    HTREEITEM hNext, hChild;
    TV_ITEM& item = search.item;

    while (hItem)
    {
        if (search.bCancelled || !IsWindow(search.res2text.m_hwndDialog))
            break;

        if (search.bFindFirst && search.hFound)
        {
            return TRUE;
        }

        DoEvents();

        if (!search.bDownward && hItem == search.hCurrent)
        {
            search.bValid = FALSE;
        }

        if (search.bValid)
        {
            ZeroMemory(&item, sizeof(item));
            item.mask = TVIF_HANDLE | TVIF_TEXT;
            item.hItem = hItem;
            item.pszText = search.szText;
            item.cchTextMax = _countof(search.szText);
            TreeView_GetItem(m_hTreeView, &item);

            if (search.bIgnoreCases)
            {
                CharUpper(search.szText);
            }

            BOOL bFound = FALSE;
            if (_tcsstr(search.szText, search.strText.c_str()) != NULL)
            {
                bFound = TRUE;
            }
            else
            {
                if (search.bInternalText)
                {
                    LPARAM lParam = TV_GetParam(m_hTreeView, hItem);
                    if (HIWORD(lParam) == I_LANG ||
                        HIWORD(lParam) == I_STRING ||
                        HIWORD(lParam) == I_MESSAGE)
                    {
                        UINT i = LOWORD(lParam);
                        ResEntry entry = m_entries[i];

                        if (HIWORD(lParam) == I_STRING || HIWORD(lParam) == I_MESSAGE)
                            entry.name.clear();

                        MString text = search.res2text.DumpEntry(entry);

                        if (search.bIgnoreCases)
                        {
                            CharUpper(&text[0]);
                        }
                        if (_tcsstr(text.c_str(), search.strText.c_str()) != NULL)
                        {
                            bFound = TRUE;
                        }
                    }
                }
            }
            if (bFound)
            {
                if (search.bValid)
                {
                    if (search.bFindFirst)
                    {
                        if (search.hFound == NULL)
                        {
                            search.hFound = hItem;
                            return TRUE;
                        }
                    }
                    else
                    {
                        search.hFound = hItem;
                    }
                }
            }
        }

        if (search.bDownward && hItem == search.hCurrent)
        {
            search.bValid = TRUE;
        }

        hChild = TreeView_GetChild(m_hTreeView, hItem);
        if (hChild)
        {
            DoItemSearch(hChild, search);
        }
        hNext = TreeView_GetNextSibling(m_hTreeView, hItem);
        hItem = hNext;
    }

    return search.hFound != NULL;
}

void MMainWnd::OnCopyAsNewName(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_NAME)
        return;

    UINT i = LOWORD(lParam);
    ResEntry entry = m_entries[i];

    MCloneInNewNameDlg dialog(m_entries, entry, m_db);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        ResEntries found;
        entry.lang = 0xFFFF;
        Res_Search(found, m_entries, entry);
        if (entry.type == RT_GROUP_ICON)
        {
            for (size_t i = 0; i < found.size(); ++i)
            {
                DoCopyGroupIcon(found[i], dialog.m_name);
            }
        }
        else if (entry.type == RT_GROUP_CURSOR)
        {
            for (size_t i = 0; i < found.size(); ++i)
            {
                DoCopyGroupCursor(found[i], dialog.m_name);
            }
        }
        else
        {
            for (size_t i = 0; i < found.size(); ++i)
            {
                found[i].name = dialog.m_name;
                Res_AddEntry(m_entries, found[i], TRUE);
            }
        }
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        entry.name = dialog.m_name;
        TV_SelectEntry(m_hTreeView, m_entries, entry);
    }
}

void MMainWnd::OnCopyAsNewLang(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG && 
        HIWORD(lParam) != I_STRING &&
        HIWORD(lParam) != I_MESSAGE)
    {
        return;
    }

    UINT i = LOWORD(lParam);
    ResEntry entry = m_entries[i];

    MCloneInNewLangDlg dialog(m_entries, entry, m_db);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        ResEntries found;
        Res_Search(found, m_entries, entry);
        if (entry.type == RT_GROUP_ICON)
        {
            for (size_t i = 0; i < found.size(); ++i)
            {
                found[i].lang = dialog.m_lang;
                DoCopyGroupIcon(found[i], found[i].name);
            }
        }
        else if (entry.type == RT_GROUP_CURSOR)
        {
            for (size_t i = 0; i < found.size(); ++i)
            {
                found[i].lang = dialog.m_lang;
                DoCopyGroupCursor(found[i], found[i].name);
            }
        }
        else if (HIWORD(lParam) == I_STRING)
        {
            WORD lang = entry.lang;
            ResEntries found;
            Res_Search(found, m_entries, RT_STRING, WORD(0), lang);

            for (size_t i = 0; i < found.size(); ++i)
            {
                found[i].lang = dialog.m_lang;
                Res_AddEntry(m_entries, found[i], TRUE);
            }
        }
        else if (HIWORD(lParam) == I_MESSAGE)
        {
            WORD lang = entry.lang;
            ResEntries found;
            Res_Search(found, m_entries, RT_MESSAGETABLE, WORD(0), lang);

            for (size_t i = 0; i < found.size(); ++i)
            {
                found[i].lang = dialog.m_lang;
                Res_AddEntry(m_entries, found[i], TRUE);
            }
        }
        else
        {
            for (size_t i = 0; i < found.size(); ++i)
            {
                found[i].lang = dialog.m_lang;
                Res_AddEntry(m_entries, found[i], TRUE);
            }
        }
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        entry.lang = dialog.m_lang;
        TV_SelectEntry(m_hTreeView, m_entries, entry);
    }
}

void MMainWnd::OnItemSearch(HWND hwnd)
{
    if (!MItemSearchDlg::Dialogs().empty())
    {
        HWND hDlg = **MItemSearchDlg::Dialogs().begin();
        SetForegroundWindow(hDlg);
        SetFocus(hDlg);
        return;
    }
    MItemSearchDlg *pDialog = new MItemSearchDlg(m_search);
    pDialog->CreateDialogDx(hwnd);
    m_search.res2text.m_hwnd = hwnd;
    m_search.res2text.m_hwndDialog = *pDialog;
    ShowWindow(*pDialog, SW_SHOWNORMAL);
    UpdateWindow(*pDialog);
}

void MMainWnd::OnItemSearchBang(HWND hwnd, MItemSearchDlg *pDialog)
{
    if (!IsWindow(pDialog->m_hwnd))
    {
        assert(0);
        return;
    }

    HTREEITEM hRoot = TreeView_GetRoot(m_hTreeView);
    HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
    if (!hItem)
    {
        hItem = hRoot;
        if (!m_search.bDownward)
            hItem = GetLastLeaf(hItem);
    }

    if (m_search.bIgnoreCases)
    {
        CharUpper(&m_search.strText[0]);
    }

    m_search.bCancelled = FALSE;
    m_search.hFound = NULL;
    m_search.hCurrent = hItem;

    if (m_search.bDownward)
    {
        m_search.bFindFirst = TRUE;
        m_search.bValid = FALSE;
    }
    else
    {
        m_search.bFindFirst = FALSE;
        m_search.bValid = TRUE;
    }

    if (DoItemSearch(hRoot, m_search))
    {
        pDialog->Done();
        TreeView_SelectItem(m_hTreeView, m_search.hFound);
        TreeView_EnsureVisible(m_hTreeView, m_search.hFound);

        PostMessageDx(MYWM_POSTSEARCH);
    }
    else
    {
        pDialog->Done();
        if (!m_search.hFound && !m_search.bCancelled)
        {
            EnableWindow(*pDialog, FALSE);
            MsgBoxDx(IDS_NOMOREITEM, MB_ICONINFORMATION);
            EnableWindow(*pDialog, TRUE);
        }
        SetFocus(*pDialog);
    }
}

void MMainWnd::OnDeleteRes(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
    if (hItem == NULL)
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    WORD i = LOWORD(lParam);
    if (i >= m_entries.size())
        return;

    ResEntry selection = m_entries[i];

    TV_Delete(m_hTreeView, m_db, hItem, m_entries);
    TV_RefreshInfo(m_hTreeView, m_db, m_entries);
    HidePreview(hwnd);

    TV_SelectEntry(m_hTreeView, m_entries, selection);
    SelectTV(hwnd, lParam, FALSE);
}

void MMainWnd::OnPlay(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    ResEntry& entry = m_entries[i];

    if (entry.type == L"WAVE")
    {
        PlaySound((LPCTSTR)&entry[0], NULL, SND_ASYNC | SND_NODEFAULT | SND_MEMORY);
    }
}

void MMainWnd::OnCancelEdit(HWND hwnd)
{
    Edit_SetModify(m_hSrcEdit, FALSE);
    Edit_SetReadOnly(m_hSrcEdit, FALSE);

    LPARAM lParam = TV_GetParam(m_hTreeView);
    SelectTV(hwnd, lParam, FALSE);
}

void MMainWnd::OnCompile(HWND hwnd)
{
    BOOL bReopen = IsWindowVisible(m_rad_window);

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (!Edit_GetModify(m_hSrcEdit))
    {
        SelectTV(hwnd, lParam, FALSE);
        return;
    }

    WORD i = LOWORD(lParam);
    if (i >= m_entries.size())
        return;

    ResEntry& entry = m_entries[i];

    ChangeStatusText(IDS_COMPILING);

    INT cchText = ::GetWindowTextLengthW(m_hSrcEdit);
    MStringW strWide;
    strWide.resize(cchText);
    GetWindowTextW(m_hSrcEdit, &strWide[0], cchText + 1);

    Edit_SetModify(m_hSrcEdit, FALSE);
    if (CompileParts(hwnd, strWide, bReopen))
    {
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, entry);
    }
}

void MMainWnd::OnGuiEdit(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (!IsEditableEntry(hwnd, lParam))
        return;

    WORD i = LOWORD(lParam);
    if (i >= m_entries.size())
        return;

    ResEntry& entry = m_entries[i];
    if (!Res_CanGuiEdit(entry.type))
    {
        return;
    }

    if (!CompileIfNecessary(hwnd, FALSE))
    {
        return;
    }

    const ResEntry::DataType& data = entry.data;
    MByteStreamEx stream(data);
    if (entry.type == RT_ACCELERATOR)
    {
        AccelRes accel_res(m_db);
        MEditAccelDlg dialog(accel_res, m_db);
        if (accel_res.LoadFromStream(stream))
        {
            ChangeStatusText(IDS_EDITINGBYGUI);
            INT nID = (INT)dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                accel_res.Update();
                entry.data = accel_res.data();
                SelectTV(hwnd, lParam, FALSE);
            }
        }
        Edit_SetReadOnly(m_hSrcEdit, FALSE);
        TV_SelectEntry(m_hTreeView, m_entries, entry);
        ChangeStatusText(IDS_READY);
    }
    else if (entry.type == RT_MENU)
    {
        MenuRes menu_res;
        if (menu_res.LoadFromStream(stream))
        {
            ChangeStatusText(IDS_EDITINGBYGUI);
            MEditMenuDlg dialog(m_db, menu_res);
            INT nID = (INT)dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                menu_res.Update();
                entry.data = menu_res.data();
                SelectTV(hwnd, lParam, FALSE);
            }
        }
        Edit_SetReadOnly(m_hSrcEdit, FALSE);
        TV_SelectEntry(m_hTreeView, m_entries, entry);
        ChangeStatusText(IDS_READY);
    }
    else if (entry.type == RT_DIALOG)
    {
        MByteStreamEx stream(entry.data);

        m_rad_window.m_dialog_res.LoadFromStream(stream);
        m_rad_window.m_dialog_res.m_lang_id = entry.lang;
        m_rad_window.clear_maps();
        m_rad_window.create_maps(entry.lang);

        m_rad_window.m_dialog_res.m_dlginit.clear();
        INT iDlgInit = Res_Find(m_entries, RT_DLGINIT, entry.name, entry.lang);
        if (iDlgInit >= 0)
        {
            ResEntry& die = m_entries[iDlgInit];
            DlgInitRes dlginit_res(m_db);
            stream.assign(die.data);
            dlginit_res.LoadFromStream(stream);
            m_rad_window.m_dialog_res.m_dlginit.entries() = dlginit_res.entries();
        }

        if (::IsWindowVisible(m_rad_window) &&
            IsWindowVisible(m_rad_window.m_rad_dialog))
        {
            m_rad_window.ReCreateRadDialog(m_rad_window);
        }
        else
        {
            if (!m_rad_window.CreateDx(m_hwnd))
            {
                ErrorBoxDx(IDS_DLGFAIL);
            }
        }
        Edit_SetReadOnly(m_hSrcEdit, FALSE);
    }
    else if (entry.type == RT_DLGINIT)
    {
        DlgInitRes dlginit_res(m_db);
        if (dlginit_res.LoadFromStream(stream))
        {
            ChangeStatusText(IDS_EDITINGBYGUI);
            MDlgInitDlg dialog(dlginit_res, m_db);
            INT nID = (INT)dialog.DialogBoxDx(hwnd);
            if (nID == IDOK)
            {
                entry.data = dlginit_res.data();
                SelectTV(hwnd, lParam, FALSE);
            }
        }
        Edit_SetReadOnly(m_hSrcEdit, FALSE);
        TV_SelectEntry(m_hTreeView, m_entries, entry);
        ChangeStatusText(IDS_READY);
    }
    else if (entry.type == RT_STRING && HIWORD(lParam) == I_STRING)
    {
        WORD lang = entry.lang;
        ResEntries found;
        Res_Search(found, m_entries, RT_STRING, WORD(0), lang);

        StringRes str_res;
        ResEntries::iterator it, end = found.end();
        for (it = found.begin(); it != end; ++it)
        {
            MByteStreamEx stream(it->data);
            if (!str_res.LoadFromStream(stream, it->name.m_id))
            {
                ErrorBoxDx(IDS_CANNOTLOAD);
                return;
            }
        }

        ChangeStatusText(IDS_EDITINGBYGUI);
        MStringsDlg dialog(m_db, str_res);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
        if (nID == IDOK)
        {
            str_res = dialog.m_str_res;

            bool shown = m_db.AreMacroIDShown();
            m_db.ShowMacroID(false);
            MStringW strWide = str_res.Dump(m_db);
            m_db.ShowMacroID(shown);

            if (CompileParts(hwnd, strWide))
            {
                ResEntry selection(RT_STRING, WORD(0), lang);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, selection);
            }
        }
        Edit_SetReadOnly(m_hSrcEdit, FALSE);
        ChangeStatusText(IDS_READY);
    }
    else if (entry.type == RT_MESSAGETABLE && HIWORD(lParam) == I_MESSAGE)
    {
        WORD lang = entry.lang;
        ResEntries found;
        Res_Search(found, m_entries, RT_MESSAGETABLE, WORD(0), lang);

        MessageRes msg_res;
        ResEntries::iterator it, end = found.end();
        for (it = found.begin(); it != end; ++it)
        {
            MByteStreamEx stream(it->data);
            if (!msg_res.LoadFromStream(stream, it->name.m_id))
            {
                ErrorBoxDx(IDS_CANNOTLOAD);
                return;
            }
        }

        ChangeStatusText(IDS_EDITINGBYGUI);
        MMessagesDlg dialog(m_db, msg_res);
        INT nID = (INT)dialog.DialogBoxDx(hwnd);
        if (nID == IDOK)
        {
            msg_res = dialog.m_msg_res;
            bool shown = m_db.AreMacroIDShown();
            m_db.ShowMacroID(false);
            MStringW strWide = msg_res.Dump(m_db);
            m_db.ShowMacroID(shown);

            if (CompileParts(hwnd, strWide))
            {
                ResEntry selection(RT_MESSAGETABLE, WORD(0), lang);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, selection);
            }
        }
        Edit_SetReadOnly(m_hSrcEdit, FALSE);
        ChangeStatusText(IDS_READY);
    }
}

void MMainWnd::OnEdit(HWND hwnd)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (!IsEditableEntry(hwnd, lParam))
        return;

    Edit_SetReadOnly(m_hSrcEdit, FALSE);
    SelectTV(hwnd, lParam, TRUE);
}

void MMainWnd::OnOpenReadMe(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        return;

    ++pch;
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"README.txt");
    if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopyW(pch, diff, L"..\\README.txt");
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            StringCchCopyW(pch, diff, L"..\\..\\README.txt");
            if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\README.txt");
                if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    return;
                }
            }
        }
    }
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

void MMainWnd::OnOpenReadMeJp(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        return;

    ++pch;
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"READMEJP.txt");
    if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopyW(pch, diff, L"..\\READMEJP.txt");
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            StringCchCopyW(pch, diff, L"..\\..\\READMEJP.txt");
            if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\READMEJP.txt");
                if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    return;
                }
            }
        }
    }
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

void MMainWnd::OnOpenHyojunka(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        return;

    ++pch;
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"HYOJUNKA.txt");
    if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopyW(pch, diff, L"..\\HYOJUNKA.txt");
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            StringCchCopyW(pch, diff, L"..\\..\\HYOJUNKA.txt");
            if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\HYOJUNKA.txt");
                if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    return;
                }
            }
        }
    }
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

void MMainWnd::OnOpenLicense(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        return;

    ++pch;
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"LICENSE.txt");
    if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopyW(pch, diff, L"..\\LICENSE.txt");
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            StringCchCopyW(pch, diff, L"..\\..\\LICENSE.txt");
            if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                StringCchCopyW(pch, diff, L"..\\..\\..\\LICENSE.txt");
                if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    return;
                }
            }
        }
    }
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

BOOL MMainWnd::DoUpxTest(LPCWSTR pszUpx, LPCWSTR pszFile)
{
    MStringW strCmdLine;
    strCmdLine += L"\"";
    strCmdLine += pszUpx;
    strCmdLine += L"\" -t \"";
    strCmdLine += pszFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        std::string strOutput;
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            if (strOutput.find("[OK]") != std::string::npos)
            {
                bSuccess = TRUE;
            }
        }
    }

    return bSuccess;
}

BOOL MMainWnd::DoUpxExtract(LPCWSTR pszUpx, LPCWSTR pszFile)
{
    MStringW strCmdLine;
    strCmdLine += L"\"";
    strCmdLine += pszUpx;
    strCmdLine += L"\" -d \"";
    strCmdLine += pszFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        std::string strOutput;
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            if (strOutput.find("Unpacked") != std::string::npos)
                bSuccess = TRUE;
        }
    }

    return bSuccess;
}

void MMainWnd::OnDebugTreeNode(HWND hwnd)
{
    if (0)
    {
        TCHAR szMsg[256];
        FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
            GetModuleHandle(NULL),
            MSGID_HELLO, 0,
            szMsg, _countof(szMsg), NULL);
        MessageBox(hwnd, szMsg, TEXT("MSGID_HELLO"), MB_ICONINFORMATION);
        FormatMessage(
            FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
            GetModuleHandle(NULL),
            MSGID_SAMPLE, 0,
            szMsg, _countof(szMsg), NULL);
        MessageBox(hwnd, szMsg, TEXT("MSGID_SAMPLE"), MB_ICONINFORMATION);
    }
    else
    {
        LPARAM lParam = TV_GetParam(m_hTreeView);
        WORD i = LOWORD(lParam);
        WORD k = HIWORD(lParam);

        static const LPCWSTR apszI_[] =
        {
            L"I_NONE",
            L"I_TYPE",
            L"I_NAME",
            L"I_LANG",
            L"I_STRING",
            L"I_MESSAGE"
        };

        const ResEntry& entry = m_entries[i];

        WCHAR sz[64];
        MStringW type = entry.type.str();
        MStringW name = entry.name.str();
        StringCchPrintfW(sz, _countof(sz),
            L"%d, %s: type:%s, name:%s, lang:0x%04X", i, apszI_[k],
            type.c_str(), name.c_str(), entry.lang);
        MsgBoxDx(sz, MB_ICONINFORMATION);
    }
}

void MMainWnd::ShowMovie(BOOL bShow/* = TRUE*/)
{
    if (bShow)
    {
        ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
        ShowWindow(m_hSrcEdit, SW_HIDE);
        m_splitter3.SetPaneCount(1);
        m_splitter3.SetPane(0, m_hBmpView);
    }
    else
    {
        ShowBmpView(FALSE);
    }
}

void MMainWnd::ShowBmpView(BOOL bShow/* = TRUE*/)
{
    ShowWindow(m_hSrcEdit, SW_SHOWNOACTIVATE);
    if (bShow)
    {
        ShowWindow(m_hBmpView, SW_SHOWNOACTIVATE);
        m_splitter3.SetPaneCount(2);
        m_splitter3.SetPane(0, m_hSrcEdit);
        m_splitter3.SetPane(1, m_hBmpView);
        m_splitter3.SetPaneExtent(1, m_settings.nBmpViewWidth);
    }
    else
    {
        if (m_splitter3.GetPaneCount() >= 2)
            m_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
        ShowWindow(m_hBmpView, SW_HIDE);
        m_splitter3.SetPaneCount(1);
        m_splitter3.SetPane(0, m_hSrcEdit);
    }
    SendMessageW(m_hBmpView, WM_COMMAND, 999, 0);
}

void MMainWnd::ShowStatusBar(BOOL bShow/* = TRUE*/)
{
    if (bShow)
        ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hStatusBar, SW_HIDE);
}

void MMainWnd::ShowBinEdit(BOOL bShow/* = TRUE*/, BOOL bShowError/* = FALSE*/)
{
    if (bShow && (m_settings.bShowBinEdit || bShowError))
    {
        ShowWindow(m_hBinEdit, SW_SHOWNOACTIVATE);
        m_splitter2.SetPaneCount(2);
        m_splitter2.SetPane(0, m_splitter3);
        m_splitter2.SetPane(1, m_hBinEdit);
        m_splitter2.SetPaneExtent(1, m_settings.nBinEditHeight);
    }
    else
    {
        if (m_splitter2.GetPaneCount() >= 2)
            m_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
        ShowWindow(m_hBinEdit, SW_HIDE);
        m_splitter2.SetPaneCount(1);
        m_splitter2.SetPane(0, m_splitter3);
    }
}

void MMainWnd::OnMove(HWND hwnd, int x, int y)
{
    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (!IsZoomed(hwnd) && !IsIconic(hwnd))
    {
        m_settings.nWindowLeft = rc.left;
        m_settings.nWindowTop = rc.top;
    }
}

void MMainWnd::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    SendMessageW(m_hToolBar, TB_AUTOSIZE, 0, 0);
    SendMessageW(m_hStatusBar, WM_SIZE, 0, 0);

    RECT rc, ClientRect;

    if (IsZoomed(hwnd))
    {
        m_settings.bMaximized = TRUE;
    }
    else if (!IsIconic(hwnd))
    {
        GetWindowRect(hwnd, &rc);
        m_settings.nWindowWidth = rc.right - rc.left;
        m_settings.nWindowHeight = rc.bottom - rc.top;
        m_settings.bMaximized = FALSE;
    }

    GetClientRect(hwnd, &ClientRect);
    SIZE sizClient = SizeFromRectDx(&ClientRect);

    INT x = 0, y = 0;
    if (::IsWindowVisible(m_hToolBar))
    {
        GetWindowRect(m_hToolBar, &rc);
        y += rc.bottom - rc.top;
        sizClient.cy -= rc.bottom - rc.top;
    }
    if (::IsWindowVisible(m_hStatusBar))
    {
        INT anWidths[] = { ClientRect.right - CX_STATUS_PART, -1 };
        SendMessage(m_hStatusBar, SB_SETPARTS, 2, (LPARAM)anWidths);
        GetWindowRect(m_hStatusBar, &rc);
        sizClient.cy -= rc.bottom - rc.top;
    }

    MoveWindow(m_splitter1, x, y, sizClient.cx, sizClient.cy, TRUE);
}

void MMainWnd::OnInitMenu(HWND hwnd, HMENU hMenu)
{
    CheckMenuItem(hMenu, ID_USEOLDLANGSTMT, MF_BYCOMMAND | MF_CHECKED);

    if (m_settings.bShowToolBar)
        CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_SHOWHIDETOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);

    BOOL bCanEditLabel = TRUE;
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) == I_TYPE)
    {
        bCanEditLabel = FALSE;
    }

    if (bCanEditLabel)
    {
        UINT i = LOWORD(lParam);
        if (i < m_entries.size())
        {
            ResEntry& entry = m_entries[i];

            if (HIWORD(lParam) == I_NAME || HIWORD(lParam) == I_LANG)
            {
                if (entry.type == RT_STRING || entry.type == RT_MESSAGETABLE)
                {
                    bCanEditLabel = FALSE;
                }
            }
        }
        else
        {
            bCanEditLabel = FALSE;
        }
    }

    if (bCanEditLabel)
        EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_ENABLED);
    else
        EnableMenuItem(hMenu, ID_EDITLABEL, MF_BYCOMMAND | MF_GRAYED);

    if (IsWindowVisible(m_hStatusBar))
        CheckMenuItem(hMenu, ID_STATUSBAR, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_STATUSBAR, MF_UNCHECKED);

    if (IsWindowVisible(m_hBinEdit))
        CheckMenuItem(hMenu, ID_BINARYPANE, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_BINARYPANE, MF_UNCHECKED);

    if (m_settings.bAlwaysControl)
        CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_ALWAYSCONTROL, MF_UNCHECKED);

    if (!m_db.AreMacroIDShown())
        CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_HIDEIDMACROS, MF_UNCHECKED);

    if (m_db.DoesUseIDC_STATIC())
        CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_CHECKED);
    else
        CheckMenuItem(hMenu, ID_USEIDC_STATIC, MF_UNCHECKED);

    if (GetWindowTextLength(m_hSrcEdit) == 0 ||
        !IsWindowVisible(m_hSrcEdit))
    {
        EnableMenuItem(hMenu, ID_FIND, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hMenu, ID_FIND, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDDOWNWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_FINDUPWARD, MF_ENABLED);
        EnableMenuItem(hMenu, ID_REPLACE, MF_ENABLED);
    }

    HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
    if (hItem == NULL)
    {
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
        EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        return;
    }

    TV_ITEM Item;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = TVIF_PARAM;
    Item.hItem = hItem;
    TreeView_GetItem(m_hTreeView, &Item);

    UINT i = LOWORD(Item.lParam);
    const ResEntry& entry = m_entries[i];

    lParam = TV_GetParam(m_hTreeView);
    BOOL bEditable = IsEditableEntry(hwnd, lParam);
    if (bEditable)
    {
        EnableMenuItem(hMenu, ID_EDIT, MF_ENABLED);
        if (Res_CanGuiEdit(entry.type))
        {
            EnableMenuItem(hMenu, ID_GUIEDIT, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
        }
    }
    else
    {
        EnableMenuItem(hMenu, ID_EDIT, MF_GRAYED);
        EnableMenuItem(hMenu, ID_GUIEDIT, MF_GRAYED);
    }

    switch (HIWORD(Item.lParam))
    {
    case I_TYPE:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        break;
    case I_NAME:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_ENABLED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        break;
    case I_LANG:
        if (entry.type == RT_GROUP_ICON || entry.type == RT_ICON ||
            entry.type == RT_ANIICON)
        {
            EnableMenuItem(hMenu, ID_EXTRACTICON, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        }
        if (entry.type == RT_GROUP_ICON || entry.type == RT_ANIICON)
        {
            EnableMenuItem(hMenu, ID_REPLACEICON, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        }

        if (entry.type == RT_BITMAP)
        {
            EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_ENABLED);
            EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
            EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        }

        if (entry.type == RT_GROUP_CURSOR || entry.type == RT_CURSOR ||
            entry.type == RT_ANICURSOR)
        {
            EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        }
        if (entry.type == RT_GROUP_CURSOR || entry.type == RT_ANICURSOR)
        {
            EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        }

        if (entry.type == RT_DIALOG || entry.type == RT_MENU)
        {
            EnableMenuItem(hMenu, ID_TEST, MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        }

        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_ENABLED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        if (entry.type == RT_STRING || entry.type == RT_MESSAGETABLE)
            EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        else
            EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
        break;
    case I_STRING: case I_MESSAGE:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_ENABLED);
        EnableMenuItem(hMenu, ID_TEST, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_ENABLED);
        break;
    default:
        EnableMenuItem(hMenu, ID_REPLACEICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACECURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_REPLACEBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTICON, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTCURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBITMAP, MF_GRAYED);
        EnableMenuItem(hMenu, ID_EXTRACTBIN, MF_GRAYED);
        EnableMenuItem(hMenu, ID_DELETERES, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWNAME, MF_GRAYED);
        EnableMenuItem(hMenu, ID_COPYASNEWLANG, MF_GRAYED);
        break;
    }
}

void MMainWnd::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    if (hwndContext != m_hTreeView)
        return;

    if (IsWindowVisible(m_rad_window))
    {
        DestroyWindow(m_rad_window);
    }

    POINT pt = {(INT)xPos, (INT)yPos};
    HTREEITEM hItem;
    if (xPos == -1 && yPos == -1)
    {
        hItem = TreeView_GetSelection(hwndContext);

        RECT rc;
        TreeView_GetItemRect(hwndContext, hItem, &rc, FALSE);
        pt.x = (rc.left + rc.right) / 2;
        pt.y = (rc.top + rc.bottom) / 2;
    }
    else
    {
        ScreenToClient(hwndContext, &pt);

        TV_HITTESTINFO HitTest;
        ZeroMemory(&HitTest, sizeof(HitTest));
        HitTest.pt = pt;
        TreeView_HitTest(hwndContext, &HitTest);

        hItem = HitTest.hItem;
    }

    TreeView_SelectItem(hwndContext, hItem);

    HMENU hMenu = LoadMenuW(m_hInst, MAKEINTRESOURCEW(IDR_POPUPMENUS));
    OnInitMenu(hwnd, hMenu);
    HMENU hSubMenu = ::GetSubMenu(hMenu, 0);
    if (hMenu == NULL || hSubMenu == NULL)
        return;

    ClientToScreen(hwndContext, &pt);

    SetForegroundWindow(hwndContext);
    INT id;
    UINT Flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
    id = TrackPopupMenu(hSubMenu, Flags, pt.x, pt.y, 0,
                        hwndContext, NULL);
    PostMessageW(hwndContext, WM_NULL, 0, 0);
    DestroyMenu(hMenu);

    if (id)
    {
        SendMessageW(hwnd, WM_COMMAND, id, 0);
    }
}

void MMainWnd::PreviewIcon(HWND hwnd, const ResEntry& entry)
{
    BITMAP bm;
    m_hBmpView.SetBitmap(CreateBitmapFromIconOrPngDx(hwnd, entry, bm));

    MStringW str;
    HICON hIcon = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, TRUE);
    if (hIcon)
    {
        str = DumpIconInfo(bm, TRUE);
    }
    else
    {
        str = DumpBitmapInfo(m_hBmpView.m_hBitmap);
    }
    DestroyIcon(hIcon);

    SetWindowTextW(m_hSrcEdit, str.c_str());

    ShowBmpView(TRUE);
}

void MMainWnd::PreviewCursor(HWND hwnd, const ResEntry& entry)
{
    BITMAP bm;
    HCURSOR hCursor = PackedDIB_CreateIcon(&entry[0], entry.size(), bm, FALSE);
    m_hBmpView.SetBitmap(CreateBitmapFromIconDx(hCursor, bm.bmWidth, bm.bmHeight, TRUE));
    MStringW str = DumpIconInfo(bm, FALSE);
    DestroyCursor(hCursor);

    SetWindowTextW(m_hSrcEdit, str.c_str());

    ShowBmpView(TRUE);
}

void MMainWnd::PreviewGroupIcon(HWND hwnd, const ResEntry& entry)
{
    m_hBmpView.SetBitmap(CreateBitmapFromIconsDx(hwnd, m_entries, entry));

    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    ShowBmpView(TRUE);
}

void MMainWnd::PreviewGroupCursor(HWND hwnd, const ResEntry& entry)
{
    m_hBmpView.SetBitmap(CreateBitmapFromCursorsDx(hwnd, m_entries, entry));
    assert(m_hBmpView);

    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    ShowBmpView(TRUE);
}

void MMainWnd::PreviewBitmap(HWND hwnd, const ResEntry& entry)
{
    HBITMAP hbm = PackedDIB_CreateBitmapFromMemory(&entry[0], entry.size());
    m_hBmpView.SetBitmap(hbm);
    ShowBmpView(TRUE);

    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

void MMainWnd::PreviewImage(HWND hwnd, const ResEntry& entry)
{
    MStringW str;

    ResToText res2text(m_settings, m_db, m_entries);
    str += res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    m_hBmpView.SetImage(&entry[0], entry.size());
    ShowBmpView(TRUE);
}

void MMainWnd::PreviewWAVE(HWND hwnd, const ResEntry& entry)
{
    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    m_hBmpView.SetPlay();
    ShowBmpView(TRUE);
}

void MMainWnd::PreviewAVI(HWND hwnd, const ResEntry& entry)
{
    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());

    m_hBmpView.SetMedia(&entry[0], entry.size());
    ShowMovie(TRUE);
}

void MMainWnd::PreviewAccel(HWND hwnd, const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    AccelRes accel(m_db);
    if (accel.LoadFromStream(stream))
    {
        MString str = GetLanguageStatement(entry.lang);
        str += accel.Dump(entry.name);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

void MMainWnd::PreviewMessage(HWND hwnd, const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    MessageRes mes;
    WORD nNameID = entry.name.m_id;
    if (mes.LoadFromStream(stream, nNameID))
    {
        MStringW str = mes.Dump(m_db, nNameID);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

void MMainWnd::PreviewString(HWND hwnd, const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    StringRes str_res;
    WORD nNameID = entry.name.m_id;
    if (str_res.LoadFromStream(stream, nNameID))
    {
        MStringW str = str_res.Dump(m_db, nNameID);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

void MMainWnd::PreviewHtml(HWND hwnd, const ResEntry& entry)
{
    MTextType type;
    type.nNewLine = MNEWLINE_CRLF;
    MStringW str = mstr_from_bin(&entry.data[0], entry.data.size(), &type);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

void MMainWnd::PreviewMenu(HWND hwnd, const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    MenuRes menu_res;
    if (menu_res.LoadFromStream(stream))
    {
        MString str = GetLanguageStatement(entry.lang);
        str += menu_res.Dump(entry.name, m_db);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

void MMainWnd::PreviewVersion(HWND hwnd, const ResEntry& entry)
{
    VersionRes ver_res;
    if (ver_res.LoadFromData(entry.data))
    {
        MString str = GetLanguageStatement(entry.lang);
        str += ver_res.Dump(entry.name);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

void MMainWnd::PreviewUnknown(HWND hwnd, const ResEntry& entry)
{
    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

void MMainWnd::PreviewRCData(HWND hwnd, const ResEntry& entry)
{
    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

void MMainWnd::PreviewDlgInit(HWND hwnd, const ResEntry& entry)
{
    ResToText res2text(m_settings, m_db, m_entries);
    MString str = res2text.DumpEntry(entry);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

void MMainWnd::PreviewDialog(HWND hwnd, const ResEntry& entry)
{
    MByteStreamEx stream(entry.data);
    DialogRes dialog_res(m_db);
    if (dialog_res.LoadFromStream(stream))
    {
        MString str = GetLanguageStatement(entry.lang);
        str += dialog_res.Dump(entry.name, !!m_settings.bAlwaysControl);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
}

void MMainWnd::PreviewAniIcon(HWND hwnd, const ResEntry& entry, BOOL bIcon)
{
    HICON hIcon = NULL;

    {
        WCHAR szPath[MAX_PATH], szTempFile[MAX_PATH];
        GetTempPathW(_countof(szPath), szPath);
        GetTempFileNameW(szPath, L"ani", 0, szTempFile);

        MFile file;
        DWORD cbWritten = 0;
        if (file.OpenFileForOutput(szTempFile) &&
            file.WriteFile(&entry[0], entry.size(), &cbWritten))
        {
            file.CloseHandle();
            if (bIcon)
            {
                hIcon = (HICON)LoadImage(NULL, szTempFile, IMAGE_ICON,
                    0, 0, LR_LOADFROMFILE);
            }
            else
            {
                hIcon = (HICON)LoadImage(NULL, szTempFile, IMAGE_CURSOR,
                    0, 0, LR_LOADFROMFILE);
            }
        }
        DeleteFileW(szTempFile);
    }

    if (hIcon)
    {
        m_hBmpView.SetIcon(hIcon, bIcon);

        ResToText res2text(m_settings, m_db, m_entries);
        MString str = res2text.DumpEntry(entry);
        SetWindowTextW(m_hSrcEdit, str.c_str());
    }
    else
    {
        m_hBmpView.DestroyView();
    }
    ShowBmpView(TRUE);
}

void MMainWnd::PreviewStringTable(HWND hwnd, const ResEntry& entry)
{
    ResEntries found;
    Res_Search(found, m_entries, RT_STRING, (WORD)0, entry.lang);

    StringRes str_res;
    ResEntries::iterator it, end = found.end();
    for (it = found.begin(); it != end; ++it)
    {
        MByteStreamEx stream(it->data);
        if (!str_res.LoadFromStream(stream, it->name.m_id))
            return;
    }

    MString str = GetLanguageStatement(entry.lang);
    str += str_res.Dump(m_db);
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

void MMainWnd::PreviewMessageTable(HWND hwnd, const ResEntry& entry)
{
    ResEntries found;
    Res_Search(found, m_entries, RT_MESSAGETABLE, (WORD)0, entry.lang);

    MessageRes msg_res;
    ResEntries::iterator it, end = found.end();
    for (it = found.begin(); it != end; ++it)
    {
        MByteStreamEx stream(it->data);
        if (!msg_res.LoadFromStream(stream, it->name.m_id))
            return;
    }

    MString str;
    str += GetLanguageStatement(entry.lang);
    str += L"#ifdef APSTUDIO_INVOKED\r\n";
    str += L"    #error Ap Studio cannot edit this message table.\r\n";
    str += L"#endif\r\n";
    str += L"#ifdef MCDX_INVOKED\r\n";
    str += msg_res.Dump(m_db);
    str += L"#endif\r\n\r\n";
    SetWindowTextW(m_hSrcEdit, str.c_str());
}

VOID MMainWnd::HidePreview(HWND hwnd)
{
    m_hBmpView.DestroyView();

    SetWindowTextW(m_hBinEdit, NULL);
    Edit_SetModify(m_hBinEdit, FALSE);

    SetWindowTextW(m_hSrcEdit, NULL);
    Edit_SetModify(m_hSrcEdit, FALSE);

    ShowBmpView(FALSE);

    PostMessageDx(WM_SIZE);
}

BOOL MMainWnd::Preview(HWND hwnd, const ResEntry& entry)
{
    HidePreview(hwnd);

    MStringW str = DumpDataAsString(entry.data);
    SetWindowTextW(m_hBinEdit, str.c_str());

    BOOL bEditable = FALSE;
    if (entry.type.m_id != 0)
    {
        WORD wType = entry.type.m_id;
        if (wType == (WORD)(UINT_PTR)RT_ICON)
        {
            PreviewIcon(hwnd, entry);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_CURSOR)
        {
            PreviewCursor(hwnd, entry);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
        {
            PreviewGroupIcon(hwnd, entry);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
        {
            PreviewGroupCursor(hwnd, entry);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_BITMAP)
        {
            PreviewBitmap(hwnd, entry);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
        {
            PreviewAccel(hwnd, entry);
            bEditable = TRUE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_STRING)
        {
            PreviewString(hwnd, entry);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_MENU)
        {
            PreviewMenu(hwnd, entry);
            bEditable = TRUE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_DIALOG)
        {
            PreviewDialog(hwnd, entry);
            bEditable = TRUE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_ANIICON)
        {
            PreviewAniIcon(hwnd, entry, TRUE);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_ANICURSOR)
        {
            PreviewAniIcon(hwnd, entry, FALSE);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
        {
            PreviewMessage(hwnd, entry);
            bEditable = FALSE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_MANIFEST || wType == (WORD)(UINT_PTR)RT_HTML)
        {
            PreviewHtml(hwnd, entry);
            bEditable = TRUE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_VERSION)
        {
            PreviewVersion(hwnd, entry);
            bEditable = TRUE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_RCDATA)
        {
            PreviewRCData(hwnd, entry);
            bEditable = TRUE;
        }
        else if (wType == (WORD)(UINT_PTR)RT_DLGINIT)
        {
            PreviewDlgInit(hwnd, entry);
            bEditable = TRUE;
        }
        else
        {
            PreviewUnknown(hwnd, entry);
            bEditable = FALSE;
        }
    }
    else
    {
        if (entry.type == L"PNG" || entry.type == L"GIF" ||
            entry.type == L"JPEG" || entry.type == L"TIFF" ||
            entry.type == L"JPG" || entry.type == L"TIF" ||
            entry.type == L"EMF" || entry.type == L"ENHMETAFILE" ||
            entry.type == L"WMF" || entry.type == L"IMAGE")
        {
            PreviewImage(hwnd, entry);
            bEditable = FALSE;
        }
        else if (entry.type == L"WAVE")
        {
            PreviewWAVE(hwnd, entry);
            bEditable = FALSE;
        }
        else if (entry.type == L"AVI")
        {
            PreviewAVI(hwnd, entry);
            bEditable = FALSE;
        }
        else
        {
            PreviewUnknown(hwnd, entry);
            bEditable = FALSE;
        }
    }

    PostMessageW(hwnd, WM_SIZE, 0, 0);
    return bEditable;
}

BOOL MMainWnd::CreateOurToolBar(HWND hwndParent)
{
    HWND hwndTB;
    hwndTB = CreateWindowW(TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_WRAPABLE | TBSTYLE_LIST,
        0, 0, 0, 0, hwndParent, (HMENU)1, GetWindowInstance(hwndParent), NULL);
    if (hwndTB == NULL)
        return FALSE;

    SendMessageW(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(0, 0));

    ToolBar_StoreStrings(hwndTB, _countof(g_buttons0), g_buttons0);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons1), g_buttons1);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons2), g_buttons2);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons3), g_buttons3);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons4), g_buttons4);
    ToolBar_StoreStrings(hwndTB, _countof(g_buttons5), g_buttons5);

    m_hToolBar = hwndTB;
    UpdateOurToolBar(3);

    return TRUE;
}

void MMainWnd::UpdateOurToolBar(INT iType)
{
    while (SendMessageW(m_hToolBar, TB_DELETEBUTTON, 0, 0))
        ;

    switch (iType)
    {
    case 0:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons0), (LPARAM)g_buttons0);
        break;
    case 1:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons1), (LPARAM)g_buttons1);
        break;
    case 2:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons2), (LPARAM)g_buttons2);
        break;
    case 3:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons3), (LPARAM)g_buttons3);
        break;
    case 4:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons4), (LPARAM)g_buttons4);
        break;
    case 5:
        SendMessageW(m_hToolBar, TB_ADDBUTTONS, _countof(g_buttons5), (LPARAM)g_buttons5);
        break;
    }

    if (m_settings.bShowToolBar)
        ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hToolBar, SW_HIDE);
}

void MMainWnd::SelectTV(HWND hwnd, LPARAM lParam, BOOL DoubleClick)
{
    HidePreview(hwnd);

    if (lParam == 0)
        lParam = TV_GetParam(m_hTreeView);

    WORD i = LOWORD(lParam);
    if (m_entries.size() <= i)
    {
        UpdateOurToolBar(3);
        return;
    }

    ResEntry& entry = m_entries[i];
    m_type = entry.type;
    m_name = entry.name;
    m_lang = entry.lang;

    BOOL bEditable, bSelectNone = FALSE;
    if (HIWORD(lParam) == I_LANG)
    {
        bEditable = Preview(hwnd, entry);
        ShowBinEdit(TRUE);
    }
    else if (HIWORD(lParam) == I_STRING)
    {
        m_hBmpView.DestroyView();
        SetWindowTextW(m_hBinEdit, NULL);
        PreviewStringTable(hwnd, entry);
        ShowBinEdit(FALSE);
        bEditable = TRUE;
        m_hBmpView.DeleteTempFile();
    }
    else if (HIWORD(lParam) == I_MESSAGE)
    {
        m_hBmpView.DestroyView();
        SetWindowTextW(m_hBinEdit, NULL);
        PreviewMessageTable(hwnd, entry);
        ShowBinEdit(FALSE);
        bEditable = TRUE;
        m_hBmpView.DeleteTempFile();
    }
    else
    {
        m_hBmpView.DestroyView();
        ShowBinEdit(FALSE);
        bEditable = FALSE;
        bSelectNone = TRUE;
        m_hBmpView.DeleteTempFile();
    }

    if (bEditable)
    {
        Edit_SetReadOnly(m_hSrcEdit, FALSE);

        if (Edit_GetModify(m_hSrcEdit))
        {
            UpdateOurToolBar(2);
        }
        else if (Res_IsTestable(entry.type))
        {
            UpdateOurToolBar(0);
        }
        else if (Res_CanGuiEdit(entry.type))
        {
            UpdateOurToolBar(4);
        }
        else
        {
            UpdateOurToolBar(3);
        }
    }
    else
    {
        Edit_SetReadOnly(m_hSrcEdit, TRUE);

        UpdateOurToolBar(3);
    }

    PostMessageDx(WM_SIZE);
}

BOOL MMainWnd::IsEditableEntry(HWND hwnd, LPARAM lParam)
{
    const WORD i = LOWORD(lParam);
    if (m_entries.size() <= i)
        return FALSE;

    const ResEntry& entry = m_entries[i];
    const MIdOrString& type = entry.type;
    switch (HIWORD(lParam))
    {
    case I_LANG:
        if (type == RT_ACCELERATOR || type == RT_DIALOG || type == RT_HTML ||
            type == RT_MANIFEST || type == RT_MENU || type == RT_VERSION ||
            type == RT_DLGINIT)
        {
            ;
        }
        else
        {
            return FALSE;
        }
        break;
    case I_STRING: case I_MESSAGE:
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL MMainWnd::CareWindresResult(HWND hwnd, ResEntries& entries, MStringA& msg)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    WORD i = LOWORD(lParam);
    if (m_entries.size() <= i)
        return FALSE;

    if (HIWORD(lParam) == I_LANG)
    {
        ResEntry& entry = m_entries[i];

        MIdOrString name = entry.name;
        if (name.is_str() && m_db.HasResID(name.str()))
        {
            name = (WORD)m_db.GetResIDValue(name.c_str());
        }

        if (entries.size() != 1 ||
            entries[0].name != name ||
            entries[0].lang != entry.lang)
        {
            msg += MWideToAnsi(CP_ACP, LoadStringDx(IDS_RESMISMATCH));
            return FALSE;
        }
        entry = entries[0];
        return TRUE;
    }
    else if (HIWORD(lParam) == I_STRING)
    {
        ResEntry entry = m_entries[i];

        Res_DeleteNames(m_entries, RT_STRING, entry.lang);

        for (size_t m = 0; m < entries.size(); ++m)
        {
            if (!Res_AddEntry(m_entries, entries[m], TRUE))
            {
                msg += MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTADDRES));
                return FALSE;
            }
        }

        return TRUE;
    }
    else if (HIWORD(lParam) == I_MESSAGE)
    {
        ResEntry entry = m_entries[i];

        Res_DeleteNames(m_entries, RT_MESSAGETABLE, entry.lang);

        for (size_t m = 0; m < entries.size(); ++m)
        {
            if (!Res_AddEntry(m_entries, entries[m], TRUE))
            {
                msg += MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTADDRES));
                return FALSE;
            }
        }

        return TRUE;
    }
    else
    {
        // FIXME
        return TRUE;
    }
}

MStringW MMainWnd::GetMacroDump()
{
    MStringW ret;
    macro_map_type::const_iterator it, end = m_settings.macros.end();
    for (it = m_settings.macros.begin(); it != end; ++it)
    {
        ret += L" -D";
        ret += it->first;
        if (it->second.size())
        {
            ret += L"=";
            ret += it->second;
        }
    }
    ret += L" ";
    return ret;
}

MStringW MMainWnd::GetIncludesDump()
{
    MStringW ret;
    for (size_t i = 0; i < m_settings.includes.size(); ++i)
    {
        const MStringW& str = m_settings.includes[i];
        if (str.empty())
            continue;
        
        ret += L" -I";
        ret += str;
    }
    ret += L" ";
    return ret;
}

BOOL MMainWnd::CompileMessageTable(HWND hwnd, const MStringW& strWide)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    WORD i = LOWORD(lParam);
    if (m_entries.size() <= i)
        return FALSE;

    ResEntry& entry = m_entries[i];
    if (entry.type != RT_MESSAGETABLE)
        return FALSE;

    MStringA strUtf8;
    strUtf8 = MWideToAnsi(CP_UTF8, strWide);

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

    // Source file #1
    lstrcpynW(szPath1, GetTempFileNameDx(L"R1"), MAX_PATH);
    MFile r1(szPath1, TRUE);

    // Source file #2 (#included)
    lstrcpynW(szPath2, GetTempFileNameDx(L"R2"), MAX_PATH);
    MFile r2(szPath2, TRUE);

    // Output resource object file (imported)
    lstrcpynW(szPath3, GetTempFileNameDx(L"R3"), MAX_PATH);
    MFile r3(szPath3, TRUE);
    r3.CloseHandle();

    if (m_szResourceH[0])
        r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
    r1.WriteFormatA("#include <windows.h>\r\n");
    r1.WriteFormatA("#include <commctrl.h>\r\n");
    r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n",
                    PRIMARYLANGID(entry.lang), SUBLANGID(entry.lang));
    r1.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n");
    r1.WriteFormatA("#include \"%S\"\r\n", szPath2);
    r1.CloseHandle();

    DWORD cbWrite = DWORD(strUtf8.size() * sizeof(char));
    DWORD cbWritten;
    r2.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
    r2.CloseHandle();

    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szMcdxExe;
    strCmdLine += L"\" ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res \"";
    strCmdLine += szPath1;
    strCmdLine += L'\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    BOOL bSuccess = FALSE;
    std::string strOutput;
    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            ResEntries entries;
            if (DoImport(hwnd, szPath3, entries))
            {
                MStringA msg;
                bSuccess = CareWindresResult(hwnd, entries, msg);
                if (msg.size())
                {
                    strOutput.append(msg);
                }
            }
        }
    }
    else
    {
        MStringA msg;
        strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
    }

    if (!bSuccess)
    {
        if (strOutput.empty())
        {
            SetWindowTextW(m_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
        }
        else
        {
            SetWindowTextA(m_hBinEdit, (char *)&strOutput[0]);
        }
        ShowBinEdit(TRUE, TRUE);
#ifdef NDEBUG
        DeleteFileW(szPath1);
        DeleteFileW(szPath2);
        DeleteFileW(szPath3);
#endif
    }
    else
    {
        DeleteFileW(szPath1);
        DeleteFileW(szPath2);
        DeleteFileW(szPath3);
    }

    PostMessageW(hwnd, WM_SIZE, 0, 0);

    return bSuccess;
}

BOOL MMainWnd::CompileParts(HWND hwnd, const MStringW& strWide, BOOL bReopen)
{
    LPARAM lParam = TV_GetParam(m_hTreeView);
    WORD i = LOWORD(lParam);
    if (m_entries.size() <= i)
        return FALSE;

    ResEntry& entry = m_entries[i];
    if (entry.type == RT_MESSAGETABLE)
    {
        return CompileMessageTable(hwnd, strWide);
    }

    MStringA strUtf8;
    strUtf8 = MWideToAnsi(CP_UTF8, strWide);
    if (HIWORD(lParam) == I_LANG)
    {
        if (Res_IsPlainText(entry.type))
        {
            if (strWide.find(L"\"UTF-8\"") != MStringW::npos)
            {
                entry.data.assign(strUtf8.begin(), strUtf8.end());

                static const BYTE bom[] = {0xEF, 0xBB, 0xBF, 0};
                entry.data.insert(entry.data.begin(), &bom[0], &bom[3]);
            }
            else
            {
                MStringA TextAnsi;
                TextAnsi = MWideToAnsi(CP_ACP, strWide);
                entry.data.assign(TextAnsi.begin(), TextAnsi.end());
            }
            SelectTV(hwnd, lParam, FALSE);

            return TRUE;    // success
        }
    }

    WCHAR szPath1[MAX_PATH], szPath2[MAX_PATH], szPath3[MAX_PATH];

    // Source file #1
    lstrcpynW(szPath1, GetTempFileNameDx(L"R1"), MAX_PATH);
    MFile r1(szPath1, TRUE);

    // Source file #2 (#included)
    lstrcpynW(szPath2, GetTempFileNameDx(L"R2"), MAX_PATH);
    MFile r2(szPath2, TRUE);

    // Output resource object file (imported)
    lstrcpynW(szPath3, GetTempFileNameDx(L"R3"), MAX_PATH);
    MFile r3(szPath3, TRUE);
    r3.CloseHandle();

    if (m_szResourceH[0])
        r1.WriteFormatA("#include \"%s\"\r\n", MWideToAnsi(CP_ACP, m_szResourceH).c_str());
    r1.WriteFormatA("#include <windows.h>\r\n");
    r1.WriteFormatA("#include <commctrl.h>\r\n");
    r1.WriteFormatA("LANGUAGE 0x%04X, 0x%04X\r\n",
                    PRIMARYLANGID(entry.lang), SUBLANGID(entry.lang));
    r1.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n");

    id_map_type::iterator it, end = m_settings.id_map.end();
    for (it = m_settings.id_map.begin(); it != end; ++it)
    {
        if (it->first == "IDC_STATIC")
        {
            r1.WriteFormatA("#undef IDC_STATIC\r\n");
            r1.WriteFormatA("#define IDC_STATIC -1\r\n");
        }
        else
        {
            r1.WriteFormatA("#undef %s\r\n", it->first.c_str());
            r1.WriteFormatA("#define %s %s\r\n", it->first.c_str(), it->second.c_str());
        }
    }

    r1.WriteFormatA("#include \"%S\"\r\n", szPath2);
    r1.CloseHandle();

    DWORD cbWrite = DWORD(strUtf8.size() * sizeof(char));
    DWORD cbWritten;
    r2.WriteFile(strUtf8.c_str(), cbWrite, &cbWritten);
    r2.CloseHandle();

    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szWindresExe;
    strCmdLine += L"\" -DRC_INVOKED ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res -F pe-i386 --preprocessor=\"";
    strCmdLine += m_szCppExe;
    strCmdLine += L"\" --preprocessor-arg=\"\" \"";
    strCmdLine += szPath1;
    strCmdLine += '\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    BOOL bSuccess = FALSE;
    std::string strOutput;
    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            ResEntries entries;
            if (DoImport(hwnd, szPath3, entries))
            {
                MStringA msg;
                bSuccess = CareWindresResult(hwnd, entries, msg);
                if (msg.size())
                {
                    strOutput.append(msg);
                }
            }
        }
    }
    else
    {
        MStringA msg;
        strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
    }

    if (!bSuccess)
    {
        if (strOutput.empty())
        {
            SetWindowTextW(m_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
        }
        else
        {
            SetWindowTextA(m_hBinEdit, (char *)&strOutput[0]);
        }
        //MessageBoxA(NULL, strOutput.c_str(), NULL, 0);
        ShowBinEdit(TRUE, TRUE);
#ifdef NDEBUG
        DeleteFileW(szPath1);
        DeleteFileW(szPath2);
        DeleteFileW(szPath3);
#endif
    }
    else
    {
        DeleteFileW(szPath1);
        DeleteFileW(szPath2);
        DeleteFileW(szPath3);

        if (bReopen && entry.type == RT_DIALOG)
        {
            PostMessage(hwnd, MYWM_REOPENRAD, 0, 0);
        }
    }

    PostMessageW(hwnd, WM_SIZE, 0, 0);

    return bSuccess;
}

BOOL MMainWnd::ReCompileOnSelChange(HWND hwnd, BOOL bReopen/* = FALSE*/)
{
    INT cchText = ::GetWindowTextLengthW(m_hSrcEdit);
    MStringW strWide;
    strWide.resize(cchText);
    GetWindowTextW(m_hSrcEdit, &strWide[0], cchText + 1);

    if (!CompileParts(hwnd, strWide))
    {
        return FALSE;
    }

    LPARAM lParam = TV_GetParam(m_hTreeView);
    WORD i = LOWORD(lParam);
    if (m_entries.size() <= i)
        return FALSE;

    Edit_SetModify(m_hSrcEdit, FALSE);

    if (IsWindow(m_rad_window))
    {
        DestroyWindow(m_rad_window);
    }

    if (bReopen)
    {
        ResEntry& entry = m_entries[i];
        if (m_type == entry.type && m_name == entry.name &&
            m_lang == entry.lang)
        {
            if (HIWORD(lParam) == I_LANG && entry.type == RT_DIALOG)
            {
                MByteStreamEx stream(entry.data);
                m_rad_window.m_dialog_res.LoadFromStream(stream);
                m_rad_window.CreateDx(m_hwnd);
            }
        }
    }

    return TRUE;
}

BOOL MMainWnd::CompileIfNecessary(HWND hwnd, BOOL bReopen/* = FALSE*/)
{
    if (Edit_GetModify(m_hSrcEdit))
    {
        INT id = MsgBoxDx(IDS_COMPILENOW, MB_ICONINFORMATION | MB_YESNOCANCEL);
        switch (id)
        {
        case IDYES:
            return ReCompileOnSelChange(hwnd, bReopen);
        case IDNO:
            Edit_SetModify(m_hSrcEdit, FALSE);
            if (IsWindow(m_rad_window))
                DestroyWindow(m_rad_window);
            break;
        case IDCANCEL:
            return FALSE;
        }
    }
    return TRUE;
}

BOOL MMainWnd::CheckDataFolder(VOID)
{
    WCHAR szPath[MAX_PATH], *pch;
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    pch = wcsrchr(szPath, L'\\');
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"\\data");
    if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopyW(pch, diff, L"\\..\\data");
        if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            StringCchCopyW(pch, diff, L"\\..\\..\\data");
            if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                StringCchCopyW(pch, diff, L"\\..\\..\\..\\data");
                if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    StringCchCopyW(pch, diff, L"\\..\\..\\..\\..\\data");
                    if (::GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        return FALSE;
                    }
                }
            }
        }
    }
    lstrcpynW(m_szDataFolder, szPath, MAX_PATH);

    DWORD cch = GetEnvironmentVariableW(L"PATH", NULL, 0);

    MStringW env, str;
    env.resize(cch);
    GetEnvironmentVariableW(L"PATH", &env[0], cch);

    str = m_szDataFolder;
    str += L"\\bin;";
    str += env;

    SetEnvironmentVariableW(L"PATH", str.c_str());

    return TRUE;
}

INT MMainWnd::CheckData(VOID)
{
    if (!CheckDataFolder())
    {
        ErrorBoxDx(TEXT("ERROR: data folder was not found!"));
        return -1;  // failure
    }

    // Constants.txt
    StringCchCopyW(m_szConstantsFile, _countof(m_szConstantsFile), m_szDataFolder);
    StringCchCatW(m_szConstantsFile, _countof(m_szConstantsFile), L"\\Constants.txt");
    if (!m_db.LoadFromFile(m_szConstantsFile))
    {
        ErrorBoxDx(TEXT("ERROR: Unable to load Constants.txt file."));
        return -2;  // failure
    }

    // cpp.exe
    StringCchCopyW(m_szCppExe, _countof(m_szCppExe), m_szDataFolder);
    StringCchCatW(m_szCppExe, _countof(m_szCppExe), L"\\bin\\cpp.exe");
    if (::GetFileAttributesW(m_szCppExe) == INVALID_FILE_ATTRIBUTES)
    {
        ErrorBoxDx(TEXT("ERROR: No cpp.exe found."));
        return -3;  // failure
    }

    // windres.exe
    StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
    StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");
    if (::GetFileAttributesW(m_szWindresExe) == INVALID_FILE_ATTRIBUTES)
    {
        ErrorBoxDx(TEXT("ERROR: No windres.exe found."));
        return -4;  // failure
    }

    // upx.exe
    StringCchCopyW(m_szUpxExe, _countof(m_szUpxExe), m_szDataFolder);
    StringCchCatW(m_szUpxExe, _countof(m_szUpxExe), L"\\bin\\upx.exe");
    if (::GetFileAttributesW(m_szUpxExe) == INVALID_FILE_ATTRIBUTES)
    {
        ErrorBoxDx(TEXT("ERROR: No upx.exe found."));
        return -5;  // failure
    }

    // mcdx.exe
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, _countof(szPath));
    LPWSTR pch = wcsrchr(szPath, L'\\');
    size_t diff = pch - szPath;
    StringCchCopyW(pch, diff, L"\\mcdx.exe");
    if (::GetFileAttributesW(szPath) != INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopyW(m_szMcdxExe, _countof(m_szMcdxExe), szPath);
    }
    else
    {
        StringCchCopyW(m_szMcdxExe, _countof(m_szMcdxExe), m_szDataFolder);
        StringCchCatW(m_szMcdxExe, _countof(m_szMcdxExe), L"\\bin\\mcdx.exe");
        if (::GetFileAttributesW(m_szMcdxExe) == INVALID_FILE_ATTRIBUTES)
        {
            ErrorBoxDx(TEXT("ERROR: No mcdx.exe found."));
            return -6;  // failure
        }
    }

    return 0;   // success
}

void MMainWnd::DoLoadLangInfo(VOID)
{
    EnumSystemLocalesW(EnumLocalesProc, LCID_SUPPORTED);
    {
        LangEntry entry;
        entry.LangID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
        entry.str = LoadStringDx(IDS_NEUTRAL);
        g_Langs.push_back(entry);
    }
    std::sort(g_Langs.begin(), g_Langs.end());
}

BOOL MMainWnd::DoLoadFile(HWND hwnd, LPCWSTR pszFileName, DWORD nFilterIndex, BOOL bForceDecompress)
{
    MWaitCursor wait;
    WCHAR szPath[MAX_PATH], szResolvedPath[MAX_PATH], *pchPart;

    if (GetPathOfShortcutDx(hwnd, pszFileName, szResolvedPath))
    {
        GetFullPathNameW(szResolvedPath, _countof(szPath), szPath, &pchPart);
    }
    else
    {
        GetFullPathNameW(pszFileName, _countof(szPath), szPath, &pchPart);
    }

    LPWSTR pch = wcsrchr(szPath, L'.');
    if (nFilterIndex == 0 || nFilterIndex == 1)
    {
        if (pch && lstrcmpiW(pch, L".res") == 0)
            nFilterIndex = 3;
        else if (pch && lstrcmpiW(pch, L".rc") == 0)
            nFilterIndex = 4;
    }

    if (nFilterIndex == 3)
    {
        // .res files
        ResEntries entries;
        if (!DoImport(hwnd, szPath, entries))
        {
            ErrorBoxDx(IDS_CANNOTOPEN);
            return FALSE;
        }

        UnloadResourceH(hwnd, FALSE);
        if (m_settings.bAutoLoadNearbyResH)
            CheckResourceH(hwnd, szPath);

        m_bLoading = TRUE;
        {
            m_bUpxCompressed = FALSE;
            m_entries = entries;
            TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        }
        m_bLoading = FALSE;

        SetFilePath(hwnd, szPath, NULL);

        if (m_szResourceH[0] && m_settings.bAutoShowIDList)
        {
            ShowIDList(hwnd, TRUE);
        }
        DoRefresh(hwnd);

        return TRUE;
    }

    if (nFilterIndex == 4)
    {
        // .rc files
        ResEntries entries;
        if (!DoLoadRC(hwnd, szPath, entries))
        {
            ErrorBoxDx(IDS_CANNOTOPEN);
            return FALSE;
        }

        UnloadResourceH(hwnd, FALSE);
        if (m_settings.bAutoLoadNearbyResH)
            CheckResourceH(hwnd, szPath);

        m_bLoading = TRUE;
        {
            m_bUpxCompressed = FALSE;
            m_entries = entries;
            TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        }
        m_bLoading = FALSE;

        SetFilePath(hwnd, szPath, NULL);

        if (m_szResourceH[0] && m_settings.bAutoShowIDList)
        {
            ShowIDList(hwnd, TRUE);
        }
        DoRefresh(hwnd);

        return TRUE;
    }

    if (m_szUpxTempFile[0])
    {
        DeleteFileW(m_szUpxTempFile);
        m_szUpxTempFile[0] = 0;
    }

    LPWSTR pszReal = szPath;
    LPWSTR pszNominal = szPath;
    m_bUpxCompressed = DoUpxTest(m_szUpxExe, pszReal);
    if (m_bUpxCompressed)
    {
        LPWSTR szMsg = LoadStringPrintfDx(IDS_FILEISUPXED, pszReal);

        INT nID;
        if (bForceDecompress)
        {
            nID = IDYES;
        }
        else
        {
            nID = MsgBoxDx(szMsg, MB_ICONINFORMATION | MB_YESNOCANCEL);
            if (nID == IDCANCEL)
                return FALSE;
        }

        if (nID == IDYES)
        {
            WCHAR szTempPath[MAX_PATH];
            GetTempPathW(_countof(szTempPath), szTempPath);

            WCHAR szTempFile[MAX_PATH];
            GetTempFileNameW(szTempPath, L"UPX", 0, szTempFile);

            if (!CopyFileW(pszReal, szTempFile, FALSE) ||
                !DoUpxExtract(m_szUpxExe, szTempFile))
            {
                DeleteFileW(szTempFile);
                ErrorBoxDx(IDS_CANTUPXEXTRACT);
                return FALSE;
            }

            lstrcpynW(m_szUpxTempFile, szTempFile, _countof(m_szUpxTempFile));
            pszReal = m_szUpxTempFile;
        }
    }

    // executable files
    HMODULE hMod;
    MString strReal = pszReal;
    hMod = LoadLibraryExW(strReal.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE |
        LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hMod == NULL)
    {
        #ifdef _WIN64
            mstr_replace_all(strReal, 
                L"C:\\Program Files\\",
                L"C:\\Program Files (x86)\\");
        #else
            mstr_replace_all(strReal, 
                L"C:\\Program Files (x86)\\",
                L"C:\\Program Files\\");
        #endif
        hMod = LoadLibraryExW(strReal.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE |
            LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hMod)
        {
            StringCchCopy(pszReal, _countof(szPath), strReal.c_str());
        }
        else
        {
            hMod = LoadLibraryW(pszReal);
            if (hMod == NULL)
            {
                ErrorBoxDx(IDS_CANNOTOPEN);
                return FALSE;
            }
        }
    }

    m_bLoading = TRUE;
    {
        m_entries.clear();
        Res_GetListFromRes(hMod, (LPARAM)&m_entries);
        FreeLibrary(hMod);
    }
    m_bLoading = FALSE;

    UnloadResourceH(hwnd, FALSE);
    if (m_settings.bAutoLoadNearbyResH)
        CheckResourceH(hwnd, pszNominal);

    m_bLoading = TRUE;
    TV_RefreshInfo(m_hTreeView, m_db, m_entries);
    m_bLoading = FALSE;

    SetFilePath(hwnd, pszReal, pszNominal);

    if (m_szResourceH[0] && m_settings.bAutoShowIDList)
    {
        ShowIDList(hwnd, TRUE);
    }
    DoRefresh(hwnd);

    return TRUE;
}

BOOL MMainWnd::UnloadResourceH(HWND hwnd, BOOL bRefresh)
{
    m_db.m_map[L"RESOURCE.ID"].clear();
    m_db.AddIDC_STATIC();
    m_settings.AddIDC_STATIC();
    m_settings.bHasIDC_STATIC = FALSE;

    m_settings.id_map.clear();
    m_settings.added_ids.clear();
    m_settings.removed_ids.clear();
    m_szResourceH[0] = 0;
    ShowIDList(hwnd, FALSE);
    if (bRefresh)
    {
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
    }

    return TRUE;
}

BOOL MMainWnd::CheckResourceH(HWND hwnd, LPCTSTR pszPath)
{
    UnloadResourceH(hwnd);

    TCHAR szPath[MAX_PATH];
    lstrcpyn(szPath, pszPath, _countof(szPath));

    TCHAR *pch = _tcsrchr(szPath, TEXT('\\'));
    if (pch == NULL)
    {
        pch = _tcsrchr(szPath, TEXT('/'));
    }
    if (pch == NULL)
    {
        return FALSE;
    }

    ++pch;
    size_t diff = pch - szPath;
    StringCchCopy(pch, diff, TEXT("resource.h"));
    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
    {
        StringCchCopy(pch, diff, TEXT("..\\resource.h"));
        if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            StringCchCopy(pch, diff, TEXT("..\\..\\resource.h"));
            if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
            {
                StringCchCopy(pch, diff, TEXT("..\\..\\..\\resource.h"));
                if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                {
                    StringCchCopy(pch, diff, TEXT("..\\src\\resource.h"));
                    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                    {
                        StringCchCopy(pch, diff, TEXT("..\\..\\src\\resource.h"));
                        if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                        {
                            StringCchCopy(pch, diff, TEXT("..\\..\\..\\src\\resource.h"));
                            if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES)
                            {
                                return FALSE;
                            }
                        }
                    }
                }
            }
        }
    }

    return DoLoadResH(hwnd, szPath);
}

BOOL MMainWnd::DoLoadMsgTables(HWND hwnd, LPCWSTR szRCFile, ResEntries& entries, MStringA& strOutput)
{
    MWaitCursor wait;

    WCHAR szPath3[MAX_PATH];
    lstrcpynW(szPath3, GetTempFileNameDx(L"R3"), MAX_PATH);
    MFile r3(szPath3, TRUE);
    r3.CloseHandle();

    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szMcdxExe;
    strCmdLine += L"\" ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res \"";
    strCmdLine += szRCFile;
    strCmdLine += L'\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    SetEnvironmentVariableW(L"LANG", L"en_US");
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            bSuccess = DoImport(hwnd, szPath3, entries);
        }
    }
    else
    {
        MStringA msg;
        strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
    }

    DeleteFileW(szPath3);

    return bSuccess;
}

BOOL MMainWnd::DoLoadRC(HWND hwnd, LPCWSTR szRCFile, ResEntries& entries)
{
    MWaitCursor wait;

    WCHAR szPath3[MAX_PATH];
    lstrcpynW(szPath3, GetTempFileNameDx(L"R3"), MAX_PATH);
    MFile r3(szPath3, TRUE);
    r3.CloseHandle();

    MStringW strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szWindresExe;
    strCmdLine += L"\" -DRC_INVOKED ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" -o \"";
    strCmdLine += szPath3;
    strCmdLine += L"\" -J rc -O res -F pe-i386 --preprocessor=\"";
    strCmdLine += m_szCppExe;
    strCmdLine += L"\" --preprocessor-arg=\"\" \"";
    strCmdLine += szRCFile;
    strCmdLine += L'\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    std::string strOutput;
    MFile hInputWrite, hOutputRead;
    SetEnvironmentVariableW(L"LANG", L"en_US");
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            bSuccess = DoImport(hwnd, szPath3, entries);
        }
        else if (strOutput.find(": no resources") != std::string::npos)
        {
            bSuccess = TRUE;
            strOutput.clear();
        }
    }
    else
    {
        MStringA msg;
        strOutput = MWideToAnsi(CP_ACP, LoadStringDx(IDS_CANNOTSTARTUP));
    }

    if (bSuccess)
    {
        DeleteFileW(szPath3);
        bSuccess = DoLoadMsgTables(hwnd, szRCFile, entries, strOutput);
    }

    if (strOutput.empty())
    {
        SetWindowTextW(m_hBinEdit, LoadStringDx(IDS_COMPILEERROR));
        ShowBinEdit(FALSE);
    }
    else
    {
        MAnsiToWide a2w(CP_ACP, strOutput.c_str());
        ErrorBoxDx(a2w.c_str());

        SetWindowTextA(m_hBinEdit, (char *)&strOutput[0]);
        ShowBinEdit(TRUE, TRUE);
    }
#ifdef NDEBUG
    DeleteFileW(szPath3);
#endif

    HidePreview(hwnd);
    PostMessageW(hwnd, WM_SIZE, 0, 0);

    return bSuccess;
}

BOOL MMainWnd::DoImport(HWND hwnd, LPCWSTR ResFile, ResEntries& entries)
{
    MWaitCursor wait;
    MByteStreamEx stream;
    if (!stream.LoadFromFile(ResFile))
        return FALSE;

    BOOL bAdded = FALSE;
    ResHeader header;
    while (header.ReadFrom(stream))
    {
        bAdded = TRUE;
        if (header.DataSize == 0)
        {
            stream.ReadDwordAlignment();
            continue;
        }

        if (header.DataSize > stream.remainder())
            return FALSE;

        ResEntry entry;
        entry.data.resize(header.DataSize);
        if (!stream.ReadData(&entry.data[0], header.DataSize))
        {
            break;
        }

        entry.lang = header.LanguageId;
        entry.updated = TRUE;
        entry.type = header.type;
        entry.name = header.name;
        entries.push_back(entry);

        stream.ReadDwordAlignment();
    }
    return bAdded;
}

void MMainWnd::OnFind(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return;
    if (!IsWindowVisible(m_hSrcEdit))
        return;

    if (IsWindow(m_hFindReplaceDlg))
    {
        SendMessage(m_hFindReplaceDlg, WM_CLOSE, 0, 0);
        m_hFindReplaceDlg = NULL;
    }

    m_fr.hwndOwner = hwnd;
    m_fr.Flags = FR_HIDEWHOLEWORD | FR_DOWN;
    m_hFindReplaceDlg = FindText(&m_fr);
}

BOOL MMainWnd::OnFindNext(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;
    if (m_szFindWhat[0] == 0)
    {
        OnFind(hwnd);
        return FALSE;
    }

    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    TCHAR szText[_countof(m_szFindWhat)];
    lstrcpyn(szText, m_szFindWhat, _countof(szText));
    if (szText[0] == 0)
        return FALSE;

    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpper(szText);
        CharUpper(&str[0]);
    }

    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
    {
        ibegin += (DWORD)substr.size();
    }
    

    size_t i = str.find(szText, ibegin);
    if (i == MString::npos)
        return FALSE;

    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);
    return TRUE;
}

BOOL MMainWnd::OnFindPrev(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;
    if (m_szFindWhat[0] == 0)
    {
        OnFind(hwnd);
        return FALSE;
    }

    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    TCHAR szText[_countof(m_szFindWhat)];
    lstrcpyn(szText, m_szFindWhat, _countof(szText));
    if (szText[0] == 0)
        return FALSE;

    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpper(szText);
        CharUpper(&str[0]);
    }

    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
        --ibegin;

    size_t i = str.rfind(szText, ibegin);
    if (i == MString::npos)
        return FALSE;

    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);
    return TRUE;
}

BOOL MMainWnd::OnReplaceNext(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;
    if (m_szFindWhat[0] == 0)
    {
        OnReplace(hwnd);
        return FALSE;
    }

    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    TCHAR szText[_countof(m_szFindWhat)];
    lstrcpyn(szText, m_szFindWhat, _countof(szText));
    if (szText[0] == 0)
        return FALSE;

    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpper(szText);
        CharUpper(&str[0]);
    }

    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
    {
        SendMessage(m_hSrcEdit, EM_REPLACESEL, TRUE, (LPARAM)m_szReplaceWith);
        Edit_SetModify(m_hSrcEdit, TRUE);
        str.replace(ibegin, iend - ibegin, m_szReplaceWith);
        ibegin += lstrlen(m_szReplaceWith);
    }

    size_t i = str.find(szText, ibegin);
    if (i == MString::npos)
        return FALSE;

    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);
    return TRUE;
}

BOOL MMainWnd::OnReplacePrev(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;
    if (m_szFindWhat[0] == 0)
    {
        OnReplace(hwnd);
        return FALSE;
    }

    DWORD ibegin, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&ibegin, (LPARAM)&iend);

    TCHAR szText[_countof(m_szFindWhat)];
    lstrcpyn(szText, m_szFindWhat, _countof(szText));
    if (szText[0] == 0)
        return FALSE;

    MString str = GetWindowText(m_hSrcEdit);
    if (str.empty())
        return FALSE;

    if (!(m_fr.Flags & FR_MATCHCASE))
    {
        CharUpper(szText);
        CharUpper(&str[0]);
    }

    MString substr = str.substr(ibegin, iend - ibegin);
    if (substr == szText)
    {
        SendMessage(m_hSrcEdit, EM_REPLACESEL, TRUE, (LPARAM)m_szReplaceWith);
        Edit_SetModify(m_hSrcEdit, TRUE);
        str.replace(ibegin, iend - ibegin, m_szReplaceWith);
        --ibegin;
    }

    size_t i = str.rfind(szText, ibegin);
    if (i == MString::npos)
        return FALSE;

    ibegin = (DWORD)i;
    iend = ibegin + lstrlen(m_szFindWhat);
    SendMessage(m_hSrcEdit, EM_SETSEL, (WPARAM)ibegin, (LPARAM)iend);
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);
    return TRUE;
}

BOOL MMainWnd::OnReplace(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;
    if (!IsWindowVisible(m_hSrcEdit))
        return FALSE;
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;
    if (IsWindow(m_hFindReplaceDlg))
    {
        SendMessage(m_hFindReplaceDlg, WM_CLOSE, 0, 0);
        m_hFindReplaceDlg = NULL;
    }

    m_fr.hwndOwner = hwnd;
    m_fr.Flags = FR_HIDEWHOLEWORD | FR_DOWN;
    m_hFindReplaceDlg = ReplaceText(&m_fr);
    return TRUE;
}

BOOL MMainWnd::OnReplaceAll(HWND hwnd)
{
    if (GetWindowTextLength(m_hSrcEdit) == 0)
        return FALSE;
    if (GetWindowStyle(m_hSrcEdit) & ES_READONLY)
        return FALSE;

    DWORD istart, iend;
    SendMessage(m_hSrcEdit, EM_GETSEL, (WPARAM)&istart, (LPARAM)&iend);

    SendMessage(m_hSrcEdit, EM_SETSEL, 0, 0);
    while (OnReplaceNext(hwnd))
        ;

    SendMessage(m_hSrcEdit, EM_SETSEL, istart, iend);
    SendMessage(m_hSrcEdit, EM_SCROLLCARET, 0, 0);
    return TRUE;
}

LRESULT MMainWnd::OnFindMsg(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (m_fr.Flags & FR_DIALOGTERM)
    {
        m_hFindReplaceDlg = NULL;
        SetFocus(m_hSrcEdit);
        return 0;
    }
    if (m_fr.Flags & FR_REPLACEALL)
    {
        OnReplaceAll(hwnd);
    }
    else if (m_fr.Flags & FR_REPLACE)
    {
        if (m_fr.Flags & FR_DOWN)
            OnReplaceNext(hwnd);
        else
            OnReplacePrev(hwnd);
    }
    else if (m_fr.Flags & FR_FINDNEXT)
    {
        if (m_fr.Flags & FR_DOWN)
        {
            OnFindNext(hwnd);
        }
        else
        {
            OnFindPrev(hwnd);
        }
    }
    return 0;
}

BOOL MMainWnd::DoExtractRes(HWND hwnd, LPCWSTR pszFileName, const ResEntries& entries)
{
    MByteStreamEx bs;
    ResHeader header;
    if (!header.WriteTo(bs))
        return FALSE;

    ResEntries::const_iterator it, end = entries.end();
    for (it = entries.begin(); it != end; ++it)
    {
        const ResEntry& entry = *it;

        header.DataSize = entry.size();
        header.HeaderSize = header.GetHeaderSize(entry.type, entry.name);
        if (header.HeaderSize == 0 || header.HeaderSize >= 0x10000)
            return FALSE;

        header.type = entry.type;
        header.name = entry.name;
        header.DataVersion = 0;
        header.MemoryFlags = MEMORYFLAG_DISCARDABLE | MEMORYFLAG_PURE |
                             MEMORYFLAG_MOVEABLE;
        header.LanguageId = entry.lang;
        header.Version = 0;
        header.Characteristics = 0;

        if (!header.WriteTo(bs))
            return FALSE;

        if (!bs.WriteData(&entry[0], entry.size()))
            return FALSE;

        bs.WriteDwordAlignment();
    }

    return bs.SaveToFile(pszFileName);
}

BOOL IsEmptyDirectoryDx(LPCTSTR pszPath)
{
    WCHAR sz[MAX_PATH];
    StringCchCopy(sz, _countof(sz), pszPath);
    StringCchCat(sz, _countof(sz), L"\\*");

    BOOL bFound = FALSE;
    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(sz, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            MString str = find.cFileName;
            if (str != L"." && str != L"..")
            {
                bFound = TRUE;
                break;
            }
        } while (FindNextFile(hFind, &find));

        FindClose(hFind);
    }

    return !bFound;
}

BOOL MMainWnd::DoWriteRCLang(MFile& file, ResToText& res2text, WORD lang)
{
    file.WriteSzA("//////////////////////////////////////////////////////////////////////////////\r\n\r\n");
    MString strLang = ::GetLanguageStatement(lang, TRUE);
    strLang += L"\r\n";
    file.WriteSzA(MWideToAnsi(CP_ACP, strLang.c_str()).c_str());

    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        ResEntry& entry = m_entries[i];
        if (entry.lang != lang)
            continue;
        if (entry.type == RT_STRING || entry.type == RT_MESSAGETABLE)
            continue;
        MString str = res2text.DumpEntry(entry);
        if (!str.empty())
        {
            mstr_trim(str);
            MTextToAnsi t2a(CP_UTF8, str.c_str());
            file.WriteSzA(t2a.c_str());
            file.WriteSzA("\r\n\r\n");
        }
    }

    ResEntries found;

    Res_Search(found, m_entries, RT_STRING, (WORD)0, lang);
    if (found.size())
    {
        StringRes str_res;
        ResEntries::iterator it, end = found.end();
        for (it = found.begin(); it != end; ++it)
        {
            if (it->lang != lang)
                continue;
            MByteStreamEx stream(it->data);
            if (!str_res.LoadFromStream(stream, it->name.m_id))
                return FALSE;
        }

        MString str = str_res.Dump(m_db);;
        mstr_trim(str);
        str += L"\r\n\r\n";

        MTextToAnsi t2a(CP_UTF8, str.c_str());
        file.WriteSzA(t2a.c_str());
    }

    Res_Search(found, m_entries, RT_MESSAGETABLE, (WORD)0, lang);
    if (found.size())
    {
        MessageRes msg_res;
        ResEntries::iterator it, end = found.end();
        for (it = found.begin(); it != end; ++it)
        {
            if (it->lang != lang)
                continue;
            MByteStreamEx stream(it->data);
            if (!msg_res.LoadFromStream(stream, it->name.m_id))
                return FALSE;
        }

        MString str;
        str += L"#ifdef APSTUDIO_INVOKED\r\n";
        str += L"    #error Ap Studio cannot edit this message table.\r\n";
        str += L"#endif\r\n";
        str += L"#ifdef MCDX_INVOKED\r\n";
        str += msg_res.Dump(m_db);
        str += L"#endif\r\n\r\n";

        MTextToAnsi t2a(CP_UTF8, str.c_str());
        file.WriteSzA(t2a.c_str());
    }

    return TRUE;
}

#define ISDOTS(psz) ((psz)[0] == '.' && ((psz)[1] == '\0' || (psz)[1] == '.' && (psz)[2] == '\0'))

BOOL DeleteDirectoryDx(LPCTSTR pszDir)
{
    TCHAR szDirOld[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA find;

    GetCurrentDirectory(MAX_PATH, szDirOld);
    if (!SetCurrentDirectory(pszDir))
        return FALSE;

    hFind = FindFirstFile(TEXT("*"), &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!ISDOTS(find.cFileName))
            {
                if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    DeleteDirectoryDx(find.cFileName);
                }
                else
                {
                    SetFileAttributes(find.cFileName, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(find.cFileName);
                }
            }
        } while(FindNextFile(hFind, &find));
        FindClose(hFind);
    }
    SetCurrentDirectory(szDirOld);

    SetFileAttributes(pszDir, FILE_ATTRIBUTE_DIRECTORY);
    return RemoveDirectory(pszDir);
}

BOOL MMainWnd::DoBackupFolder(LPCWSTR pszPath, UINT nCount)
{
    if (GetFileAttributes(pszPath) != 0xFFFFFFFF)
    {
        if (nCount < s_nBackupMaxCount)
        {
            MString strPath = pszPath;
            strPath += L"-old";
            DoBackupFolder(strPath.c_str(), nCount + 1);
            return MoveFileEx(pszPath, strPath.c_str(),
                MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING);
        }
        else
        {
            DeleteDirectoryDx(pszPath);
        }
    }
    return TRUE;
}

BOOL MMainWnd::DoBackupFile(LPCWSTR pszPath, UINT nCount)
{
    if (GetFileAttributes(pszPath) != 0xFFFFFFFF)
    {
        if (nCount < s_nBackupMaxCount)
        {
            MString strPath = pszPath;
            strPath += L"-old";
            DoBackupFile(strPath.c_str(), nCount + 1);
            return CopyFile(pszPath, strPath.c_str(), FALSE);
        }
    }
    return TRUE;
}

BOOL MMainWnd::DoWriteRC(LPCWSTR pszFileName, LPCWSTR pszResH)
{
    ResToText res2text(m_settings, m_db, m_entries);
    res2text.m_bHumanReadable = FALSE;
    res2text.m_bNoLanguage = TRUE;

    DoBackupFile(pszFileName);
    MFile file(pszFileName, TRUE);
    if (!file)
        return FALSE;

    // dump header
    if (pszResH && pszResH[0])
        file.WriteFormatA("#include \"resource.h\"\r\n");
    file.WriteFormatA("#define APSTUDIO_HIDDEN_SYMBOLS\r\n");
    file.WriteFormatA("#include <windows.h>\r\n");
    file.WriteFormatA("#include <commctrl.h>\r\n");
    file.WriteFormatA("#undef APSTUDIO_HIDDEN_SYMBOLS\r\n");
    file.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n\r\n");

    // get languages
    std::set<WORD> langs;
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        langs.insert(m_entries[i].lang);
    }

    if (m_settings.bStoreToResFolder)
        res2text.m_strFilePrefix = L"res/";

    if (m_settings.bSepFilesByLang)
    {
        // dump neutral
        if (langs.count(0) > 0)
        {
            if (!DoWriteRCLang(file, res2text, 0))
                return FALSE;
        }

        // create "lang" directory path
        TCHAR szLangDir[MAX_PATH];
        StringCchCopy(szLangDir, _countof(szLangDir), pszFileName);
        TCHAR *pch = mstrrchr(szLangDir, TEXT('\\'));
        size_t diff = pch - szLangDir;
        *pch = 0;
        StringCchCat(szLangDir, _countof(szLangDir), TEXT("/lang"));

        // backup and create "lang" directory
        std::set<WORD>::const_iterator it, end = langs.end();
        for (it = langs.begin(); it != end; ++it)
        {
            WORD lang = *it;
            if (!lang)
                continue;

            DoBackupFolder(szLangDir);
            CreateDirectory(szLangDir, NULL);
            break;
        }

        // for each language
        end = langs.end();
        for (it = langs.begin(); it != end; ++it)
        {
            WORD lang = *it;
            if (!lang)
                continue;

            // create lang/XX_XX.rc file
            TCHAR szLangFile[MAX_PATH];
            StringCchCopy(szLangFile, _countof(szLangFile), szLangDir);
            StringCchCat(szLangFile, _countof(szLangFile), TEXT("/"));
            MString lang_name = m_db.GetName(L"LANGUAGES", lang);
            StringCchCat(szLangFile, _countof(szLangFile), lang_name.c_str());
            StringCchCat(szLangFile, _countof(szLangFile), TEXT(".rc"));
            //MessageBox(NULL, szLangFile, NULL, 0);

            // dump to lang/XX_XX.rc file
            DoBackupFile(szLangFile);
            MFile lang_file(szLangFile, TRUE);
            lang_file.WriteFormatA("#pragma code_page(65001) // UTF-8\r\n\r\n");
            if (!lang_file)
                return FALSE;
            if (!DoWriteRCLang(lang_file, res2text, lang))
                return FALSE;
        }
    }
    else
    {
        std::set<WORD>::const_iterator it, end = langs.end();
        for (it = langs.begin(); it != end; ++it)
        {
            WORD lang = *it;
            if (!DoWriteRCLang(file, res2text, lang))
                return FALSE;
        }
    }

    // dump language includes
    if (m_settings.bSepFilesByLang)
    {
        file.WriteSzA("//////////////////////////////////////////////////////////////////////////////\r\n");
        if (m_settings.bSelectableByMacro)
        {
            std::set<WORD>::const_iterator it, end = langs.end();
            for (it = langs.begin(); it != end; ++it)
            {
                WORD lang = *it;
                if (!lang)
                    continue;

                MString lang_name1 = m_db.GetName(L"LANGUAGES", lang);
                MString lang_name2 = lang_name1;
                CharUpper(&lang_name2[0]);
                file.WriteSzA("#ifdef LANGUAGE_");
                MWideToAnsi lang2_w2a(CP_ACP, lang_name2.c_str());
                file.WriteSzA(lang2_w2a.c_str());
                file.WriteSzA("\r\n");
                file.WriteSzA("    #include \"lang/");
                MWideToAnsi lang1_w2a(CP_ACP, lang_name1.c_str());
                file.WriteSzA(lang1_w2a.c_str());
                file.WriteSzA(".rc\"\r\n");
                file.WriteSzA("#endif\r\n");
            }
        }
        else
        {
            std::set<WORD>::const_iterator it, end = langs.end();
            for (it = langs.begin(); it != end; ++it)
            {
                WORD lang = *it;
                if (!lang)
                    continue;
                MString lang_name1 = m_db.GetName(L"LANGUAGES", lang);
                file.WriteSzA("#include \"lang/");
                file.WriteSzA(MWideToAnsi(CP_ACP, lang_name1.c_str()).c_str());
                file.WriteSzA(".rc\"\r\n");
            }
        }
    }

    file.WriteSzA("//////////////////////////////////////////////////////////////////////////////\r\n");

    // write three TEXTINCLUDE's
    file.WriteSzA("#ifdef APSTUDIO_INVOKED\r\n");
    file.WriteSzA("\r\n");
    file.WriteSzA("1 TEXTINCLUDE\r\n");
    file.WriteSzA("BEGIN\r\n");
    file.WriteSzA("    \"resource.h\\0\"\r\n");
    file.WriteSzA("END\r\n");
    file.WriteSzA("\r\n");
    file.WriteSzA("2 TEXTINCLUDE\r\n");
    file.WriteSzA("BEGIN\r\n");
    file.WriteSzA("    \"#define APSTUDIO_HIDDEN_SYMBOLS\\r\\n\"\r\n");
    file.WriteSzA("    \"#include <windows.h>\\r\\n\"\r\n");
    file.WriteSzA("    \"#include <commctrl.h>\\r\\n\"\r\n");
    file.WriteSzA("    \"#undef APSTUDIO_HIDDEN_SYMBOLS\\r\\n\"\r\n");
    file.WriteSzA("    \"\\0\"\r\n");
    file.WriteSzA("END\r\n");
    file.WriteSzA("\r\n");
    file.WriteSzA("3 TEXTINCLUDE\r\n");
    file.WriteSzA("BEGIN\r\n");
    file.WriteSzA("    \"\\r\\n\"\r\n");
    file.WriteSzA("    \"\\0\"\r\n");
    file.WriteSzA("END\r\n");
    file.WriteSzA("\r\n");
    file.WriteSzA("#endif    // APSTUDIO_INVOKED\r\n");

    file.WriteSzA("//////////////////////////////////////////////////////////////////////////////\r\n");

    return TRUE;
}

BOOL MMainWnd::DoWriteResH(LPCWSTR pszRCFile, LPCWSTR pszFileName)
{
    DoBackupFile(pszFileName);
    MFile file(pszFileName, TRUE);
    if (!file)
        return FALSE;

    file.WriteSzA("//{{NO_DEPENDENCIES}}\r\n");
    file.WriteSzA("// Microsoft Visual C++ Compatible\r\n");

    if (pszRCFile)
    {
        // get file title
        TCHAR szFileTitle[64];
        GetFileTitle(pszRCFile, szFileTitle, _countof(szFileTitle));

        // change extension to .rc
        LPTSTR pch = mstrrchr(szFileTitle, TEXT('.'));
        *pch = 0;
        StringCchCatW(szFileTitle, _countof(szFileTitle), TEXT(".rc"));

        // write file title
        file.WriteSzA("// ");
        file.WriteSzA(MTextToAnsi(CP_ACP, szFileTitle).c_str());
        file.WriteSzA("\r\n");
    }

    id_map_type::iterator it, end = m_settings.id_map.end();
    for (it = m_settings.id_map.begin(); it != end; ++it)
    {
        if (it->first == "IDC_STATIC")
        {
            file.WriteFormatA("#define IDC_STATIC -1\r\n");
        }
        else
        {
            file.WriteFormatA("#define %s %s\r\n",
                it->first.c_str(), it->second.c_str());
        }
    }

    UINT anValues[5];
    DoIDStat(anValues);

    file.WriteFormatA("#ifdef APSTUDIO_INVOKED\r\n");
    file.WriteFormatA("    #ifndef APSTUDIO_READONLY_SYMBOLS\r\n");
    file.WriteFormatA("        #define _APS_NO_MFC                 %u\r\n", anValues[0]);
    file.WriteFormatA("        #define _APS_NEXT_RESOURCE_VALUE    %u\r\n", anValues[1]);
    file.WriteFormatA("        #define _APS_NEXT_COMMAND_VALUE     %u\r\n", anValues[2]);
    file.WriteFormatA("        #define _APS_NEXT_CONTROL_VALUE     %u\r\n", anValues[3]);
    file.WriteFormatA("        #define _APS_NEXT_SYMED_VALUE       %u\r\n", anValues[4]);
    file.WriteFormatA("    #endif\r\n");
    file.WriteFormatA("#endif\r\n");

    return TRUE;
}

void MMainWnd::DoIDStat(UINT anValues[5])
{
    const size_t count = 4;
    INT anNext[count];
    MString prefixes[count];
    prefixes[0] = m_settings.assoc_map[L"Resource.ID"];
    prefixes[1] = m_settings.assoc_map[L"Command.ID"];
    prefixes[2] = m_settings.assoc_map[L"New.Command.ID"];
    prefixes[3] = m_settings.assoc_map[L"Control.ID"];

    for (size_t i = 0; i < count; ++i)
    {
        ConstantsDB::TableType table;
        table = m_db.GetTableByPrefix(L"RESOURCE.ID", prefixes[i]);

        UINT nMax = 0;
        {
            ConstantsDB::TableType::iterator it, end = table.end();
            for (it = table.begin(); it != end; ++it)
            {
                if (it->name == L"IDC_STATIC")
                    continue;

                if (i == 3)
                {
                    INT k = Res_Find2(m_entries, RT_CURSOR, WORD(it->value), 0xFFFF, FALSE);
                    if (k != -1)
                        continue;   // it was Cursor.ID, not Control.ID
                }

                if (nMax < it->value)
                    nMax = it->value;
            }
        }

        anNext[i] = nMax + 1;
    }

    anValues[0] = 1;
    anValues[1] = anNext[0];
#undef max
    anValues[2] = std::max(anNext[1], anNext[2]);
    anValues[3] = anNext[3];
    anValues[4] = 300;

    // fix for preferable values
    if (anValues[1] < 100)
        anValues[1] = 100;
    if (anValues[2] < 100)
        anValues[2] = 100;
    if (anValues[3] < 1000)
        anValues[3] = 1000;
}

inline BOOL MMainWnd::DoExtract(const ResEntry& entry, BOOL bExporting)
{
    ResToText res2text(m_settings, m_db, m_entries);

    if (bExporting)
    {
        if (m_settings.bStoreToResFolder)
        {
            res2text.m_strFilePrefix = L"res\\";
        }
    }

    MString filename = res2text.GetEntryFileName(entry);
    if (filename.empty())
        return TRUE;

    //MessageBox(NULL, filename.c_str(), NULL, 0);

    if (entry.type.is_int())
    {
        WORD wType = entry.type.m_id;
        if (wType == (WORD)(UINT_PTR)RT_CURSOR)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_BITMAP)
        {
            return PackedDIB_Extract(filename.c_str(), &entry[0], entry.size(), FALSE);
        }
        if (wType == (WORD)(UINT_PTR)RT_ICON)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_MENU)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_DIALOG)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_STRING)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_FONTDIR)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_FONT)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_ACCELERATOR)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_RCDATA)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_MESSAGETABLE)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_GROUP_CURSOR)
        {
            return DoExtractCursor(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_GROUP_ICON)
        {
            return DoExtractIcon(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_VERSION)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_DLGINIT)
        {
            // No output file
            return TRUE;
        }
        if (wType == (WORD)(UINT_PTR)RT_DLGINCLUDE)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_PLUGPLAY)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_VXD)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_ANICURSOR)
        {
            return DoExtractCursor(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_ANIICON)
        {
            return DoExtractIcon(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_HTML)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (wType == (WORD)(UINT_PTR)RT_MANIFEST)
        {
            return DoExtractBin(filename.c_str(), entry);
        }
    }
    else
    {
        if (entry.type == L"AVI")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"PNG")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"GIF")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"JPEG")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"JPG")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"TIFF")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"TIF")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"EMF")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"ENHMETAFILE")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"WMF")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"WAVE")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
        if (entry.type == L"IMAGE")
        {
            return DoExtractBin(filename.c_str(), entry);
        }
    }
    return DoExtractBin(filename.c_str(), entry);
}

BOOL MMainWnd::DoExport(LPCWSTR pszFileName)
{
    MWaitCursor wait;

    if (m_entries.empty())
    {
        ErrorBoxDx(IDS_DATAISEMPTY);
        return FALSE;
    }

    WCHAR szPath[MAX_PATH];
    StringCchCopy(szPath, _countof(szPath), pszFileName);
    WCHAR *pch = mstrrchr(szPath, L'\\');
    *pch = 0;

    BOOL bHasExternFile = FALSE;
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        ResToText res2text(m_settings, m_db, m_entries);
        MString filename = res2text.GetEntryFileName(m_entries[i]);
        if (filename.size())
        {
            bHasExternFile = TRUE;
            break;
        }
    }

    if (!IsEmptyDirectoryDx(szPath))
    {
        if (bHasExternFile && !m_settings.bStoreToResFolder)
        {
            ErrorBoxDx(IDS_MUSTBEEMPTYDIR);
            return FALSE;
        }
    }

    *pch = 0;

    WCHAR szCurDir[MAX_PATH];
    GetCurrentDirectory(_countof(szCurDir), szCurDir);

    if (!SetCurrentDirectory(szPath))
        return FALSE;

    if (bHasExternFile)
    {
        if (m_settings.bStoreToResFolder)
        {
            MString strResDir = szPath;
            strResDir += TEXT("\\res");
            DoBackupFolder(strResDir.c_str());
            CreateDirectory(strResDir.c_str(), NULL);
        }

        for (size_t i = 0; i < m_entries.size(); ++i)
        {
            if (m_entries[i].type == RT_STRING ||
                m_entries[i].type == RT_MESSAGETABLE)
            {
                continue;
            }
            if (!DoExtract(m_entries[i], TRUE))
                return FALSE;
        }
    }

    BOOL bOK = FALSE;
    if (m_szResourceH && !m_settings.id_map.empty())
    {
        StringCchCopyW(pch, _countof(szPath), L"\\resource.h");
        bOK = DoWriteResH(pszFileName, szPath) && DoWriteRC(pszFileName, szPath);
    }
    else
    {
        bOK = DoWriteRC(pszFileName, NULL);
    }

    SetCurrentDirectory(szCurDir);

    return bOK;
}

BOOL MMainWnd::DoExtractBin(LPCWSTR pszFileName, const ResEntry& entry)
{
    MByteStreamEx bs(entry.data);
    return bs.SaveToFile(pszFileName);
}

BOOL MMainWnd::DoSaveResAs(HWND hwnd, LPCWSTR pszExeFile)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return FALSE;

    if (DoExtractRes(hwnd, pszExeFile, m_entries))
    {
        Res_Optimize(m_entries);
        SetFilePath(hwnd, pszExeFile, NULL);
        return TRUE;
    }
    return FALSE;
}

BOOL MMainWnd::DoSaveAs(HWND hwnd, LPCWSTR pszExeFile)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return TRUE;

    return DoSaveExeAs(hwnd, pszExeFile);
}

BOOL MMainWnd::DoSaveExeAs(HWND hwnd, LPCWSTR pszExeFile)
{
    if (m_bUpxCompressed && m_szUpxTempFile[0] == 0)
    {
        ErrorBoxDx(IDS_CANTSAVEUPXED);
        return FALSE;
    }

    MFile file(m_szRealFile);
    BYTE ab[2] = { 0 };
    DWORD dwSize;
    BOOL bOK = file.ReadFile(ab, sizeof(ab), &dwSize);
    file.CloseHandle();

    if (memcmp(ab, "MZ", 2) != 0)
        bOK = FALSE;

    BOOL b1, b2, b3;
    if (!bOK || GetFileAttributesW(m_szRealFile) == INVALID_FILE_ATTRIBUTES)
    {
        b3 = Res_UpdateExe(hwnd, pszExeFile, m_entries);
        if (b3)
        {
            Res_Optimize(m_entries);
            SetFilePath(hwnd, pszExeFile, NULL);
            return TRUE;
        }
    }
    else
    {
        LPWSTR TempFile = GetTempFileNameDx(L"ERE");
        b1 = ::CopyFileW(m_szRealFile, TempFile, FALSE);
        b2 = b1 && Res_UpdateExe(hwnd, TempFile, m_entries);
        b3 = b2 && ::CopyFileW(TempFile, pszExeFile, FALSE);
        if (b3)
        {
            DeleteFileW(TempFile);
            Res_Optimize(m_entries);
            SetFilePath(hwnd, pszExeFile, NULL);
            if (m_settings.bCompressByUPX)
            {
                DoUpxCompress(m_szUpxExe, pszExeFile);
            }
            return TRUE;
        }
        DeleteFileW(TempFile);
    }

    return FALSE;
}

BOOL MMainWnd::DoUpxCompress(LPCWSTR pszUpx, LPCWSTR pszExeFile)
{
    MStringW strCmdLine;
    strCmdLine += L"\"";
    strCmdLine += pszUpx;
    strCmdLine += L"\" -9 \"";
    strCmdLine += pszExeFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bSuccess = FALSE;

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        std::string strOutput;
        pmaker.ReadAll(strOutput, hOutputRead);

        if (pmaker.GetExitCode() == 0)
        {
            if (strOutput.find("Packed") != std::string::npos)
                bSuccess = TRUE;
        }
    }

    return bSuccess;
}

BOOL MMainWnd::DoExtractIcon(LPCWSTR pszFileName, const ResEntry& entry)
{
    if (entry.type == RT_GROUP_ICON)
    {
        return Res_ExtractGroupIcon(m_entries, entry, pszFileName);
    }
    else if (entry.type == RT_ICON)
    {
        return Res_ExtractIcon(m_entries, entry, pszFileName);
    }
    else if (entry.type == RT_ANIICON)
    {
        MFile file;
        DWORD cbWritten = 0;
        if (file.OpenFileForOutput(pszFileName) &&
            file.WriteFile(&entry[0], entry.size(), &cbWritten))
        {
            file.CloseHandle();
            return TRUE;
        }
    }
    return FALSE;
}

BOOL MMainWnd::DoExtractCursor(LPCWSTR pszFileName, const ResEntry& entry)
{
    if (entry.type == RT_GROUP_CURSOR)
    {
        return Res_ExtractGroupCursor(m_entries, entry, pszFileName);
    }
    else if (entry.type == RT_CURSOR)
    {
        return Res_ExtractCursor(m_entries, entry, pszFileName);
    }
    else if (entry.type == RT_ANICURSOR)
    {
        MFile file;
        DWORD cbWritten = 0;
        if (file.OpenFileForOutput(pszFileName) &&
            file.WriteFile(&entry[0], entry.size(), &cbWritten))
        {
            file.CloseHandle();
            return TRUE;
        }
    }
    return FALSE;
}


void MMainWnd::OnDropFiles(HWND hwnd, HDROP hdrop)
{
    MWaitCursor wait;
    WCHAR file[MAX_PATH], *pch;

    ChangeStatusText(IDS_EXECUTINGCMD);

    DragQueryFileW(hdrop, 0, file, _countof(file));
    DragFinish(hdrop);

    SetForegroundWindow(hwnd);

    pch = wcsrchr(file, L'.');
    if (pch)
    {
        if (lstrcmpiW(pch, L".ico") == 0)
        {
            MAddIconDlg dialog(m_db, m_entries);
            dialog.file = file;
            dialog.DialogBoxDx(hwnd);
            DoRefreshIDList(hwnd);
            TV_RefreshInfo(m_hTreeView, m_db, m_entries);
            TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
            ChangeStatusText(IDS_READY);
            return;
        }
        else if (lstrcmpiW(pch, L".cur") == 0 || lstrcmpiW(pch, L".ani") == 0)
        {
            MAddCursorDlg dialog(m_db, m_entries);
            dialog.m_file = file;
            dialog.DialogBoxDx(hwnd);
            DoRefreshIDList(hwnd);
            TV_RefreshInfo(m_hTreeView, m_db, m_entries);
            TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
            ChangeStatusText(IDS_READY);
            return;
        }
        else if (lstrcmpiW(pch, L".wav") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_type = L"WAVE";
            dialog.m_file = file;
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".bmp") == 0 || lstrcmpiW(pch, L".dib") == 0)
        {
            MAddBitmapDlg dialog(m_db, m_entries);
            dialog.m_file = file;
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".png") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_file = file;
            dialog.m_type = L"PNG";
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".gif") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_file = file;
            dialog.m_type = L"GIF";
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".jpg") == 0 || lstrcmpiW(pch, L".jpeg") == 0 ||
                 lstrcmpiW(pch, L".jpe") == 0 || lstrcmpiW(pch, L".jfif") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_file = file;
            dialog.m_type = L"JPEG";
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".tif") == 0 || lstrcmpiW(pch, L".tiff") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_file = file;
            dialog.m_type = L"TIFF";
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".avi") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_file = file;
            dialog.m_type = L"AVI";
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".res") == 0)
        {
            DoLoadFile(hwnd, file);
            ChangeStatusText(IDS_READY);
            return;
        }
        else if (lstrcmpiW(pch, L".rc") == 0)
        {
            DoLoadFile(hwnd, file);
            ChangeStatusText(IDS_READY);
            return;
        }
        else if (lstrcmpiW(pch, L".h") == 0)
        {
            UnloadResourceH(hwnd);
            CheckResourceH(hwnd, file);
            if (m_szResourceH[0] && m_settings.bAutoShowIDList)
            {
                ShowIDList(hwnd, TRUE);
            }
            DoRefresh(hwnd);
            ChangeStatusText(IDS_READY);
            return;
        }
        else if (lstrcmpiW(pch, L".wmf") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_file = file;
            dialog.m_type = L"WMF";
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
        else if (lstrcmpiW(pch, L".emf") == 0)
        {
            MAddResDlg dialog(m_entries, m_db);
            dialog.m_file = file;
            dialog.m_type = L"EMF";
            if (dialog.DialogBoxDx(hwnd) == IDOK)
            {
                DoRefreshIDList(hwnd);
                TV_RefreshInfo(m_hTreeView, m_db, m_entries);
                TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
                ChangeStatusText(IDS_READY);
            }
            return;
        }
    }

    DoLoadFile(hwnd, file);
    TV_RefreshInfo(m_hTreeView, m_db, m_entries);
    ChangeStatusText(IDS_READY);
}

void MMainWnd::OnLoadResH(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
    {
        return;
    }

    WCHAR szFile[MAX_PATH];
    if (m_szResourceH[0])
        StringCchCopyW(szFile, _countof(szFile), m_szResourceH);
    else
        StringCchCopyW(szFile, _countof(szFile), L"resource.h");

    if (GetFileAttributesW(szFile) == INVALID_FILE_ATTRIBUTES)
        szFile[0] = 0;

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_LOADRESH);
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST |
        OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"h";
    if (GetOpenFileNameW(&ofn))
    {
        DoLoadResH(hwnd, szFile);
        if (m_szResourceH[0])
        {
            ShowIDList(hwnd, TRUE);
        }
        DoRefresh(hwnd);
    }
}

void MMainWnd::OnLoadResHBang(HWND hwnd)
{
    if (m_szResourceH[0])
    {
        MString strFile = m_szResourceH;
        DoLoadResH(hwnd, strFile.c_str());
        if (m_szResourceH[0])
        {
            ShowIDList(hwnd, TRUE);
        }
    }
}

void MMainWnd::OnDestroy(HWND hwnd)
{
    if (m_szUpxTempFile[0])
    {
        DeleteFileW(m_szUpxTempFile);
        m_szUpxTempFile[0] = 0;
    }
    
    SaveSettings(hwnd);

    if (IsWindow(m_rad_window))
    {
        DestroyWindow(m_rad_window);
    }
    if (IsWindow(m_hBinEdit))
    {
        DestroyWindow(m_hBinEdit);
    }
    if (IsWindow(m_hSrcEdit))
    {
        DestroyWindow(m_hSrcEdit);
    }

    m_hBmpView.DestroyView();
    if (IsWindow(m_hBmpView))
    {
        DestroyWindow(m_hBmpView);
    }

    if (IsWindow(m_id_list_dlg))
    {
        DestroyWindow(m_id_list_dlg);
    }

    //DestroyIcon(m_hIcon);     // LR_SHARED
    DestroyIcon(m_hIconSm);
    DestroyAcceleratorTable(m_hAccel);
    ImageList_Destroy(m_hImageList);
    DestroyIcon(m_hFileIcon);
    DestroyIcon(m_hFolderIcon);
    DeleteObject(m_hSrcFont);
    DeleteObject(m_hBinFont);

    //DestroyIcon(MRadCtrl::Icon());    // LR_SHARED
    DeleteObject(MRadCtrl::Bitmap());
    DestroyCursor(MSplitterWnd::CursorNS());
    DestroyCursor(MSplitterWnd::CursorWE());

    DestroyWindow(m_hTreeView);
    DestroyWindow(m_hToolBar);
    DestroyWindow(m_hStatusBar);
    DestroyWindow(m_hFindReplaceDlg);

    DestroyWindow(m_splitter1);
    DestroyWindow(m_splitter2);
    DestroyWindow(m_splitter3);

    PostQuitMessage(0);
}

BOOL MMainWnd::ParseMacros(HWND hwnd, LPCTSTR pszFile, std::vector<MStringA>& macros, MStringA& str)
{
    std::vector<MStringA> lines;
    mstr_trim(str);
    mstr_split(lines, str, "\n");

    size_t len = lines.size() - 1;
    if (macros.size() < len)
        len = macros.size();

    for (size_t i = 0; i < len; ++i)
    {
        const MStringA& macro = macros[i];
        const MStringA& line = lines[i + 1];
        using namespace MacroParser;
        StringScanner scanner(line);
        TokenStream stream(scanner);
        stream.read_tokens();
        Parser parser(stream);
        if (parser.parse())
        {
            if (is_str(parser.ast()))
            {
                string_type value;
                if (eval_string(parser.ast(), value))
                {
                    m_settings.id_map[macro] = value;
                }
            }
            else
            {
                int value = 0;
                if (eval_int(parser.ast(), value))
                {
                    char sz[32];
                    if (m_id_list_dlg.m_nBase == 16)
                        StringCchPrintfA(sz, _countof(sz), "0x%X", value);
                    else
                        StringCchPrintfA(sz, _countof(sz), "%d", value);

                    if (macro != "WIN32" && macro != "WINNT" && macro != "i386")
                        m_settings.id_map[macro] = sz;
                }
            }
        }
    }

    ConstantsDB::TableType& table = m_db.m_map[L"RESOURCE.ID"];
    table.clear();
    m_settings.bHasIDC_STATIC = FALSE;
    id_map_type::iterator it, end = m_settings.id_map.end();
    for (it = m_settings.id_map.begin(); it != end; ++it)
    {
        MStringW str1 = MAnsiToWide(CP_ACP, it->first).c_str();
        MStringW str2 = MAnsiToWide(CP_ACP, it->second).c_str();
        DWORD value2 = mstr_parse_int(str2.c_str());
        ConstantsDB::EntryType entry(str1, value2);
        table.push_back(entry);
        if (str1 == L"IDC_STATIC")
            m_settings.bHasIDC_STATIC = TRUE;
    }
    m_db.AddIDC_STATIC();
    m_settings.AddIDC_STATIC();

    lstrcpynW(m_szResourceH, pszFile, _countof(m_szResourceH));

    return TRUE;
}

BOOL MMainWnd::ParseResH(HWND hwnd, LPCTSTR pszFile, const char *psz, DWORD len)
{
    MStringA str(psz, len);
    std::vector<MStringA> lines, macros;
    mstr_split(lines, str, "\n");

    for (size_t i = 0; i < lines.size(); ++i)
    {
        MStringA& line = lines[i];
        mstr_trim(line);
        if (line.empty())
            continue;
        if (line.find("#define _") != MStringA::npos)
            continue;
        size_t found0 = line.find("#define ");
        if (found0 == MStringA::npos)
            continue;
        line = line.substr(strlen("#define "));
        size_t found1 = line.find_first_of(" \t");
        size_t found2 = line.find('(');
        if (found1 == MStringA::npos)
            continue;
        if (found2 != MStringA::npos && found2 < found1)
            continue;
        macros.push_back(line.substr(0, found1));
    }

    m_settings.id_map.clear();

    if (macros.empty())
    {
        return TRUE;
    }

    WCHAR szTempFile1[MAX_PATH];
    lstrcpynW(szTempFile1, GetTempFileNameDx(L"R1"), MAX_PATH);

    DWORD cbWritten;
    MFile file1(szTempFile1, TRUE);
    char buf[MAX_PATH + 64];
    WCHAR szFile[MAX_PATH];
    StringCchCopyW(szFile, _countof(szFile), pszFile);
    file1.WriteSzA("#include \"", &cbWritten);
    file1.WriteSzA(MTextToAnsi(CP_ACP, szFile).c_str(), &cbWritten);
    file1.WriteSzA("\"\n", &cbWritten);
    file1.WriteSzA("#pragma RisohEditor\n", &cbWritten);
    for (size_t i = 0; i < macros.size(); ++i)
    {
        StringCchPrintfA(buf, _countof(buf), "%s\n", macros[i].c_str());
        file1.WriteSzA(buf, &cbWritten);
    }
    file1.CloseHandle();

    MString strCmdLine;
    strCmdLine += L'\"';
    strCmdLine += m_szCppExe;
    strCmdLine += L"\" ";
    strCmdLine += GetMacroDump();
    strCmdLine += GetIncludesDump();
    strCmdLine += L" -Wp,-E \"";
    strCmdLine += szTempFile1;
    strCmdLine += L'\"';
    //MessageBoxW(hwnd, strCmdLine.c_str(), NULL, 0);

    BOOL bOK = FALSE;

    MProcessMaker pmaker;
    pmaker.SetShowWindow(SW_HIDE);
    pmaker.SetCreationFlags(CREATE_NEW_CONSOLE);

    std::string strOutput;
    MFile hInputWrite, hOutputRead;
    if (pmaker.PrepareForRedirect(&hInputWrite, &hOutputRead) &&
        pmaker.CreateProcessDx(NULL, strCmdLine.c_str()))
    {
        pmaker.ReadAll(strOutput, hOutputRead);
        pmaker.CloseAll();

        size_t pragma_found = strOutput.find("#pragma RisohEditor");
        if (pragma_found != MStringA::npos)
        {
            DeleteFileW(szTempFile1);
            strOutput = strOutput.substr(pragma_found);
            bOK = ParseMacros(hwnd, pszFile, macros, strOutput);
        }
    }

    DeleteFileW(szTempFile1);
    return bOK;
}

BOOL MMainWnd::DoLoadResH(HWND hwnd, LPCTSTR pszFile)
{
    UnloadResourceH(hwnd);

    WCHAR szTempFile[MAX_PATH];
    lstrcpynW(szTempFile, GetTempFileNameDx(L"R1"), MAX_PATH);

    MFile file(szTempFile, TRUE);
    file.CloseHandle();

    MString strCmdLine;
    strCmdLine += L"-E -dM -DRC_INVOKED -o \"";
    strCmdLine += szTempFile;
    strCmdLine += L"\" -x none \"";
    strCmdLine += pszFile;
    strCmdLine += L"\"";
    //MessageBoxW(hwnd, szCmdLine, NULL, 0);

    BOOL bOK = FALSE;
    SHELLEXECUTEINFOW info;
    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_UNICODE | SEE_MASK_FLAG_NO_UI;
    info.hwnd = hwnd;
    info.lpFile = m_szCppExe;
    info.lpParameters = &strCmdLine[0];
    info.nShow = SW_HIDE;
    if (ShellExecuteExW(&info))
    {
        WaitForSingleObject(info.hProcess, INFINITE);
        CloseHandle(info.hProcess);
        if (file.OpenFileForInput(szTempFile))
        {
            DWORD cbRead;
            CHAR szBuf[512];
            std::string data;
            while (file.ReadFile(szBuf, 512, &cbRead) && cbRead)
            {
                data.append(&szBuf[0], cbRead);
            }
            file.CloseHandle();
            DeleteFileW(szTempFile);
            bOK = ParseResH(hwnd, pszFile, &data[0], DWORD(data.size()));
        }
    }
    DeleteFileW(szTempFile);

    return bOK;
}

void MMainWnd::DoRefreshIDList(HWND hwnd)
{
    ShowIDList(hwnd, IsWindow(m_id_list_dlg));
}

void MMainWnd::DoRefresh(HWND hwnd, BOOL bRefreshAll)
{
    if (bRefreshAll)
    {
        DoRefreshIDList(hwnd);
    }

    LPARAM lParam = TV_GetParam(m_hTreeView);
    UINT i = LOWORD(lParam);

    ResEntry selection;
    if (i < m_entries.size())
        selection = m_entries[i];

    switch (HIWORD(lParam))
    {
    case I_NONE:
        selection.clear();
        break;
    case I_TYPE:
        selection.name.clear();
        selection.lang = 0xFFFF;
        break;
    case I_NAME:
        selection.lang = 0xFFFF;
        break;
    case I_LANG:
        break;
    case I_STRING:
    case I_MESSAGE:
        selection.name.clear();
        break;
    }

    TV_RefreshInfo(m_hTreeView, m_db, m_entries);

    if (HIWORD(lParam) != I_NONE)
    {
        TV_SelectEntry(m_hTreeView, m_entries, selection);
        lParam = TV_GetParam(m_hTreeView);
        SelectTV(hwnd, lParam, FALSE);
    }
}

void MMainWnd::OnAdviceResH(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    MString str;

    if (!m_settings.removed_ids.empty() &&
        (m_settings.removed_ids.size() != 1 ||
         m_settings.removed_ids.find("IDC_STATIC") == m_settings.removed_ids.end()))
    {
        str += LoadStringDx(IDS_DELETENEXTIDS);

        id_map_type::iterator it, end = m_settings.removed_ids.end();
        for (it = m_settings.removed_ids.begin(); it != end; ++it)
        {
            if (it->first == "IDC_STATIC")
                continue;

            str += TEXT("#define ");
            str += MAnsiToText(CP_ACP, it->first).c_str();
            str += TEXT(" ");
            str += MAnsiToText(CP_ACP, it->second).c_str();
            str += TEXT("\r\n");
        }
        str += TEXT("\r\n");
    }

    if (!m_settings.added_ids.empty() &&
        (m_settings.added_ids.size() != 1 ||
         m_settings.added_ids.find("IDC_STATIC") == m_settings.added_ids.end() ||
         !m_settings.bHasIDC_STATIC))
    {
        str += LoadStringDx(IDS_ADDNEXTIDS);

        id_map_type::iterator it, end = m_settings.added_ids.end();
        for (it = m_settings.added_ids.begin(); it != end; ++it)
        {
            if (it->first == "IDC_STATIC" && m_settings.bHasIDC_STATIC)
                continue;

            str += TEXT("#define ");
            str += MAnsiToText(CP_ACP, it->first).c_str();
            str += TEXT(" ");
            str += MAnsiToText(CP_ACP, it->second).c_str();
            str += TEXT("\r\n");
        }
        str += TEXT("\r\n");
    }

    if (str.empty())
    {
        str += LoadStringDx(IDS_NOCHANGE);
        str += TEXT("\r\n");
    }

    MAdviceResHDlg dialog(m_settings, str);
    dialog.DialogBoxDx(hwnd);
}

void MMainWnd::OnUnloadResH(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    UnloadResourceH(hwnd);
}

void MMainWnd::OnConfig(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MConfigDlg dialog(m_settings, m_db);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        ReSetPaths(hwnd);
        ReCreateFonts(hwnd);
        DoRefresh(hwnd);
    }
}

void MMainWnd::ReSetPaths(HWND hwnd)
{
    if (m_settings.strWindResExe.size())
    {
        StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_settings.strWindResExe.c_str());
    }
    else
    {
        StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
        StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");
    }

    if (m_settings.strCppExe.size())
    {
        StringCchCopy(m_szCppExe, _countof(m_szCppExe), m_settings.strCppExe.c_str());
    }
    else
    {
        StringCchCopyW(m_szCppExe, _countof(m_szCppExe), m_szDataFolder);
        StringCchCatW(m_szCppExe, _countof(m_szCppExe), L"\\bin\\cpp.exe");
    }
}

void MMainWnd::OnUseIDC_STATIC(HWND hwnd)
{
    BOOL bUseIDC_STATIC = m_db.DoesUseIDC_STATIC();
    bUseIDC_STATIC = !bUseIDC_STATIC;
    m_settings.bUseIDC_STATIC = bUseIDC_STATIC;
    m_db.UseIDC_STATIC(!!bUseIDC_STATIC);

    DoRefresh(hwnd, TRUE);
}

void MMainWnd::OnHideIDMacros(HWND hwnd)
{
    BOOL bHideID = !m_db.AreMacroIDShown();
    bHideID = !bHideID;
    m_settings.bHideID = bHideID;

    m_db.ShowMacroID(!m_settings.bHideID);

    DoRefresh(hwnd, TRUE);
}

void MMainWnd::ShowIDList(HWND hwnd, BOOL bShow/* = TRUE*/)
{
    if (bShow)
    {
        if (IsWindow(m_id_list_dlg))
            DestroyWindow(m_id_list_dlg);
        m_id_list_dlg.CreateDialogDx(NULL);
        ShowWindow(m_id_list_dlg, SW_SHOWNOACTIVATE);
        UpdateWindow(m_id_list_dlg);
    }
    else
    {
        DestroyWindow(m_id_list_dlg);
    }
}

void MMainWnd::OnIDList(HWND hwnd)
{
    ShowIDList(hwnd, TRUE);
}

void MMainWnd::OnIdAssoc(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    MIdAssocDlg dialog(m_settings);
    dialog.DialogBoxDx(hwnd);
    UpdatePrefixDB(hwnd);
}

void MMainWnd::OnShowLangs(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    MLangsDlg dialog;
    dialog.DialogBoxDx(hwnd);
}

void MMainWnd::OnShowHideToolBar(HWND hwnd)
{
    m_settings.bShowToolBar = !m_settings.bShowToolBar;

    if (m_settings.bShowToolBar)
        ShowWindow(m_hToolBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hToolBar, SW_HIDE);

    PostMessageDx(WM_SIZE);
}

void MMainWnd::OnUseOldStyleLangStmt(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    DoRefresh(hwnd, TRUE);
}

void MMainWnd::OnSetPaths(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    MPathsDlg dialog(m_settings);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        ReSetPaths(hwnd);
    }
}

void MMainWnd::OnEditLabel(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) == I_TYPE)
    {
        return;
    }

    UINT i = LOWORD(lParam);
    if (i >= m_entries.size())
        return;

    ResEntry& entry = m_entries[i];

    if (HIWORD(lParam) == I_NAME || HIWORD(lParam) == I_LANG)
    {
        if (entry.type == RT_STRING || entry.type == RT_MESSAGETABLE)
        {
            return;
        }
    }

    HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
    TreeView_EditLabel(m_hTreeView, hItem);
}

void MMainWnd::OnPredefMacros(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, TRUE))
        return;

    MMacrosDlg dialog(m_settings);
    INT_PTR nID = dialog.DialogBoxDx(hwnd);
    switch (INT(nID))
    {
    case IDOK:
        m_settings.macros = dialog.m_map;
        break;
    case psh6:
        m_settings.ResetMacros();
        break;
    }
}

void MMainWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    MWaitCursor wait;
    if (codeNotify == EN_CHANGE && m_hSrcEdit == hwndCtl)
    {
        UpdateOurToolBar(2);
        ChangeStatusText(IDS_READY);
        return;
    }

    if (!::IsWindow(m_rad_window) && id >= 100)
        ChangeStatusText(IDS_EXECUTINGCMD);

    static INT s_nCount = 0;
    ++s_nCount;
    BOOL bUpdateStatus = TRUE;
    switch (id)
    {
    case ID_NEW:
        OnNew(hwnd);
        break;
    case ID_OPEN:
        OnOpen(hwnd);
        break;
    case ID_SAVEAS:
        OnSaveAs(hwnd);
        break;
    case ID_IMPORT:
        OnImport(hwnd);
        break;
    case ID_EXIT:
        DestroyWindow(hwnd);
        break;
    case ID_ADDICON:
        OnAddIcon(hwnd);
        break;
    case ID_ADDCURSOR:
        OnAddCursor(hwnd);
        break;
    case ID_ADDBITMAP:
        OnAddBitmap(hwnd);
        break;
    case ID_ADDRES:
        OnAddRes(hwnd);
        break;
    case ID_REPLACEICON:
        OnReplaceIcon(hwnd);
        break;
    case ID_REPLACECURSOR:
        OnReplaceCursor(hwnd);
        break;
    case ID_REPLACEBITMAP:
        OnReplaceBitmap(hwnd);
        break;
    case ID_REPLACEBIN:
        OnReplaceBin(hwnd);
        break;
    case ID_DELETERES:
        OnDeleteRes(hwnd);
        break;
    case ID_EDIT:
        OnEdit(hwnd);
        break;
    case ID_EXTRACTICON:
        OnExtractIcon(hwnd);
        break;
    case ID_EXTRACTCURSOR:
        OnExtractCursor(hwnd);
        break;
    case ID_EXTRACTBITMAP:
        OnExtractBitmap(hwnd);
        break;
    case ID_EXTRACTBIN:
        OnExtractBin(hwnd);
        break;
    case ID_ABOUT:
        OnAbout(hwnd);
        break;
    case ID_TEST:
        OnTest(hwnd);
        break;
    case ID_CANCELEDIT:
        OnCancelEdit(hwnd);
        break;
    case ID_COMPILE:
        OnCompile(hwnd);
        break;
    case ID_GUIEDIT:
        OnGuiEdit(hwnd);
        break;
    case ID_DESTROYRAD:
        OnCancelEdit(hwnd);
        break;
    case ID_UPDATEDLGRES:
        OnUpdateDlgRes(hwnd);
        bUpdateStatus = FALSE;
        break;
    case ID_DELCTRL:
        MRadCtrl::DeleteSelection();
        break;
    case ID_ADDCTRL:
        m_rad_window.OnAddCtrl(m_rad_window);
        break;
    case ID_CTRLPROP:
        m_rad_window.OnCtrlProp(m_rad_window);
        break;
    case ID_DLGPROP:
        m_rad_window.OnDlgProp(m_rad_window);
        break;
    case ID_CTRLINDEXTOP:
        m_rad_window.IndexTop(m_rad_window);
        break;
    case ID_CTRLINDEXBOTTOM:
        m_rad_window.IndexBottom(m_rad_window);
        break;
    case ID_CTRLINDEXMINUS:
        m_rad_window.IndexMinus(m_rad_window);
        break;
    case ID_CTRLINDEXPLUS:
        m_rad_window.IndexPlus(m_rad_window);
        break;
    case ID_SHOWHIDEINDEX:
        m_rad_window.OnShowHideIndex(m_rad_window);
        break;
    case ID_TOPALIGN:
        m_rad_window.OnTopAlign(m_rad_window);
        break;
    case ID_BOTTOMALIGN:
        m_rad_window.OnBottomAlign(m_rad_window);
        break;
    case ID_LEFTALIGN:
        m_rad_window.OnLeftAlign(m_rad_window);
        break;
    case ID_RIGHTALIGN:
        m_rad_window.OnRightAlign(m_rad_window);
        break;
    case ID_FITTOGRID:
        m_rad_window.OnFitToGrid(m_rad_window);
        bUpdateStatus = FALSE;
        break;
    case ID_STATUSBAR:
        m_settings.bShowStatusBar = !m_settings.bShowStatusBar;
        ShowStatusBar(m_settings.bShowStatusBar);
        PostMessageDx(WM_SIZE);
        break;
    case ID_BINARYPANE:
        m_settings.bShowBinEdit = !m_settings.bShowBinEdit;
        ShowBinEdit(m_settings.bShowBinEdit);
        break;
    case ID_ALWAYSCONTROL:
        {
            m_settings.bAlwaysControl = !m_settings.bAlwaysControl;
            LPARAM lParam = TV_GetParam(m_hTreeView);
            SelectTV(hwnd, lParam, TRUE);
        }
        break;
    case ID_MRUFILE0:
    case ID_MRUFILE1:
    case ID_MRUFILE2:
    case ID_MRUFILE3:
    case ID_MRUFILE4:
    case ID_MRUFILE5:
    case ID_MRUFILE6:
    case ID_MRUFILE7:
    case ID_MRUFILE8:
    case ID_MRUFILE9:
    case ID_MRUFILE10:
    case ID_MRUFILE11:
    case ID_MRUFILE12:
    case ID_MRUFILE13:
    case ID_MRUFILE14:
    case ID_MRUFILE15:
        {
            DWORD i = id - ID_MRUFILE0;
            if (i < m_settings.vecRecentlyUsed.size())
            {
                DoLoadFile(hwnd, m_settings.vecRecentlyUsed[i].c_str());
            }
        }
        break;
    case ID_PLAY:
        OnPlay(hwnd);
        break;
    case ID_READY:
        break;
    case ID_IDASSOC:
        OnIdAssoc(hwnd);
        break;
    case ID_LOADRESH:
        OnLoadResH(hwnd);
        break;
    case ID_IDLIST:
        OnIDList(hwnd);
        break;
    case ID_UNLOADRESH:
        OnUnloadResH(hwnd);
        break;
    case ID_HIDEIDMACROS:
        OnHideIDMacros(hwnd);
        break;
    case ID_USEIDC_STATIC:
        OnUseIDC_STATIC(hwnd);
        break;
    case ID_CONFIG:
        OnConfig(hwnd);
        break;
    case ID_ADVICERESH:
        OnAdviceResH(hwnd);
        break;
    case ID_UPDATEID:
        DoRefresh(hwnd, FALSE);
        break;
    case ID_OPENREADME:
        OnOpenReadMe(hwnd);
        break;
    case ID_OPENREADMEJP:
        OnOpenReadMeJp(hwnd);
        break;
    case ID_LOADWCLIB:
        OnLoadWCLib(hwnd);
        break;
    case ID_FIND:
        OnFind(hwnd);
        break;
    case ID_FINDDOWNWARD:
        OnFindNext(hwnd);
        break;
    case ID_FINDUPWARD:
        OnFindPrev(hwnd);
        break;
    case ID_REPLACE:
        OnReplace(hwnd);
        break;
    case ID_ADDMENU:
        OnAddMenu(hwnd);
        break;
    case ID_ADDVERINFO:
        OnAddVerInfo(hwnd);
        break;
    case ID_ADDMANIFEST:
        OnAddManifest(hwnd);
        break;
    case ID_ADDDIALOG:
        OnAddDialog(hwnd);
        break;
    case ID_ADDSTRINGTABLE:
        OnAddStringTable(hwnd);
        break;
    case ID_ADDMESSAGETABLE:
        OnAddMessageTable(hwnd);
        break;
    case ID_ADDHTML:
        OnAddHtml(hwnd);
        break;
    case ID_ADDACCEL:
        OnAddAccel(hwnd);
        break;
    case ID_COPYASNEWNAME:
        OnCopyAsNewName(hwnd);
        break;
    case ID_COPYASNEWLANG:
        OnCopyAsNewLang(hwnd);
        break;
    case ID_ITEMSEARCH:
        OnItemSearch(hwnd);
        break;
    case ID_ITEMSEARCHBANG:
        OnItemSearchBang(hwnd, reinterpret_cast<MItemSearchDlg *>(hwndCtl));
        break;
    case ID_UPDATERESHBANG:
        OnUpdateResHBang(hwnd);
        break;
    case ID_OPENLICENSE:
        OnOpenLicense(hwnd);
        break;
    case ID_OPENHYOJUNKA:
        OnOpenHyojunka(hwnd);
        break;
    case ID_DEBUGTREENODE:
        OnDebugTreeNode(hwnd);
        break;
    case ID_LOADRESHBANG:
        OnLoadResHBang(hwnd);
        break;
    case ID_REFRESHDIALOG:
        m_rad_window.OnRefresh(m_rad_window);
        break;
    case ID_REFRESHALL:
        DoRefresh(hwnd, TRUE);
        break;
    case ID_EXPORT:
        OnExport(hwnd);
        break;
    case ID_FONTS:
        OnFonts(hwnd);
        break;
    case ID_REFRESH:
        DoRefresh(hwnd, FALSE);
        break;
    case ID_PREDEFMACROS:
        OnPredefMacros(hwnd);
        break;
    case ID_EDITLABEL:
        OnEditLabel(hwnd);
        break;
    case ID_SETPATHS:
        OnSetPaths(hwnd);
        break;
    case ID_USEOLDLANGSTMT:
        OnUseOldStyleLangStmt(hwnd);
        break;
    case ID_SETDEFAULTS:
        SetDefaultSettings(hwnd);
        break;
    case ID_SHOWLANGS:
        OnShowLangs(hwnd);
        break;
    case ID_SHOWHIDETOOLBAR:
        OnShowHideToolBar(hwnd);
        break;
    default:
        bUpdateStatus = FALSE;
        break;
    }
    --s_nCount;

    if (bUpdateStatus && !::IsWindow(m_rad_window) && s_nCount == 0)
        ChangeStatusText(IDS_READY);

#if !defined(NDEBUG) && (WINVER >= 0x0500)
    HANDLE hProcess = GetCurrentProcess();
    TCHAR szText[64];
    StringCchPrintf(szText, _countof(szText), TEXT("GDI:%ld, USER:%ld"), 
             GetGuiResources(hProcess, GR_GDIOBJECTS),
             GetGuiResources(hProcess, GR_USEROBJECTS));
    ChangeStatusText(szText);
#endif
}

WORD GetLangFromText(const WCHAR *pszLang, BOOL bFirstAction = TRUE)
{
    WCHAR szText[128];
    StringCchCopyW(szText, _countof(szText), pszLang);

    ReplaceFullWithHalf(szText);

    if (szText[0] == 0)
    {
        return 0;
    }
    else if (mchr_is_digit(szText[0]) || szText[0] == L'-' || szText[0] == L'+')
    {
        return WORD(mstr_parse_int(szText));
    }
    else
    {
        MStringW str = szText;
        size_t i = str.rfind(L'('); // ')'
        if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
        {
            return WORD(mstr_parse_int(&str[i + 1]));
        }
        for (size_t i = 0; i < g_Langs.size(); ++i)
        {
            WCHAR szText[MAX_PATH];

            StringCchCopyW(szText, _countof(szText), g_Langs[i].str.c_str());
            if (lstrcmpiW(szText, szText) == 0)
            {
                return g_Langs[i].LangID;
            }
            StringCchPrintfW(szText, _countof(szText), L"%s (%u)", g_Langs[i].str.c_str(), g_Langs[i].LangID);
            if (lstrcmpiW(szText, szText) == 0)
            {
                return g_Langs[i].LangID;
            }
        }
    }

    return WORD(0xFFFF);
}

MIdOrString GetNameFromText(const WCHAR *pszText)
{
    WCHAR szText[128];
    StringCchCopyW(szText, _countof(szText), pszText);

    MStringW strFullWidth = LoadStringDx(IDS_FULLWIDTH);
    MStringW strHalfWidth = LoadStringDx(IDS_HALFWIDTH);

    for (DWORD i = 0; szText[i]; ++i)
    {
        size_t k = strFullWidth.find(szText[i]);
        if (k != MStringW::npos)
        {
            szText[i] = strHalfWidth[k];
        }
    }

    if (szText[0] == 0)
    {
        return (WORD)0;
    }
    else if (mchr_is_digit(szText[0]) || szText[0] == L'-' || szText[0] == L'+')
    {
        return WORD(mstr_parse_int(szText));
    }
    else
    {
        MStringW str = szText;
        size_t i = str.rfind(L'('); // ')'
        if (i != MStringW::npos && mchr_is_digit(str[i + 1]))
        {
            return WORD(mstr_parse_int(&str[i + 1]));
        }

        MIdOrString id(szText);
        return id;
    }
}

std::vector<INT> GetPrefixIndexes(RisohSettings& settings, ConstantsDB& db, const MString& prefix)
{
    std::vector<INT> ret;
    assoc_map_type::const_iterator it, end = settings.assoc_map.end();
    for (it = settings.assoc_map.begin(); it != end; ++it)
    {
        if (prefix == it->second && !it->second.empty())
        {
            INT nIDTYPE_ = INT(db.GetValue(L"RESOURCE.ID.TYPE", it->first));
            ret.push_back(nIDTYPE_);
        }
    }
    return ret;
}

LRESULT MMainWnd::OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    MWaitCursor wait;
    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (pnmhdr->code == MSplitterWnd::NOTIFY_CHANGED)
    {
        if (pnmhdr->hwndFrom == m_splitter1)
        {
            if (m_splitter1.GetPaneCount() >= 1)
                m_settings.nTreeViewWidth = m_splitter1.GetPaneExtent(0);
        }
        else if (pnmhdr->hwndFrom == m_splitter2)
        {
            if (m_splitter2.GetPaneCount() >= 2)
                m_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
        }
        else if (pnmhdr->hwndFrom == m_splitter3)
        {
            if (m_splitter3.GetPaneCount() >= 2)
                m_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
        }
    }
    else if (pnmhdr->code == NM_DBLCLK)
    {
        if (pnmhdr->hwndFrom == m_hTreeView)
        {
            switch (HIWORD(lParam))
            {
            case I_LANG:
                OnEdit(hwnd);
                if (m_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case I_STRING:
                OnEdit(hwnd);
                if (m_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case I_MESSAGE:
                OnEdit(hwnd);
                if (m_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            }
        }
    }
    else if (pnmhdr->code == TVN_SELCHANGING)
    {
        if (!m_bLoading)
        {
            if (!CompileIfNecessary(hwnd, FALSE))
                return TRUE;
        }
        if (IsWindow(m_rad_window))
        {
            DestroyWindow(m_rad_window);
        }
    }
    else if (pnmhdr->code == TVN_SELCHANGED)
    {
        if (!m_bLoading)
        {
            NM_TREEVIEWW *pTV = (NM_TREEVIEWW *)pnmhdr;
            LPARAM lParam = pTV->itemNew.lParam;
            SelectTV(hwnd, lParam, FALSE);
        }
    }
    else if (pnmhdr->code == NM_RETURN)
    {
        if (pnmhdr->hwndFrom == m_hTreeView)
        {
            switch (HIWORD(lParam))
            {
            case I_LANG:
                OnEdit(hwnd);
                if (m_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case I_STRING:
                OnEdit(hwnd);
                if (m_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            case I_MESSAGE:
                OnEdit(hwnd);
                if (m_settings.bGuiByDblClick)
                {
                    OnGuiEdit(hwnd);
                }
                return 1;
            }
        }
    }
    else if (pnmhdr->code == TVN_KEYDOWN)
    {
        TV_KEYDOWN *pTVKD = (TV_KEYDOWN *)pnmhdr;
        switch (pTVKD->wVKey)
        {
        case VK_DELETE:
            PostMessageW(hwnd, WM_COMMAND, ID_DELETERES, 0);
            return TRUE;
        case VK_F2:
            {
                LPARAM lParam = TV_GetParam(m_hTreeView);
                if (HIWORD(lParam) == I_TYPE)
                {
                    return TRUE;
                }

                UINT i = LOWORD(lParam);
                ResEntry& entry = m_entries[i];

                if (HIWORD(lParam) == I_NAME || HIWORD(lParam) == I_LANG)
                {
                    if (entry.type == RT_STRING || entry.type == RT_MESSAGETABLE)
                    {
                        return TRUE;
                    }
                }

                HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
                TreeView_EditLabel(m_hTreeView, hItem);
            }
            return TRUE;
        }
    }
    else
    {
        static WORD old_lang = 0xFFFF;
        static WCHAR szOldText[128] = L"";

        if (pnmhdr->code == TVN_BEGINLABELEDIT)
        {
            TV_DISPINFO *pInfo = (TV_DISPINFO *)pnmhdr;
            LPARAM lParam = pInfo->item.lParam;
            HTREEITEM hItem = pInfo->item.hItem;
            LPWSTR pszOldText = pInfo->item.pszText;

            if (HIWORD(lParam) == I_TYPE)
            {
                return TRUE;    // prevent
            }

            UINT i = LOWORD(lParam);
            ResEntry entry = m_entries[i];

            if (HIWORD(lParam) == I_NAME || HIWORD(lParam) == I_LANG)
            {
                if (entry.type == RT_STRING || entry.type == RT_MESSAGETABLE)
                {
                    return TRUE;    // prevent
                }
            }

            StringCchCopyW(szOldText, _countof(szOldText), pszOldText);
            mstr_trim(szOldText);

            if (HIWORD(lParam) == I_LANG || HIWORD(lParam) == I_STRING ||
                HIWORD(lParam) == I_MESSAGE)
            {
                old_lang = GetLangFromText(szOldText);
                if (old_lang == 0xFFFF)
                {
                    return TRUE;    // prevent
                }
            }

            return FALSE;       // accept
        }
        else if (pnmhdr->code == TVN_ENDLABELEDIT)
        {
            TV_DISPINFO *pInfo = (TV_DISPINFO *)pnmhdr;
            LPARAM lParam = pInfo->item.lParam;
            HTREEITEM hItem = pInfo->item.hItem;
            LPWSTR pszNewText = pInfo->item.pszText;
            if (pszNewText == NULL)
                return FALSE;   // reject

            if (HIWORD(lParam) == I_TYPE)
            {
                return FALSE;   // reject
            }

            UINT i = LOWORD(lParam);
            if (i >= m_entries.size())
                return FALSE;   // reject

            ResEntry entry = m_entries[i];

            if (HIWORD(lParam) == I_NAME || HIWORD(lParam) == I_LANG)
            {
                if (entry.type == RT_STRING || entry.type == RT_MESSAGETABLE)
                {
                    return FALSE;   // reject
                }
            }

            WCHAR szNewText[128];
            StringCchCopyW(szNewText, _countof(szNewText), pszNewText);
            mstr_trim(szNewText);

            if (HIWORD(lParam) == I_NAME)
            {
                MIdOrString old_name = GetNameFromText(szOldText);
                MIdOrString new_name = GetNameFromText(szNewText);

                if (old_name.empty())
                    return FALSE;   // reject

                if (new_name.empty())
                    return FALSE;   // reject

                if (old_name.is_str())
                    CharUpper(&old_name.m_str[0]);
                if (new_name.is_str())
                    CharUpper(&new_name.m_str[0]);

                if (old_name == new_name)
                    return FALSE;   // reject

                DoRenameEntry(entry, old_name, new_name);
                return TRUE;   // accept
            }
            else if (HIWORD(lParam) == I_LANG || HIWORD(lParam) == I_STRING ||
                     HIWORD(lParam) == I_MESSAGE)
            {
                old_lang = GetLangFromText(szOldText);
                if (old_lang == 0xFFFF)
                    return FALSE;   // reject

                WORD new_lang = GetLangFromText(szNewText);
                if (new_lang == 0xFFFF)
                    return FALSE;   // reject

                if (old_lang == new_lang)
                    return FALSE;   // reject

                if (HIWORD(lParam) == I_STRING || HIWORD(lParam) == I_MESSAGE)
                    entry.name.clear();

                DoRelangEntry(entry, old_lang, new_lang);
                return TRUE;   // accept
            }

            return FALSE;   // reject
        }
    }
    return 0;
}

void MMainWnd::DoRenameEntry(ResEntry selector, const MIdOrString& old_name, const MIdOrString& new_name)
{
    selector.lang = 0xFFFF;

    for (;;)
    {
        INT iEntry = Res_Find2(m_entries, selector, FALSE);
        if (iEntry == -1)
            break;

        ResEntry& entry = m_entries[iEntry];
        assert(entry.name == old_name);
        entry.name = new_name;
    }

    TV_RefreshInfo(m_hTreeView, m_db, m_entries);

    selector.name = new_name;
    TV_SelectEntry(m_hTreeView, m_entries, selector);
}

void MMainWnd::DoRelangEntry(ResEntry selector, WORD old_lang, WORD new_lang)
{
    for (;;)
    {
        INT iEntry = Res_Find(m_entries, selector, FALSE);
        if (iEntry == -1)
            break;

        ResEntry& entry = m_entries[iEntry];
        assert(entry.lang == old_lang);
        entry.lang = new_lang;
    }

    TV_RefreshInfo(m_hTreeView, m_db, m_entries);

    selector.lang = new_lang;
    TV_SelectEntry(m_hTreeView, m_entries, selector);
}

void MMainWnd::OnTest(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    HTREEITEM hItem = TreeView_GetSelection(m_hTreeView);
    if (hItem == NULL)
        return;

    TV_ITEM Item;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = TVIF_PARAM;
    Item.hItem = hItem;
    TreeView_GetItem(m_hTreeView, &Item);

    if (HIWORD(Item.lParam) != 3)
        return;

    UINT i = LOWORD(Item.lParam);
    const ResEntry& entry = m_entries[i];
    if (entry.type == RT_DIALOG)
    {
        MByteStreamEx stream(entry.data);

        // detach menu
        DialogRes dialog_res(m_db);
        dialog_res.LoadFromStream(stream);

        if (!dialog_res.m_class.empty())
        {
            ErrorBoxDx(IDS_CANTTESTCLASSDLG);
        }
        else
        {
            MIdOrString menu = dialog_res.m_menu;
            dialog_res.m_menu.clear();
            stream.clear();

            // TODO: OLE support
            dialog_res.Fixup2(false);
            dialog_res.SaveToStream(stream);
            dialog_res.Fixup2(true);

            // show test dialog
            if (dialog_res.m_style & WS_CHILD)
            {
                MTestParentWnd *window = new MTestParentWnd(m_entries, dialog_res, menu, entry.lang, stream);
                window->CreateWindowDx(hwnd, LoadStringDx(IDS_PARENTWND),
                    WS_DLGFRAME | WS_POPUPWINDOW, WS_EX_APPWINDOW);

                ShowWindow(*window, SW_SHOWNORMAL);
                UpdateWindow(*window);
            }
            else
            {
                MTestDialog dialog(m_entries, dialog_res, menu, entry.lang);
                dialog.DialogBoxIndirectDx(hwnd, stream.ptr());
            }
        }
    }
    else if (entry.type == RT_MENU)
    {
        HMENU hMenu = LoadMenuIndirect(&entry[0]);
        if (hMenu)
        {
            MTestMenuDlg dialog(hMenu);
            dialog.DialogBoxDx(hwnd, IDD_MENUTEST);
            DestroyMenu(hMenu);
        }
    }
}

void MMainWnd::JoinLinesByBackslash(std::vector<std::string>& lines)
{
    // join by '\\'
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string& line = lines[i];
        if (line.size())
        {
            if (line[line.size() - 1] == '\\')
            {
                line = line.substr(0, line.size() - 1);
                lines[i] = line + lines[i + 1];
                lines.erase(lines.begin() + (i + 1));
                --i;
            }
        }
    }
}

void MMainWnd::DeleteIncludeGuard(std::vector<std::string>& lines)
{
    size_t k0 = -1, k1 = -1;
    std::string name0;

    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "ifndef", 6) == 0 && mchr_is_space(pch[6]))
        {
            // #ifndef
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            std::string name(pch0, pch);

            if (name0.empty())
            {
                k0 = i;
                name0 = name;
            }
            else
            {
                name0.clear();
                break;
            }
        }
        else if (memcmp(pch, "if", 2) == 0)
        {
            break;
        }
        else if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
        {
            if (name0.empty())
                break;

            // #define
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            std::string name(pch0, pch);
            if (name0 == name)
            {
                k1 = i;
                break;
            }
        }
        else
        {
            // otherwise
            break;
        }
    }

    if (name0.empty())
        return;

    for (size_t i = lines.size(); i > 0; )
    {
        --i;
        std::string& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "endif", 5) == 0)
        {
            lines.erase(lines.begin() + i);
            lines.erase(lines.begin() + k1);
            lines.erase(lines.begin() + k0);
            break;
        }
        else
        {
            break;
        }
    }
}

void MMainWnd::AddHeadComment(std::vector<std::string>& lines)
{
    if (m_szNominalFile[0])
    {
        WCHAR title[64];
        GetFileTitleW(m_szNominalFile, title, _countof(title));
        std::string line = "// ";
        line += MWideToAnsi(CP_ACP, title).c_str();
        lines.insert(lines.begin(), line);
    }
    lines.insert(lines.begin(), "// Microsoft Visual C++ Compatible");
    lines.insert(lines.begin(), "//{{NO_DEPENDENCIES}}");
}

void MMainWnd::DeleteHeadComment(std::vector<std::string>& lines)
{
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string& line = lines[i];
        if (line.find("//") == 0)
        {
            if (line.find("{{NO_DEPENDENCIES}}") != std::string::npos ||
                line.find("Microsoft Visual C++") != std::string::npos ||
                line.find(".rc") != std::string::npos)
            {
                lines.erase(lines.begin() + i);
                --i;
            }
        }
    }
}

void MMainWnd::DeleteSpecificMacroLines(std::vector<std::string>& lines)
{
    for (size_t i = lines.size(); i > 0; )
    {
        --i;
        std::string& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
        {
            // #define
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            std::string name(pch0, pch);

            if (m_settings.removed_ids.find(name) != m_settings.removed_ids.end())
            {
                lines.erase(lines.begin() + i);
            }
        }
    }
}

void MMainWnd::AddAdditionalMacroLines(std::vector<std::string>& lines)
{
    std::string str;
    id_map_type::iterator it, end = m_settings.added_ids.end();
    for (it = m_settings.added_ids.begin(); it != end; ++it)
    {
        std::string line = "#define ";
        if (it->first == "IDC_STATIC")
        {
            line += "IDC_STATIC -1";
        }
        else
        {
            line += it->first;
            line += " ";
            line += it->second;
        }
        lines.push_back(line);
    }
}

void MMainWnd::AddApStudioBlock(std::vector<std::string>& lines)
{
    UINT anValues[5];
    DoIDStat(anValues);

    lines.push_back("#ifdef APSTUDIO_INVOKED");
    lines.push_back("    #ifndef APSTUDIO_READONLY_SYMBOLS");

    char buf[256];
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NO_MFC                 %u", anValues[0]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_RESOURCE_VALUE    %u", anValues[1]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_COMMAND_VALUE     %u", anValues[2]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_CONTROL_VALUE     %u", anValues[3]);
    lines.push_back(buf);
    StringCchPrintfA(buf, _countof(buf), "        #define _APS_NEXT_SYMED_VALUE       %u", anValues[4]);
    lines.push_back(buf);
    lines.push_back("    #endif");
    lines.push_back("#endif");
}

void MMainWnd::DeleteApStudioBlock(std::vector<std::string>& lines)
{
    bool inside = false, found = false;
    size_t nest = 0, k = -1;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        std::string& line = lines[i];
        const char *pch = mstr_skip_space(&line[0]);
        if (*pch != '#')
            continue;

        ++pch;
        pch = mstr_skip_space(pch);
        if (memcmp(pch, "ifdef", 5) == 0 && mchr_is_space(pch[5]))
        {
            // #ifdef
            pch += 5;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            std::string name(pch0, pch);

            if (name == "APSTUDIO_INVOKED")
            {
                inside = true;
                found = false;
                k = i;
                ++nest;
            }
        }
        else if (memcmp(pch, "if", 2) == 0)
        {
            ++nest;
        }
        else if (memcmp(pch, "define", 6) == 0 && mchr_is_space(pch[6]))
        {
            if (!inside)
                continue;

            // #define
            pch += 6;
            const char *pch0 = pch = mstr_skip_space(pch);
            while (std::isalnum(*pch) || *pch == '_')
            {
                ++pch;
            }
            std::string name(pch0, pch);

            if (name == "_APS_NEXT_RESOURCE_VALUE")
            {
                found = true;
            }
        }
        else if (memcmp(pch, "endif", 5) == 0)
        {
            --nest;
            if (nest == 0 && k != -1)
            {
                lines.erase(lines.begin() + k, lines.begin() + i + 1);
                break;
            }
        }
    }
}

void MMainWnd::UpdateResHLines(std::vector<std::string>& lines)
{
    JoinLinesByBackslash(lines);
    DeleteIncludeGuard(lines);
    DeleteHeadComment(lines);
    DeleteSpecificMacroLines(lines);
    AddAdditionalMacroLines(lines);
    DeleteApStudioBlock(lines);
    AddApStudioBlock(lines);
    AddHeadComment(lines);
}

void MMainWnd::ReadResHLines(FILE *fp, std::vector<std::string>& lines)
{
    // read lines
    CHAR buf[512];
    while (fgets(buf, _countof(buf), fp) != NULL)
    {
        size_t len = std::strlen(buf);
        if (len == 0)
            break;
        if (buf[len - 1] == '\n')
            buf[len - 1] = 0;
        lines.push_back(buf);
    }
}

void MMainWnd::OnUpdateResHBang(HWND hwnd)
{
    BOOL bListOpen = IsWindow(m_id_list_dlg);

    DestroyWindow(m_id_list_dlg);

    if (MsgBoxDx(IDS_UPDATERESH, MB_ICONINFORMATION | MB_YESNO) == IDNO)
    {
        ShowIDList(hwnd, bListOpen);
        return;
    }

    if (m_szResourceH[0] == 0 || GetFileAttributes(m_szResourceH) == 0xFFFFFFFF)
    {
        WCHAR szResH[MAX_PATH];

        // build file path
        if (m_szNominalFile[0])
        {
            StringCchCopyW(szResH, _countof(szResH), m_szNominalFile);
            WCHAR *pch = wcsrchr(szResH, L'\\');
            *pch = 0;
            StringCchCatW(pch, _countof(szResH), L"\\resource.h");
        }
        else
        {
            StringCchCopyW(szResH, _countof(szResH), L"resource.h");
        }

        if (GetFileAttributesW(szResH) == INVALID_FILE_ATTRIBUTES)
            szResH[0] = 0;

        // query file name
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_HEADFILTER));
        ofn.lpstrFile = szResH;
        ofn.nMaxFile = _countof(szResH);
        ofn.lpstrTitle = LoadStringDx(IDS_SAVERESH);
        ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY |
            OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"h";
        if (!GetSaveFileNameW(&ofn))
        {
            return;
        }

        // create new
        if (!DoWriteResH(NULL, szResH))
        {
            ErrorBoxDx(IDS_CANTWRITERESH);
            ShowIDList(hwnd, bListOpen);
            return;
        }

        lstrcpynW(m_szResourceH, szResH, MAX_PATH);
    }
    else
    {
        DoBackupFile(m_szResourceH);

        // open file
        FILE *fp;
        _wfopen_s(&fp, m_szResourceH, L"r");
        if (!fp)
        {
            ErrorBoxDx(IDS_CANTWRITERESH);
            ShowIDList(hwnd, bListOpen);
            return;
        }

        std::vector<std::string> lines;

        ReadResHLines(fp, lines);

        fclose(fp);

        UpdateResHLines(lines);

        // write now
        _wfopen_s(&fp, m_szResourceH, L"w");
        if (!fp)
        {
            ErrorBoxDx(IDS_CANTWRITERESH);
            ShowIDList(hwnd, bListOpen);
            return;
        }
        for (size_t i = 0; i < lines.size(); ++i)
        {
            fprintf(fp, "%s\n", lines[i].c_str());
        }
        fclose(fp);
    }

    // clear modification
    m_settings.added_ids.clear();
    m_settings.removed_ids.clear();

    ShowIDList(hwnd, bListOpen);
}

void MMainWnd::OnAddIcon(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddIconDlg dialog(m_db, m_entries);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnReplaceIcon(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    MReplaceIconDlg dialog(m_db, m_entries, m_entries[i]);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnReplaceCursor(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    MReplaceCursorDlg dialog(m_db, m_entries, m_entries[i]);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddBitmap(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddBitmapDlg dialog(m_db, m_entries);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnReplaceBitmap(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    LPARAM lParam = TV_GetParam(m_hTreeView);
    if (HIWORD(lParam) != I_LANG)
        return;

    UINT i = LOWORD(lParam);
    MReplaceBitmapDlg dialog(m_db, m_entries, m_entries[i]);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddCursor(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddCursorDlg dialog(m_db, m_entries);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddRes(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddMenu(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_MENU;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddStringTable(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_STRING;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddMessageTable(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_MESSAGETABLE;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddHtml(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_HTML;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddAccel(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_ACCELERATOR;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddVerInfo(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_VERSION;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddManifest(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_MANIFEST;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

void MMainWnd::OnAddDialog(HWND hwnd)
{
    if (!CompileIfNecessary(hwnd, FALSE))
        return;

    MAddResDlg dialog(m_entries, m_db);
    dialog.m_type = RT_DIALOG;
    if (dialog.DialogBoxDx(hwnd) == IDOK)
    {
        DoRefreshIDList(hwnd);
        TV_RefreshInfo(m_hTreeView, m_db, m_entries);
        TV_SelectEntry(m_hTreeView, m_entries, dialog.m_entry_copy);
    }
}

BOOL MMainWnd::SetFilePath(HWND hwnd, LPCWSTR pszRealFile, LPCWSTR pszNominal)
{
    if (pszRealFile == 0 || pszRealFile[0] == 0)
    {
        SetWindowTextW(hwnd, LoadStringDx(IDS_APPNAME));
        return TRUE;
    }

    if (pszNominal == NULL)
        pszNominal = pszRealFile;

    WCHAR szPath[MAX_PATH], *pch;

    GetFullPathNameW(pszRealFile, _countof(szPath), szPath, &pch);
    lstrcpynW(m_szRealFile, szPath, _countof(m_szRealFile));

    GetFullPathNameW(pszNominal, _countof(szPath), szPath, &pch);
    lstrcpynW(m_szNominalFile, szPath, _countof(m_szNominalFile));

    pch = wcsrchr(szPath, L'\\');
    if (pch == NULL)
        pch = wcsrchr(szPath, L'/');
    if (pch)
    {
        SetWindowTextW(hwnd, LoadStringPrintfDx(IDS_TITLEWITHFILE, pch + 1));
    }
    else
    {
        SetWindowTextW(hwnd, LoadStringDx(IDS_APPNAME));
    }

    m_settings.AddFile(m_szNominalFile);
    UpdateMenu();
    return TRUE;
}

void MMainWnd::SetDefaultSettings(HWND hwnd)
{
    m_settings.bShowBinEdit = TRUE;
    m_settings.bAlwaysControl = FALSE;
    m_settings.bShowStatusBar = TRUE;
    m_settings.nTreeViewWidth = TV_WIDTH;
    m_settings.nBmpViewWidth = BV_WIDTH;
    m_settings.nBinEditHeight = BE_HEIGHT;
    m_settings.bGuiByDblClick = TRUE;
    m_settings.bResumeWindowPos = TRUE;
    m_settings.bAutoLoadNearbyResH = TRUE;
    m_settings.bAutoShowIDList = TRUE;
    m_settings.bShowDotsOnDialog = TRUE;
    m_settings.nComboHeight = 300;
    m_settings.vecRecentlyUsed.clear();
    m_settings.nWindowLeft = CW_USEDEFAULT;
    m_settings.nWindowTop = CW_USEDEFAULT;
    m_settings.nWindowWidth = 760;
    m_settings.nWindowHeight = 480;
    m_settings.bMaximized = FALSE;
    m_settings.nIDListLeft = CW_USEDEFAULT;
    m_settings.nIDListTop = CW_USEDEFAULT;
    m_settings.nIDListWidth = 366;
    m_settings.nIDListHeight = 490;
    m_settings.nRadLeft = CW_USEDEFAULT;
    m_settings.nRadTop = CW_USEDEFAULT;
    m_settings.bAskUpdateResH = FALSE;
    m_settings.bCompressByUPX = FALSE;

    HFONT hFont;
    LOGFONTW lf, lfBin, lfSrc;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);

    ZeroMemory(&lfBin, sizeof(lfBin));
    lfBin.lfHeight = 10;
    lfBin.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lfBin.lfCharSet = lf.lfCharSet;
    hFont = CreateFontIndirectW(&lfBin);
    GetObject(hFont, sizeof(lfBin), &lfBin);
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        SelectObject(hDC, hFont);
        GetTextFace(hDC, LF_FACESIZE, lfBin.lfFaceName);
        DeleteDC(hDC);
    }
    DeleteObject(hFont);

    ZeroMemory(&lfSrc, sizeof(lfSrc));
    lfSrc.lfHeight = 13;
    lfSrc.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lfSrc.lfCharSet = lf.lfCharSet;
    hFont = CreateFontIndirectW(&lfSrc);
    GetObject(hFont, sizeof(lfSrc), &lfSrc);
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        SelectObject(hDC, hFont);
        GetTextFace(hDC, LF_FACESIZE, lfSrc.lfFaceName);
        DeleteDC(hDC);
    }
    DeleteObject(hFont);

    m_settings.strSrcFont = lfSrc.lfFaceName;
    m_settings.strBinFont = lfBin.lfFaceName;

    m_settings.nSrcFontSize = 12;
    m_settings.nBinFontSize = 9;

    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        if (lfBin.lfHeight < 0)
        {
            m_settings.nBinFontSize = -MulDiv(lfBin.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }
        else
        {
            m_settings.nBinFontSize = MulDiv(lfBin.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }

        if (lfSrc.lfHeight < 0)
        {
            m_settings.nSrcFontSize = -MulDiv(lfSrc.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }
        else
        {
            m_settings.nSrcFontSize = MulDiv(lfSrc.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
        }

        DeleteDC(hDC);
    }

    ConstantsDB::TableType table1, table2;
    table1 = m_db.GetTable(L"RESOURCE.ID.TYPE");
    table2 = m_db.GetTable(L"RESOURCE.ID.PREFIX");
    assert(table1.size() == table2.size());

    m_settings.assoc_map.clear();
    if (table1.size() && table1.size() == table2.size())
    {
        for (size_t i = 0; i < table1.size(); ++i)
        {
            m_settings.assoc_map.insert(std::make_pair(table1[i].name, table2[i].name));
        }
    }
    else
    {
        m_settings.ResetAssoc();
    }

    m_settings.id_map.clear();
    m_settings.added_ids.clear();
    m_settings.removed_ids.clear();

    m_settings.ResetMacros();

    m_settings.includes.clear();

    m_settings.strWindResExe.clear();
    m_settings.strCppExe.clear();

    // cpp.exe
    StringCchCopyW(m_szCppExe, _countof(m_szCppExe), m_szDataFolder);
    StringCchCatW(m_szCppExe, _countof(m_szCppExe), L"\\bin\\cpp.exe");

    // windres.exe
    StringCchCopyW(m_szWindresExe, _countof(m_szWindresExe), m_szDataFolder);
    StringCchCatW(m_szWindresExe, _countof(m_szWindresExe), L"\\bin\\windres.exe");

    m_settings.strPrevVersion.clear();

    m_settings.bSepFilesByLang = FALSE;
    m_settings.bStoreToResFolder = TRUE;
    m_settings.bSelectableByMacro = FALSE;

    m_settings.captions.clear();

    m_settings.bShowToolBar = TRUE;

    m_settings.strAtlAxWin = L"AtlAxWin110";
}

void MMainWnd::UpdatePrefixDB(HWND hwnd)
{
    // update "RESOURCE.ID.PREFIX" table
    ConstantsDB::TableType& table = m_db.m_map[L"RESOURCE.ID.PREFIX"];
    for (size_t i = 0; i < table.size(); ++i)
    {
        assoc_map_type::const_iterator it, end = m_settings.assoc_map.end();
        for (it = m_settings.assoc_map.begin(); it != end; ++it)
        {
            if (table[i].name == it->first)
            {
                table[i].value = mstr_parse_int(it->second.c_str());
                break;
            }
        }
    }
}

BOOL MMainWnd::LoadSettings(HWND hwnd)
{
    SetDefaultSettings(hwnd);

    MRegKey key(HKCU, TEXT("Software"));
    if (!key)
        return FALSE;

    MRegKey keySoftware(key, TEXT("Katayama Hirofumi MZ"));
    if (!keySoftware)
        return FALSE;

    MRegKey keyRisoh(keySoftware, TEXT("RisohEditor"));
    if (!keyRisoh)
        return FALSE;

    BOOL bHideID = !m_db.AreMacroIDShown();
    keyRisoh.QueryDword(TEXT("HIDE.ID"), (DWORD&)bHideID);
    m_db.ShowMacroID(!bHideID);
    m_settings.bHideID = bHideID;

    BOOL bUseIDC_STATIC = m_db.DoesUseIDC_STATIC();
    keyRisoh.QueryDword(TEXT("bUseIDC_STATIC"), (DWORD&)bUseIDC_STATIC);
    m_settings.bUseIDC_STATIC = bUseIDC_STATIC;
    m_db.UseIDC_STATIC(!!bUseIDC_STATIC);
    m_db.AddIDC_STATIC();
    m_settings.AddIDC_STATIC();

    keyRisoh.QueryDword(TEXT("ShowStatusBar"), (DWORD&)m_settings.bShowStatusBar);
    keyRisoh.QueryDword(TEXT("ShowBinEdit"), (DWORD&)m_settings.bShowBinEdit);
    keyRisoh.QueryDword(TEXT("AlwaysControl"), (DWORD&)m_settings.bAlwaysControl);
    keyRisoh.QueryDword(TEXT("TreeViewWidth"), (DWORD&)m_settings.nTreeViewWidth);
    keyRisoh.QueryDword(TEXT("BmpViewWidth"), (DWORD&)m_settings.nBmpViewWidth);
    keyRisoh.QueryDword(TEXT("BinEditHeight"), (DWORD&)m_settings.nBinEditHeight);
    keyRisoh.QueryDword(TEXT("bGuiByDblClick"), (DWORD&)m_settings.bGuiByDblClick);
    keyRisoh.QueryDword(TEXT("bResumeWindowPos"), (DWORD&)m_settings.bResumeWindowPos);
    keyRisoh.QueryDword(TEXT("bAutoLoadNearbyResH"), (DWORD&)m_settings.bAutoLoadNearbyResH);
    keyRisoh.QueryDword(TEXT("bAutoShowIDList"), (DWORD&)m_settings.bAutoShowIDList);
    keyRisoh.QueryDword(TEXT("bShowDotsOnDialog"), (DWORD&)m_settings.bShowDotsOnDialog);
    keyRisoh.QueryDword(TEXT("nComboHeight"), (DWORD&)m_settings.nComboHeight);
    keyRisoh.QueryDword(TEXT("nWindowLeft"), (DWORD&)m_settings.nWindowLeft);
    keyRisoh.QueryDword(TEXT("nWindowTop"), (DWORD&)m_settings.nWindowTop);
    keyRisoh.QueryDword(TEXT("nWindowWidth"), (DWORD&)m_settings.nWindowWidth);
    keyRisoh.QueryDword(TEXT("nWindowHeight"), (DWORD&)m_settings.nWindowHeight);
    keyRisoh.QueryDword(TEXT("bMaximized"), (DWORD&)m_settings.bMaximized);
    keyRisoh.QueryDword(TEXT("nIDListLeft"), (DWORD&)m_settings.nIDListLeft);
    keyRisoh.QueryDword(TEXT("nIDListTop"), (DWORD&)m_settings.nIDListTop);
    keyRisoh.QueryDword(TEXT("nIDListWidth"), (DWORD&)m_settings.nIDListWidth);
    keyRisoh.QueryDword(TEXT("nIDListHeight"), (DWORD&)m_settings.nIDListHeight);
    keyRisoh.QueryDword(TEXT("nRadLeft"), (DWORD&)m_settings.nRadLeft);
    keyRisoh.QueryDword(TEXT("nRadTop"), (DWORD&)m_settings.nRadTop);
    keyRisoh.QueryDword(TEXT("bAskUpdateResH"), (DWORD&)m_settings.bAskUpdateResH);
    keyRisoh.QueryDword(TEXT("bCompressByUPX"), (DWORD&)m_settings.bCompressByUPX);

    TCHAR szText[128];
    TCHAR szValueName[128];

    DWORD dwMacroCount = 0;
    if (keyRisoh.QueryDword(TEXT("dwMacroCount"), dwMacroCount) == ERROR_SUCCESS)
    {
        m_settings.macros.clear();

        for (DWORD i = 0; i < dwMacroCount; ++i)
        {
            MString key, value;

            StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroName%lu"), i);
            if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
                key = szText;

            StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroValue%lu"), i);
            if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
                value = szText;

            if (!key.empty())
                m_settings.macros.insert(std::make_pair(key, value));
        }
    }

    DWORD dwNumIncludes = 0;
    if (keyRisoh.QueryDword(TEXT("dwNumIncludes"), dwNumIncludes) == ERROR_SUCCESS)
    {
        m_settings.includes.clear();

        for (DWORD i = 0; i < dwNumIncludes; ++i)
        {
            MString value;

            StringCchPrintf(szValueName, _countof(szValueName), TEXT("Include%lu"), i);
            if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
                value = szText;

            if (!value.empty())
                m_settings.includes.push_back(value);
        }
    }

    if (keyRisoh.QuerySz(TEXT("strSrcFont"), szText, _countof(szText)) == ERROR_SUCCESS)
        m_settings.strSrcFont = szText;
    keyRisoh.QueryDword(TEXT("nSrcFontSize"), (DWORD&)m_settings.nSrcFontSize);

    if (keyRisoh.QuerySz(TEXT("strBinFont"), szText, _countof(szText)) == ERROR_SUCCESS)
        m_settings.strBinFont = szText;
    keyRisoh.QueryDword(TEXT("nBinFontSize"), (DWORD&)m_settings.nBinFontSize);

    INT xVirtualScreen = GetSystemMetrics(SM_XVIRTUALSCREEN);
    INT yVirtualScreen = GetSystemMetrics(SM_YVIRTUALSCREEN);
    INT cxVirtualScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    INT cyVirtualScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    INT cxMin = 200, cyMin = 180;

    if (m_settings.nWindowLeft < xVirtualScreen)
        m_settings.nWindowLeft = CW_USEDEFAULT;
    if (m_settings.nWindowTop < yVirtualScreen)
        m_settings.nWindowTop = CW_USEDEFAULT;
    if (m_settings.nWindowWidth <= cxMin)
        m_settings.nWindowWidth = 760;
    if (m_settings.nWindowHeight <= cyMin)
        m_settings.nWindowHeight = 480;
    if (m_settings.nWindowLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
        m_settings.nWindowLeft = CW_USEDEFAULT;
    if (m_settings.nWindowTop + cyMin >= yVirtualScreen + cyVirtualScreen)
        m_settings.nWindowTop = CW_USEDEFAULT;
    if (m_settings.nWindowTop == CW_USEDEFAULT)
        m_settings.nWindowLeft = CW_USEDEFAULT;

    if (m_settings.nIDListLeft < xVirtualScreen)
        m_settings.nIDListLeft = CW_USEDEFAULT;
    if (m_settings.nIDListTop < yVirtualScreen)
        m_settings.nIDListTop = CW_USEDEFAULT;
    if (m_settings.nIDListWidth <= cxMin)
        m_settings.nIDListWidth = 366;
    if (m_settings.nIDListHeight <= cyMin)
        m_settings.nIDListHeight = 490;
    if (m_settings.nIDListLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
        m_settings.nIDListLeft = CW_USEDEFAULT;
    if (m_settings.nIDListTop + cyMin >= yVirtualScreen + cyVirtualScreen)
        m_settings.nIDListTop = CW_USEDEFAULT;
    if (m_settings.nIDListTop == CW_USEDEFAULT)
        m_settings.nIDListLeft = CW_USEDEFAULT;

    if (m_settings.nRadLeft < xVirtualScreen)
        m_settings.nRadLeft = CW_USEDEFAULT;
    if (m_settings.nRadTop < yVirtualScreen)
        m_settings.nRadTop = CW_USEDEFAULT;
    if (m_settings.nRadLeft + cxMin >= xVirtualScreen + cxVirtualScreen)
        m_settings.nRadLeft = CW_USEDEFAULT;
    if (m_settings.nRadTop + cyMin >= yVirtualScreen + cyVirtualScreen)
        m_settings.nRadTop = CW_USEDEFAULT;
    if (m_settings.nRadTop == CW_USEDEFAULT)
        m_settings.nRadLeft = CW_USEDEFAULT;

    DWORD i, dwCount;
    keyRisoh.QueryDword(TEXT("FileCount"), dwCount);
    if (dwCount > MAX_MRU)
        dwCount = MAX_MRU;

    TCHAR szFormat[32], szFile[MAX_PATH];
    for (i = 0; i < dwCount; ++i)
    {
        StringCchPrintf(szFormat, _countof(szFormat), TEXT("File%lu"), i);
        if (keyRisoh.QuerySz(szFormat, szFile, _countof(szFile)) == ERROR_SUCCESS)
        {
            if (GetFileAttributes(szFile) != 0xFFFFFFFF)
            {
                m_settings.vecRecentlyUsed.push_back(szFile);
            }
        }
    }

    if (keyRisoh.QuerySz(TEXT("strWindResExe"), szText, _countof(szText)) == ERROR_SUCCESS)
    {
        if (GetFileAttributesW(szText) != 0xFFFFFFFF)
            m_settings.strWindResExe = szText;
    }

    if (keyRisoh.QuerySz(TEXT("strCppExe"), szText, _countof(szText)) == ERROR_SUCCESS)
    {
        if (GetFileAttributesW(szText) != 0xFFFFFFFF)
            m_settings.strCppExe = szText;
    }

    if (keyRisoh.QuerySz(TEXT("strPrevVersion"), szText, _countof(szText)) == ERROR_SUCCESS)
    {
        m_settings.strPrevVersion = szText;
    }

    // update association if version > 3.8
    if (!m_settings.strPrevVersion.empty() && m_settings.strPrevVersion > L"3.8")
    {
        TCHAR szName[MAX_PATH];
        assoc_map_type::iterator it, end = m_settings.assoc_map.end();
        for (it = m_settings.assoc_map.begin(); it != end; ++it)
        {
            if (keyRisoh.QuerySz(it->first.c_str(), szName, _countof(szName)) == ERROR_SUCCESS)
            {
                it->second = szName;
            }
        }
        UpdatePrefixDB(hwnd);
    }

    keyRisoh.QueryDword(TEXT("bSepFilesByLang"), (DWORD&)m_settings.bSepFilesByLang);
    keyRisoh.QueryDword(TEXT("bStoreToResFolder"), (DWORD&)m_settings.bStoreToResFolder);
    keyRisoh.QueryDword(TEXT("bSelectableByMacro"), (DWORD&)m_settings.bSelectableByMacro);

    DWORD dwNumCaptions = 0;
    keyRisoh.QueryDword(TEXT("dwNumCaptions"), (DWORD&)dwNumCaptions);

    if (dwNumCaptions > s_nMaxCaptions)
        dwNumCaptions = s_nMaxCaptions;

    captions_type captions;
    for (DWORD i = 0; i < dwNumCaptions; ++i)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("Caption%lu"), i);
        if (keyRisoh.QuerySz(szValueName, szText, _countof(szText)) == ERROR_SUCCESS)
        {
            m_settings.captions.push_back(szText);
        }
    }

    keyRisoh.QueryDword(TEXT("bShowToolBar"), (DWORD&)m_settings.bShowToolBar);

    if (keyRisoh.QuerySz(L"strAtlAxWin", szText, _countof(szText)) == ERROR_SUCCESS)
    {
        m_settings.strAtlAxWin = szText;
    }

    return TRUE;
}

BOOL MMainWnd::SaveSettings(HWND hwnd)
{
    MRegKey key(HKCU, TEXT("Software"), TRUE);
    if (!key)
        return FALSE;

    MRegKey keySoftware(key, TEXT("Katayama Hirofumi MZ"), TRUE);
    if (!keySoftware)
        return FALSE;

    MRegKey keyRisoh(keySoftware, TEXT("RisohEditor"), TRUE);
    if (!keyRisoh)
        return FALSE;

    if (m_splitter3.GetPaneCount() >= 2)
        m_settings.nBmpViewWidth = m_splitter3.GetPaneExtent(1);
    if (m_splitter2.GetPaneCount() >= 2)
        m_settings.nBinEditHeight = m_splitter2.GetPaneExtent(1);
    if (m_splitter1.GetPaneCount() >= 1)
        m_settings.nTreeViewWidth = m_splitter1.GetPaneExtent(0);

    BOOL bHideID = !m_db.AreMacroIDShown();
    m_settings.bHideID = bHideID;
    keyRisoh.SetDword(TEXT("HIDE.ID"), m_settings.bHideID);

    BOOL bUseIDC_STATIC = m_db.DoesUseIDC_STATIC();
    m_settings.bUseIDC_STATIC = bUseIDC_STATIC;
    keyRisoh.SetDword(TEXT("bUseIDC_STATIC"), m_settings.bUseIDC_STATIC);

    keyRisoh.SetDword(TEXT("ShowStatusBar"), m_settings.bShowStatusBar);
    keyRisoh.SetDword(TEXT("ShowBinEdit"), m_settings.bShowBinEdit);
    keyRisoh.SetDword(TEXT("AlwaysControl"), m_settings.bAlwaysControl);
    keyRisoh.SetDword(TEXT("TreeViewWidth"), m_settings.nTreeViewWidth);
    keyRisoh.SetDword(TEXT("BmpViewWidth"), m_settings.nBmpViewWidth);
    keyRisoh.SetDword(TEXT("BinEditHeight"), m_settings.nBinEditHeight);
    keyRisoh.SetDword(TEXT("bGuiByDblClick"), m_settings.bGuiByDblClick);
    keyRisoh.SetDword(TEXT("bResumeWindowPos"), m_settings.bResumeWindowPos);
    keyRisoh.SetDword(TEXT("bAutoLoadNearbyResH"), m_settings.bAutoLoadNearbyResH);
    keyRisoh.SetDword(TEXT("bAutoShowIDList"), m_settings.bAutoShowIDList);
    keyRisoh.SetDword(TEXT("bShowDotsOnDialog"), m_settings.bShowDotsOnDialog);
    keyRisoh.SetDword(TEXT("nComboHeight"), m_settings.nComboHeight);
    keyRisoh.SetDword(TEXT("nWindowLeft"), m_settings.nWindowLeft);
    keyRisoh.SetDword(TEXT("nWindowTop"), m_settings.nWindowTop);
    keyRisoh.SetDword(TEXT("nWindowWidth"), m_settings.nWindowWidth);
    keyRisoh.SetDword(TEXT("nWindowHeight"), m_settings.nWindowHeight);
    keyRisoh.SetDword(TEXT("bMaximized"), m_settings.bMaximized);
    keyRisoh.SetDword(TEXT("nIDListLeft"), m_settings.nIDListLeft);
    keyRisoh.SetDword(TEXT("nIDListTop"), m_settings.nIDListTop);
    keyRisoh.SetDword(TEXT("nIDListWidth"), m_settings.nIDListWidth);
    keyRisoh.SetDword(TEXT("nIDListHeight"), m_settings.nIDListHeight);
    keyRisoh.SetDword(TEXT("nRadLeft"), m_settings.nRadLeft);
    keyRisoh.SetDword(TEXT("nRadTop"), m_settings.nRadTop);
    keyRisoh.SetDword(TEXT("bAskUpdateResH"), m_settings.bAskUpdateResH);
    keyRisoh.SetDword(TEXT("bCompressByUPX"), m_settings.bCompressByUPX);
    keyRisoh.SetSz(TEXT("strSrcFont"), m_settings.strSrcFont.c_str());
    keyRisoh.SetDword(TEXT("nSrcFontSize"), m_settings.nSrcFontSize);
    keyRisoh.SetSz(TEXT("strBinFont"), m_settings.strBinFont.c_str());
    keyRisoh.SetDword(TEXT("nBinFontSize"), m_settings.nBinFontSize);

    DWORD i, dwCount = (DWORD)m_settings.vecRecentlyUsed.size();
    if (dwCount > MAX_MRU)
        dwCount = MAX_MRU;
    keyRisoh.SetDword(TEXT("FileCount"), dwCount);

    TCHAR szValueName[128];

    for (i = 0; i < dwCount; ++i)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("File%lu"), i);
        keyRisoh.SetSz(szValueName, m_settings.vecRecentlyUsed[i].c_str());
    }

    {
        assoc_map_type::const_iterator it, end = m_settings.assoc_map.end();
        for (it = m_settings.assoc_map.begin(); it != end; ++it)
        {
            keyRisoh.SetSz(it->first.c_str(), it->second.c_str());
            //MessageBoxW(NULL, it->first.c_str(), it->second.c_str(), 0);
        }
    }

    DWORD dwMacroCount = DWORD(m_settings.macros.size());
    keyRisoh.SetDword(TEXT("dwMacroCount"), dwMacroCount);

    {
        i = 0;
        macro_map_type::const_iterator it, end = m_settings.macros.end();
        for (it = m_settings.macros.begin(); it != end; ++it, ++i)
        {
            StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroName%lu"), i);
            keyRisoh.SetSz(szValueName, it->first.c_str());

            StringCchPrintf(szValueName, _countof(szValueName), TEXT("MacroValue%lu"), i);
            keyRisoh.SetSz(szValueName, it->second.c_str());
        }
    }

    DWORD dwNumIncludes = DWORD(m_settings.includes.size());
    keyRisoh.SetDword(TEXT("dwNumIncludes"), dwNumIncludes);

    {
        for (i = 0; i < dwNumIncludes; ++i)
        {
            StringCchPrintf(szValueName, _countof(szValueName), TEXT("Include%lu"), i);
            keyRisoh.SetSz(szValueName, m_settings.includes[i].c_str());
        }
    }

    keyRisoh.SetSz(TEXT("strWindResExe"), m_settings.strWindResExe.c_str());
    keyRisoh.SetSz(TEXT("strCppExe"), m_settings.strCppExe.c_str());

    // always use old style
    keyRisoh.SetDword(TEXT("bOldStyle"), TRUE);

    keyRisoh.SetSz(TEXT("strPrevVersion"), TEXT(RE_VERSION));

    keyRisoh.SetDword(TEXT("bSepFilesByLang"), m_settings.bSepFilesByLang);
    keyRisoh.SetDword(TEXT("bStoreToResFolder"), m_settings.bStoreToResFolder);
    keyRisoh.SetDword(TEXT("bSelectableByMacro"), m_settings.bSelectableByMacro);

    DWORD dwNumCaptions = DWORD(m_settings.captions.size());

    if (dwNumCaptions > s_nMaxCaptions)
        dwNumCaptions = s_nMaxCaptions;

    keyRisoh.SetDword(TEXT("dwNumCaptions"), dwNumCaptions);

    for (DWORD i = 0; i < dwNumCaptions; ++i)
    {
        StringCchPrintf(szValueName, _countof(szValueName), TEXT("Caption%lu"), i);
        keyRisoh.SetSz(szValueName, m_settings.captions[i].c_str());
    }

    keyRisoh.SetDword(TEXT("bShowToolBar"), m_settings.bShowToolBar);

    keyRisoh.SetSz(L"strAtlAxWin", m_settings.strAtlAxWin.c_str());

    return TRUE;
}

BOOL MMainWnd::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    m_id_list_dlg.m_hMainWnd = hwnd;

    DoLoadLangInfo();

    INT nRet = CheckData();
    if (nRet)
        return FALSE;

    LoadSettings(hwnd);

    if (m_settings.bResumeWindowPos)
    {
        if (m_settings.nWindowLeft != CW_USEDEFAULT)
        {
            POINT pt = { m_settings.nWindowLeft, m_settings.nWindowTop };
            SetWindowPosDx(&pt);
        }
        if (m_settings.nWindowWidth != CW_USEDEFAULT)
        {
            SIZE siz = { m_settings.nWindowWidth, m_settings.nWindowHeight };
            SetWindowPosDx(NULL, &siz);
        }
    }

    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 3, 1);
    if (m_hImageList == NULL)
        return FALSE;
    m_hFileIcon = LoadSmallIconDx(IDI_FILE);
    m_hFolderIcon = LoadSmallIconDx(IDI_FOLDER);
    ImageList_AddIcon(m_hImageList, m_hFileIcon);
    ImageList_AddIcon(m_hImageList, m_hFolderIcon);

    if (!CreateOurToolBar(hwnd))
        return FALSE;

    DWORD style, exstyle;

    style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_LEFTALIGN;
    if (!m_splitter1.CreateDx(hwnd, 2, style))
        return FALSE;

    style = WS_CHILD | WS_VISIBLE | SWS_VERT | SWS_BOTTOMALIGN;
    if (!m_splitter2.CreateDx(m_splitter1, 2, style))
        return FALSE;

    style = WS_CHILD | WS_VISIBLE | SWS_HORZ | SWS_RIGHTALIGN;
    if (!m_splitter3.CreateDx(m_splitter2, 1, style))
        return FALSE;

    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
        TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES |
        TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_EDITLABELS;
    m_hTreeView = CreateWindowExW(WS_EX_CLIENTEDGE,
        WC_TREEVIEWW, NULL, style, 0, 0, 0, 0, m_splitter1,
        (HMENU)1, m_hInst, NULL);
    if (m_hTreeView == NULL)
        return FALSE;

    TreeView_SetImageList(m_hTreeView, m_hImageList, TVSIL_NORMAL);

    m_splitter1.SetPane(0, m_hTreeView);
    m_splitter1.SetPane(1, m_splitter2);
    m_splitter1.SetPaneExtent(0, m_settings.nTreeViewWidth);

    style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_TABSTOP |
        ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
        ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
    exstyle = WS_EX_CLIENTEDGE;
    m_hBinEdit.CreateAsChildDx(m_splitter2, NULL, style, exstyle, 3);
    m_splitter2.SetPane(0, m_splitter3);
    m_splitter2.SetPane(1, m_hBinEdit);
    m_splitter2.SetPaneExtent(1, BE_HEIGHT);

    style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP |
        ES_AUTOVSCROLL | ES_LEFT | ES_MULTILINE |
        ES_NOHIDESEL | ES_READONLY | ES_WANTRETURN;
    exstyle = WS_EX_CLIENTEDGE;
    m_hSrcEdit.CreateAsChildDx(m_splitter3, NULL, style, exstyle, 2);
    if (!m_hBmpView.CreateDx(m_splitter3, 4))
        return FALSE;

    m_splitter3.SetPane(0, m_hSrcEdit);
    //m_splitter3.SetPane(1, m_hBmpView);

    style = WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | CCS_BOTTOM;
    m_hStatusBar = CreateStatusWindow(style, LoadStringDx(IDS_STARTING), hwnd, 8);
    if (m_hStatusBar == NULL)
        return FALSE;

    INT anWidths[] = { -1 };
    SendMessage(m_hStatusBar, SB_SETPARTS, 1, (LPARAM)anWidths);
    ChangeStatusText(IDS_STARTING);

    if (m_settings.bShowStatusBar)
        ShowWindow(m_hStatusBar, SW_SHOWNOACTIVATE);
    else
        ShowWindow(m_hStatusBar, SW_HIDE);

    ReCreateFonts(hwnd);

    if (m_argc >= 2)
    {
        DoLoadFile(hwnd, m_targv[1]);
    }

    DragAcceptFiles(hwnd, TRUE);
    SetFocus(m_hTreeView);
    UpdateMenu();

    if (m_settings.strWindResExe.size())
    {
        StringCchCopy(m_szWindresExe, _countof(m_szWindresExe), m_settings.strWindResExe.c_str());
    }
    if (m_settings.strCppExe.size())
    {
        StringCchCopy(m_szCppExe, _countof(m_szCppExe), m_settings.strCppExe.c_str());
    }

    PostMessageDx(WM_COMMAND, ID_READY);

    return TRUE;
}

/*virtual*/ LRESULT CALLBACK
MMainWnd::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static UINT s_uFindMsg = RegisterWindowMessage(FINDMSGSTRING);
    switch (uMsg)
    {
        DO_MSG(WM_CREATE, OnCreate);
        DO_MSG(WM_COMMAND, OnCommand);
        DO_MSG(WM_DESTROY, OnDestroy);
        DO_MSG(WM_DROPFILES, OnDropFiles);
        DO_MSG(WM_MOVE, OnMove);
        DO_MSG(WM_SIZE, OnSize);
        DO_MSG(WM_NOTIFY, OnNotify);
        DO_MSG(WM_CONTEXTMENU, OnContextMenu);
        DO_MSG(WM_INITMENU, OnInitMenu);
        DO_MSG(WM_ACTIVATE, OnActivate);
        DO_MSG(WM_SYSCOLORCHANGE, OnSysColorChange);
        DO_MESSAGE(MYWM_CLEARSTATUS, OnClearStatus);
        DO_MESSAGE(MYWM_MOVESIZEREPORT, OnMoveSizeReport);
        DO_MESSAGE(MYWM_COMPILECHECK, OnCompileCheck);
        DO_MESSAGE(MYWM_REOPENRAD, OnReopenRad);
        DO_MESSAGE(MYWM_POSTSEARCH, OnPostSearch);
        DO_MESSAGE(MYWM_IDJUMPBANG, OnIDJumpBang);
    default:
        if (uMsg == s_uFindMsg)
        {
            return OnFindMsg(hwnd, wParam, lParam);
        }
        return DefaultProcDx();
    }
}

void MMainWnd::SelectString(HWND hwnd)
{
    ResEntries found;
    Res_Search(found, m_entries, RT_STRING, WORD(0), 0xFFFF);
    if (found.size())
    {
        ResEntry entry(RT_STRING, found[0].name, found[0].lang);
        TV_SelectEntry(m_hTreeView, m_entries, entry);
    }
}

void MMainWnd::SelectMessage(HWND hwnd)
{
    ResEntries found;
    Res_Search(found, m_entries, RT_MESSAGETABLE, WORD(0), 0xFFFF);
    if (found.size())
    {
        ResEntry entry(RT_MESSAGETABLE, found[0].name, found[0].lang);
        TV_SelectEntry(m_hTreeView, m_entries, entry);
    }
}

void MMainWnd::OnIDJumpBang2(HWND hwnd, const MString& name, MString& strType)
{
    if (strType == L"Unknown.ID")
        return;

    ReplaceResTypeString(strType, true);

    MString prefix = name.substr(0, name.find(L'_') + 1);
    std::vector<INT> indexes = GetPrefixIndexes(m_settings, m_db, prefix);
    for (size_t i = 0; i < indexes.size(); ++i)
    {
        INT nIDTYPE_ = indexes[i];
        if (nIDTYPE_ == IDTYPE_STRING || nIDTYPE_ == IDTYPE_PROMPT)
        {
            SelectString(hwnd);
            return;
        }
        if (nIDTYPE_ == IDTYPE_MESSAGE)
        {
            SelectMessage(hwnd);
            return;
        }
    }

    MIdOrString type = WORD(m_db.GetValue(L"RESOURCE", strType));
    if (type.empty())
        type.m_str = strType;

    WORD wName = WORD(m_db.GetResIDValue(name));
    MIdOrString name_or_id(wName);
    if (wName == 0)
    {
        id_map_type::iterator it;
        MStringA strA = MTextToAnsi(CP_ACP, name).c_str();
        it = m_settings.id_map.find(strA);
        if (it != m_settings.id_map.end())
        {
            MStringA strA = it->second;
            if (strA[0] == 'L')
                strA = strA.substr(1);
            mstr_unquote(strA);
            CharUpperA(&strA[0]);
            name_or_id.m_str = MAnsiToWide(CP_ACP, strA).c_str();
        }
    }

    ResEntries found;
    Res_Search(found, m_entries, type, name_or_id, 0xFFFF);

    if (found.size())
    {
        TV_SelectEntry(m_hTreeView, m_entries, found[0]);

        SetForegroundWindow(m_hwnd);
        BringWindowToTop(m_hwnd);
        SetFocus(m_hwnd);
    }
}

LRESULT MMainWnd::OnIDJumpBang(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT iItem = (INT)wParam;
    if (iItem == -1)
        return 0;

    TCHAR szText[128];
    ListView_GetItemText(m_id_list_dlg.m_hLst1, iItem, 0, szText, _countof(szText));
    MString name = szText;
    ListView_GetItemText(m_id_list_dlg.m_hLst1, iItem, 1, szText, _countof(szText));
    MString strTypes = szText;

    std::vector<MString> vecTypes;
    mstr_split(vecTypes, strTypes, L"/");

    if (vecTypes.empty() || vecTypes.size() <= size_t(lParam))
        return 0;

    OnIDJumpBang2(hwnd, name, vecTypes[lParam]);

    return 0;
}

LRESULT MMainWnd::OnPostSearch(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    m_fr.Flags = FR_DOWN;
    if (m_search.bInternalText)
    {
        lstrcpyn(m_szFindWhat, m_search.strText.c_str(), _countof(m_szFindWhat));
        if (!m_search.bIgnoreCases)
        {
            m_fr.Flags |= FR_MATCHCASE;
        }
        OnFindNext(hwnd);
    }
    return 0;
}

BOOL MMainWnd::StartDx()
{
    MSplitterWnd::CursorNS() = LoadCursor(m_hInst, MAKEINTRESOURCE(IDC_CURSORNS));
    MSplitterWnd::CursorWE() = LoadCursor(m_hInst, MAKEINTRESOURCE(IDC_CURSORWE));

    m_hIcon = LoadIconDx(IDI_MAIN);
    m_hIconSm = LoadSmallIconDx(IDI_MAIN);
    m_hAccel = ::LoadAccelerators(m_hInst, MAKEINTRESOURCE(IDR_MAINACCEL));

    if (!CreateWindowDx(NULL, MAKEINTRESOURCE(IDS_APPNAME),
        WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, CW_USEDEFAULT, 760, 480))
    {
        ErrorBoxDx(TEXT("failure of CreateWindow"));
        return FALSE;
    }
    assert(IsWindow(m_hwnd));

    if (m_settings.bResumeWindowPos && m_settings.bMaximized)
    {
        ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    }
    else
    {
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
    }
    UpdateWindow(m_hwnd);

    return TRUE;
}

void MMainWnd::DoEvents()
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        DoMsg(msg);
    }
}

void MMainWnd::DoMsg(MSG& msg)
{
    if (IsWindow(m_rad_window.m_rad_dialog))
    {
        if (::IsDialogMessage(m_rad_window.m_rad_dialog, &msg))
            return;
    }
    if (IsWindow(m_id_list_dlg))
    {
        if (::IsDialogMessage(m_id_list_dlg, &msg))
            return;
    }
    if (m_hAccel && IsWindow(m_hwnd))
    {
        if (::TranslateAccelerator(m_hwnd, m_hAccel, &msg))
            return;
    }
    if (IsWindow(m_hFindReplaceDlg))
    {
        if (::IsDialogMessage(m_hFindReplaceDlg, &msg))
            return;
    }
    if (MItemSearchDlg::Dialogs().size())
    {
        typedef MItemSearchDlg::dialogs_type dialogs_type;
        dialogs_type::iterator it, end = MItemSearchDlg::Dialogs().end();
        BOOL bProcessed = FALSE;
        for (it = MItemSearchDlg::Dialogs().begin(); it != end; ++it)
        {
            if (IsDialogMessage(**it, &msg))
            {
                bProcessed = TRUE;
                break;
            }
        }
        if (bProcessed)
            return;
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

INT_PTR MMainWnd::RunDx()
{
    MSG msg;

    while (BOOL bGot = ::GetMessage(&msg, NULL, 0, 0))
    {
        if (bGot < 0)
        {
            DebugPrintDx(TEXT("Application fatal error: %ld\n"), GetLastError());
            DebugBreak();
            return -1;
        }
        DoMsg(msg);
    }
    return INT(msg.wParam);
}

//////////////////////////////////////////////////////////////////////////////

MString GetLanguageStatement(WORD langid, BOOL bOldStyle)
{
    MString strPrim, strSub;

#define SWITCH_SUBLANG() switch (SUBLANGID(langid))

    switch (PRIMARYLANGID(langid))
    {
    case LANG_NEUTRAL: strPrim = TEXT("LANG_NEUTRAL");
        break;
    case LANG_INVARIANT: strPrim = TEXT("LANG_INVARIANT");
        break;
    case LANG_AFRIKAANS: strPrim = TEXT("LANG_AFRIKAANS");
        SWITCH_SUBLANG()
        {
        case SUBLANG_AFRIKAANS_SOUTH_AFRICA: strSub = TEXT("SUBLANG_AFRIKAANS_SOUTH_AFRICA"); break;
        }
        break;
    case LANG_ALBANIAN: strPrim = TEXT("LANG_ALBANIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ALBANIAN_ALBANIA: strSub = TEXT("SUBLANG_ALBANIAN_ALBANIA"); break;
        }
        break;
    case LANG_ALSATIAN: strPrim = TEXT("LANG_ALSATIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ALSATIAN_FRANCE: strSub = TEXT("SUBLANG_ALSATIAN_FRANCE"); break;
        }
        break;
    case LANG_AMHARIC: strPrim = TEXT("LANG_AMHARIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_AMHARIC_ETHIOPIA: strSub = TEXT("SUBLANG_AMHARIC_ETHIOPIA"); break;
        }
        break;
    case LANG_ARABIC: strPrim = TEXT("LANG_ARABIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ARABIC_SAUDI_ARABIA: strSub = TEXT("SUBLANG_ARABIC_SAUDI_ARABIA"); break;
        case SUBLANG_ARABIC_IRAQ: strSub = TEXT("SUBLANG_ARABIC_IRAQ"); break;
        case SUBLANG_ARABIC_EGYPT: strSub = TEXT("SUBLANG_ARABIC_EGYPT"); break;
        case SUBLANG_ARABIC_LIBYA: strSub = TEXT("SUBLANG_ARABIC_LIBYA"); break;
        case SUBLANG_ARABIC_ALGERIA: strSub = TEXT("SUBLANG_ARABIC_ALGERIA"); break;
        case SUBLANG_ARABIC_MOROCCO: strSub = TEXT("SUBLANG_ARABIC_MOROCCO"); break;
        case SUBLANG_ARABIC_TUNISIA: strSub = TEXT("SUBLANG_ARABIC_TUNISIA"); break;
        case SUBLANG_ARABIC_OMAN: strSub = TEXT("SUBLANG_ARABIC_OMAN"); break;
        case SUBLANG_ARABIC_YEMEN: strSub = TEXT("SUBLANG_ARABIC_YEMEN"); break;
        case SUBLANG_ARABIC_SYRIA: strSub = TEXT("SUBLANG_ARABIC_SYRIA"); break;
        case SUBLANG_ARABIC_JORDAN: strSub = TEXT("SUBLANG_ARABIC_JORDAN"); break;
        case SUBLANG_ARABIC_LEBANON: strSub = TEXT("SUBLANG_ARABIC_LEBANON"); break;
        case SUBLANG_ARABIC_KUWAIT: strSub = TEXT("SUBLANG_ARABIC_KUWAIT"); break;
        case SUBLANG_ARABIC_UAE: strSub = TEXT("SUBLANG_ARABIC_UAE"); break;
        case SUBLANG_ARABIC_BAHRAIN: strSub = TEXT("SUBLANG_ARABIC_BAHRAIN"); break;
        case SUBLANG_ARABIC_QATAR: strSub = TEXT("SUBLANG_ARABIC_QATAR"); break;
        }
        break;
    case LANG_ARMENIAN: strPrim = TEXT("LANG_ARMENIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ARMENIAN_ARMENIA: strSub = TEXT("SUBLANG_ARMENIAN_ARMENIA"); break;
        }
        break;
    case LANG_ASSAMESE: strPrim = TEXT("LANG_ASSAMESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ASSAMESE_INDIA: strSub = TEXT("SUBLANG_ASSAMESE_INDIA"); break;
        }
        break;
    case LANG_AZERI: strPrim = TEXT("LANG_AZERI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_AZERI_LATIN: strSub = TEXT("SUBLANG_AZERI_LATIN"); break;
        case SUBLANG_AZERI_CYRILLIC: strSub = TEXT("SUBLANG_AZERI_CYRILLIC"); break;
        }
        break;
    //case LANG_AZERBAIJANI: strPrim = TEXT("LANG_AZERBAIJANI"); // same as LANG_AZERI
    //    SWITCH_SUBLANG()
    //    {
    //        case SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN: strSub = TEXT("SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN"); break;
    //        case SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC: strSub = TEXT("SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC"); break;
    //    }
    //    break;
    //case LANG_BANGLA: strPrim = TEXT("LANG_BANGLA"); // same as LANG_BENGALI
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_BANGLA_INDIA: strSub = TEXT("SUBLANG_BANGLA_INDIA"); break;
    //    case SUBLANG_BANGLA_BANGLADESH: strSub = TEXT("SUBLANG_BANGLA_BANGLADESH"); break;
    //    }
    //    break;
    case LANG_BASHKIR: strPrim = TEXT("LANG_BASHKIR");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BASHKIR_RUSSIA: strSub = TEXT("SUBLANG_BASHKIR_RUSSIA"); break;
        }
        break;
    case LANG_BASQUE: strPrim = TEXT("LANG_BASQUE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BASQUE_BASQUE: strSub = TEXT("SUBLANG_BASQUE_BASQUE"); break;
        }
        break;
    case LANG_BELARUSIAN: strPrim = TEXT("LANG_BELARUSIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BELARUSIAN_BELARUS: strSub = TEXT("SUBLANG_BELARUSIAN_BELARUS"); break;
        }
        break;
    case LANG_BENGALI: strPrim = TEXT("LANG_BENGALI");
        SWITCH_SUBLANG()
        {
            case SUBLANG_BENGALI_INDIA: strSub = TEXT("SUBLANG_BENGALI_INDIA"); break;
            case SUBLANG_BENGALI_BANGLADESH: strSub = TEXT("SUBLANG_BENGALI_BANGLADESH"); break;
        }
        break;
    case LANG_BRETON: strPrim = TEXT("LANG_BRETON");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BRETON_FRANCE: strSub = TEXT("SUBLANG_BRETON_FRANCE"); break;
        }
        break;
    case LANG_BOSNIAN: strPrim = TEXT("LANG_BOSNIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
        }
        break;
    case LANG_BOSNIAN_NEUTRAL: strPrim = TEXT("LANG_BOSNIAN_NEUTRAL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
        case SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
        }
        break;
    case LANG_BULGARIAN: strPrim = TEXT("LANG_BULGARIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_BULGARIAN_BULGARIA: strSub = TEXT("SUBLANG_BULGARIAN_BULGARIA"); break;
        }
        break;
    case LANG_CATALAN: strPrim = TEXT("LANG_CATALAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CATALAN_CATALAN: strSub = TEXT("SUBLANG_CATALAN_CATALAN"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_CENTRAL_KURDISH: strPrim = TEXT("LANG_CENTRAL_KURDISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CENTRAL_KURDISH_IRAQ: strSub = TEXT("SUBLANG_CENTRAL_KURDISH_IRAQ"); break;
        }
        break;
#endif
#ifdef ENABLE_NEW_LANGS
    case LANG_CHEROKEE: strPrim = TEXT("LANG_CHEROKEE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CHEROKEE_CHEROKEE: strSub = TEXT("SUBLANG_CHEROKEE_CHEROKEE"); break;
        }
        break;
#endif
    case LANG_CHINESE: strPrim = TEXT("LANG_CHINESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
        case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
        case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
        case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
        case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
        }
        break;
    //case LANG_CHINESE_SIMPLIFIED: strPrim = TEXT("LANG_CHINESE_SIMPLIFIED"); // same as LANG_CHINESE
    //    SWITCH_SUBLANG()
    //    {
    //        case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
    //        case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
    //        case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
    //        case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
    //        case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
    //    }
    //    break;
    case LANG_CHINESE_TRADITIONAL: strPrim = TEXT("LANG_CHINESE_TRADITIONAL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CHINESE_TRADITIONAL: strSub = TEXT("SUBLANG_CHINESE_TRADITIONAL"); break;
        case SUBLANG_CHINESE_SIMPLIFIED: strSub = TEXT("SUBLANG_CHINESE_SIMPLIFIED"); break;
        case SUBLANG_CHINESE_HONGKONG: strSub = TEXT("SUBLANG_CHINESE_HONGKONG"); break;
        case SUBLANG_CHINESE_SINGAPORE: strSub = TEXT("SUBLANG_CHINESE_SINGAPORE"); break;
        case SUBLANG_CHINESE_MACAU: strSub = TEXT("SUBLANG_CHINESE_MACAU"); break;
        }
        break;
    case LANG_CORSICAN: strPrim = TEXT("LANG_CORSICAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CORSICAN_FRANCE: strSub = TEXT("SUBLANG_CORSICAN_FRANCE"); break;
        }
        break;
    //case LANG_CROATIAN: strPrim = TEXT("LANG_CROATIAN"); // same as LANG_BOSNIAN
    //    SWITCH_SUBLANG()
    //    {
    //        case SUBLANG_CROATIAN_CROATIA: strSub = TEXT("SUBLANG_CROATIAN_CROATIA"); break;
    //        case SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
    //    }
    //    break;
    case LANG_CZECH: strPrim = TEXT("LANG_CZECH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_CZECH_CZECH_REPUBLIC: strSub = TEXT("SUBLANG_CZECH_CZECH_REPUBLIC"); break;
        }
        break;
    case LANG_DANISH: strPrim = TEXT("LANG_DANISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DANISH_DENMARK: strSub = TEXT("SUBLANG_DANISH_DENMARK"); break;
        }
        break;
    case LANG_DARI: strPrim = TEXT("LANG_DARI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DARI_AFGHANISTAN: strSub = TEXT("SUBLANG_DARI_AFGHANISTAN"); break;
        }
        break;
    case LANG_DIVEHI: strPrim = TEXT("LANG_DIVEHI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DIVEHI_MALDIVES: strSub = TEXT("SUBLANG_DIVEHI_MALDIVES"); break;
        }
        break;
    case LANG_DUTCH: strPrim = TEXT("LANG_DUTCH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_DUTCH: strSub = TEXT("SUBLANG_DUTCH"); break;
        case SUBLANG_DUTCH_BELGIAN: strSub = TEXT("SUBLANG_DUTCH_BELGIAN"); break;
        }
        break;
    case LANG_ENGLISH: strPrim = TEXT("LANG_ENGLISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ENGLISH_US: strSub = TEXT("SUBLANG_ENGLISH_US"); break;
        case SUBLANG_ENGLISH_UK: strSub = TEXT("SUBLANG_ENGLISH_UK"); break;
        case SUBLANG_ENGLISH_AUS: strSub = TEXT("SUBLANG_ENGLISH_AUS"); break;
        case SUBLANG_ENGLISH_CAN: strSub = TEXT("SUBLANG_ENGLISH_CAN"); break;
        case SUBLANG_ENGLISH_NZ: strSub = TEXT("SUBLANG_ENGLISH_NZ"); break;
        //case SUBLANG_ENGLISH_IRELAND: strSub = TEXT("SUBLANG_ENGLISH_IRELAND"); break; // same as SUBLANG_ENGLISH_EIRE
        case SUBLANG_ENGLISH_EIRE: strSub = TEXT("SUBLANG_ENGLISH_EIRE"); break;
        case SUBLANG_ENGLISH_SOUTH_AFRICA: strSub = TEXT("SUBLANG_ENGLISH_SOUTH_AFRICA"); break;
        case SUBLANG_ENGLISH_JAMAICA: strSub = TEXT("SUBLANG_ENGLISH_JAMAICA"); break;
        case SUBLANG_ENGLISH_CARIBBEAN: strSub = TEXT("SUBLANG_ENGLISH_CARIBBEAN"); break;
        case SUBLANG_ENGLISH_BELIZE: strSub = TEXT("SUBLANG_ENGLISH_BELIZE"); break;
        case SUBLANG_ENGLISH_TRINIDAD: strSub = TEXT("SUBLANG_ENGLISH_TRINIDAD"); break;
        case SUBLANG_ENGLISH_ZIMBABWE: strSub = TEXT("SUBLANG_ENGLISH_ZIMBABWE"); break;
        case SUBLANG_ENGLISH_PHILIPPINES: strSub = TEXT("SUBLANG_ENGLISH_PHILIPPINES"); break;
        case SUBLANG_ENGLISH_INDIA: strSub = TEXT("SUBLANG_ENGLISH_INDIA"); break;
        case SUBLANG_ENGLISH_MALAYSIA: strSub = TEXT("SUBLANG_ENGLISH_MALAYSIA"); break;
        case SUBLANG_ENGLISH_SINGAPORE: strSub = TEXT("SUBLANG_ENGLISH_SINGAPORE"); break;
        }
        break;
    case LANG_ESTONIAN: strPrim = TEXT("LANG_ESTONIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ESTONIAN_ESTONIA: strSub = TEXT("SUBLANG_ESTONIAN_ESTONIA"); break;
        }
        break;
    case LANG_FAEROESE: strPrim = TEXT("LANG_FAEROESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FAEROESE_FAROE_ISLANDS: strSub = TEXT("SUBLANG_FAEROESE_FAROE_ISLANDS"); break;
        }
        break;
    //case LANG_FARSI: strPrim = TEXT("LANG_FARSI"); // same as LANG_PERSIAN
    //    break;
    case LANG_FILIPINO: strPrim = TEXT("LANG_FILIPINO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FILIPINO_PHILIPPINES: strSub = TEXT("SUBLANG_FILIPINO_PHILIPPINES"); break;
        }
        break;
    case LANG_FINNISH: strPrim = TEXT("LANG_FINNISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FINNISH_FINLAND: strSub = TEXT("SUBLANG_FINNISH_FINLAND"); break;
        }
        break;
    case LANG_FRENCH: strPrim = TEXT("LANG_FRENCH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FRENCH: strSub = TEXT("SUBLANG_FRENCH"); break;
        case SUBLANG_FRENCH_BELGIAN: strSub = TEXT("SUBLANG_FRENCH_BELGIAN"); break;
        case SUBLANG_FRENCH_CANADIAN: strSub = TEXT("SUBLANG_FRENCH_CANADIAN"); break;
        case SUBLANG_FRENCH_SWISS: strSub = TEXT("SUBLANG_FRENCH_SWISS"); break;
        case SUBLANG_FRENCH_LUXEMBOURG: strSub = TEXT("SUBLANG_FRENCH_LUXEMBOURG"); break;
        case SUBLANG_FRENCH_MONACO: strSub = TEXT("SUBLANG_FRENCH_MONACO"); break;
        }
        break;
    case LANG_FRISIAN: strPrim = TEXT("LANG_FRISIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_FRISIAN_NETHERLANDS: strSub = TEXT("SUBLANG_FRISIAN_NETHERLANDS"); break;
        }
        break;
    //case LANG_FULAH: strPrim = TEXT("LANG_FULAH"); // same as LANG_PULAR
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_FULAH_SENEGAL: strSub = TEXT("SUBLANG_FULAH_SENEGAL"); break;
    //    }
    //    break;
    case LANG_GALICIAN: strPrim = TEXT("LANG_GALICIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GALICIAN_GALICIAN: strSub = TEXT("SUBLANG_GALICIAN_GALICIAN"); break;
        }
        break;
    case LANG_GEORGIAN: strPrim = TEXT("LANG_GEORGIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GEORGIAN_GEORGIA: strSub = TEXT("SUBLANG_GEORGIAN_GEORGIA"); break;
        }
        break;
    case LANG_GERMAN: strPrim = TEXT("LANG_GERMAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GERMAN: strSub = TEXT("SUBLANG_GERMAN"); break;
        case SUBLANG_GERMAN_SWISS: strSub = TEXT("SUBLANG_GERMAN_SWISS"); break;
        case SUBLANG_GERMAN_AUSTRIAN: strSub = TEXT("SUBLANG_GERMAN_AUSTRIAN"); break;
        case SUBLANG_GERMAN_LUXEMBOURG: strSub = TEXT("SUBLANG_GERMAN_LUXEMBOURG"); break;
        case SUBLANG_GERMAN_LIECHTENSTEIN: strSub = TEXT("SUBLANG_GERMAN_LIECHTENSTEIN"); break;
        }
        break;
    case LANG_GREEK: strPrim = TEXT("LANG_GREEK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GREEK_GREECE: strSub = TEXT("SUBLANG_GREEK_GREECE"); break;
        }
        break;
    case LANG_GREENLANDIC: strPrim = TEXT("LANG_GREENLANDIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GREENLANDIC_GREENLAND: strSub = TEXT("SUBLANG_GREENLANDIC_GREENLAND"); break;
        }
        break;
    case LANG_GUJARATI: strPrim = TEXT("LANG_GUJARATI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_GUJARATI_INDIA: strSub = TEXT("SUBLANG_GUJARATI_INDIA"); break;
        }
        break;
    case LANG_HAUSA: strPrim = TEXT("LANG_HAUSA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HAUSA_NIGERIA_LATIN: strSub = TEXT("SUBLANG_HAUSA_NIGERIA_LATIN"); break;
        }
        break;
    case LANG_HEBREW: strPrim = TEXT("LANG_HEBREW");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HEBREW_ISRAEL: strSub = TEXT("SUBLANG_HEBREW_ISRAEL"); break;
        }
        break;
    case LANG_HINDI: strPrim = TEXT("LANG_HINDI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HINDI_INDIA: strSub = TEXT("SUBLANG_HINDI_INDIA"); break;
        }
        break;
    case LANG_HUNGARIAN: strPrim = TEXT("LANG_HUNGARIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_HUNGARIAN_HUNGARY: strSub = TEXT("SUBLANG_HUNGARIAN_HUNGARY"); break;
        }
        break;
    case LANG_ICELANDIC: strPrim = TEXT("LANG_ICELANDIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ICELANDIC_ICELAND: strSub = TEXT("SUBLANG_ICELANDIC_ICELAND"); break;
        }
        break;
    case LANG_IGBO: strPrim = TEXT("LANG_IGBO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_IGBO_NIGERIA: strSub = TEXT("SUBLANG_IGBO_NIGERIA"); break;
        }
        break;
    case LANG_INDONESIAN: strPrim = TEXT("LANG_INDONESIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_INDONESIAN_INDONESIA: strSub = TEXT("SUBLANG_INDONESIAN_INDONESIA"); break;
        }
        break;
    case LANG_INUKTITUT: strPrim = TEXT("LANG_INUKTITUT");
        SWITCH_SUBLANG()
        {
        case SUBLANG_INUKTITUT_CANADA: strSub = TEXT("SUBLANG_INUKTITUT_CANADA"); break;
        case SUBLANG_INUKTITUT_CANADA_LATIN: strSub = TEXT("SUBLANG_INUKTITUT_CANADA_LATIN"); break;
        }
        break;
    case LANG_IRISH: strPrim = TEXT("LANG_IRISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_IRISH_IRELAND: strSub = TEXT("SUBLANG_IRISH_IRELAND"); break;
        }
        break;
    case LANG_ITALIAN: strPrim = TEXT("LANG_ITALIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ITALIAN: strSub = TEXT("SUBLANG_ITALIAN"); break;
        case SUBLANG_ITALIAN_SWISS: strSub = TEXT("SUBLANG_ITALIAN_SWISS"); break;
        }
        break;
    case LANG_JAPANESE: strPrim = TEXT("LANG_JAPANESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_JAPANESE_JAPAN: strSub = TEXT("SUBLANG_JAPANESE_JAPAN"); break;
        }
        break;
    case LANG_KANNADA: strPrim = TEXT("LANG_KANNADA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KANNADA_INDIA: strSub = TEXT("SUBLANG_KANNADA_INDIA"); break;
        }
        break;
    case LANG_KASHMIRI: strPrim = TEXT("LANG_KASHMIRI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KASHMIRI_INDIA: strSub = TEXT("SUBLANG_KASHMIRI_INDIA"); break;
        //case SUBLANG_KASHMIRI_SASIA: strSub = TEXT("SUBLANG_KASHMIRI_SASIA"); break; // same as SUBLANG_KASHMIRI_INDIA
        }
        break;
    case LANG_KAZAK: strPrim = TEXT("LANG_KAZAK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KAZAK_KAZAKHSTAN: strSub = TEXT("SUBLANG_KAZAK_KAZAKHSTAN"); break;
        }
        break;
    case LANG_KHMER: strPrim = TEXT("LANG_KHMER");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KHMER_CAMBODIA: strSub = TEXT("SUBLANG_KHMER_CAMBODIA"); break;
        }
        break;
    case LANG_KICHE: strPrim = TEXT("LANG_KICHE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KICHE_GUATEMALA: strSub = TEXT("SUBLANG_KICHE_GUATEMALA"); break;
        }
        break;
    case LANG_KINYARWANDA: strPrim = TEXT("LANG_KINYARWANDA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KINYARWANDA_RWANDA: strSub = TEXT("SUBLANG_KINYARWANDA_RWANDA"); break;
        }
        break;
    case LANG_KONKANI: strPrim = TEXT("LANG_KONKANI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KONKANI_INDIA: strSub = TEXT("SUBLANG_KONKANI_INDIA"); break;
        }
        break;
    case LANG_KOREAN: strPrim = TEXT("LANG_KOREAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KOREAN: strSub = TEXT("SUBLANG_KOREAN"); break;
        }
        break;
    case LANG_KYRGYZ: strPrim = TEXT("LANG_KYRGYZ");
        SWITCH_SUBLANG()
        {
        case SUBLANG_KYRGYZ_KYRGYZSTAN: strSub = TEXT("SUBLANG_KYRGYZ_KYRGYZSTAN"); break;
        }
        break;
    case LANG_LAO: strPrim = TEXT("LANG_LAO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_LAO_LAO: strSub = TEXT("SUBLANG_LAO_LAO"); break;
        }
        break;
    case LANG_LATVIAN: strPrim = TEXT("LANG_LATVIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_LATVIAN_LATVIA: strSub = TEXT("SUBLANG_LATVIAN_LATVIA"); break;
        }
        break;
    case LANG_LITHUANIAN: strPrim = TEXT("LANG_LITHUANIAN");
#if (WINVER >= 0x0600) && defined(ENABLE_NEW_LANGS)
        SWITCH_SUBLANG()
        {
        case SUBLANG_LITHUANIAN_LITHUANIA: strSub = TEXT("SUBLANG_LITHUANIAN_LITHUANIA"); break;
        }
#endif
        break;
    //case LANG_LOWER_SORBIAN: strPrim = TEXT("LANG_LOWER_SORBIAN"); // same as LANG_UPPER_SORBIAN
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_LOWER_SORBIAN_GERMANY: strSub = TEXT("SUBLANG_LOWER_SORBIAN_GERMANY"); break;
    //    }
    //    break;
    case LANG_LUXEMBOURGISH: strPrim = TEXT("LANG_LUXEMBOURGISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_LUXEMBOURGISH_LUXEMBOURG: strSub = TEXT("SUBLANG_LUXEMBOURGISH_LUXEMBOURG"); break;
        }
        break;
    case LANG_MACEDONIAN: strPrim = TEXT("LANG_MACEDONIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MACEDONIAN_MACEDONIA: strSub = TEXT("SUBLANG_MACEDONIAN_MACEDONIA"); break;
        }
        break;
    case LANG_MALAY: strPrim = TEXT("LANG_MALAY");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MALAY_MALAYSIA: strSub = TEXT("SUBLANG_MALAY_MALAYSIA"); break;
        case SUBLANG_MALAY_BRUNEI_DARUSSALAM: strSub = TEXT("SUBLANG_MALAY_BRUNEI_DARUSSALAM"); break;
        }
        break;
    case LANG_MALAYALAM: strPrim = TEXT("LANG_MALAYALAM");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MALAYALAM_INDIA: strSub = TEXT("SUBLANG_MALAYALAM_INDIA"); break;
        }
        break;
    case LANG_MALTESE: strPrim = TEXT("LANG_MALTESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MALTESE_MALTA: strSub = TEXT("SUBLANG_MALTESE_MALTA"); break;
        }
        break;
    case LANG_MANIPURI: strPrim = TEXT("LANG_MANIPURI");
        break;
    case LANG_MAORI: strPrim = TEXT("LANG_MAORI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MAORI_NEW_ZEALAND: strSub = TEXT("SUBLANG_MAORI_NEW_ZEALAND"); break;
        }
        break;
    case LANG_MAPUDUNGUN: strPrim = TEXT("LANG_MAPUDUNGUN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MAPUDUNGUN_CHILE: strSub = TEXT("SUBLANG_MAPUDUNGUN_CHILE"); break;
        }
        break;
    case LANG_MARATHI: strPrim = TEXT("LANG_MARATHI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MARATHI_INDIA: strSub = TEXT("SUBLANG_MARATHI_INDIA"); break;
        }
        break;
    case LANG_MOHAWK: strPrim = TEXT("LANG_MOHAWK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MOHAWK_MOHAWK: strSub = TEXT("SUBLANG_MOHAWK_MOHAWK"); break;
        }
        break;
    case LANG_MONGOLIAN: strPrim = TEXT("LANG_MONGOLIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA: strSub = TEXT("SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA"); break;
        case SUBLANG_MONGOLIAN_PRC: strSub = TEXT("SUBLANG_MONGOLIAN_PRC"); break;
        }
        break;
    case LANG_NEPALI: strPrim = TEXT("LANG_NEPALI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_NEPALI_NEPAL: strSub = TEXT("SUBLANG_NEPALI_NEPAL"); break;
        case SUBLANG_NEPALI_INDIA: strSub = TEXT("SUBLANG_NEPALI_INDIA"); break;
        }
        break;
    case LANG_NORWEGIAN: strPrim = TEXT("LANG_NORWEGIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_NORWEGIAN_BOKMAL: strSub = TEXT("SUBLANG_NORWEGIAN_BOKMAL"); break;
        case SUBLANG_NORWEGIAN_NYNORSK: strSub = TEXT("SUBLANG_NORWEGIAN_NYNORSK"); break;
        }
        break;
    case LANG_OCCITAN: strPrim = TEXT("LANG_OCCITAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_OCCITAN_FRANCE: strSub = TEXT("SUBLANG_OCCITAN_FRANCE"); break;
        }
        break;
    //case LANG_ODIA: strPrim = TEXT("LANG_ODIA");  // same as LANG_ORIYA
    //    break;
    case LANG_ORIYA: strPrim = TEXT("LANG_ORIYA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ORIYA_INDIA: strSub = TEXT("SUBLANG_ORIYA_INDIA"); break;
        }
        break;
    case LANG_PASHTO: strPrim = TEXT("LANG_PASHTO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PASHTO_AFGHANISTAN: strSub = TEXT("SUBLANG_PASHTO_AFGHANISTAN"); break;
        }
        break;
    case LANG_PERSIAN: strPrim = TEXT("LANG_PERSIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PERSIAN_IRAN: strSub = TEXT("SUBLANG_PERSIAN_IRAN"); break;
        }
        break;
    case LANG_POLISH: strPrim = TEXT("LANG_POLISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_POLISH_POLAND: strSub = TEXT("SUBLANG_POLISH_POLAND"); break;
        }
        break;
    case LANG_PORTUGUESE: strPrim = TEXT("LANG_PORTUGUESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PORTUGUESE: strSub = TEXT("SUBLANG_PORTUGUESE"); break;
        case SUBLANG_PORTUGUESE_BRAZILIAN: strSub = TEXT("SUBLANG_PORTUGUESE_BRAZILIAN"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_PULAR: strPrim = TEXT("LANG_PULAR"); // same as LANG_FULAH
        SWITCH_SUBLANG()
        {
        case SUBLANG_PULAR_SENEGAL: strSub = TEXT("SUBLANG_PULAR_SENEGAL"); break;
        default: break;
        }
#endif
        break;
    case LANG_PUNJABI: strPrim = TEXT("LANG_PUNJABI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_PUNJABI_INDIA: strSub = TEXT("SUBLANG_PUNJABI_INDIA"); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_PUNJABI_PAKISTAN: strSub = TEXT("SUBLANG_PUNJABI_PAKISTAN"); break;
#endif
        default: break;
        }
        break;
    case LANG_QUECHUA: strPrim = TEXT("LANG_QUECHUA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_QUECHUA_BOLIVIA: strSub = TEXT("SUBLANG_QUECHUA_BOLIVIA"); break;
        case SUBLANG_QUECHUA_ECUADOR: strSub = TEXT("SUBLANG_QUECHUA_ECUADOR"); break;
        case SUBLANG_QUECHUA_PERU: strSub = TEXT("SUBLANG_QUECHUA_PERU"); break;
        }
        break;
    case LANG_ROMANIAN: strPrim = TEXT("LANG_ROMANIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ROMANIAN_ROMANIA: strSub = TEXT("SUBLANG_ROMANIAN_ROMANIA"); break;
        }
        break;
    case LANG_ROMANSH: strPrim = TEXT("LANG_ROMANSH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ROMANSH_SWITZERLAND: strSub = TEXT("SUBLANG_ROMANSH_SWITZERLAND"); break;
        }
        break;
    case LANG_RUSSIAN: strPrim = TEXT("LANG_RUSSIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_RUSSIAN_RUSSIA: strSub = TEXT("SUBLANG_RUSSIAN_RUSSIA"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_SAKHA: strPrim = TEXT("LANG_SAKHA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SAKHA_RUSSIA: strSub = TEXT("SUBLANG_SAKHA_RUSSIA"); break;
        default: break;
        }
        break;
#endif
    case LANG_SAMI: strPrim = TEXT("LANG_SAMI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SAMI_NORTHERN_NORWAY: strSub = TEXT("SUBLANG_SAMI_NORTHERN_NORWAY"); break;
        case SUBLANG_SAMI_NORTHERN_SWEDEN: strSub = TEXT("SUBLANG_SAMI_NORTHERN_SWEDEN"); break;
        case SUBLANG_SAMI_NORTHERN_FINLAND: strSub = TEXT("SUBLANG_SAMI_NORTHERN_FINLAND"); break;
        case SUBLANG_SAMI_LULE_NORWAY: strSub = TEXT("SUBLANG_SAMI_LULE_NORWAY"); break;
        case SUBLANG_SAMI_LULE_SWEDEN: strSub = TEXT("SUBLANG_SAMI_LULE_SWEDEN"); break;
        case SUBLANG_SAMI_SOUTHERN_NORWAY: strSub = TEXT("SUBLANG_SAMI_SOUTHERN_NORWAY"); break;
        case SUBLANG_SAMI_SOUTHERN_SWEDEN: strSub = TEXT("SUBLANG_SAMI_SOUTHERN_SWEDEN"); break;
        case SUBLANG_SAMI_SKOLT_FINLAND: strSub = TEXT("SUBLANG_SAMI_SKOLT_FINLAND"); break;
        case SUBLANG_SAMI_INARI_FINLAND: strSub = TEXT("SUBLANG_SAMI_INARI_FINLAND"); break;
        }
        break;
    case LANG_SANSKRIT: strPrim = TEXT("LANG_SANSKRIT");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SANSKRIT_INDIA: strSub = TEXT("SUBLANG_SANSKRIT_INDIA"); break;
        }
        break;
#ifdef ENABLE_NEW_LANGS
    case LANG_SCOTTISH_GAELIC: strPrim = TEXT("LANG_SCOTTISH_GAELIC");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SCOTTISH_GAELIC: strSub = TEXT("SUBLANG_SCOTTISH_GAELIC"); break;
        default: break;
        }
        break;
#endif
    //case LANG_SERBIAN: strPrim = TEXT("LANG_SERBIAN"); // same as LANG_BOSNIAN
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_SERBIAN_LATIN: strSub = TEXT("SUBLANG_SERBIAN_LATIN"); break;
    //    case SUBLANG_SERBIAN_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_CYRILLIC"); break;
    //    case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
    //    case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
    //    case SUBLANG_SERBIAN_MONTENEGRO_LATIN: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_LATIN"); break;
    //    case SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC"); break;
    //    case SUBLANG_SERBIAN_SERBIA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_LATIN"); break;
    //    case SUBLANG_SERBIAN_SERBIA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_CYRILLIC"); break;
    //    }
    //    break;
    case LANG_SERBIAN_NEUTRAL: strPrim = TEXT("LANG_SERBIAN_NEUTRAL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SERBIAN_LATIN: strSub = TEXT("SUBLANG_SERBIAN_LATIN"); break;
        case SUBLANG_SERBIAN_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_CYRILLIC"); break;
        case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN"); break;
        case SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC"); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_SERBIAN_MONTENEGRO_LATIN: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_LATIN"); break;
        case SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC"); break;
        case SUBLANG_SERBIAN_SERBIA_LATIN: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_LATIN"); break;
        case SUBLANG_SERBIAN_SERBIA_CYRILLIC: strSub = TEXT("SUBLANG_SERBIAN_SERBIA_CYRILLIC"); break;
#endif
        }
        break;
    case LANG_SINDHI: strPrim = TEXT("LANG_SINDHI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SINDHI_INDIA: strSub = TEXT("SUBLANG_SINDHI_INDIA"); break;
        case SUBLANG_SINDHI_AFGHANISTAN: strSub = TEXT("SUBLANG_SINDHI_AFGHANISTAN"); break;
        //case SUBLANG_SINDHI_PAKISTAN: strSub = TEXT("SUBLANG_SINDHI_PAKISTAN"); break; // same as SUBLANG_SINDHI_AFGHANISTAN
        }
        break;
    case LANG_SINHALESE: strPrim = TEXT("LANG_SINHALESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SINHALESE_SRI_LANKA: strSub = TEXT("SUBLANG_SINHALESE_SRI_LANKA"); break;
        }
        break;
    case LANG_SLOVAK: strPrim = TEXT("LANG_SLOVAK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SLOVAK_SLOVAKIA: strSub = TEXT("SUBLANG_SLOVAK_SLOVAKIA"); break;
        }
        break;
    case LANG_SLOVENIAN: strPrim = TEXT("LANG_SLOVENIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SLOVENIAN_SLOVENIA: strSub = TEXT("SUBLANG_SLOVENIAN_SLOVENIA"); break;
        }
        break;
    case LANG_SOTHO: strPrim = TEXT("LANG_SOTHO");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA: strSub = TEXT("SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA"); break;
        }
        break;
    case LANG_SPANISH: strPrim = TEXT("LANG_SPANISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SPANISH: strSub = TEXT("SUBLANG_SPANISH"); break;
        case SUBLANG_SPANISH_MEXICAN: strSub = TEXT("SUBLANG_SPANISH_MEXICAN"); break;
        case SUBLANG_SPANISH_MODERN: strSub = TEXT("SUBLANG_SPANISH_MODERN"); break;
        case SUBLANG_SPANISH_GUATEMALA: strSub = TEXT("SUBLANG_SPANISH_GUATEMALA"); break;
        case SUBLANG_SPANISH_COSTA_RICA: strSub = TEXT("SUBLANG_SPANISH_COSTA_RICA"); break;
        case SUBLANG_SPANISH_PANAMA: strSub = TEXT("SUBLANG_SPANISH_PANAMA"); break;
        case SUBLANG_SPANISH_DOMINICAN_REPUBLIC: strSub = TEXT("SUBLANG_SPANISH_DOMINICAN_REPUBLIC"); break;
        case SUBLANG_SPANISH_VENEZUELA: strSub = TEXT("SUBLANG_SPANISH_VENEZUELA"); break;
        case SUBLANG_SPANISH_COLOMBIA: strSub = TEXT("SUBLANG_SPANISH_COLOMBIA"); break;
        case SUBLANG_SPANISH_PERU: strSub = TEXT("SUBLANG_SPANISH_PERU"); break;
        case SUBLANG_SPANISH_ARGENTINA: strSub = TEXT("SUBLANG_SPANISH_ARGENTINA"); break;
        case SUBLANG_SPANISH_ECUADOR: strSub = TEXT("SUBLANG_SPANISH_ECUADOR"); break;
        case SUBLANG_SPANISH_CHILE: strSub = TEXT("SUBLANG_SPANISH_CHILE"); break;
        case SUBLANG_SPANISH_URUGUAY: strSub = TEXT("SUBLANG_SPANISH_URUGUAY"); break;
        case SUBLANG_SPANISH_PARAGUAY: strSub = TEXT("SUBLANG_SPANISH_PARAGUAY"); break;
        case SUBLANG_SPANISH_BOLIVIA: strSub = TEXT("SUBLANG_SPANISH_BOLIVIA"); break;
        case SUBLANG_SPANISH_EL_SALVADOR: strSub = TEXT("SUBLANG_SPANISH_EL_SALVADOR"); break;
        case SUBLANG_SPANISH_HONDURAS: strSub = TEXT("SUBLANG_SPANISH_HONDURAS"); break;
        case SUBLANG_SPANISH_NICARAGUA: strSub = TEXT("SUBLANG_SPANISH_NICARAGUA"); break;
        case SUBLANG_SPANISH_PUERTO_RICO: strSub = TEXT("SUBLANG_SPANISH_PUERTO_RICO"); break;
        case SUBLANG_SPANISH_US: strSub = TEXT("SUBLANG_SPANISH_US"); break;
        }
        break;
    case LANG_SWAHILI: strPrim = TEXT("LANG_SWAHILI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_SWAHILI_KENYA: strSub = TEXT("SUBLANG_SWAHILI_KENYA"); break;
        }
        break;
    case LANG_SWEDISH: strPrim = TEXT("LANG_SWEDISH");
        SWITCH_SUBLANG()
        {
        //case SUBLANG_SWEDISH_SWEDEN: strSub = TEXT("SUBLANG_SWEDISH_SWEDEN"); break; // same as SUBLANG_SWEDISH
        case SUBLANG_SWEDISH: strSub = TEXT("SUBLANG_SWEDISH"); break;
        case SUBLANG_SWEDISH_FINLAND: strSub = TEXT("SUBLANG_SWEDISH_FINLAND"); break;
        }
        break;
    case LANG_SYRIAC: strPrim = TEXT("LANG_SYRIAC");
#if defined(ENABLE_NEW_LANGS)
        SWITCH_SUBLANG()
        {
        case SUBLANG_SYRIAC: strSub = TEXT("SUBLANG_SYRIAC"); break;
        }
#endif
        break;
    case LANG_TAJIK: strPrim = TEXT("LANG_TAJIK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TAJIK_TAJIKISTAN: strSub = TEXT("SUBLANG_TAJIK_TAJIKISTAN"); break;
        }
        break;
    case LANG_TAMAZIGHT: strPrim = TEXT("LANG_TAMAZIGHT");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TAMAZIGHT_ALGERIA_LATIN   : strSub = TEXT("SUBLANG_TAMAZIGHT_ALGERIA_LATIN   "); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH: strSub = TEXT("SUBLANG_TAMAZIGHT_MOROCCO_TIFINAGH"); break;
#endif
        }
        break;
    case LANG_TAMIL: strPrim = TEXT("LANG_TAMIL");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TAMIL_INDIA: strSub = TEXT("SUBLANG_TAMIL_INDIA"); break;
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_TAMIL_SRI_LANKA: strSub = TEXT("SUBLANG_TAMIL_SRI_LANKA"); break;
#endif
        default: break;
        }
        break;
    case LANG_TATAR: strPrim = TEXT("LANG_TATAR");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TATAR_RUSSIA: strSub = TEXT("SUBLANG_TATAR_RUSSIA"); break;
        }
        break;
    case LANG_TELUGU: strPrim = TEXT("LANG_TELUGU");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TELUGU_INDIA: strSub = TEXT("SUBLANG_TELUGU_INDIA"); break;
        }
        break;
    case LANG_THAI: strPrim = TEXT("LANG_THAI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_THAI_THAILAND: strSub = TEXT("SUBLANG_THAI_THAILAND"); break;
        }
        break;
    case LANG_TIBETAN: strPrim = TEXT("LANG_TIBETAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TIBETAN_PRC: strSub = TEXT("SUBLANG_TIBETAN_PRC"); break;
#if defined(ENABLE_NEW_LANGS)
        case SUBLANG_TIBETAN_BHUTAN: strSub = TEXT("SUBLANG_TIBETAN_BHUTAN"); break;
#endif
        }
        break;
    case LANG_TIGRIGNA: strPrim = TEXT("LANG_TIGRIGNA"); // same as LANG_TIGRINYA
        SWITCH_SUBLANG()
        {
        case SUBLANG_TIGRIGNA_ERITREA: strSub = TEXT("SUBLANG_TIGRIGNA_ERITREA"); break;
        default: break;
        }
        break;
    //case LANG_TIGRINYA: strPrim = TEXT("LANG_TIGRINYA"); // same as LANG_TIGRIGNA
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_TIGRINYA_ERITREA: strSub = TEXT("SUBLANG_TIGRINYA_ERITREA"); break;
    //    case SUBLANG_TIGRINYA_ETHIOPIA: strSub = TEXT("SUBLANG_TIGRINYA_ETHIOPIA"); break;
    //    }
    //    break;
    case LANG_TSWANA: strPrim = TEXT("LANG_TSWANA");
        SWITCH_SUBLANG()
        {
#ifdef ENABLE_NEW_LANGS
        case SUBLANG_TSWANA_BOTSWANA: strSub = TEXT("SUBLANG_TSWANA_BOTSWANA"); break;
#endif
        case SUBLANG_TSWANA_SOUTH_AFRICA: strSub = TEXT("SUBLANG_TSWANA_SOUTH_AFRICA"); break;
        }
        break;
    case LANG_TURKISH: strPrim = TEXT("LANG_TURKISH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TURKISH_TURKEY: strSub = TEXT("SUBLANG_TURKISH_TURKEY"); break;
        }
        break;
    case LANG_TURKMEN: strPrim = TEXT("LANG_TURKMEN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_TURKMEN_TURKMENISTAN: strSub = TEXT("SUBLANG_TURKMEN_TURKMENISTAN"); break;
        }
        break;
    case LANG_UIGHUR: strPrim = TEXT("LANG_UIGHUR");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UIGHUR_PRC: strSub = TEXT("SUBLANG_UIGHUR_PRC"); break;
        }
        break;
    case LANG_UKRAINIAN: strPrim = TEXT("LANG_UKRAINIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UKRAINIAN_UKRAINE: strSub = TEXT("SUBLANG_UKRAINIAN_UKRAINE"); break;
        }
        break;
    case LANG_UPPER_SORBIAN: strPrim = TEXT("LANG_UPPER_SORBIAN");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UPPER_SORBIAN_GERMANY: strSub = TEXT("SUBLANG_UPPER_SORBIAN_GERMANY"); break;
        }
        break;
    case LANG_URDU: strPrim = TEXT("LANG_URDU");
        SWITCH_SUBLANG()
        {
        case SUBLANG_URDU_PAKISTAN: strSub = TEXT("SUBLANG_URDU_PAKISTAN"); break;
        case SUBLANG_URDU_INDIA: strSub = TEXT("SUBLANG_URDU_INDIA"); break;
        }
        break;
    case LANG_UZBEK: strPrim = TEXT("LANG_UZBEK");
        SWITCH_SUBLANG()
        {
        case SUBLANG_UZBEK_LATIN: strSub = TEXT("SUBLANG_UZBEK_LATIN"); break;
        case SUBLANG_UZBEK_CYRILLIC: strSub = TEXT("SUBLANG_UZBEK_CYRILLIC"); break;
        }
        break;
    //case LANG_VALENCIAN: strPrim = TEXT("LANG_VALENCIAN"); // same as LANG_CATALAN
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_VALENCIAN_VALENCIA: strSub = TEXT("SUBLANG_VALENCIAN_VALENCIA"); break;
    //    }
    //    break;
    case LANG_VIETNAMESE: strPrim = TEXT("LANG_VIETNAMESE");
        SWITCH_SUBLANG()
        {
        case SUBLANG_VIETNAMESE_VIETNAM: strSub = TEXT("SUBLANG_VIETNAMESE_VIETNAM"); break;
        }
        break;
    case LANG_WELSH: strPrim = TEXT("LANG_WELSH");
        SWITCH_SUBLANG()
        {
        case SUBLANG_WELSH_UNITED_KINGDOM: strSub = TEXT("SUBLANG_WELSH_UNITED_KINGDOM"); break;
        }
        break;
    case LANG_WOLOF: strPrim = TEXT("LANG_WOLOF");
        SWITCH_SUBLANG()
        {
        case SUBLANG_WOLOF_SENEGAL: strSub = TEXT("SUBLANG_WOLOF_SENEGAL"); break;
        }
        break;
    case LANG_XHOSA: strPrim = TEXT("LANG_XHOSA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_XHOSA_SOUTH_AFRICA: strSub = TEXT("SUBLANG_XHOSA_SOUTH_AFRICA"); break;
        }
        break;
    //case LANG_YAKUT: strPrim = TEXT("LANG_YAKUT"); // same as LANG_SAKHA
    //    SWITCH_SUBLANG()
    //    {
    //    case SUBLANG_YAKUT_RUSSIA: strSub = TEXT("SUBLANG_YAKUT_RUSSIA"); break;
    //    }
    //    break;
    case LANG_YI: strPrim = TEXT("LANG_YI");
        SWITCH_SUBLANG()
        {
        case SUBLANG_YI_PRC: strSub = TEXT("SUBLANG_YI_PRC"); break;
        }
        break;
    case LANG_YORUBA: strPrim = TEXT("LANG_YORUBA");
        SWITCH_SUBLANG()
        {
        case SUBLANG_YORUBA_NIGERIA: strSub = TEXT("SUBLANG_YORUBA_NIGERIA"); break;
        }
        break;
    case LANG_ZULU: strPrim = TEXT("LANG_ZULU");
        SWITCH_SUBLANG()
        {
        case SUBLANG_ZULU_SOUTH_AFRICA: strSub = TEXT("SUBLANG_ZULU_SOUTH_AFRICA"); break;
        }
        break;
    default:
        break;
    }

    TCHAR szText[32];
    if (strPrim.empty())
    {
        StringCchPrintf(szText, _countof(szText), TEXT("0x%04X"), PRIMARYLANGID(langid));
        strPrim = szText;
    }

    if (bOldStyle)
        strSub.clear();

    if (strSub.empty())
    {
        switch (SUBLANGID(langid))
        {
        case SUBLANG_NEUTRAL: strSub = TEXT("SUBLANG_NEUTRAL"); break;
        case SUBLANG_DEFAULT: strSub = TEXT("SUBLANG_DEFAULT"); break;
        case SUBLANG_SYS_DEFAULT: strSub = TEXT("SUBLANG_SYS_DEFAULT"); break;
        case SUBLANG_CUSTOM_DEFAULT: strSub = TEXT("SUBLANG_CUSTOM_DEFAULT"); break;
        case SUBLANG_CUSTOM_UNSPECIFIED: strSub = TEXT("SUBLANG_CUSTOM_UNSPECIFIED"); break;
        case SUBLANG_UI_CUSTOM_DEFAULT: strSub = TEXT("SUBLANG_UI_CUSTOM_DEFAULT"); break;
        default:
            break;
        }
    }
    if (strSub.empty())
    {
        StringCchPrintf(szText, _countof(szText), TEXT("0x%04X"), SUBLANGID(langid));
        strSub = szText;
    }
#undef SWITCH_SUBLANG

    MString str = TEXT("LANGUAGE ");
    str += strPrim;
    str += TEXT(", ");
    str += strSub;
    str += TEXT("\r\n");
    return str;
}

////////////////////////////////////////////////////////////////////////////

#pragma comment(linker,"/manifestdependency:\"type='win32' \
  name='Microsoft.Windows.Common-Controls' \
  version='6.0.0.0' \
  processorArchitecture='x86' \
  publicKeyToken='6595b64144ccf1df' \
  language='*'\"")

IMPLEMENT_DYNAMIC(MOleCtrl)

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    // get Unicode command line
    INT argc = 0;
    LPWSTR *targv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // initialize the libraries
    OleInitialize(NULL);

    // register MOleCtrl window class
    MOleCtrl::RegisterDx();

    // initialize common controls
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_WIN95_CLASSES |
                 ICC_DATE_CLASSES |
                 ICC_USEREX_CLASSES |
                 ICC_COOL_CLASSES |
                 ICC_INTERNET_CLASSES |
                 ICC_PAGESCROLLER_CLASS |
                 ICC_NATIVEFNTCTL_CLASS |
                 ICC_STANDARD_CLASSES |
                 ICC_LINK_CLASS;
    InitCommonControlsEx(&iccx);

    // load RichEdit
    HINSTANCE hinstRichEdit = LoadLibrary(TEXT("RICHED32.DLL"));

    // load GDI+
    Gdiplus::GdiplusStartupInput gp_startup_input;
    ULONG_PTR gp_token;
    Gdiplus::GdiplusStartup(&gp_token, &gp_startup_input, NULL);

    // main process
    int ret;
    MEditCtrl::SetCtrlAHookDx(TRUE);
    {
        MMainWnd app(argc, targv, hInstance);

        if (app.StartDx())
        {
            // main loop
            ret = INT(app.RunDx());
        }
        else
        {
            ret = 2;
        }
    }
    MEditCtrl::SetCtrlAHookDx(FALSE);

    // free GDI+
    Gdiplus::GdiplusShutdown(gp_token);

    // free the libraries
    FreeLibrary(hinstRichEdit);
    OleUninitialize();
    FreeWCLib();

    // free command line
    LocalFree(targv);

    // check object counts
    assert(MacroParser::BaseAst::alive_count() == 0);

#if (WINVER >= 0x0500)
    HANDLE hProcess = GetCurrentProcess();
    DebugPrintDx(TEXT("Count of GDI objects: %ld\n"),
                 GetGuiResources(hProcess, GR_GDIOBJECTS));
    DebugPrintDx(TEXT("Count of USER objects: %ld\n"),
                 GetGuiResources(hProcess, GR_USEROBJECTS));
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
    // for detecting memory leak (MSVC only)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    return ret;
}

//////////////////////////////////////////////////////////////////////////////
