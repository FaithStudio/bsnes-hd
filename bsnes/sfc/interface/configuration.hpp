struct Configuration {
  auto read() -> string;
  auto read(string) -> string;
  auto write(string) -> bool;
  auto write(string, string) -> bool;

  struct System {
    struct CPU {
      uint version = 2;
    } cpu;
    struct PPU1 {
      uint version = 1;
      struct VRAM {
        uint size = 0x10000;
      } vram;
    } ppu1;
    struct PPU2 {
      uint version = 3;
    } ppu2;
  } system;

  struct Video {
    bool blurEmulation = true;
    bool colorEmulation = true;
  } video;

  struct Hacks {
    struct PPU {
      bool fast = true;
      bool noSpriteLimit = true;
      struct Mode7 {
        uint scale = 2;
        uint perspective = 1;
        uint widescreen = 72;
        uint wsbg1 = 16;
        uint wsbg2 = 16;
        uint wsbg3 = 16;
        uint wsbg4 = 16;
        uint wsobj = 0;
        uint igwin = 1;
        uint igwinx = 128;
        uint bgGrad = 4;
		uint windRad = 0;
        uint wsMode = 1;
        uint wsBgCol = 1;
        uint unintrMode = 1;
        uint unintrTop = 10;
        uint unintrBottom = 10;
        uint unintrLeft = 20;
        uint unintrRight = 20;
        uint wsMarker = 0;
        uint wsMarkerAlpha = 1;
        uint supersample = 1;
        uint mosaic = 1;
      } mode7;
    } ppu;
    struct DSP {
      bool fast = true;
      bool cubic = false;
    } dsp;
    struct Coprocessors {
      bool delayedSync = true;
      bool hle = true;
    } coprocessors;
  } hacks;

private:
  auto process(Markup::Node document, bool load) -> void;
};

extern Configuration configuration;
