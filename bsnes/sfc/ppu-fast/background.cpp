auto PPUfast::Line::renderBackground(PPUfast::IO::Background& self, uint source) -> void {
  if(!self.aboveEnable && !self.belowEnable) return;
  if(self.tileMode == TileMode::Mode7) return renderMode7(self, source);
  if(self.tileMode == TileMode::Inactive) return;

  uint wsConf = ppufast.wsbg(source);
  if(wsConf == 14) {
    return;
  }
  bool autoCrop = false;
  int ws = (int)ppufast.widescreen();
  if(ws > 0) {
    if(ppufast.wsOverride()) {
      ws = 0;
    } else if(wsConf == 12) {
      ws = -8;
    } else if(wsConf == 13) {
      ws = 0;
      autoCrop = true;
    } else if(wsConf == 15) {
      if (self.tileSize == 0 && self.hoffset == 0) ws = 0;
    } else if(wsConf == 16) {
      if (self.tileSize == 0 && self.hoffset == 0 && self.voffset == 0) ws = 0;
    } else if(wsConf == 0 || (wsConf != 1 && (((wsConf % 2) != 0) == (y < (((int)(wsConf / 2)) * 40))))) {
      ws = 0;
    }
	} 

  array<bool[256]> windowAbove;
  array<bool[256]> windowBelow;
  renderWindow(self.window, self.window.aboveEnable, windowAbove);
  renderWindow(self.window, self.window.belowEnable, windowBelow);

  bool hires = io.bgMode == 5 || io.bgMode == 6;
  bool offsetPerTileMode = io.bgMode == 2 || io.bgMode == 4 || io.bgMode == 6;
  bool directColorMode = io.col.directColor && source == Source::BG1 && (io.bgMode == 3 || io.bgMode == 4);
  int width = 256 << hires;

  uint tileHeight = 3 + self.tileSize;
  uint tileWidth = !hires ? tileHeight : 4;
  uint tileMask = 0x0fff >> self.tileMode;
  uint tiledataIndex = self.tiledataAddress >> 3 + self.tileMode;

  uint paletteBase = io.bgMode == 0 ? source << 5 : 0;
  uint paletteShift = 2 << self.tileMode;

  uint hscroll = self.hoffset;
  uint vscroll = self.voffset;
  uint hmask = (width << self.tileSize << self.screenSize.bit(0)) - 1;
  uint vmask = (width << self.tileSize << self.screenSize.bit(1)) - 1;

  uint y = this->y - (self.mosaicEnable ? this->y % (1 + io.mosaicSize) : 0);
  if(hires) {
    hscroll <<= 1;
    if(io.interlace) y = y << 1 | ppufast.field();
  }

  uint mosaicCounter = 1;
  uint mosaicPalette = 0;
  uint mosaicPriority = 0;
  uint mosaicColor = 0;

  bool lastCropped = false;
  int x = 0 - (hscroll & 7) - ws;
  while(x < (width + ws)) {
    uint hoffset = x + hscroll;
    uint voffset = y + vscroll;
    if(offsetPerTileMode) {
      uint validBit = 0x2000 << source;
      uint offsetX = x + (hscroll & 7);
      if(offsetX >= 8) {  //first column is exempt
        uint hlookup = getTile(io.bg3, (offsetX - 8) + (io.bg3.hoffset & ~7), io.bg3.voffset + 0);
        if(io.bgMode == 4) {
          if(hlookup & validBit) {
            if(!(hlookup & 0x8000)) {
              hoffset = offsetX + (hlookup & ~7);
            } else {
              voffset = y + hlookup;
            }
          }
        } else {
          uint vlookup = getTile(io.bg3, (offsetX - 8) + (io.bg3.hoffset & ~7), io.bg3.voffset + 8);
          if(hlookup & validBit) {
            hoffset = offsetX + (hlookup & ~7);
          }
          if(vlookup & validBit) {
            voffset = y + vlookup;
          }
        }
      }
    }
    hoffset &= hmask;
    voffset &= vmask;

    uint tileNumber = getTile(self, hoffset, voffset);
    uint mirrorY = tileNumber & 0x8000 ? 7 : 0;
    uint mirrorX = tileNumber & 0x4000 ? 7 : 0;
    uint tilePriority = self.priority[bool(tileNumber & 0x2000)];
    uint paletteNumber = tileNumber >> 10 & 7;
    uint paletteIndex = paletteBase + (paletteNumber << paletteShift) & 0xff;

    if(tileWidth  == 4 && (bool(hoffset & 8) ^ bool(mirrorX))) tileNumber +=  1;
    if(tileHeight == 4 && (bool(voffset & 8) ^ bool(mirrorY))) tileNumber += 16;
    tileNumber = (tileNumber & 0x03ff) + tiledataIndex & tileMask;

    auto tiledata = ppufast.tilecache[self.tileMode] + (tileNumber << 6);
    tiledata += (voffset & 7 ^ mirrorY) << 3;

    for(uint tileX = 0; tileX < 8; tileX++, x++) {
      if(x < -ws || x >= width + ws) continue;   //if(x & width) continue;  //x < 0 || x >= width
      if(!self.mosaicEnable || --mosaicCounter == 0) {
        mosaicCounter = 1 + io.mosaicSize;
        mosaicPalette = tiledata[tileX ^ mirrorX];
        mosaicPriority = tilePriority;
        if(directColorMode) {
          mosaicColor = directColor(paletteNumber, mosaicPalette);
        } else {
          mosaicColor = cgram[paletteIndex + mosaicPalette];
        }
      }
      if(!mosaicPalette) { lastCropped = false; continue; };
      if(autoCrop && (lastCropped || x < 8 || x > 255-8) && mosaicColor == 0) { lastCropped = true; continue; } else { lastCropped = false; }

      if(!hires) {
        if(self.aboveEnable && !windowAbove[ppufast.winXad(x, false)]) plotAbove(x, source, mosaicPriority, mosaicColor);
        if(self.belowEnable && !windowBelow[ppufast.winXad(x, true)]) plotBelow(x, source, mosaicPriority, mosaicColor);
      } else {
        int X = x / 2;
        if(!ppufast.hd()) {
          if(x & 1) {
            if(self.aboveEnable && !windowAbove[ppufast.winXad(X, false)]) plotAbove(X, source, mosaicPriority, mosaicColor);
          } else {
            if(self.belowEnable && !windowBelow[ppufast.winXad(X, true)]) plotBelow(X, source, mosaicPriority, mosaicColor);
          }
        } else {
          if(self.aboveEnable && !windowAbove[ppufast.winXad(X, false)]) plotHD(above, X, source, mosaicPriority, Emulator::video.processColor(mosaicColor, io.displayBrightness), true, x & 1);
          if(self.belowEnable && !windowBelow[ppufast.winXad(X, true)]) plotHD(below, X, source, mosaicPriority, Emulator::video.processColor(mosaicColor, io.displayBrightness), true, x & 1);
        }
      }
    }
  }
}

auto PPUfast::Line::getTile(PPUfast::IO::Background& self, uint hoffset, uint voffset) -> uint {
  bool hires = io.bgMode == 5 || io.bgMode == 6;
  uint tileHeight = 3 + self.tileSize;
  uint tileWidth = !hires ? tileHeight : 4;
  uint screenX = self.screenSize.bit(0) ? 32 << 5 : 0;
  uint screenY = self.screenSize.bit(1) ? 32 << 5 + self.screenSize.bit(0) : 0;
  uint tileX = hoffset >> tileWidth;
  uint tileY = voffset >> tileHeight;
  uint offset = (tileY & 0x1f) << 5 | (tileX & 0x1f);
  if(tileX & 0x20) offset += screenX;
  if(tileY & 0x20) offset += screenY;
  uint15 address = self.screenAddress + offset;
  return ppufast.vram[address];
}
