// TextProperty.cpp : implementation file
//

#include "stdafx.h"
#include "jtdraw.h"
#include "TextProperty.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// reserve lobyte for charset
#define PRINTER_FONT 0x0100
#define TT_FONT 0x0200
#define DEVICE_FONT 0x0400

#define BMW 16
#define BMH 15

LOGFONT NEAR CTextProperty::m_lfDefFont;
LOGFONT NEAR CTextProperty::m_lfDefFontOld;
LOGFONT NEAR CTextProperty::m_lfDefPrintFont;
LOGFONT NEAR CTextProperty::m_lfDefPrintFontOld;


/////////////////////////////////////////////////////////////////////////////
// CTextProperty property page

IMPLEMENT_DYNCREATE(CTextProperty, CPropertyPage)

CTextProperty::CTextProperty() : CPropertyPage(CTextProperty::IDD)
{
	//{{AFX_DATA_INIT(CTextProperty)
	m_text = _T("");
	m_borderType = -1;
	m_allignment = -1;
	//}}AFX_DATA_INIT
    //load all font name to the list
	//fontCombo.SubclassDlgItem(IDC_FONT,this);
	m_bFont=false;
}

CTextProperty::~CTextProperty()
{
}

void CTextProperty::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextProperty)
	DDX_Text(pDX, IDC_TEXT_INPUT, m_text);
	DDX_CBIndex(pDX, IDC_BORDER, m_borderType);
	DDX_CBIndex(pDX, IDC_TEXT_ALLIGNMENT, m_allignment);
	//}}AFX_DATA_MAP
//	DDX_ColourPicker(pDX, IDC_FONT_COLOR, m_textColor);
}


BEGIN_MESSAGE_MAP(CTextProperty, CPropertyPage)
	//{{AFX_MSG_MAP(CTextProperty)
	ON_BN_CLICKED(IDC_FONT, OnFont)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextProperty message handlers





void CTextProperty::OnFont() 
{
   LOGFONT lf;
	int x=m_font.GetLogFont(&lf);
   //::GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);

	CFontDialog dlg(&lf, CF_EFFECTS|CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT);
    
	
	if (dlg.DoModal() == IDOK)
	{
		// switch to new font.
		m_font.DeleteObject();
        m_bFont=false; 
		if (m_font.CreateFontIndirect(&lf))
		{
			CWaitCursor wait;
			//SetFont(&m_font);
			m_lfDefFont = lf;
			m_bFont=true; //create font successfully

	/*		if (GetPrinterFont() == NULL)
			{
				// notify container that content has changed
				GetDocument()->UpdateAllItems(NULL);
			}*/
			m_color=dlg.GetColor();
		}
	}
       
} 

BOOL CTextProperty::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here

	


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
