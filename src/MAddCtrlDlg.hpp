// MAddCtrlDlg
//////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MADDCTRLDLG_HPP_
#define MZC4_MADDCTRLDLG_HPP_

#include "RisohEditor.hpp"
#include "DialogRes.hpp"
#include "MString.hpp"
#include "resource.h"

class MAddCtrlDlg;

//////////////////////////////////////////////////////////////////////////////

void GetSelection(HWND hLst, std::vector<BYTE>& sel);
void GetSelection(std::vector<BYTE>& sel,
                  const ConstantsDB::TableType& table, DWORD dwValue);
DWORD AnalyseDifference(DWORD dwValue, ConstantsDB::TableType& table,
    std::vector<BYTE>& old_sel, std::vector<BYTE>& new_sel);
void InitStyleListBox(HWND hLst, ConstantsDB::TableType& table);
void InitClassComboBox(HWND hCmb, ConstantsDB& db);
void InitCtrlIDComboBox(HWND hCmb, ConstantsDB& db);

//////////////////////////////////////////////////////////////////////////////

class MAddCtrlDlg : public MDialogBase
{
public:
    DialogRes&      m_dialog_res;
    BOOL            m_bUpdating;
    DWORD           m_dwStyle;
    DWORD           m_dwExStyle;
    POINT           m_pt;
    ConstantsDB&    m_db;
    ConstantsDB::TableType  m_style_table;
    ConstantsDB::TableType  m_exstyle_table;
    std::vector<BYTE>       m_style_selection;
    std::vector<BYTE>       m_exstyle_selection;

    MAddCtrlDlg(DialogRes& dialog_res, ConstantsDB& db, POINT pt)
        : MDialogBase(IDD_ADDCTRL), m_dialog_res(dialog_res),
          m_db(db), m_bUpdating(FALSE), m_pt(pt)
    {
    }

    void InitTables(LPCTSTR pszClass)
    {
        ConstantsDB::TableType table;

        m_style_table.clear();
        if (pszClass && pszClass[0])
        {
            table = m_db.GetTable(pszClass);
            if (table.size())
            {
                m_style_table.insert(m_style_table.end(),
                    table.begin(), table.end());
            }
        }
        table = m_db.GetTable(TEXT("STYLE"));
        if (table.size())
        {
            m_style_table.insert(m_style_table.end(),
                table.begin(), table.end());
        }
        m_style_selection.resize(m_style_table.size());

        m_exstyle_table.clear();
        table = m_db.GetTable(TEXT("EXSTYLE"));
        if (table.size())
        {
            m_exstyle_table.insert(m_exstyle_table.end(),
                table.begin(), table.end());
        }
        m_exstyle_selection.resize(m_exstyle_table.size());
    }

    void ApplySelection(HWND hLst, std::vector<BYTE>& sel)
    {
        m_bUpdating = TRUE;
        INT iTop = ListBox_GetTopIndex(hLst);
        for (size_t i = 0; i < sel.size(); ++i)
        {
            ListBox_SetSel(hLst, sel[i], (DWORD)i);
        }
        ListBox_SetTopIndex(hLst, iTop);
        m_bUpdating = FALSE;
    }

    void ApplySelection(HWND hLst, ConstantsDB::TableType& table,
                        std::vector<BYTE>& sel, DWORD dwValue)
    {
        m_bUpdating = TRUE;
        INT iTop = ListBox_GetTopIndex(hLst);
        for (size_t i = 0; i < table.size(); ++i)
        {
            sel[i] = ((dwValue & table[i].mask) == table[i].value);
            ListBox_SetSel(hLst, sel[i], (DWORD)i);
        }
        ListBox_SetTopIndex(hLst, iTop);
        m_bUpdating = FALSE;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        SetDlgItemInt(hwnd, edt1, m_pt.x, FALSE);
        SetDlgItemInt(hwnd, edt2, m_pt.y, FALSE);

        INT cx = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("WIDTH"));
        INT cy = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("HEIGHT"));
        SetDlgItemInt(hwnd, edt3, cx, FALSE);
        SetDlgItemInt(hwnd, edt4, cy, FALSE);

        SetDlgItemInt(hwnd, cmb5, 0, FALSE);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        InitClassComboBox(hCmb1, m_db);

        HWND hCmb3 = GetDlgItem(hwnd, cmb3);
        InitCtrlIDComboBox(hCmb3, m_db);

        InitTables(NULL);

        TCHAR szText[64];

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        m_dwStyle = WS_VISIBLE | WS_CHILD;
        GetSelection(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        ::SendDlgItemMessage(hwnd, edt6, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        m_dwExStyle = 0;
        GetSelection(m_exstyle_selection, m_exstyle_table, m_dwExStyle);
        InitStyleListBox(hLst2, m_exstyle_table);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        wsprintf(szText, TEXT("%08lX"), m_dwExStyle);
        SetDlgItemText(hwnd, edt7, szText);
        ::SendDlgItemMessage(hwnd, edt7, EM_SETLIMITTEXT, 8, 0);
        m_bUpdating = FALSE;

        CenterWindowDx();
        return TRUE;
    }

    void OnOK(HWND hwnd)
    {
        MString strCaption = GetDlgItemText(cmb2);
        mstr_trim(strCaption);
        if (strCaption[0] == TEXT('"'))
        {
            mstr_unquote(strCaption);
        }

        INT x = GetDlgItemInt(hwnd, edt1, NULL, TRUE);
        INT y = GetDlgItemInt(hwnd, edt2, NULL, TRUE);
        INT cx = GetDlgItemInt(hwnd, edt3, NULL, TRUE);
        INT cy = GetDlgItemInt(hwnd, edt4, NULL, TRUE);
        MString strID = GetDlgItemText(cmb3);
        mstr_trim(strID);
        UINT id;
        if ((TEXT('0') <= strID[0] && strID[0] <= TEXT('9')) ||
            strID[0] == TEXT('-') || strID[0] == TEXT('+'))
        {
            id = _tcstol(strID.c_str(), NULL, 0);
        }
        else if (m_db.HasResID(strID))
        {
            id = m_db.GetResIDValue(strID);
        }
        else if (m_db.HasCtrlID(strID))
        {
            id = m_db.GetCtrlIDValue(strID);
        }
        else
        {
            HWND hCmb3 = GetDlgItem(hwnd, cmb3);
            Edit_SetSel(hCmb3, 0, -1);
            SetFocus(hCmb3);
            ErrorBoxDx(IDS_NOSUCHID);
            return;
        }

        MString strClass = GetDlgItemText(cmb4);
        mstr_trim(strClass);
        if (strClass.empty())
        {
            SetFocus(GetDlgItem(hwnd, cmb4));
            MsgBoxDx(LoadStringDx(IDS_ENTERCLASS), MB_ICONERROR);
            return;
        }

        MString strHelp = GetDlgItemText(cmb5);
        mstr_trim(strHelp);
        DWORD help;
        if (m_db.HasResID(strHelp))
        {
            help = m_db.GetResIDValue(strHelp);
        }
        else
        {
            help = _tcstoul(strHelp.c_str(), NULL, 0);
        }

        MString strStyle = GetDlgItemText(edt6);
        mstr_trim(strStyle);
        DWORD style = _tcstoul(strStyle.c_str(), NULL, 16);

        MString strExStyle = GetDlgItemText(edt7);
        mstr_trim(strExStyle);
        DWORD exstyle = _tcstoul(strExStyle.c_str(), NULL, 16);

        DialogItem item;
        item.m_HelpID = help;
        item.m_Style = style;
        item.m_ExStyle = exstyle;
        item.m_pt.x = x;
        item.m_pt.y = y;
        item.m_siz.cx = cx;
        item.m_siz.cy = cy;
        item.m_ID = id;
        item.m_Class = strClass.c_str();
        item.m_Title = strCaption.c_str();

        m_dialog_res.m_cItems++;
        m_dialog_res.Items.push_back(item);

        EndDialog(IDOK);
    }

    void OnLst1(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst1 = GetDlgItem(hwnd, lst1);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetSelection(hLst1, m_style_selection);

        m_dwStyle = AnalyseDifference(m_dwStyle, m_style_table,
                                      old_style_selection, m_style_selection);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;
    }

    void OnLst2(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        HWND hLst2 = GetDlgItem(hwnd, lst2);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetSelection(hLst2, m_exstyle_selection);

        m_dwExStyle = AnalyseDifference(m_dwExStyle, m_exstyle_table,
                                        old_exstyle_selection, m_exstyle_selection);
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, m_dwExStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwExStyle);
        SetDlgItemText(hwnd, edt7, szText);
        m_bUpdating = FALSE;
    }

    void OnEdt6(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        MString text = GetDlgItemText(hwnd, edt6);
        mstr_trim(text);
        DWORD dwStyle = _tcstoul(text.c_str(), NULL, 16);

        std::vector<BYTE> old_style_selection = m_style_selection;
        GetSelection(m_style_selection, m_style_table, dwStyle);

        HWND hLst1 = GetDlgItem(hwnd, lst1);
        m_dwStyle = dwStyle;
        ApplySelection(hLst1, m_style_table, m_style_selection, dwStyle);
    }

    void OnEdt7(HWND hwnd)
    {
        if (m_bUpdating)
            return;

        MString text = GetDlgItemText(hwnd, edt7);
        mstr_trim(text);
        DWORD dwExStyle = _tcstoul(text.c_str(), NULL, 16);

        std::vector<BYTE> old_exstyle_selection = m_exstyle_selection;
        GetSelection(m_exstyle_selection, m_exstyle_table, dwExStyle);

        HWND hLst2 = GetDlgItem(hwnd, lst2);
        m_dwExStyle = dwExStyle;
        ApplySelection(hLst2, m_exstyle_table, m_exstyle_selection, dwExStyle);
    }

    void UpdateClass(HWND hwnd, HWND hLst1, const MString& strClass)
    {
        MString strSuper;
        DWORD dwType = m_db.GetValue(TEXT("CONTROL.CLASSES"), strClass);
        if (dwType >= 3)
        {
            ConstantsDB::TableType table;
            table = m_db.GetTable(strClass + TEXT(".SUPERCLASS"));
            if (table.size())
            {
                strSuper = table[0].name;
            }
        }

        if (strSuper.size())
            InitTables(strSuper.c_str());
        else
            InitTables(strClass.c_str());

        MString str = strClass + TEXT(".DEFAULT.STYLE");
        m_dwStyle = m_db.GetValue(str, TEXT("STYLE"));

        GetSelection(m_style_selection, m_style_table, m_dwStyle);
        InitStyleListBox(hLst1, m_style_table);
        ApplySelection(hLst1, m_style_table, m_style_selection, m_dwStyle);

        m_bUpdating = TRUE;
        TCHAR szText[32];
        wsprintf(szText, TEXT("%08lX"), m_dwStyle);
        SetDlgItemText(hwnd, edt6, szText);
        m_bUpdating = FALSE;

        ListBox_SetTopIndex(hLst1, 0);

        INT cx, cy;
        cx = m_db.GetValue(strClass + TEXT(".SIZE"), TEXT("WIDTH"));
        cy = m_db.GetValue(strClass + TEXT(".SIZE"), TEXT("HEIGHT"));
        if (cx == 0 && cy == 0)
        {
            cx = m_db.GetValue(strSuper + TEXT(".SIZE"), TEXT("WIDTH"));
            cy = m_db.GetValue(strSuper + TEXT(".SIZE"), TEXT("HEIGHT"));
        }
        if (cx == 0 && cy == 0)
        {
            cx = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("WIDTH"));
            cy = m_db.GetValue(TEXT("CONTROL.SIZE"), TEXT("HEIGHT"));
        }
        SetDlgItemInt(hwnd, edt3, cx, FALSE);
        SetDlgItemInt(hwnd, edt4, cy, FALSE);

        if (strSuper.size())
            SetDlgItemText(hwnd, cmb4, strSuper.c_str());
        else
            SetDlgItemText(hwnd, cmb4, strClass.c_str());
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        TCHAR szText[64];
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(IDCANCEL);
            break;
        case cmb1:
            if (codeNotify == CBN_SELCHANGE)
            {
                INT nIndex = ComboBox_GetCurSel(hCmb1);
                ComboBox_GetLBText(hCmb1, nIndex, szText);
                MString text = szText;
                mstr_trim(text);
                UpdateClass(hwnd, hLst1, text);
            }
            else if (codeNotify == CBN_EDITCHANGE)
            {
                MString text = GetDlgItemText(hwnd, cmb1);
                mstr_trim(text);
                InitTables(text.c_str());
                UpdateClass(hwnd, hLst1, text);
            }
            break;
        case lst1:
            if (codeNotify == LBN_SELCHANGE)
            {
                OnLst1(hwnd);
            }
            break;
        case lst2:
            if (codeNotify == LBN_SELCHANGE)
            {
                OnLst2(hwnd);
            }
            break;
        case edt6:
            if (codeNotify == EN_CHANGE)
            {
                OnEdt6(hwnd);
            }
            break;
        case edt7:
            if (codeNotify == EN_CHANGE)
            {
                OnEdt7(hwnd);
            }
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return DefaultProcDx();
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MADDCTRLDLG_HPP_
