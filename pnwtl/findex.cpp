/**
 * @file findex.cpp
 * @brief Find and Replace dialogs for PN 2
 * @author Simon Steele
 * @note Copyright (c) 2004 Simon Steele <s.steele@pnotepad.org>
 *
 * Programmers Notepad 2 : The license file (license.[txt|html]) describes 
 * the conditions under which this source may be modified / distributed.
 */

#include "stdafx.h"
#include "resource.h"
#include "CustomAutoComplete.h"
#include "include/accombo.h"
#include "findex.h"
#include "childfrm.h"

#define SWP_SIMPLEMOVE (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)

const UINT CHECKBOX_CONTROL_IDS [] = {
	IDC_MATCHWHOLE_CHECK, 
	IDC_MATCHCASE_CHECK, 
	IDC_REGEXP_CHECK, 
	IDC_BACKSLASHES_CHECK,
	IDC_SEARCHUP_CHECK,
	IDC_SUBDIRS_CHECK
};

const UINT RADIO_CONTROL_IDS [] = {
	IDC_CURRENTDOC_RADIO,
	IDC_ALLOPEN_RADIO,
	IDC_CURRENTPROJ_RADIO,
	IDC_INSELECTION_RADIO
};

const int CHECKBOX_CONTROL_COUNT = 6;

const UINT FIND_CHECKBOXES [] = {
	IDC_MATCHWHOLE_CHECK, 
	IDC_MATCHCASE_CHECK, 
	IDC_REGEXP_CHECK, 
	IDC_BACKSLASHES_CHECK,
	IDC_SEARCHUP_CHECK
};

const int nFIND_CHECKBOXES = 5;

const UINT REPLACE_CHECKBOXES [] = {
	IDC_MATCHWHOLE_CHECK, 
	IDC_MATCHCASE_CHECK, 
	IDC_REGEXP_CHECK,
	IDC_BACKSLASHES_CHECK,
	IDC_SEARCHUP_CHECK
};

const int nREPLACE_CHECKBOXES = 4;

const UINT FINDINFILES_CHECKBOXES [] = {
	//IDC_MATCHWHOLE_CHECK, 
	IDC_MATCHCASE_CHECK, 
	//IDC_REGEXP_CHECK, 
	//IDC_SEARCHUP_CHECK, 
	//IDC_BACKSLASHES_CHECK
	IDC_SUBDIRS_CHECK
};

int nFINDINFILES_CHECKBOXES = 2;

class ATL_NO_VTABLE ClientRect : public CRect
{
public:
	ClientRect(CWindow& wnd, CWindow& owner)
	{
		set(wnd, owner);
	}

	void set(CWindow& wnd, CWindow& owner)
	{
		wnd.GetWindowRect(this);
		owner.ScreenToClient(this);
	}
};

CFindExDialog::CFindExDialog()
{
	m_SearchWhere = 0;
	m_type = eftFind;

	m_lastVisibleCB = -1;
	m_bottom = -1;
}

BOOL CFindExDialog::PreTranslateMessage(MSG* pMsg)
{
	return IsDialogMessage(pMsg);
}

void CFindExDialog::Show(EFindDialogType type, LPCTSTR findText)
{
	m_type = type;
	updateLayout();

	m_FindText = findText;
	DoDataExchange(FALSE);

	// Place the dialog so it doesn't hide the text under the caret
	CChildFrame* editor = getCurrentEditorWnd();
	if(editor)
	{
		// Get the position on screen of the caret.
		CTextView* textView = editor->GetTextView();
		long pos = textView->GetCurrentPos();
		POINT pt = {
			textView->PointXFromPosition(pos),
			textView->PointYFromPosition(pos)
		};
		int lineHeight = textView->TextHeight(0);
		editor->ClientToScreen(&pt);
		
		// Place the window away from the caret.
		placeWindow(pt, lineHeight);
	}

	ShowWindow(SW_SHOW);
	
	m_FindTextCombo.SetFocus();
}

LRESULT CFindExDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// Set up the tab control images.
	m_imageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 4, 4);
	
	HBITMAP hBitmap = (HBITMAP)::LoadImage(
		_Module.m_hInst,
		MAKEINTRESOURCE(IDB_FINDTOOLBAR),
		IMAGE_BITMAP, 0, 0, LR_SHARED);

	m_imageList.Add(hBitmap, RGB(255,0,255));
	m_tabControl.SetImageList(m_imageList.m_hImageList);
	
	// Position and create the tab control.
	DWORD dwStyle = WS_TABSTOP | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CTCS_HOTTRACK | CTCS_FLATEDGE;

	CRect rcTabs;
	GetClientRect(rcTabs);
	rcTabs.top = 2;
	rcTabs.left = 2;
	rcTabs.bottom = calcTabAreaHeight();

	m_tabControl.Create(m_hWnd, rcTabs, NULL, dwStyle, 0, IDC_FINDEX_TABS);
	m_tabControl.SetWindowPos(GetDlgItem(IDC_FINDEXTABS_PLACEHOLDER), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);

	// Move the line that goes beneath the tab control.
	CStatic s(GetDlgItem(IDC_FINDEX_LINE));
	ClientRect rc(s, *this);
	int height = rc.Height();
	rc.top = rcTabs.bottom + 1;
	s.SetWindowPos(HWND_TOP, rc, SWP_SIMPLEMOVE);

	// Sort out the combo boxes.
	CSize size = getGUIFontSize();
	
	rc.set(GetDlgItem(IDC_FINDTEXT_DUMMY), *this);
	rc.bottom = rc.top + (size.cy * 10);

	m_FindTextCombo.Create(m_hWnd, rc, _T("FINDTEXTCOMBO"), CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CHILD | WS_VSCROLL | WS_VISIBLE | WS_TABSTOP, 0, IDC_FINDTEXT_COMBO,
		_T("Software\\Echo Software\\PN2\\AutoComplete\\Find"), IDC_FINDTEXT_DUMMY);

	rc.set(GetDlgItem(IDC_REPLACETEXT_DUMMY), *this);
	rc.bottom = rc.top + (size.cy * 10);

	m_ReplaceTextCombo.Create(m_hWnd, rc, _T("REPLACETEXTCOMBO"), CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, IDC_REPLACETEXT_COMBO,
		_T("Software\\Echo Software\\PN2\\AutoComplete\\Replace"), IDC_REPLACETEXT_DUMMY);

	// Store the position of the second combo.
	m_group2Top = rc.top;

	rc.set(GetDlgItem(IDC_FINDWHERE_DUMMY), *this);
	rc.bottom = rc.top + (size.cy * 10);

	m_FindWhereCombo.Create(m_hWnd, rc, _T("FINDWHERECOMBO"), CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, IDC_FINDWHERE_COMBO,
		IDC_FINDWHERE_DUMMY);

	rc.set(GetDlgItem(IDC_FINDTYPE_DUMMY), *this);
	rc.bottom = rc.top + (size.cy * 10);

	m_FindTypeCombo.Create(m_hWnd, rc, _T("FINDTYPECOMBO"), CBS_DROPDOWN | CBS_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, IDC_FINDTYPE_COMBO);
	m_FindTypeCombo.SetFont(GetFont());
	m_FindTypeCombo.SetWindowPos(GetDlgItem(IDC_FINDTYPE_DUMMY), 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);

	m_ReHelperBtn.SubclassWindow(GetDlgItem(IDC_REHELPER_BUTTON));
	m_ReHelperBtn2.SubclassWindow(GetDlgItem(IDC_RHELPER_BUTTON));

	UIEnable(IDC_REHELPER_BUTTON, true);

	UIAddChildWindowContainer(m_hWnd);

	CenterWindow(GetParent());

	// Work out some combo box vertical positions.
	ClientRect rcCombo(m_FindWhereCombo, *this);

	m_comboDistance = rcCombo.top - m_group2Top;
	m_group3Bottom = rcCombo.bottom;

	// Move all the combo boxes into position.
	moveUp(m_comboDistance, m_FindWhereCombo);
	moveUp(m_comboDistance, m_FindTypeCombo);
	moveUp(m_comboDistance, CStatic(GetDlgItem(IDC_FINDWHERE_LABEL)));
	moveUp(m_comboDistance, CStatic(GetDlgItem(IDC_FINDTYPE_LABEL)));

	rcCombo.set(m_ReplaceTextCombo, *this);
	
	m_group2Bottom = rcCombo.bottom;
	m_group1Bottom = m_group2Bottom - m_comboDistance;

	// Add tabs
	addTab(_T("Find"), 2);
	addTab(_T("Replace"), 3);
	addTab(_T("Find in Files"), 0);

	updateLayout();

	DlgResize_Init();

	m_FindTextCombo.SetFocus();

	return TRUE;
}

LRESULT CFindExDialog::OnShowWindow(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if((BOOL)wParam)
	{
		SReplaceOptions* pOptions = OPTIONS->GetReplaceOptions();
		//SetFindText(m_FindText);
		if(m_FindText.GetLength() == 0 && pOptions->FindText.GetLength())
			m_FindText = pOptions->FindText;
		m_ReplaceText = pOptions->ReplaceText;
		m_bMatchCase = pOptions->MatchCase;
		m_bMatchWhole = pOptions->MatchWholeWord;
		m_bSearchUp = !pOptions->Direction;
		m_bRegExp = pOptions->UseRegExp;
		m_bUseSlashes = pOptions->UseSlashes;

		//m_bSearchAll = pOptions->SearchAll;
		
		// Do the funky DDX thang...
		DoDataExchange(FALSE);

		m_FindTextCombo.SetFocus();

		UIEnable(IDC_REHELPER_BUTTON, m_bRegExp);
		UIEnable(IDC_RHELPER_BUTTON, m_bRegExp);
		UIUpdateChildWindows();
	}

	return 0;
}

LRESULT CFindExDialog::OnCloseWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	GetWindow(GW_OWNER).SetFocus();
	ShowWindow(SW_HIDE);
	return 0;
}

LRESULT CFindExDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	/*CChildFrame* pChild = GetCurrentEditorWnd();
	if(pChild != NULL)
		pChild->SetFocus();
	else*/
	GetWindow(GW_OWNER).SetFocus();

	ShowWindow(SW_HIDE);
	
	return 0;
}

LRESULT CFindExDialog::OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(findNext())
	{
		if(m_type != eftReplace && !OPTIONS->Get(PNSK_INTERFACE, _T("FindStaysOpen"), false))
			// Default Visual C++, old PN and others behaviour:
			ShowWindow(SW_HIDE);
	}
	else
	{
		CString strTextToFind = m_FindText;
		CString strMsg;

		if (strTextToFind.IsEmpty())
			strTextToFind = _T("(empty)");

		strMsg.Format(IDS_FINDNOTFOUND, strTextToFind);
		MessageBox((LPCTSTR)strMsg, _T("Programmers Notepad"), MB_OK | MB_ICONINFORMATION);
	}

	return TRUE;

	findNext();
}

LRESULT CFindExDialog::OnReHelperClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CRect rc;
	GetDlgItem(wID).GetWindowRect(&rc);
	doRegExpHelperMenu(rc, wID == IDC_RHELPER_BUTTON);

	return TRUE;
}

LRESULT CFindExDialog::OnReInsertClicked(WORD /*wNotifyCode*/, WORD nID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CString Text;
	int offset = getRegExpString(nID, Text);

	DoDataExchange(TRUE);
	doRegExpInsert(&m_FindTextCombo, Text, m_FindText, offset);

	return TRUE;
}

LRESULT CFindExDialog::OnReplaceClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// Call this before GetCurrentEditorWnd - it will check if the editor
	// has changed and if so set Found to false.
	SReplaceOptions* pOptions = getOptions();

	CChildFrame* pEditor = getCurrentEditorWnd();		
		
	if(pEditor)
		pEditor->Replace(pOptions);

	return 0;
}

LRESULT CFindExDialog::OnReplaceAllClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	SReplaceOptions* pOptions = getOptions();

	CChildFrame* pEditor = getCurrentEditorWnd();
		
	if(pEditor)
	{
		int count = pEditor->ReplaceAll(pOptions);
		CString s;
		s.Format(IDS_NREPLACEMENTS, count);
		MessageBox((LPCTSTR)s, _T("Programmers Notepad"), MB_OK | MB_ICONINFORMATION);
	}

	return 0;
}

LRESULT CFindExDialog::OnReMatchesMenuItemClicked(WORD /*wNotifyCode*/, WORD nID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int matchno = (nID - ID_REGEXP_TAGGEDEXPRESSION1) + 1;
	CString Text;
	Text.Format(_T("\\%d"), matchno);

	DoDataExchange(TRUE);
	doRegExpInsert(&m_ReplaceTextCombo, Text, m_ReplaceText, 0);

	return 0;
}

LRESULT CFindExDialog::OnUseRegExpClicked(WORD /*wNotifyCode*/, WORD /*nID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_bRegExp = !m_bRegExp;
	UIEnable(IDC_REHELPER_BUTTON, m_bRegExp);
	UIEnable(IDC_RHELPER_BUTTON, m_bRegExp);
	UIUpdateChildWindows();
	return TRUE;
}

LRESULT CFindExDialog::OnSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
{
	int nNewTab = m_tabControl.GetCurSel();
	m_type = (EFindDialogType)nNewTab;
	updateLayout();
	return 0;
}

int CFindExDialog::addTab(LPCTSTR name, int iconIndex)
{
	CDotNetButtonTabCtrl<>::TItem* pItem = m_tabControl.CreateNewItem();
	if(pItem)
	{
		pItem->SetText(name);
		pItem->SetImageIndex(iconIndex);

		// The tab control takes ownership of the new item
		return m_tabControl.InsertItem(m_tabControl.GetItemCount(), pItem);
	}

	return -1;
}

// A derived class might not need to override this although they can.
// (but they will probably need to specialize SetTabAreaHeight)
int CFindExDialog::calcTabAreaHeight(void)
{
	// Dynamically figure out a reasonable tab area height
	// based on the tab's font metrics

	const int nNominalHeight = 28;
	const int nNominalFontLogicalUnits = 11;	// 8 point Tahoma with 96 DPI

	// Initialize nFontLogicalUnits to the typical case
	// appropriate for CDotNetTabCtrl
	LOGFONT lfIcon = { 0 };
	::SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lfIcon), &lfIcon, 0);
	int nFontLogicalUnits = -lfIcon.lfHeight;

	// Use the actual font of the tab control
	if(m_tabControl.IsWindow())
	{
		HFONT hFont = m_tabControl.GetFont();
		if(hFont != NULL)
		{
			CDC dc = m_tabControl.GetDC();
			CFontHandle hFontOld = dc.SelectFont(hFont);
			TEXTMETRIC tm = {0};
			dc.GetTextMetrics(&tm);
			nFontLogicalUnits = tm.tmAscent;
			dc.SelectFont(hFontOld);
		}
	}

	int nNewTabAreaHeight = nNominalHeight + ( ::MulDiv(nNominalHeight, nFontLogicalUnits, nNominalFontLogicalUnits) - nNominalHeight ) / 2;
	return nNewTabAreaHeight;
}

void CFindExDialog::doRegExpHelperMenu(LPRECT rc, bool bDoMatches)
{
	HMENU hRegExpMenu;
	if(bDoMatches)
	{
		hRegExpMenu = ::LoadMenu(_Module.m_hInst, MAKEINTRESOURCE(IDR_POPUP_REGEXPM));
	}
	else
	{
		hRegExpMenu = ::LoadMenu(_Module.m_hInst, MAKEINTRESOURCE(IDR_POPUP_REGEXP));
	}

	HMENU hPopup = ::GetSubMenu(hRegExpMenu, 0);

	::TrackPopupMenu(hPopup, 0, rc->right, rc->top, 0, m_hWnd, NULL);

	::DestroyMenu(hRegExpMenu);
}

int CFindExDialog::doRegExpInsert(BXT::CComboBoxAC* pCB, LPCTSTR insert, CString& str, int offset)
{
	CEdit cbedit(pCB->m_edit);
	DWORD dwSel = cbedit.GetSel();
	cbedit.ReplaceSel(insert);

	int pos = LOWORD(dwSel);
	if(offset != 0)
		pos += offset;
	else
		pos += _tcslen(insert);
	cbedit.SetFocus();
	cbedit.SetSel(pos, pos);

	return pos;
}

bool CFindExDialog::editorChanged()
{
	return m_pLastEditor != getCurrentEditorWnd();// CChildFrame::FromHandle(GetCurrentEditor());
}

bool CFindExDialog::findNext()
{
	int found = 0;

	CChildFrame* editor = getCurrentEditorWnd();

	if(editor != NULL)
	{
		SFindOptions* pOptions = getOptions();
		found = editor->FindNext(pOptions);			
	}
	
	return found != 0;
}

int CFindExDialog::getRegExpString(int nID, CString& Text)
{
	int offset = 0;

	switch(nID)
	{
		case ID_REGEXP_ANYCHARACTER:
			Text = _T(".");
			break;
		case ID_REGEXP_CHARACTERINRANGE:
			Text = _T("[]");
			offset = 1;
			break;
		case ID_REGEXP_CHARACTERNOTINRANGE:
			Text = _T("[^]");
			offset = 2;
			break;
		case ID_REGEXP_BEGINNINGOFLINE:
			Text = _T("^");
			break;
		case ID_REGEXP_ENDOFLINE:
			Text = _T("$");
			break;
		case ID_REGEXP_TAGGEDEXPRESSSION:
			Text = _T("\\(\\)");
			offset = 2;
			break;
		case ID_REGEXP_NOT:
			Text = _T("~");
			break;
		case ID_REGEXP_OR:
			Text = _T("\\!");
			break;
		case ID_REGEXP_0ORMOREMATCHES:
			Text = _T("*");
			break;
		case ID_REGEXP_1ORMOREMATCHES:
			Text = _T("+");
			break;
		case ID_REGEXP_GROUP:
			Text = _T("\\{\\}");
			offset = 2;
			break;
	};

	return offset;
}

CChildFrame* CFindExDialog::getCurrentEditorWnd()
{
	m_pLastEditor = CChildFrame::FromHandle(GetCurrentEditor());
	return m_pLastEditor;
}

CSize CFindExDialog::getGUIFontSize()
{
	CClientDC dc(m_hWnd);
	dc.SelectFont((HFONT) GetStockObject( DEFAULT_GUI_FONT ));		
	TEXTMETRIC tm;
	dc.GetTextMetrics( &tm );

	return CSize( tm.tmAveCharWidth, tm.tmHeight + tm.tmExternalLeading);
}

SReplaceOptions* CFindExDialog::getOptions()
{
	DoDataExchange(TRUE);

	//SFindOptions* pOptions = OPTIONS->GetFindOptions();
	SReplaceOptions* pOptions = OPTIONS->GetReplaceOptions();

	// If the user has changed to a differnent scintilla window
	// then Found is no longer necessarily true.
	if(editorChanged())
		pOptions->Found = false;

	ELookWhere where = (ELookWhere)m_SearchWhere;

	pOptions->FindText			= m_FindText;
	pOptions->ReplaceText		= m_ReplaceText;
	pOptions->Direction			= (m_bSearchUp == FALSE);
	pOptions->MatchCase			= (m_bMatchCase == TRUE);
	pOptions->MatchWholeWord	= (m_bMatchWhole == TRUE);
	pOptions->UseRegExp			= (m_bRegExp == TRUE);
	pOptions->UseSlashes		= (m_bUseSlashes == TRUE);
	pOptions->InSelection		= (where == elwSelection);
	pOptions->SearchAll			= (where == elwAllDocs);
	
	///@todo Add a user interface counterpart for the loop search option.
	pOptions->Loop				= true;

	return pOptions;
}

void CFindExDialog::moveUp(int offset, CWindow& ctrl)
{
	ClientRect rc(ctrl, *this);
	rc.top -= offset;
	ctrl.SetWindowPos(HWND_TOP, rc, SWP_SIMPLEMOVE);
}

/**
 * @brief Places the window so that it does not cover the cursor.
 * @param pt constant reference to the point that should not be covered by the dialog
 * @param lineHeight line height as specified by Scintilla.
 */
void CFindExDialog::placeWindow(const POINT& pt, int lineHeight)
{
	// Get current pos and size
	RECT rc;
	GetWindowRect(&rc);
	POINT winPos = { rc.left, rc.top };
	SIZE winSize = { rc.right - rc.left, rc.bottom - rc.top };

	// Default to place the dialog above the line
	POINT pos = winPos;
	if(rc.bottom >= pt.y && rc.top <= pt.y)
		pos.y = pt.y - winSize.cy - lineHeight;
	if(pos.y < 0)
		pos.y = max(0, pt.y + lineHeight);

	::GetWindowRect(GetWindow(GW_OWNER), &rc);
	// TODO: 
	pos.x = rc.right - winSize.cx - ::GetSystemMetrics(SM_CXVSCROLL);

	// Ensure that the dialog is inside the work area
	RECT rcWa;
	
	if(g_Context.OSVersion.dwMajorVersion >= 5) // support multiple monitors on 2k+
	{
		rcWa.top = 0;
		rcWa.left = 0;
		rcWa.right = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
		rcWa.bottom = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
	}
	else
	{
		// On older systems we rely on GetWorkArea which doesn't support 
		// multiple monitors.
		::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcWa, NULL);
	}

	if(pos.x + winSize.cx > rcWa.right - rcWa.left)
		pos.x = rcWa.right - rcWa.left - winSize.cx;
	if(pos.x < rcWa.left)
		pos.x = rcWa.left;
	if(pos.y + winSize.cy > rcWa.bottom - rcWa.top)
		pos.y = rcWa.bottom - rcWa.top - winSize.cy;
	if(pos.y < rcWa.top)
		pos.y = rcWa.top;

	// Move when the pos changed only
	if(winPos.x != pos.x || winPos.y != pos.y)
		// note no SWP_SHOWWINDOW - this causes OnShowWindow not to be called on creation.
		SetWindowPos(NULL, pos.x, pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER /*| SWP_SHOWWINDOW*/);
}

int CFindExDialog::positionChecks(int top, const UINT* checkboxIDs, int nCheckboxIDs)
{
	int chkDist = m_checkDist;

	ClientRect rc(CButton(GetDlgItem(checkboxIDs[0])), *this);
	rc.top = top;

	for(int j = 0; j < CHECKBOX_CONTROL_COUNT; j++)
	{
		CButton(GetDlgItem(CHECKBOX_CONTROL_IDS[j])).ShowWindow(SW_HIDE);
	}

	for(int i = 0; i < nCheckboxIDs; i++)
	{
		CButton cb(GetDlgItem(checkboxIDs[i]));
		cb.SetWindowPos(HWND_TOP, rc, SWP_SIMPLEMOVE);
		rc.top += chkDist;
		cb.ShowWindow(SW_SHOW);
	}

	m_lastVisibleCB = checkboxIDs[nCheckboxIDs-1];

	return rc.top;
}

void CFindExDialog::updateLayout()
{
	int restTop;
	const UINT* checkboxes = NULL;
	int nCheckboxes = 0;

	if(m_lastVisibleCB == -1)
	{
		// First time through, we calculate the gap between checkboxes
		// and work out where the initial last-visible checkbox is.
		int chkDist;
		CButton cb1(GetDlgItem(CHECKBOX_CONTROL_IDS[0]));
		ClientRect rc(cb1, *this);
		
		chkDist = rc.top;
		
		CButton cb2(GetDlgItem(CHECKBOX_CONTROL_IDS[1]));
		rc.set(cb2, *this);
		
		m_checkDist = rc.top - chkDist;

		m_lastVisibleCB = CHECKBOX_CONTROL_IDS[CHECKBOX_CONTROL_COUNT-1];

		ClientRect rcLastCheck(GetDlgItem(CHECKBOX_CONTROL_IDS[CHECKBOX_CONTROL_COUNT-1]), *this);
		m_bottom = rcLastCheck.bottom;
	}
	
	int oldBottom = m_bottom;

	switch(m_type)
	{
	case eftFind:
		{
			checkboxes = FIND_CHECKBOXES;
			nCheckboxes = nFIND_CHECKBOXES;
			
			CStatic(GetDlgItem(IDC_REPLACETEXT_LABEL)).ShowWindow(SW_HIDE);
			m_ReplaceTextCombo.ShowWindow(SW_HIDE);
			m_ReHelperBtn2.ShowWindow(SW_HIDE);

			CStatic(GetDlgItem(IDC_FINDWHERE_LABEL)).ShowWindow(SW_HIDE);
			m_FindWhereCombo.ShowWindow(SW_HIDE);
			CStatic(GetDlgItem(IDC_FINDTYPE_LABEL)).ShowWindow(SW_HIDE);
			m_FindTypeCombo.ShowWindow(SW_HIDE);

			CButton(GetDlgItem(IDC_REPLACE_BUTTON)).ShowWindow(SW_HIDE);
			CButton(GetDlgItem(IDC_REPLACEALL_BUTTON)).ShowWindow(SW_HIDE);

			CButton(GetDlgItem(IDC_MARKALL_BUTTON)).ShowWindow(SW_SHOW);

			CButton(GetDlgItem(IDC_CURRENTDOC_RADIO)).EnableWindow(TRUE);
			CButton(GetDlgItem(IDC_INSELECTION_RADIO)).EnableWindow(FALSE);
			
			restTop = m_group1Bottom + 12;
		}
		break;

	case eftReplace:
		{
			checkboxes = REPLACE_CHECKBOXES;
			nCheckboxes = nREPLACE_CHECKBOXES;
			
			CStatic(GetDlgItem(IDC_FINDWHERE_LABEL)).ShowWindow(SW_HIDE);
			m_FindWhereCombo.ShowWindow(SW_HIDE);
			CStatic(GetDlgItem(IDC_FINDTYPE_LABEL)).ShowWindow(SW_HIDE);
			m_FindTypeCombo.ShowWindow(SW_HIDE);

			CButton(GetDlgItem(IDC_MARKALL_BUTTON)).ShowWindow(SW_SHOW);

			CStatic(GetDlgItem(IDC_REPLACETEXT_LABEL)).ShowWindow(SW_SHOW);
			m_ReplaceTextCombo.ShowWindow(SW_SHOW);
			m_ReHelperBtn2.ShowWindow(SW_SHOW);

			CButton(GetDlgItem(IDC_REPLACE_BUTTON)).ShowWindow(SW_SHOW);
			CButton(GetDlgItem(IDC_REPLACEALL_BUTTON)).ShowWindow(SW_SHOW);

			CButton(GetDlgItem(IDC_CURRENTDOC_RADIO)).EnableWindow(TRUE);
			CButton(GetDlgItem(IDC_INSELECTION_RADIO)).EnableWindow(TRUE);

			restTop = m_group2Bottom + 12;
		}
		break;

	case eftFindInFiles:
		{
			checkboxes = FINDINFILES_CHECKBOXES;
			nCheckboxes = nFINDINFILES_CHECKBOXES;

			int nCheckboxes = 2;

			CStatic(GetDlgItem(IDC_REPLACETEXT_LABEL)).ShowWindow(SW_HIDE);
			m_ReplaceTextCombo.ShowWindow(SW_HIDE);
			m_ReHelperBtn2.ShowWindow(SW_HIDE);

			CButton(GetDlgItem(IDC_MARKALL_BUTTON)).ShowWindow(SW_HIDE);
			CButton(GetDlgItem(IDC_REPLACE_BUTTON)).ShowWindow(SW_HIDE);
			CButton(GetDlgItem(IDC_REPLACEALL_BUTTON)).ShowWindow(SW_HIDE);

			CStatic(GetDlgItem(IDC_FINDWHERE_LABEL)).ShowWindow(SW_SHOW);
			m_FindWhereCombo.ShowWindow(SW_SHOW);
			CStatic(GetDlgItem(IDC_FINDTYPE_LABEL)).ShowWindow(SW_SHOW);
			m_FindTypeCombo.ShowWindow(SW_SHOW);

			CButton(GetDlgItem(IDC_CURRENTDOC_RADIO)).EnableWindow(FALSE);
			CButton(GetDlgItem(IDC_INSELECTION_RADIO)).EnableWindow(FALSE);

			restTop = m_group3Bottom + 12;
		}
		break;
	}

	int bottomChecks = positionChecks(restTop, checkboxes, nCheckboxes);

	CStatic dirgroup(GetDlgItem(IDC_SEARCHIN_GROUP));
	ClientRect rc(dirgroup, *this);
	int dy = rc.top - (restTop);
	rc.top = restTop;
	dirgroup.SetWindowPos(HWND_TOP, rc, SWP_SIMPLEMOVE);

	for(int i = 0; i < 4; i++)
	{
		CButton up(GetDlgItem(RADIO_CONTROL_IDS[i]));
		rc.set(up, *this);
		rc.top -= dy;
		up.SetWindowPos(HWND_TOP, rc, SWP_SIMPLEMOVE);
	}

	// Get the new position of the bottom of the window.
	CRect rcMe;
	GetWindowRect(rcMe);
	int newBottom = rcMe.bottom - (oldBottom - bottomChecks);
	
	// Check whether it's below the groupbox.
	CRect rcGroup;
	dirgroup.GetWindowRect(rcGroup);
	newBottom = max(newBottom, rcGroup.bottom+12);

	// Get that difference and offset our minmaxinfo
	dy = rcMe.bottom - newBottom;
	m_ptMinTrackSize.y -= dy;

	m_bottom = rcMe.bottom = newBottom;
	//m_bottom = rcMe.bottom;

	SetWindowPos(HWND_TOP, rcMe, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);	
}