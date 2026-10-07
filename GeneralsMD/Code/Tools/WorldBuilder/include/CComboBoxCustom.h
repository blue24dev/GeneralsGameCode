//MODDD - new file. CComboBox subclass that acts like a CBS_DROPDOWNLIST (can't edit the current choice's text) but the
// combobox is still capable of displaying unique text in case of an invalid choice.
//TODO - header stuff

#pragma once

class CComboBoxCustom;

// First, the helper text field class for the combo box's selection text field.
// Needed so it always appears white regardless of being set to read-only
class CComboBoxCustom_CEdit : public CEdit
{
	DECLARE_DYNAMIC(CComboBoxCustom_CEdit)
protected:
	COLORREF m_textColor;
	COLORREF m_bkColor;
	CBrush m_bkBrush;
	COLORREF m_highlightTextColor;
	COLORREF m_highlightBkColor;
	CBrush m_highlightBkBrush;
	BOOL m_hasFocus;

public:
	CComboBoxCustom_CEdit();
	virtual ~CComboBoxCustom_CEdit() override;

	virtual void PreSubclassWindow() override;
	void onDropdownShown();

protected:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pOldWnd);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

	DECLARE_MESSAGE_MAP();
};

// The custom combobox class
class CComboBoxCustom : public CComboBox
{
	DECLARE_DYNAMIC(CComboBoxCustom)
protected:
	CComboBoxCustom_CEdit m_currentSelectionTextField;
	BOOL m_hasFocus;

public:
	CComboBoxCustom();
	virtual ~CComboBoxCustom() override;

	virtual void PreSubclassWindow() override;

	BOOL hasFocus() {return m_hasFocus;}

protected:
public:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pOldWnd);
	afx_msg void OnCbnDropdown();
protected:

	DECLARE_MESSAGE_MAP();
};

