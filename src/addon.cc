#include <napi.h>

#include <optional>
#include <stdexcept>

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#else
#error "Unsupported platform"
#endif

namespace {

struct Point {
  double x;
  double y;
};

enum class MouseButton {
  Left,
  Right,
};

#if defined(__APPLE__)

Point GetCurrentMouseLocation() {
  CGEventRef event = CGEventCreate(nullptr);
  if (!event) {
    return Point{0, 0};
  }
  CGPoint location = CGEventGetLocation(event);
  CFRelease(event);
  return Point{location.x, location.y};
}

void PostMouseClick(MouseButton button, const std::optional<Point>& maybeLocation) {
  Point location = maybeLocation.has_value() ? *maybeLocation : GetCurrentMouseLocation();
  CGPoint cgLocation = CGPointMake(location.x, location.y);

  CGMouseButton cgButton = (button == MouseButton::Left) ? kCGMouseButtonLeft : kCGMouseButtonRight;
  CGEventType downType = (button == MouseButton::Left) ? kCGEventLeftMouseDown : kCGEventRightMouseDown;
  CGEventType upType = (button == MouseButton::Left) ? kCGEventLeftMouseUp : kCGEventRightMouseUp;

  CGEventRef down = CGEventCreateMouseEvent(nullptr, downType, cgLocation, cgButton);
  if (!down) {
    throw std::runtime_error("Failed to create mouse down event");
  }
  CGEventRef up = CGEventCreateMouseEvent(nullptr, upType, cgLocation, cgButton);
  if (!up) {
    CFRelease(down);
    throw std::runtime_error("Failed to create mouse up event");
  }

  CGEventPost(kCGHIDEventTap, down);
  CGEventPost(kCGHIDEventTap, up);

  CFRelease(down);
  CFRelease(up);
}

void MoveMouseTo(Point location) {
  CGPoint cgLocation = CGPointMake(location.x, location.y);
  CGWarpMouseCursorPosition(cgLocation);
  CGAssociateMouseAndMouseCursorPosition(true);

  CGEventRef move = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, cgLocation, kCGMouseButtonLeft);
  if (!move) {
    throw std::runtime_error("Failed to create mouse move event");
  }
  CGEventPost(kCGHIDEventTap, move);
  CFRelease(move);
}

#elif defined(_WIN32)

Point GetCurrentMouseLocation() {
  POINT pt;
  if (!GetCursorPos(&pt)) {
    throw std::runtime_error("GetCursorPos failed");
  }
  return Point{static_cast<double>(pt.x), static_cast<double>(pt.y)};
}

void MoveMouseTo(Point location) {
  int x = static_cast<int>(location.x);
  int y = static_cast<int>(location.y);
  if (!SetCursorPos(x, y)) {
    throw std::runtime_error("SetCursorPos failed");
  }
}

void PostMouseClick(MouseButton button, const std::optional<Point>& maybeLocation) {
  if (maybeLocation.has_value()) {
    MoveMouseTo(*maybeLocation);
  }

  DWORD downFlag = (button == MouseButton::Left) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
  DWORD upFlag = (button == MouseButton::Left) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;

  INPUT inputs[2];
  ZeroMemory(inputs, sizeof(inputs));

  inputs[0].type = INPUT_MOUSE;
  inputs[0].mi.dwFlags = downFlag;

  inputs[1].type = INPUT_MOUSE;
  inputs[1].mi.dwFlags = upFlag;

  UINT sent = SendInput(2, inputs, sizeof(INPUT));
  if (sent != 2) {
    throw std::runtime_error("SendInput failed");
  }
}

#elif defined(__linux__)

class XDisplay {
 public:
  XDisplay() : display_(XOpenDisplay(nullptr)) {
    if (!display_) {
      throw std::runtime_error("X11 display not available (is DISPLAY set? are you on Wayland?)");
    }
  }
  ~XDisplay() {
    if (display_) {
      XCloseDisplay(display_);
    }
  }
  XDisplay(const XDisplay&) = delete;
  XDisplay& operator=(const XDisplay&) = delete;

  Display* get() const { return display_; }

 private:
  Display* display_;
};

Point GetCurrentMouseLocation() {
  XDisplay d;
  Display* display = d.get();
  Window root = DefaultRootWindow(display);

  Window retRoot, retChild;
  int rootX, rootY;
  int winX, winY;
  unsigned int mask;

  Bool ok = XQueryPointer(display, root, &retRoot, &retChild, &rootX, &rootY, &winX, &winY, &mask);
  if (!ok) {
    throw std::runtime_error("XQueryPointer failed");
  }
  return Point{static_cast<double>(rootX), static_cast<double>(rootY)};
}

void MoveMouseTo(Point location) {
  XDisplay d;
  Display* display = d.get();
  Window root = DefaultRootWindow(display);
  XWarpPointer(display, None, root, 0, 0, 0, 0, static_cast<int>(location.x), static_cast<int>(location.y));
  XFlush(display);
}

void PostMouseClick(MouseButton button, const std::optional<Point>& maybeLocation) {
  XDisplay d;
  Display* display = d.get();
  Window root = DefaultRootWindow(display);

  if (maybeLocation.has_value()) {
    XWarpPointer(display, None, root, 0, 0, 0, 0, static_cast<int>(maybeLocation->x), static_cast<int>(maybeLocation->y));
    XFlush(display);
  }

  int buttonNumber = (button == MouseButton::Left) ? 1 : 3;
  if (!XTestFakeButtonEvent(display, buttonNumber, True, CurrentTime)) {
    throw std::runtime_error("XTestFakeButtonEvent(down) failed");
  }
  if (!XTestFakeButtonEvent(display, buttonNumber, False, CurrentTime)) {
    throw std::runtime_error("XTestFakeButtonEvent(up) failed");
  }
  XFlush(display);
}

#endif

std::optional<Point> ParseOptionalPoint(const Napi::CallbackInfo& info) {
  if (info.Length() == 0) {
    return std::nullopt;
  }

  if (info.Length() == 1 && info[0].IsObject()) {
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.Has("x") || !obj.Has("y")) {
      throw Napi::TypeError::New(info.Env(), "Point object must have x and y");
    }
    double x = obj.Get("x").ToNumber().DoubleValue();
    double y = obj.Get("y").ToNumber().DoubleValue();
    return Point{x, y};
  }

  if (info.Length() >= 2 && info[0].IsNumber() && info[1].IsNumber()) {
    double x = info[0].ToNumber().DoubleValue();
    double y = info[1].ToNumber().DoubleValue();
    return Point{x, y};
  }

  throw Napi::TypeError::New(info.Env(), "Usage: fn(), fn(x, y) or fn({x, y})");
}

Point ParseRequiredPoint(const Napi::CallbackInfo& info) {
  if (info.Length() == 1 && info[0].IsObject()) {
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.Has("x") || !obj.Has("y")) {
      throw Napi::TypeError::New(info.Env(), "Point object must have x and y");
    }
    double x = obj.Get("x").ToNumber().DoubleValue();
    double y = obj.Get("y").ToNumber().DoubleValue();
    return Point{x, y};
  }

  if (info.Length() >= 2 && info[0].IsNumber() && info[1].IsNumber()) {
    double x = info[0].ToNumber().DoubleValue();
    double y = info[1].ToNumber().DoubleValue();
    return Point{x, y};
  }

  throw Napi::TypeError::New(info.Env(), "Usage: fn(x, y) or fn({x, y})");
}

Napi::Value LeftClick(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  try {
    auto pt = ParseOptionalPoint(info);
    PostMouseClick(MouseButton::Left, pt);
    return env.Undefined();
  } catch (const Napi::Error& e) {
    e.ThrowAsJavaScriptException();
    return env.Undefined();
  } catch (const std::exception& e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Value RightClick(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  try {
    auto pt = ParseOptionalPoint(info);
    PostMouseClick(MouseButton::Right, pt);
    return env.Undefined();
  } catch (const Napi::Error& e) {
    e.ThrowAsJavaScriptException();
    return env.Undefined();
  } catch (const std::exception& e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Value GetMousePos(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  try {
    Point pt = GetCurrentMouseLocation();
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("x", Napi::Number::New(env, pt.x));
    obj.Set("y", Napi::Number::New(env, pt.y));
    return obj;
  } catch (const Napi::Error& e) {
    e.ThrowAsJavaScriptException();
    return env.Undefined();
  } catch (const std::exception& e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Value MoveMouse(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  try {
    Point pt = ParseRequiredPoint(info);
    MoveMouseTo(pt);
    return env.Undefined();
  } catch (const Napi::Error& e) {
    e.ThrowAsJavaScriptException();
    return env.Undefined();
  } catch (const std::exception& e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    return env.Undefined();
  }
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("leftClick", Napi::Function::New(env, LeftClick));
  exports.Set("rightClick", Napi::Function::New(env, RightClick));
  exports.Set("getMousePos", Napi::Function::New(env, GetMousePos));
  exports.Set("moveMouse", Napi::Function::New(env, MoveMouse));
  return exports;
}

} // namespace

NODE_API_MODULE(crobot, Init)
