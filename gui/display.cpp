// Copyright 1994-2008, 2023 by Jon Dart. All Rights Reserved.

#include "stdafx.h"
#include "chess.h"
#include "display.h"
#include "resource.h"
#include "arasan.h"

#define TEXT_OFFSET             30                /* from right edge */
#define COORD_OFFSET            5
#define TIME_Y                  COORD_OFFSET+5
#define MOVE_Y                  158
#define STATUS_Y                76
#define PLAYERS_Y               92
#define RESULT_Y                175
#define ECO_Y                   230

static CFont *textFont = NULL;
static CFont *coordFont = NULL;

static DWORD square_width,square_height;
DWORD Display::spacing;

void Display::clearRect(CDC *pDC, CRect &rect)
{
   CRgn rgn;
   rgn.CreateRectRgnIndirect(rect);
   CBrush brush;
   brush.CreateSolidBrush(messageAreaColor);
   pDC->FillRgn(&rgn,&brush);
}


Display::Display( CWnd *pWin, const CRect &initialSize )
: turned(FALSE)
{
   myWin = pWin;
   CDC *pDC = pWin->GetDC();
   GuiOptions::BoardSize boardSize = guiOptions->getBoardSize();
   int sz = calcSquareSize(boardSize);
   square_width = square_height = sz;
   if (!pieces.load(guiOptions->getPieceSet()))
      AfxMessageBox("failed to load SVG piece set");
   pieces.setSquareSize(sz);

   textFont = new CFont();
   NONCLIENTMETRICS ncm;
   TEXTMETRIC tm;

   SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0);
   textFont->CreatePointFont(90,ncm.lfMessageFont.lfFaceName);
   CFont *oldFont = (CFont*)pDC->SelectObject(textFont);
   pDC->GetOutputTextMetrics(&tm);

   spacing = tm.tmHeight + tm.tmExternalLeading;
   messageAreaColor = COLORREF(0xffffff);

   coordFont = new CFont();
   coordFont->CreatePointFont(80,ncm.lfMessageFont.lfFaceName);

   CRect timeRect;
   pDC->DrawText("00:00:00", &timeRect, DT_CALCRECT);
   timeWidth = timeRect.Width();

   updateLayout();
   pDC->SelectObject(oldFont);
   pWin->ReleaseDC(pDC);
   setSize(initialSize);

   activeTimeColor = COLORREF(0x1f1f1f);
   activeTimeBrush.CreateSolidBrush(activeTimeColor);
   messageAreaBrush.CreateSolidBrush(messageAreaColor);
}


void Display::getRect( DisplayRegion region,CRect &rect)
{
   switch(region) {
      case SideToMove:
         rect.left = textX;
         rect.top = 12;
         rect.right = width-5;
         rect.bottom = 52;
         break;
      case ECO:
         rect.left=textX;
         rect.top=ECO_Y;
         rect.right=width-5;
         rect.bottom=ECO_Y+5*spacing;
         break;
      case Players:
         rect.left=textX;
         rect.top=PLAYERS_Y;
         rect.right=width-5;
         rect.bottom=PLAYERS_Y+4*spacing;
         break;
      case Result:
         rect.left=textX;
         rect.top=RESULT_Y;
         rect.right=width-5;
         rect.bottom=RESULT_Y+spacing;
         break;
   }
}


void Display::setPieceFont(CDC *pDC, LPCSTR fontName, GuiOptions::BoardSize boardSize)
{
   // Inert: SVG pieces have replaced the chess font. Retained so the
   // Appearance font dialog still links; removed in Phase 3.
}


void Display::setPieceSet(CDC *pDC, LPCSTR setName, GuiOptions::BoardSize boardSize)
{
   int sz = calcSquareSize(boardSize);
   square_width = square_height = sz;
   if (!pieces.load(setName)) {
      ::MessageBox(NULL,"Cannot load piece set!","Error",MB_ICONEXCLAMATION);
      return;
   }
   pieces.setSquareSize(sz);
   updateLayout();
}


void Display::resize(CDC *pDC, GuiOptions::BoardSize boardSize)
{
   int sz = calcSquareSize(boardSize);
   square_width = square_height = sz;
   pieces.setSquareSize(sz);
   updateLayout();
}


void Display::updateLayout()
{
   board_right_edge = square_width*8 + 25;
   textX = 8*square_width + TEXT_OFFSET;
   coordX = 8*square_width + COORD_OFFSET;
}


Display::~Display()
{
   delete textFont;
   delete coordFont;
}


void Display::drawMessageArea(CDC *pDC, CRgn *pRgn)
{
   CBrush brush;
   brush.CreateSolidBrush(COLORREF(0x000000));
   pDC->FrameRect(&messageRect,&brush);
   showTimeArea(pDC);
}


void Display::drawBoard( CDC *pDC, const Board &board, const CRect &drawArea, BOOL turned)
{
   int vert = 0;
   for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
         Square sq;
         if (turned)
            sq = MakeSquare(8-j,8-i,Black);
         else
            sq = MakeSquare(j+1,i+1,Black);
         drawPiece(pDC,sq,board[sq]);
      }
      vert += square_height;
   }
   showSide(pDC,board.sideToMove());
}


void Display::drawPiece(CDC *pDC, Square sq, Piece p)
{
   CRect loc;
   getSquareRect(sq,turned,loc);
   // Draw the colored square background, then composite the (transparent)
   // SVG piece over it.
   COLORREF light = guiOptions->getLightSquareColor();
   COLORREF dark = guiOptions->getDarkSquareColor();
   pDC->FillSolidRect(loc, SquareColor(sq) == White ? light : dark);
   pieces.blit(pDC,loc.left,loc.top,p);
}


void Display::getSquareRect(Square sq,BOOL turned,CRect &loc)
{
   int f = File(sq);
   int r = Rank(sq,Black);
   if (turned) {
      r = 9-r;
      f = 9-f;
   }

   int vert = square_height*(r-1);
   int horiz = square_width*(f-1);
   CRect sqloc(horiz,vert,horiz+square_width,vert+square_height);
   loc = sqloc;
}


void Display::setSize(const CRect &size)
{
   this->size = size;
   width = size.Width();
   messageRect = CRect(board_right_edge,10,size.Width()-10,size.Height()-10);
   int hpad = messageRect.Width()/6;
   whiteTimeRegion.DeleteObject();
   blackTimeRegion.DeleteObject();
   CRect wRect(board_right_edge + hpad,20,
      board_right_edge+hpad+timeWidth,20+2*spacing);
   wRect.InflateRect(CSize(2,2));
   whiteTimeRegion.CreateRectRgn(wRect.right,wRect.top,wRect.left,wRect.bottom);
   CRect bRect(messageRect.right-hpad-timeWidth,20,
      messageRect.right-hpad,20+2*spacing);
   bRect.InflateRect(CSize(2,2));
   blackTimeRegion.CreateRectRgn(bRect.right,bRect.top,bRect.left,bRect.bottom);
}


void Display::showSide(CDC *pDC, ColorType side)
{
   sideToMove = side;
}


void Display::showTimeArea(CDC *pDC)
{
   if (!textFont)                                 // not initialized yet
      return;

   pDC->FrameRgn(&whiteTimeRegion,
      CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)),1,1);
   pDC->FrameRgn(&blackTimeRegion,
      CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)),1,1);
}


void Display::showTime(CDC *pDC, time_t time, ColorType side, int active)
{
   if (!textFont)                                 // not initialized yet
      return;

   // internal time is in 1/100 second increments
   unsigned hours = (unsigned)(time/360000U);
   unsigned minutes = (unsigned) ((time - (hours*360000U))/6000U);
   unsigned seconds = (unsigned)(time - (hours*360000U) - (minutes*6000U))/100;
   // convert time to ASCII:
   char time_str[50];
   wsprintf(time_str,"%02d:%02d:%02d",
      (int)hours,(int)minutes,(int)seconds);
   CRect box;
   CRgn *rgn;
   CString text;
   if (side == White) {
      whiteTimeRegion.GetRgnBox(&box);
      rgn = &whiteTimeRegion;
      text.LoadString(IDS_WHITE);
   }
   else {
      blackTimeRegion.GetRgnBox(&box);
      rgn = &blackTimeRegion;
      text.LoadString(IDS_BLACK);
   }
   box.DeflateRect(CSize(2,2));
   CFont *oldFont = (CFont*)pDC->SelectObject(textFont);
   COLORREF oldBkColor, oldTextColor;
   if (active) {
      pDC->FillRgn(rgn,&activeTimeBrush);
      oldBkColor = pDC->SetBkColor(activeTimeColor);
      oldTextColor = pDC->SetTextColor((COLORREF)0xffffff);
   }
   else {
      pDC->FillRgn(rgn,&messageAreaBrush);
      oldBkColor = pDC->SetBkColor(messageAreaColor);
      oldTextColor = pDC->SetTextColor((COLORREF)0x000000);
   }
   pDC->FrameRgn(rgn,
      CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)),1,1);
   CRect rcText;
   rcText.SetRect(box.left,box.top,box.right,box.top+spacing);
   pDC->DrawText(text,rcText,DT_CENTER);
   rcText.SetRect(box.left,box.top+spacing,box.right,box.top+2*spacing);
   pDC->DrawText(time_str,rcText,DT_CENTER);
   pDC->SelectObject(oldFont);
   pDC->SetTextColor(oldTextColor);
   pDC->SetBkColor(oldBkColor);
}


void Display::showECO(CDC *pDC, const char *eco, const char *openingName)
{
   if (!textFont)                                 // not initialized yet
      return;
   int right = messageRect.right-5;
   int left = messageRect.left+5;
   COLORREF oldColor = pDC->SetBkColor(messageAreaColor);
   pDC->SelectObject(textFont);
   CRect rcText(left,ECO_Y,right,ECO_Y+spacing);
   clearRect(pDC,rcText);
   rcText.DeflateRect(CSize(2,0));
   char msg[20];
   *msg = '\0';
   if (*eco)
      sprintf(msg,"ECO: %s",eco);
   pDC->DrawText(msg,strlen(msg),&rcText,DT_LEFT);
   rcText.SetRect(left,ECO_Y+spacing,right,ECO_Y+4*spacing);
   clearRect(pDC,rcText);
   rcText.DeflateRect(CSize(2,0));

   pDC->DrawText(openingName,strlen(openingName),
      &rcText, DT_LEFT | DT_WORDBREAK);
   pDC->SetBkColor(oldColor);
}


void Display::showMove(class CDC *pDC,const string &image,int moveCount,ColorType side)
{

   int right = messageRect.right-5;
   int left = messageRect.left+5;
   CFont *oldFont = (CFont*)pDC->SelectObject(textFont);
   CRect rcText(left,MOVE_Y,right,MOVE_Y+spacing);
   char text[80];
   // erase any previous text:
   clearRect(pDC,rcText);
   rcText.DeflateRect(CSize(2,0));
   if (image.length()==0) {
      pDC->SelectObject(oldFont);
      return;
   }
   int move_num = (moveCount-1)/2;
   if (side == White)
      wsprintf(text,"%d  %s",move_num+1,image.c_str());
   else
      wsprintf(text,"%d ... %s",move_num+1,image.c_str());
   COLORREF oldColor = pDC->SetBkColor(messageAreaColor);
   pDC->DrawText(text,strlen(text), &rcText, DT_LEFT | DT_WORDBREAK);
   pDC->SelectObject(oldFont);
   pDC->SetBkColor(oldColor);
}


void Display::showResult(CDC *pDC, const string &result)
{
   int right = messageRect.right-5;
   int left = messageRect.left+5;
   pDC->SelectObject(textFont);
   CRect rcText(left,RESULT_Y,right,(int)RESULT_Y+spacing);
   // erase any previous text:
   clearRect(pDC,rcText);
   rcText.DeflateRect(CSize(2,0));
   if (result.length()==0 || (result.compare("*")==0)) {
      return;
   }
   COLORREF oldColor = pDC->SetBkColor(messageAreaColor);
   pDC->DrawText(result.c_str(),result.length(), &rcText, DT_LEFT | DT_WORDBREAK);
   pDC->SetBkColor(oldColor);
}


void Display::showCoordinates(CDC *pDC)
{
   CFont *oldFont = pDC->SelectObject(coordFont);
   TEXTMETRIC tm;
   pDC->GetTextMetrics(&tm);
   int fontHeight = tm.tmHeight + tm.tmExternalLeading;
   CString letters;
   letters.LoadString(IDS_LETTER_COORDS);
   CString numbers;
   numbers.LoadString(IDS_NUMBER_COORDS);
   int i;
   for (i = 0; i < 8; i++) {
      CRect loc(i*square_width,square_height*8+6,(i+1)*square_width,square_height*8+6+fontHeight);
      int midY = i*square_height + square_height/2;
      CRect loc2(coordX,midY-fontHeight/2,coordX+12,midY+fontHeight/2);
      if (turned) {
         pDC->DrawText((LPCSTR)letters.Mid(7-i,1),1,&loc,DT_CENTER);
         pDC->DrawText((LPCSTR)numbers.Mid(i,1),1,&loc2,DT_CENTER | DT_VCENTER );
      }
      else {
         pDC->DrawText((LPCSTR)letters.Mid(i,1),1,&loc,DT_CENTER);
         pDC->DrawText((LPCSTR)numbers.Mid(7-i,1),1,&loc2,DT_CENTER | DT_VCENTER );
      }
   }
   pDC->SelectObject(oldFont);
}


Square Display::mouseLocation(const CPoint &p) const
{
   int x,y;
   x = (p.x/square_width)+1;
   if (turned)
      x = 9-x;
   y = (p.y/square_height)+1;
   if (x > 8 || y > 8)
      return InvalidSquare;
   else if (turned)
      return MakeSquare(x,y,White);
   else
      return MakeSquare(x,y,Black);
}


void Display::showHeader(CDC *pDC, const string &header)
{
   if (!textFont)                                 // not initialized yet
      return;
   pDC->SelectObject(textFont);
   int left=messageRect.left+5;
   int right=messageRect.right-5;
   CRect rcText(left,PLAYERS_Y,right,PLAYERS_Y+4*spacing);
   clearRect(pDC,rcText);
   rcText.DeflateRect(CSize(2,0));
   COLORREF oldColor = pDC->SetBkColor(messageAreaColor);

   pDC->DrawText(header.c_str(),header.length(),
      &rcText, DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
   pDC->SetBkColor(oldColor);
}


int Display::calcSquareSize(GuiOptions::BoardSize boardSize)
{
   switch (boardSize) {
      case GuiOptions::Small: return 32;
      case GuiOptions::Medium: return 48;
      case GuiOptions::Large: return 72;
      case GuiOptions::XLarge: return 96;
      case GuiOptions::XXLarge: return 144;
   }
   return 48;
}


void Display::calcWindowSize(GuiOptions::BoardSize boardSize, int &w, int &h)
{
   // should really base this on the board size (which is a function
   // of the font size) but the window sizing happens before the font
   // initialization, so sizes must be constants.
   // Try to compensate for higher DPI screens (120 DPI):
   HDC screen = GetDC(0);
   int xDPI = GetDeviceCaps(screen, LOGPIXELSX);
   int wextra = 0;
   if (xDPI > 96) wextra = 20;
   ReleaseDC(0, screen);
   switch (boardSize) {
      case GuiOptions::Small: w = 256+220+wextra; h = 200+256; break;
      case GuiOptions::Medium: w = 384+220+wextra; h = 175+384; break;
      case GuiOptions::Large: w = 576+220+wextra; h = 150+576; break;
      case GuiOptions::XLarge: w = 768+220+wextra; h = 125+768; break;
   }

}
