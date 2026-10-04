//MODDD - new file. CComboBox subclass that acts like a CBS_DROPDOWNLIST (can't edit the current choice's text) but the
// combobox is still capable of displaying unique text in case of an invalid choice.
//TODO - header stuff

#pragma once

// First, the helper text field class for the combo box's selection text field.
// Needed so it always appears white regardless of being set to read-only
class CComboBoxCustom_CEdit : public CEdit
{
  DECLARE_DYNAMIC(CComboBoxCustom_CEdit)
protected:

public:
	virtual ~CComboBoxCustom_CEdit() override;

	virtual void PreSubclassWindow() override;

protected:
  afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);

	DECLARE_MESSAGE_MAP();
};

// The custom combobox class
class CComboBoxCustom : public CComboBox
{
  DECLARE_DYNAMIC(CComboBoxCustom)
protected:
		CComboBoxCustom_CEdit m_currentSelectionTextField;

public:
	virtual ~CComboBoxCustom() override;

	virtual void PreSubclassWindow() override;

protected:

	DECLARE_MESSAGE_MAP();
};

