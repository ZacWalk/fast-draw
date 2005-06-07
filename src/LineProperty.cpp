// LineProperty.cpp : implementation file
//

#include "stdafx.h"
#include "jtdraw.h"
#include "LineProperty.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLineProperty property page

IMPLEMENT_DYNCREATE(CLineProperty, CPropertyPage)

CLineProperty::CLineProperty() : CPropertyPage(CLineProperty::IDD)
{
	//{{AFX_DATA_INIT(CLineProperty)
	m_bNoFill = FALSE;
	m_penSize = 0;
	m_nArrowStart = 0;
	m_nArrowEnd = 0;
	m_round = FALSE;
	//}}AFX_DATA_INIT
}

CLineProperty::~CLineProperty()
{
}

void CLineProperty::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLineProperty)
	DDX_Control(pDX, IDC_LINEPICKER, m_linePicker);
	DDX_Control(pDX, IDC_ARROW_END, m_arrowEndPicker);
	DDX_Control(pDX, IDC_ARROW_START, m_arrowStartPicker);
	DDX_Control(pDX, IDC_FILL_COLOR, m_FillPicker);
	DDX_Control(pDX, IDC_LINE_COLOR, m_LinePicker);
	DDX_Check(pDX, IDC_FILL, m_bNoFill);
	DDX_Text(pDX, IDC_LINE_SIZE, m_penSize);
	DDX_Text(pDX, IDC_ARROW_START_SIZE, m_nArrowStart);
	DDX_Text(pDX, IDC_ARROW_END_SIZE, m_nArrowEnd);
	DDX_Check(pDX, IDC_ROUND_PEN, m_round);
	//}}AFX_DATA_MAP
	DDX_ArrowPicker(pDX, IDC_ARROW_END, m_arrowEnd);
	DDX_ArrowPicker(pDX, IDC_ARROW_START, m_arrowStart);
	DDX_ColourPicker(pDX, IDC_FILL_COLOR, m_FillColor);
	DDX_ColourPicker(pDX, IDC_LINE_COLOR, m_LineColor);
	DDX_LinePicker(pDX, IDC_LINEPICKER, m_lineStyle);
}


BEGIN_MESSAGE_MAP(CLineProperty, CPropertyPage)
	//{{AFX_MSG_MAP(CLineProperty)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLineProperty message handlers

BOOL CLineProperty::OnInitDialog() 
{
	
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	// subclass the control
	//VERIFY(m_lineStyle.SubclassDlgItem(IDC_LIST_LINE_STYLE, this));
    //when should i subclass?
	// add 8 colors to the listbox (primary + secondary color only)
/*	for (int red = 0; red <= 255; red += 255)
		for (int green = 0; green <= 255; green += 255)
			for (int blue = 0; blue <= 255; blue += 255)
				m_lineStyle.AddColorItem(RGB(red, green, blue));

	return TRUE;  // return TRUE unless you set the focus to a control 
	              // EXCEPTION: OCX Property Pages should return FALSE

*/ 

	CSpinButtonCtrl *p;
	p=(CSpinButtonCtrl *)GetDlgItem(IDC_SPIN1);
	p->SetRange(0,100);
	p=(CSpinButtonCtrl *)GetDlgItem(IDC_SPIN2);
	p->SetRange(0,100);
	p=(CSpinButtonCtrl *)GetDlgItem(IDC_SPIN3);
	p->SetRange(0,100);
	EnableArrowSetting(m_isArrow);
	return TRUE;

 }

void CLineProperty::EnableArrowSetting(bool set)
{
    CButton *pb;
	pb=(CButton *)GetDlgItem(IDC_ARROW_START);
	pb->EnableWindow(set);
	pb=(CButton *)GetDlgItem(IDC_ARROW_END);
	pb->EnableWindow(set);
    CEdit *pe;
	pe=(CEdit *)GetDlgItem(IDC_ARROW_START_SIZE);
	pe->EnableWindow(set);
	pe=(CEdit *)GetDlgItem(IDC_ARROW_END_SIZE);
	pe->EnableWindow(set);
}
