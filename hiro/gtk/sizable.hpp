#if defined(Hiro_Sizable)

namespace hiro {

struct pSizable : pObject {
  Declare(Sizable, Object)

  virtual auto minimumSize() const -> Size;
  virtual auto setCollapsible(bool collapsible) -> void;
  virtual auto setGeometry(Geometry geometry) -> void;
};

}

#endif
