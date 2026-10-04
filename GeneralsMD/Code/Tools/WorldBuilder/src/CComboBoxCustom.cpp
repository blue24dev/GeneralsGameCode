//MODDD - new file. CComboBox subclass that acts like a CBS_DROPDOWNLIST (can't edit the current choice's text) but the
// combobox is still capable of displaying unique text in case of an invalid choice.
//TODO - header stuff

#include "StdAfx.h"

#include "CComboBoxCustom.h"

IMPLEMENT_DYNAMIC(CComboBoxCustom_CEdit, CEdit)

CComboBoxCustom_CEdit::~CComboBoxCustom_CEdit()
{
	DestroyWindow();
}

// basically 'init' for when the combobox's inner selection text field is subclassed
void CComboBoxCustom_CEdit::PreSubclassWindow()
{
	this->SetReadOnly(true);
}

// makes the background always white, not readonly-gray
HBRUSH CComboBoxCustom_CEdit::CtlColor(CDC* pDC, UINT nCtlColor)
{
	return (HBRUSH)GetStockObject(WHITE_BRUSH);
}

BEGIN_MESSAGE_MAP(CComboBoxCustom_CEdit, CEdit)
	// hooks to 'CtlColor'
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

// ------------------------------------------------

IMPLEMENT_DYNAMIC(CComboBoxCustom, CComboBox)

CComboBoxCustom::~CComboBoxCustom()
{
	DestroyWindow();
}

// Can be treated as reliable init - called when using the new 'DDX_Control' way & the existing manual 'Create' call
void CComboBoxCustom::PreSubclassWindow()
{
	HWND textFieldHandle = this->GetWindow(GW_CHILD)->GetSafeHwnd();
	m_currentSelectionTextField.SubclassWindow(textFieldHandle);
}

BEGIN_MESSAGE_MAP(CComboBoxCustom, CComboBox)
	// none
END_MESSAGE_MAP()
