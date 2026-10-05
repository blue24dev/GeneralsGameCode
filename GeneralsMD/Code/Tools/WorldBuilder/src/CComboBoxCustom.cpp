//MODDD - new file. CComboBox subclass that acts like a CBS_DROPDOWNLIST (can't edit the current choice's text) but the
// combobox is still capable of displaying unique text in case of an invalid choice.
//TODO - header stuff

#include "StdAfx.h"

#include "CComboBoxCustom.h"

IMPLEMENT_DYNAMIC(CComboBoxCustom_CEdit, CEdit)

CComboBoxCustom_CEdit::CComboBoxCustom_CEdit() : m_hasFocus(FALSE)
{
	m_textColor = RGB(0, 0, 0);
	m_bkColor = RGB(255, 255, 255);
	m_bkBrush.CreateSolidBrush(m_bkColor);
	m_highlightTextColor = RGB(255, 255, 255);
	m_highlightBkColor = ::GetSysColor(COLOR_HIGHLIGHT);
	m_highlightBkBrush.CreateSolidBrush(m_highlightBkColor);
}

CComboBoxCustom_CEdit::~CComboBoxCustom_CEdit()
{
	DestroyWindow();
}

// basically 'init' for when the combobox's inner selection text field is subclassed
void CComboBoxCustom_CEdit::PreSubclassWindow()
{
	this->SetReadOnly(true);
	// nope, nothing works when you do this
	//this->EnableWindow(FALSE);
}

void CComboBoxCustom_CEdit::onDropdownShown()
{
	this->SetSel(-1, 0);
	this->HideCaret();
	//this->Invalidate();
}

// makes the background always white, not readonly-gray
HBRUSH CComboBoxCustom_CEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	CComboBoxCustom* parentWnd = (CComboBoxCustom*)this->GetParent();

	// I wanted to get the text field to have an unselected appearance when the dropdown is open like the original
	// 'CBS_DROPDOWNLIST'-styled combobox, but it seems this text field's label having a select-blue background
	// around just the text is very persistent and difficult to avoid.
	// Try other things like owner-draw to fix this if you really want to, but I'm done here.
	// Making the entire text field background select-blue when the dropdown is open is the next best thing I suppose
	// (and when the combobox is in focus for any other reason as usual).
	// Also, would "GetFocus() == <wnd reference>" between this & 'parentWnd' be enough to not need my 'm_hasFocus' fields? Unsure.	
	if ((m_hasFocus || parentWnd->hasFocus()) /*&& !parentWnd->GetDroppedState()*/)
	{
		pDC->SetTextColor(m_highlightTextColor);
		pDC->SetBkColor(m_highlightBkColor);
		return (HBRUSH)m_highlightBkBrush.GetSafeHandle();
	}
	pDC->SetTextColor(m_textColor);
	pDC->SetBkColor(m_bkColor);
	return (HBRUSH)m_bkBrush.GetSafeHandle();
}

void CComboBoxCustom_CEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	CEdit::OnLButtonDown(nFlags, point);
	this->SetSel(-1, 0);
	this->HideCaret();

	// didn't appear to work (nor calling the parent's 'OnLButtonDown' directly)
	//this->GetParent()->SendMessage(BM_CLICK, 0, 0);

	CComboBoxCustom* parentWnd = (CComboBoxCustom*)this->GetParent();
	parentWnd->SetFocus();
	parentWnd->ShowDropDown(TRUE);
}
void CComboBoxCustom_CEdit::OnLButtonUp(UINT nFlags, CPoint point)
{
	CEdit::OnLButtonUp(nFlags, point);
	this->SetSel(-1, 0);
	this->HideCaret();
}
BOOL CComboBoxCustom_CEdit::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// overriding to be this simple stops the cursor from changing on mousing over the inner text field
	return TRUE;
}
void CComboBoxCustom_CEdit::OnSetFocus(CWnd* pOldWnd)
{
	m_hasFocus = true;
	CEdit::OnSetFocus(pOldWnd);
	this->SetSel(-1, 0);
	this->HideCaret();
	// needed for reaching the dropdown by tab-focus-traversal to look consistent
	this->Invalidate();
	//CComboBoxCustom* parentWnd = (CComboBoxCustom*)this->GetParent();
	//parentWnd->SetFocus();
}
void CComboBoxCustom_CEdit::OnKillFocus(CWnd* pOldWnd)
{
	m_hasFocus = false;
	CEdit::OnKillFocus(pOldWnd);
	
	CComboBoxCustom* parentWnd = (CComboBoxCustom*)this->GetParent();
	if (!parentWnd->hasFocus())
	{
		// modern science cannot explain why this hack-around is needed to remove some leftover blue
		// on leaving the dropdown focus-wise
		CRect rect;
		this->GetWindowRect(&rect);
		ScreenToClient(&rect);
		this->RedrawWindow(rect);
	}
}
void CComboBoxCustom_CEdit::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// empty - no right-click menu
}

BEGIN_MESSAGE_MAP(CComboBoxCustom_CEdit, CEdit)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

// ------------------------------------------------

IMPLEMENT_DYNAMIC(CComboBoxCustom, CComboBox)

CComboBoxCustom::CComboBoxCustom() : m_hasFocus(FALSE)
{
}

CComboBoxCustom::~CComboBoxCustom()
{
	DestroyWindow();
}

// Can be treated as reliable init - called when using the new 'DDX_Control' way & the existing manual 'Create' call
void CComboBoxCustom::PreSubclassWindow()
{
	HWND innerTextFieldHandle = this->GetWindow(GW_CHILD)->GetSafeHwnd();
	m_currentSelectionTextField.SubclassWindow(innerTextFieldHandle);
}

void CComboBoxCustom::OnLButtonDown(UINT nFlags, CPoint point)
{
	CComboBox::OnLButtonDown(nFlags, point);
}
void CComboBoxCustom::OnLButtonUp(UINT nFlags, CPoint point)
{
	CComboBox::OnLButtonDown(nFlags, point);
}
void CComboBoxCustom::OnSetFocus(CWnd* pOldWnd)
{
	m_hasFocus = true;
	CComboBox::OnSetFocus(pOldWnd);
	//this->GetWindow(GW_CHILD)->Invalidate();
}
void CComboBoxCustom::OnKillFocus(CWnd* pOldWnd)
{
	m_hasFocus = false;
	CComboBox::OnKillFocus(pOldWnd);
	//this->GetWindow(GW_CHILD)->Invalidate();
}

void CComboBoxCustom::OnCbnDropdown()
{
	//this->GetWindow(GW_CHILD)->Invalidate();
	//this->GetWindow(GW_CHILD)->Invalidate();
	//this->SetFocus();
	//Default();
	m_currentSelectionTextField.onDropdownShown();
}

BEGIN_MESSAGE_MAP(CComboBoxCustom, CComboBox)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, &CComboBoxCustom::OnCbnDropdown)
	// Beware - it seems the 'CBN_SELENDOK' one will replace hooks from other files like playerlistdlg.cpp's
	//   ON_CBN_SELENDOK(... OnEditchangePlayerfaction)
	// The others didn't appear to have the effect I wanted anyway.
	// (these would be 'void'-return-type, no-param methods, to call 'this->setFocus();', not sure if 'Default();' is
	// supposed to be there)
	/*
	ON_CONTROL_REFLECT(CBN_SELCHANGE, &CComboBoxCustom::OnCbnSelChange)
	ON_CONTROL_REFLECT(CBN_SELENDOK, &CComboBoxCustom::OnCbnSelEndOk)
	ON_CONTROL_REFLECT(CBN_SELENDCANCEL, &CComboBoxCustom::OnCbnSelEndCancel)
	*/
END_MESSAGE_MAP()
