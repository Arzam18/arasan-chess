// Copyright 2026 by Jon Dart. All Rights Reserved.

#include "stdafx.h"
#include "svgpieces.h"
#include "lunasvg.h"
#include <memory>

// AlphaBlend lives in Msimg32.lib
#pragma comment(lib, "Msimg32.lib")

// 12 pieces: white P,N,B,R,Q,K then black P,N,B,R,Q,K
static const int NUM_PIECES = 12;

// PieceType is Empty=0,Pawn=1,Knight=2,Bishop=3,Rook=4,Queen=5,King=6.
// lichess uses these letters for the file name suffix:
static const char pieceLetter[] = { '?', 'P', 'N', 'B', 'R', 'Q', 'K' };

// Map a Piece to a 0..11 slot, or -1 if it is empty/invalid.
static int pieceIndex(Piece p)
{
   PieceType type = TypeOfPiece(p);
   if (type < Pawn || type > King)
      return -1;
   int base = (PieceColor(p) == White) ? 0 : 6;
   return base + (int)(type - Pawn);
}

struct SvgPieceSet::Impl
{
   std::unique_ptr<lunasvg::Document> docs[NUM_PIECES];
   CBitmap bitmaps[NUM_PIECES];                    // premultiplied DIB sections at current size
   CDC memDC;                                      // source DC for AlphaBlend

   ~Impl()
   {
      freeBitmaps();
   }

   void freeBitmaps()
   {
      for (int i = 0; i < NUM_PIECES; i++) {
         if (bitmaps[i].GetSafeHandle())
            bitmaps[i].DeleteObject();
      }
   }
};

// Directory holding the executable, with a trailing backslash.
static CString moduleDir()
{
   char buf[MAX_PATH];
   ::GetModuleFileName(NULL, buf, MAX_PATH);
   char *slash = strrchr(buf, '\\');
   if (slash)
      *(slash + 1) = '\0';
   else
      buf[0] = '\0';
   return CString(buf);
}

SvgPieceSet::SvgPieceSet()
: impl(new Impl), loaded(FALSE), size(0)
{
}

SvgPieceSet::~SvgPieceSet()
{
   delete impl;
}

BOOL SvgPieceSet::load(LPCSTR setName)
{
   loaded = FALSE;
   size = 0;
   impl->freeBitmaps();

   CString dir = moduleDir() + "pieces\\" + setName + "\\";
   std::unique_ptr<lunasvg::Document> loaded_docs[NUM_PIECES];
   for (int i = 0; i < NUM_PIECES; i++) {
      char color = (i < 6) ? 'w' : 'b';
      char letter = pieceLetter[(i % 6) + 1];      // P,N,B,R,Q,K
      CString path;
      path.Format("%s%c%c.svg", (LPCSTR)dir, color, letter);
      loaded_docs[i] = lunasvg::Document::loadFromFile((LPCSTR)path);
      if (!loaded_docs[i])
         return FALSE;                             // missing/unparseable: leave unloaded
   }
   for (int i = 0; i < NUM_PIECES; i++)
      impl->docs[i] = std::move(loaded_docs[i]);
   loaded = TRUE;
   return TRUE;
}

void SvgPieceSet::setSquareSize(int squareSize)
{
   if (!loaded || squareSize <= 0 || squareSize == size)
      return;

   if (!impl->memDC.GetSafeHdc())
      impl->memDC.CreateCompatibleDC(NULL);

   impl->freeBitmaps();

   BITMAPINFO bmi;
   ZeroMemory(&bmi, sizeof(bmi));
   bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biWidth = squareSize;
   bmi.bmiHeader.biHeight = -squareSize;          // top-down
   bmi.bmiHeader.biPlanes = 1;
   bmi.bmiHeader.biBitCount = 32;
   bmi.bmiHeader.biCompression = BI_RGB;

   for (int i = 0; i < NUM_PIECES; i++) {
      // lunasvg renders to a premultiplied ARGB32 buffer, which in memory is
      // B,G,R,A byte order -- exactly what a 32-bpp DIB and AlphaBlend with
      // AC_SRC_ALPHA expect, so we can copy the rows directly.
      lunasvg::Bitmap src = impl->docs[i]->renderToBitmap(squareSize, squareSize);
      void *bits = NULL;
      HBITMAP hbm = ::CreateDIBSection(impl->memDC.GetSafeHdc(), &bmi,
                                       DIB_RGB_COLORS, &bits, NULL, 0);
      if (hbm == NULL)
         continue;
      if (!src.isNull() && bits) {
         const unsigned char *srcRow = src.data();
         unsigned char *dstRow = (unsigned char *)bits;
         int dstStride = squareSize * 4;
         int srcStride = src.stride();
         int copyBytes = (dstStride < srcStride) ? dstStride : srcStride;
         for (int y = 0; y < squareSize; y++) {
            memcpy(dstRow, srcRow, copyBytes);
            srcRow += srcStride;
            dstRow += dstStride;
         }
      }
      impl->bitmaps[i].Attach(hbm);
   }
   size = squareSize;
}

void SvgPieceSet::blit(CDC *pDC, int x, int y, Piece p)
{
   if (!loaded || size <= 0)
      return;
   int idx = pieceIndex(p);
   if (idx < 0 || !impl->bitmaps[idx].GetSafeHandle())
      return;

   CBitmap *oldBmp = impl->memDC.SelectObject(&impl->bitmaps[idx]);
   BLENDFUNCTION bf;
   bf.BlendOp = AC_SRC_OVER;
   bf.BlendFlags = 0;
   bf.SourceConstantAlpha = 255;
   bf.AlphaFormat = AC_SRC_ALPHA;                  // source is premultiplied
   pDC->AlphaBlend(x, y, size, size, &impl->memDC, 0, 0, size, size, bf);
   impl->memDC.SelectObject(oldBmp);
}
