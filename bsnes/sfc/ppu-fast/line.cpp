uint PPUfast::Line::start = 0;
uint PPUfast::Line::count = 0;

auto PPUfast::Line::flush() -> void {
  ppufast.ind = 0;
  uint perspCorMode = ppufast.hdPerspective();
  if(perspCorMode > 0) {
    #define isLineMode7(l) (l.io.bg1.tileMode == TileMode::Mode7 \
        && !l.io.displayDisable && (l.io.bg1.aboveEnable || l.io.bg1.belowEnable))
    bool state = false;
    uint y;
    int offsPart = 8;
    if(perspCorMode == 2 || perspCorMode == 5) {
      offsPart = 6;
    } else if(perspCorMode == 3 || perspCorMode == 6) {
      offsPart = 4;
    }
    for(y = 0; y < Line::count; y++) {
      if(state != isLineMode7(ppufast.lines[Line::start + y])) {
        state = !state;
        if(state) {
          ppufast.starts[ppufast.ind] = ppufast.lines[Line::start + y].y;
        } else {
          ppufast.ends[ppufast.ind] = ppufast.lines[Line::start + y].y - 1;
          int offs = (ppufast.ends[ppufast.ind] - ppufast.starts[ppufast.ind]) / offsPart;
          ppufast.startsp[ppufast.ind] = ppufast.starts[ppufast.ind] + offs;
          ppufast.endsp[ppufast.ind] = ppufast.ends[ppufast.ind] - offs;
          ppufast.ind++;
        }
      }
    }
    #undef isLineMode7
    if(state) {
      ppufast.ends[ppufast.ind] = ppufast.lines[Line::start + y].y - 1;
      int offs = (ppufast.ends[ppufast.ind] - ppufast.starts[ppufast.ind]) / offsPart;
      ppufast.startsp[ppufast.ind] = ppufast.starts[ppufast.ind] + offs;
      ppufast.endsp[ppufast.ind] = ppufast.ends[ppufast.ind] - offs;
      ppufast.ind++;
    }

    if(perspCorMode < 4) {  
      for(int i = 0; i < ppufast.ind; i++) {
        int la = -1;
        int lb = -1;
        int lc = -1;
        int ld = -1;
        bool abd= false;
        bool bbd= false;
        bool cbd= false;
        bool dbd= false;
        bool ab= false;
        bool bb= false;
        bool cb= false;
        bool db= false;
        for(y = ppufast.startsp[i]; y <= ppufast.endsp[i]; y++) {
          int a = ((int)((int16)(ppufast.lines[y].io.mode7.a)));
          int b = ((int)((int16)(ppufast.lines[y].io.mode7.b)));
          int c = ((int)((int16)(ppufast.lines[y].io.mode7.c)));
          int d = ((int)((int16)(ppufast.lines[y].io.mode7.d)));
          if(la > 0 && a > 0 && a != la) {
            if(!abd) {
              abd = true;
              ab = a > la;
            } else if(ab != a > la) {
              ppufast.startsp[i] = -1;
              ppufast.endsp[i] = -1;
              break;
            }
          }
          if(lb > 0 && b > 0 && b != lb) {
            if(!bbd) {
              bbd = true;
              bb = b > lb;
            } else if(bb != b > lb) {
              ppufast.startsp[i] = -1;
              ppufast.endsp[i] = -1;
              break;
            }
          }
          if(lc > 0 && c > 0 && c != lc) {
            if(!cbd) {
              cbd = true;
              cb = c > lc;
            } else if(cb != c > lc) {
              ppufast.startsp[i] = -1;
              ppufast.endsp[i] = -1;
              break;
            }
          }
          if(ld > 0 && d > 0 && d != ld) {
            if(!dbd) {
              dbd = true;
              db = d > ld;
            } else if(db != d > ld) {
              ppufast.startsp[i] = -1;
              ppufast.endsp[i] = -1;
              break;
            }
          }
          la = a;
          lb = b;
          lc = c;
          ld = d;
        }
      }
    }
  } else if(ppufast.wsOverrideCandidate()) {
    #define isLineMode7(l) (l.io.bg1.tileMode == TileMode::Mode7 \
        && !l.io.displayDisable && (l.io.bg1.aboveEnable || l.io.bg1.belowEnable))
    for(uint y = 0; y < Line::count; y++) {
      if(isLineMode7(ppufast.lines[Line::start + y])) {
        ppufast.ind = 1;
        ppufast.starts[0] = -1;
        ppufast.ends[0] = -1;
        ppufast.startsp[0] = -1;
        ppufast.endsp[0] = -1;
        break;
      }
    }
    #undef isLineMode7
  }

  if(Line::count) {
    #pragma omp parallel for if(Line::count >= 8)
    for(uint y = 0; y < Line::count; y++) {
      ppufast.lines[Line::start + y].render();
    }
    Line::start = 0;
    Line::count = 0;
  }
}

auto PPUfast::Line::avgBgC(uint dist, uint offset) const -> uint32 {
  uint32 t = Emulator::video.processColor(io.col.fixedColor, io.displayBrightness);
  if(dist < 1) return t;
  uint32 a = (t >> 16) & 255;
  uint32 b = (t >>  8) & 255;
  uint32 c = (t >>  0) & 255;
  int scale = ppufast.hd() ? ppufast.hdScale() : 1;
  int hdY = y * scale + offset;
  int count = 1;
  for (int i = 1; i <= dist*scale; i++) {
    int uY = (hdY-i)/scale;
    int dY = (hdY+i)/scale;
    if(uY < 0 || dY >= 224) break; /////////////////////////////
    auto uL = ppufast.lines[uY];
    auto dL = ppufast.lines[dY];
    if( io.col.halve     != dL.io.col.halve     || io.col.halve     != uL.io.col.halve     ||
        io.col.mathMode  != dL.io.col.mathMode  || io.col.mathMode  != uL.io.col.mathMode  ||
        io.col.blendMode != dL.io.col.blendMode || io.col.blendMode != uL.io.col.blendMode ||
        io.col.enable[0] != dL.io.col.enable[0] || io.col.enable[0] != uL.io.col.enable[0] ||
        io.col.enable[1] != dL.io.col.enable[1] || io.col.enable[1] != uL.io.col.enable[1] ||
        io.col.enable[2] != dL.io.col.enable[2] || io.col.enable[2] != uL.io.col.enable[2] ||
        io.col.enable[3] != dL.io.col.enable[3] || io.col.enable[3] != uL.io.col.enable[3] ||
        io.col.enable[4] != dL.io.col.enable[4] || io.col.enable[4] != uL.io.col.enable[4] ||
        io.col.enable[5] != dL.io.col.enable[5] || io.col.enable[5] != uL.io.col.enable[5] ||
        io.col.enable[6] != dL.io.col.enable[6] || io.col.enable[6] != uL.io.col.enable[6] ||
        io.bg1.tileMode  != dL.io.bg1.tileMode  || io.bg1.tileMode  != uL.io.bg1.tileMode  ||
        io.bg2.tileMode  != dL.io.bg2.tileMode  || io.bg2.tileMode  != uL.io.bg2.tileMode  ||
        io.bg3.tileMode  != dL.io.bg3.tileMode  || io.bg3.tileMode  != uL.io.bg3.tileMode  ||
        io.bg4.tileMode  != dL.io.bg4.tileMode  || io.bg4.tileMode  != uL.io.bg4.tileMode) break; 
    t = Emulator::video.processColor(uL.io.col.fixedColor, io.displayBrightness);
    a += (t >> 16) & 255;
    b += (t >>  8) & 255;
    c += (t >>  0) & 255;
    t = Emulator::video.processColor(dL.io.col.fixedColor, io.displayBrightness);
    a += (t >> 16) & 255;
    b += (t >>  8) & 255;
    c += (t >>  0) & 255;
    count += 2;
  }
  a /= count;
  b /= count;
  c /= count;
  return (a << 16) + (b << 8) + (c << 0);
}

auto PPUfast::Line::render() -> void {
  auto hd = ppufast.hd();
  auto ss = ppufast.ss();
  auto scale = ppufast.hd() ? ppufast.hdScale() : 1;
  auto output = ppufast.output + (!hd
  ? (y * 1024 + (ppufast.interlace() && ppufast.field() ? 512 : 0))
  : (y * (256+2*ppufast.widescreen()) * scale * scale)
  );
  auto width = (!hd
  ? (!ppufast.hires() ? 256 : 512)
  : ((256+2*ppufast.widescreen()) * scale * scale));

  if(io.displayDisable) {
    memory::fill<uint32>(output, width);
    return;
  }

  bool hires = io.pseudoHires || io.bgMode == 5 || io.bgMode == 6;
  auto aboveColor = Emulator::video.processColor(cgram[0], io.displayBrightness);
  uint32 *bgFixedColors = new uint32[10];
  uint32 *belowColors = new uint32[10];
  for (int i = 0; i < scale; i++) {
    bgFixedColors[i] = avgBgC(ppufast.bgGrad(), i);
    belowColors[i]  = hires ? aboveColor : bgFixedColors[i];
  }
  
  uint xa =  (hd || ss) && ppufast.interlace() && ppufast.field() ? 256 * scale * scale / 2 : 0;
  uint xb = !(hd || ss) ? 256 : ppufast.interlace() && !ppufast.field() ? (256+2*ppufast.widescreen()) * scale * scale / 2 : (256+2*ppufast.widescreen()) * scale * scale;
  if (hd && ppufast.wsBgCol()) {
    for(uint x = xa; x < xb; x++) {
      int cx = (x % ((256+2*ppufast.widescreen()) * scale)) - (ppufast.widescreen() * scale);
      if (cx >= 0 && cx <= (256 * scale)) {
        above[x] = {Source::COL, 0, aboveColor};
        below[x] = {Source::COL, 0, belowColors[x / ((256+2*ppufast.widescreen()) * scale)]};
      } else {
        above[x] = {Source::COL, 0, 0};
        below[x] = {Source::COL, 0, 0};
      }
    }
  } else {
    for(uint x = xa; x < xb; x++) {
      above[x] = {Source::COL, 0, aboveColor};
      below[x] = {Source::COL, 0, belowColors[x / ((256+2*ppufast.widescreen()) * scale)]};
    }
  }

  renderBackground(io.bg1, Source::BG1);
  if(!io.extbg) renderBackground(io.bg2, Source::BG2);
  renderBackground(io.bg3, Source::BG3);
  renderBackground(io.bg4, Source::BG4);
  renderObject(io.obj);
  if(io.extbg) renderBackground(io.bg2, Source::BG2);

  //TODO: move to own method
  uint windRad = ppufast.windRad();
  for (int offset = 0; offset < scale; offset++) {
    uint oneLeft  = io.window.oneLeft;
    uint oneRight = io.window.oneRight;
    uint twoLeft  = io.window.twoLeft;
    uint twoRight = io.window.twoRight;

    int hdY = y * scale + offset;
    int count = 1;
    for (int i = 1; i <= windRad*scale; i++) {
      int uY = (hdY-i)/scale;
      int dY = (hdY+i)/scale;
      if(uY <= 0 || dY >= 224) break;
      auto uL = ppufast.lines[uY];
      auto dL = ppufast.lines[dY];

      if( io.col.halve     != dL.io.col.halve     || io.col.halve     != uL.io.col.halve     ||
          io.col.mathMode  != dL.io.col.mathMode  || io.col.mathMode  != uL.io.col.mathMode  ||
          io.col.blendMode != dL.io.col.blendMode || io.col.blendMode != uL.io.col.blendMode ||
          (io.window.oneLeft >= io.window.oneRight) != (dL.io.window.oneLeft >= dL.io.window.oneRight) ||
          (io.window.oneLeft >= io.window.oneRight) != (uL.io.window.oneLeft >= uL.io.window.oneRight) ||
          (io.window.twoLeft >= io.window.twoRight) != (dL.io.window.twoLeft >= dL.io.window.twoRight) ||
          (io.window.twoLeft >= io.window.twoRight) != (uL.io.window.twoLeft >= uL.io.window.twoRight) ||
          io.col.window.oneEnable != dL.io.col.window.oneEnable ||
          io.col.window.oneEnable != uL.io.col.window.oneEnable ||
          io.col.window.oneInvert != dL.io.col.window.oneInvert ||
          io.col.window.oneInvert != uL.io.col.window.oneInvert ||
          io.col.window.twoEnable != dL.io.col.window.twoEnable ||
          io.col.window.twoEnable != uL.io.col.window.twoEnable ||
          io.col.window.twoInvert != dL.io.col.window.twoInvert ||
          io.col.window.twoInvert != uL.io.col.window.twoInvert ||
          io.col.window.mask != dL.io.col.window.mask ||
          io.col.window.mask != uL.io.col.window.mask ||
          io.col.window.aboveMask != dL.io.col.window.aboveMask ||
          io.col.window.aboveMask != uL.io.col.window.aboveMask ||
          io.col.window.belowMask != dL.io.col.window.belowMask ||
          io.col.window.belowMask != uL.io.col.window.belowMask
          ) break; 

      oneLeft  += dL.io.window.oneLeft  + uL.io.window.oneLeft;
      oneRight += dL.io.window.oneRight + uL.io.window.oneRight;
      twoLeft  += dL.io.window.twoLeft  + uL.io.window.twoLeft;
      twoRight += dL.io.window.twoRight + uL.io.window.twoRight;

      count += 2;
    }
    oneLeft  = oneLeft  * scale / count;
    oneRight = oneRight * scale / count + scale - 1;
    twoLeft  = twoLeft  * scale / count;
    twoRight = twoRight * scale / count + scale - 1;

    renderWindow(io.col.window, io.col.window.aboveMask, windowAbove,
                oneLeft, oneRight, twoLeft, twoRight, scale, 256*scale*offset);
    renderWindow(io.col.window, io.col.window.belowMask, windowBelow,
                oneLeft, oneRight, twoLeft, twoRight, scale, 256*scale*offset);
  }

  uint wsm = (ppufast.widescreen() == 0 || ppufast.wsOverride()) ? 0 : ppufast.wsMarker();
  uint wsma = ppufast.wsMarkerAlpha();

  if(hd) {
    int x = 0;
    int xWindow = 0;
    for(uint ySub : range(scale)) {
      for(uint i : range(ppufast.widescreen() * scale)) {
        *output++ = pixel(xWindow, above[x], below[x], wsm, wsma, bgFixedColors[ySub]);
        x++;
      }
      for(uint i : range(256 * scale)) {
        *output++ = pixel(xWindow, above[x], below[x], wsm, wsma, bgFixedColors[ySub]);
        x++;
        xWindow++;
      }
      xWindow--;
      for(uint i : range(ppufast.widescreen() * scale)) {
        *output++ = pixel(xWindow, above[x], below[x], wsm, wsma, bgFixedColors[ySub]);
        x++;
      }
      xWindow++;
    }
  } else if(width == 256) for(uint x : range(256)) {
    *output++ = pixel(x, above[x], below[x], wsm, wsma, bgFixedColors[0]);
  } else if(!hires) for(uint x : range(256)) {
    auto color = pixel(x, above[x], below[x], wsm, wsma, bgFixedColors[0]);
    *output++ = color;
    *output++ = color;
  } else for(uint x : range(256)) {
    *output++ = pixel(x, below[x], above[x], wsm, wsma, bgFixedColors[0]);
    *output++ = pixel(x, above[x], below[x], wsm, wsma, bgFixedColors[0]);
  }
}

auto PPUfast::Line::pixel(uint x, Pixel above, Pixel below, uint wsm, uint wsma, uint32 bgFixedColor) const -> uint32 {
  uint32 r = 0;
  if(!windowAbove[x]) above.color = 0x0000;
  if(!windowBelow[x]) r = above.color;
  else if(!io.col.enable[above.source]) r = above.color;
  else if(!io.col.blendMode) r = blend(above.color, bgFixedColor, io.col.halve && windowAbove[x]);
  else r = blend(above.color, below.color, io.col.halve && windowAbove[x] && below.source != Source::COL);
  if(wsm > 0) {
    x = (x / ppufast.hdScale()) % 256;
    if(wsm == 1 && (x == 1 || x == 254)
        || wsm == 2 && (x == 0 || x == 255)) {
      int b = wsm == 2 ? 0 : ((y / 4) % 2 == 0) ? 0 : 255;
      r = ((((((r >> 16) & 255) * wsma) + b) / (wsma + 1)) << 16)
        + ((((((r >>  8) & 255) * wsma) + b) / (wsma + 1)) <<  8)
        + ((((((r >>  0) & 255) * wsma) + b) / (wsma + 1)) <<  0);
    }
  }
  return r;
}

auto PPUfast::Line::blend(uint x, uint y, bool halve) const -> uint32 {
  if(!io.col.mathMode) {  //add
    if(!halve) {
      uint sum = x + y;
      uint carry = (sum - ((x ^ y) & 0x00010101)) & 0x01010100;
      return (sum - carry) | (carry - (carry >> 8));
    } else {
      return (x + y - ((x ^ y) & 0x00010101)) >> 1;
    }
  } else {  //sub
    uint diff = x - y + 0x01010100;
    uint borrow = (diff - ((x ^ y) & 0x01010100)) & 0x01010100;
    if(!halve) {
      return   (diff - borrow) & (borrow - (borrow >> 8));
    } else {
      return (((diff - borrow) & (borrow - (borrow >> 8))) & 0x00fefefe) >> 1;
    }
  }
}

auto PPUfast::Line::directColor(uint paletteIndex, uint paletteColor) const -> uint15 {
  //paletteIndex = bgr
  //paletteColor = BBGGGRRR
  //output       = 0 BBb00 GGGg0 RRRr0
  return (paletteColor << 2 & 0x001c) + (paletteIndex <<  1 & 0x0002)   //R
       + (paletteColor << 4 & 0x0380) + (paletteIndex <<  5 & 0x0040)   //G
       + (paletteColor << 7 & 0x6000) + (paletteIndex << 10 & 0x1000);  //B
}

auto PPUfast::Line::plotAbove(int x, uint source, uint priority, uint col) -> void {
  uint32 color = Emulator::video.processColor(col, io.displayBrightness);
  if(ppufast.hd() || ppufast.ss()) return plotHD(above, x, source, priority, color, false, false);
  if(priority > above[x].priority) above[x] = {source, priority, color};
}

auto PPUfast::Line::plotBelow(int x, uint source, uint priority, uint col) -> void {
  uint32 color = Emulator::video.processColor(col, io.displayBrightness);
  if(ppufast.hd() || ppufast.ss()) return plotHD(below, x, source, priority, color, false, false);
  if(priority > below[x].priority) below[x] = {source, priority, color};
}

//todo: name these variables more clearly ...
auto PPUfast::Line::plotHD(Pixel* pixel, int x, uint source, uint priority, uint32 color, bool hires, bool subpixel) -> void {
  int scale = ppufast.hdScale();
  int wss = ppufast.widescreen() * scale;
  int xss = hires && subpixel ? scale / 2 : 0;
  int ys = ppufast.interlace() && ppufast.field() ? scale / 2 : 0;
  if(priority > pixel[x * scale + xss + ys * 256 * scale + wss].priority) {
    Pixel p = {source, priority, color};
    int xsm = hires && !subpixel ? scale / 2 : scale;
    int ysm = ppufast.interlace() && !ppufast.field() ? scale / 2 : scale;
    for(int xs = xss; xs < xsm; xs++) {
      pixel[x * scale + xs + ys * 256 * scale + wss] = p;
    }
    int size = sizeof(Pixel) * (xsm - xss);
    Pixel* source = &pixel[x * scale + xss + ys * 256 * scale + wss];
    for(int yst = ys + 1; yst < ysm; yst++) {
      memcpy(&pixel[x * scale + xss + yst * (256+2*ppufast.widescreen()) * scale + wss], source, size);
    }
  }
}
