// Copyright 2002-2007, 2013 by Jon Dart. All Rights Reserved.
//

#include "stdafx.h"
#include "arasan.h"
#include "apprdlg.h"
#include "display.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AppearanceDialog dialog

#define DIALOG_HELP_INDEX 14

AppearanceDialog::AppearanceDialog(CWnd* pParent, LPCSTR currentSet)
: CDialog(AppearanceDialog::IDD, pParent)
{
   //{{AFX_DATA_INIT(AppearanceDialog)
   m_pieceSet = _T("");
   m_boardSize = -1;
   m_Coordinates = FALSE;
   //}}AFX_DATA_INIT
   m_pieceSet = currentSet;
   m_boardSize = (int)guiOptions->getBoardSize();
   m_Coordinates = guiOptions->show_coordinates();
}


void AppearanceDialog::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(AppearanceDialog)
   DDX_Control(pDX, IDC_FONT_COMBO, m_ComboBox);
   DDX_CBString(pDX, IDC_FONT_COMBO, m_pieceSet);
   DDX_CBIndex(pDX, IDC_BOARD_SIZE_COMBO, m_boardSize);
   DDX_Check(pDX, IDC_SHOW_COORD_CHECK, m_Coordinates);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(AppearanceDialog, CDialog)
//{{AFX_MSG_MAP(AppearanceDialog)
ON_BN_CLICKED(ID_HELP, OnHelp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AppearanceDialog message handlers

BOOL AppearanceDialog::OnInitDialog()
{
   CDialog::OnInitDialog();

   // Populate the combo with the piece sets found under <exe dir>\pieces\.
   char buf[MAX_PATH];
   ::GetModuleFileName(NULL, buf, MAX_PATH);
   char *slash = strrchr(buf, '\\');
   if (slash)
      *(slash + 1) = '\0';
   else
      buf[0] = '\0';
   CString pattern = CString(buf) + "pieces\\*";
   WIN32_FIND_DATA fd;
   HANDLE h = ::FindFirstFile(pattern, &fd);
   if (h != INVALID_HANDLE_VALUE) {
      do {
         if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
             fd.cFileName[0] != '.')
            m_ComboBox.AddString(fd.cFileName);
      } while (::FindNextFile(h, &fd));
      ::FindClose(h);
   }
   if (m_ComboBox.SelectString(-1, m_pieceSet) == CB_ERR &&
       m_ComboBox.GetCount() > 0)
      m_ComboBox.SetCurSel(0);
   return TRUE;                                   // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


void AppearanceDialog::OnHelp()
{
   ::HtmlHelp(NULL, AfxGetApp()->m_pszHelpFilePath ,HH_HELP_CONTEXT, DIALOG_HELP_INDEX );
}
