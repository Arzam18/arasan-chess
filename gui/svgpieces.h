// Copyright 2026 by Jon Dart. All Rights Reserved.
#ifndef _SVGPIECES_H
#define _SVGPIECES_H

#include <afxwin.h>
#include "chess.h"

// Renders chess pieces from a set of SVG files (lichess piece-set layout:
// wK.svg, wQ.svg, ... bP.svg) using the lunasvg library. Each piece is
// rasterized once per board size into a premultiplied 32-bpp DIB section
// and blitted onto the board with AlphaBlend, so it composites cleanly
// over the colored square underneath.
//
// This class knows nothing about board layout: callers supply the pixel
// position and size of each square.

class SvgPieceSet
{
   public:
      SvgPieceSet();
      ~SvgPieceSet();

      // Load the 12 piece SVGs for the named set. Files are looked up in
      // <module directory>\pieces\<setName>\. Returns FALSE (and leaves the
      // object unloaded) if any of the 12 files is missing or unparseable.
      BOOL load(LPCSTR setName);

      // (Re)rasterize all pieces to squareSize x squareSize pixels. A no-op
      // if the size is unchanged. Must be called before blit().
      void setSquareSize(int squareSize);

      // Composite piece p with its top-left corner at (x,y). The destination
      // square background should already be drawn. Empty/invalid pieces and
      // un-rasterized sets are ignored.
      void blit(CDC *pDC, int x, int y, Piece p);

      BOOL isLoaded() const { return loaded; }

      int squareSize() const { return size; }

   private:
      struct Impl;
      Impl *impl;
      BOOL loaded;
      int size;

      // not copyable (owns GDI + lunasvg resources)
      SvgPieceSet(const SvgPieceSet &);
      SvgPieceSet &operator=(const SvgPieceSet &);
};

#endif
