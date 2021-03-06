auto PPUfast::Line::renderWindow(PPUfast::IO::WindowLayer& self, bool enable, array<bool[256]>& output) -> void {
  if(!enable || (!self.oneEnable && !self.twoEnable)) {
    output.fill(0);
    return;
  }

  if(self.oneEnable && !self.twoEnable) {
    bool set = 1 ^ self.oneInvert, clear = !set;
    for(uint x : range(256)) {
      output[x] = x >= io.window.oneLeft && x <= io.window.oneRight ? set : clear;
    }
    return;
  }

  if(self.twoEnable && !self.oneEnable) {
    bool set = 1 ^ self.twoInvert, clear = !set;
    for(uint x : range(256)) {
      output[x] = x >= io.window.twoLeft && x <= io.window.twoRight ? set : clear;
    }
    return;
  }

  for(uint x : range(256)) {
    bool oneMask = (x >= io.window.oneLeft && x <= io.window.oneRight) ^ self.oneInvert;
    bool twoMask = (x >= io.window.twoLeft && x <= io.window.twoRight) ^ self.twoInvert;
    switch(self.mask) {
    case 0: output[x] = (oneMask | twoMask) == 1; break;
    case 1: output[x] = (oneMask & twoMask) == 1; break;
    case 2: output[x] = (oneMask ^ twoMask) == 1; break;
    case 3: output[x] = (oneMask ^ twoMask) == 0; break;
    }
  }
}

auto PPUfast::Line::renderWindow(PPUfast::IO::WindowColor& self, uint mask, bool *output,
                                 uint oneLeft, uint oneRight, uint twoLeft, uint twoRight, uint scale, uint offset) -> void {
  bool set, clear;
  switch(mask) {
  case 0: /*output.fill(1)*/ for(uint x : range(256*scale*scale)) *output++ = 1; return;     //always
  case 1: set = 1, clear = 0; break;  //inside
  case 2: set = 0, clear = 1; break;  //outside
  case 3: /*output.fill(0)*/ for(uint x : range(256*scale*scale)) *output++ = 0; return;     //never
  }

  if(!self.oneEnable && !self.twoEnable) {
    for(uint x : range(256*scale*scale)) *output++ = clear;
    return;
  }

  if(self.oneEnable && !self.twoEnable) {
    if(self.oneInvert) set ^= 1, clear ^= 1;
    for(uint x : range(256*scale)) {
      output[x+offset] = x >= oneLeft && x <= oneRight ? set : clear;
    }
    return;
  }

  if(self.twoEnable && !self.oneEnable) {
    if(self.twoInvert) set ^= 1, clear ^= 1;
    for(uint x : range(256*scale)) {
      output[x+offset] = x >= twoLeft && x <= twoRight ? set : clear;
    }
    return;
  }

  for(uint x : range(256*scale)) {
    bool oneMask = (x >= oneLeft && x <= oneRight) ^ self.oneInvert;
    bool twoMask = (x >= twoLeft && x <= twoRight) ^ self.twoInvert;
    switch(self.mask) {
    case 0: output[x+offset] = (oneMask | twoMask) == 1 ? set : clear; break;
    case 1: output[x+offset] = (oneMask & twoMask) == 1 ? set : clear; break;
    case 2: output[x+offset] = (oneMask ^ twoMask) == 1 ? set : clear; break;
    case 3: output[x+offset] = (oneMask ^ twoMask) == 0 ? set : clear; break;
    }
  }
}
