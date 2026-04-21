#include <napi.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <stdexcept>

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <windows.h>
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

struct Modifiers {
  bool shift = false;
  bool ctrl = false;
  bool alt = false;
  bool meta = false;
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

void PostMouseClick(MouseButton button, const std::optional<Point>& maybeLocation, int clickCount) {
  if (clickCount <= 0) {
    throw std::runtime_error("clickCount must be positive");
  }

  Point location = maybeLocation.has_value() ? *maybeLocation : GetCurrentMouseLocation();
  CGPoint cgLocation = CGPointMake(location.x, location.y);

  CGMouseButton cgButton = (button == MouseButton::Left) ? kCGMouseButtonLeft : kCGMouseButtonRight;
  CGEventType downType = (button == MouseButton::Left) ? kCGEventLeftMouseDown : kCGEventRightMouseDown;
  CGEventType upType = (button == MouseButton::Left) ? kCGEventLeftMouseUp : kCGEventRightMouseUp;

  for (int i = 1; i <= clickCount; i++) {
    CGEventRef down = CGEventCreateMouseEvent(nullptr, downType, cgLocation, cgButton);
    if (!down) {
      throw std::runtime_error("Failed to create mouse down event");
    }
    CGEventRef up = CGEventCreateMouseEvent(nullptr, upType, cgLocation, cgButton);
    if (!up) {
      CFRelease(down);
      throw std::runtime_error("Failed to create mouse up event");
    }

    // For multi-click, macOS expects click state to be 1 for the first click, 2 for the second, etc.
    CGEventSetIntegerValueField(down, kCGMouseEventClickState, i);
    CGEventSetIntegerValueField(up, kCGMouseEventClickState, i);

    CGEventPost(kCGHIDEventTap, down);
    CGEventPost(kCGHIDEventTap, up);

    CFRelease(down);
    CFRelease(up);
  }
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

using NativeKeyCode = uint16_t; // CGKeyCode

void PostKeyEvent(NativeKeyCode key, bool down) {
  CGEventRef ev = CGEventCreateKeyboardEvent(nullptr, static_cast<CGKeyCode>(key), down);
  if (!ev) {
    throw std::runtime_error("Failed to create keyboard event");
  }
  CGEventPost(kCGHIDEventTap, ev);
  CFRelease(ev);
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

void PostMouseClick(MouseButton button, const std::optional<Point>& maybeLocation, int clickCount) {
  if (clickCount <= 0) {
    throw std::runtime_error("clickCount must be positive");
  }

  if (maybeLocation.has_value()) {
    MoveMouseTo(*maybeLocation);
  }

  DWORD downFlag = (button == MouseButton::Left) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
  DWORD upFlag = (button == MouseButton::Left) ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_RIGHTUP;

  for (int i = 0; i < clickCount; i++) {
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
}

using NativeKeyCode = WORD; // Virtual-Key code

void PostKeyEvent(NativeKeyCode vk, bool down) {
  INPUT input;
  ZeroMemory(&input, sizeof(input));
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vk;
  input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
  UINT sent = SendInput(1, &input, sizeof(INPUT));
  if (sent != 1) {
    throw std::runtime_error("SendInput(keyboard) failed");
  }
}
#endif

std::string ToLower(std::string s) {
  for (auto& ch : s) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    }
  }
  return s;
}

Modifiers ParseOptionalModifiers(const Napi::Env& env, const Napi::Value& v) {
  Modifiers m;
  if (v.IsUndefined() || v.IsNull()) {
    return m;
  }
  if (!v.IsObject()) {
    throw Napi::TypeError::New(env, "Modifiers must be an object");
  }
  Napi::Object o = v.As<Napi::Object>();
  auto getBool = [&](const char* key) -> bool {
    if (!o.Has(key)) return false;
    return o.Get(key).ToBoolean().Value();
  };
  m.shift = getBool("shift");
  m.ctrl = getBool("ctrl") || getBool("control");
  m.alt = getBool("alt") || getBool("option");
  m.meta = getBool("meta") || getBool("cmd") || getBool("command") || getBool("win");
  return m;
}

// Map key name to native key code.
// Note: key codes are layout-dependent on macOS; this mapping matches US ANSI layout.
NativeKeyCode KeyNameToNativeKeyCode(const std::string& keyName) {
  const std::string k = ToLower(keyName);

#if defined(__APPLE__)
  static const std::unordered_map<std::string, NativeKeyCode> map = {
      {"a", 0},   {"s", 1},   {"d", 2},   {"f", 3},   {"h", 4},   {"g", 5},
      {"z", 6},   {"x", 7},   {"c", 8},   {"v", 9},   {"b", 11},  {"q", 12},
      {"w", 13},  {"e", 14},  {"r", 15},  {"y", 16},  {"t", 17},  {"1", 18},
      {"2", 19},  {"3", 20},  {"4", 21},  {"6", 22},  {"5", 23},  {"=", 24},
      {"9", 25},  {"7", 26},  {"-", 27},  {"8", 28},  {"0", 29},  {"]", 30},
      {"o", 31},  {"u", 32},  {"[", 33},  {"i", 34},  {"p", 35},  {"l", 37},
      {"j", 38},  {"'", 39},  {"k", 40},  {";", 41},  {"\\", 42}, {",", 43},
      {"/", 44},  {"n", 45},  {"m", 46},  {".", 47},  {"`", 50},
      {"enter", 36},
      {"return", 36},
      {"tab", 48},
      {"space", 49},
      {"backspace", 51},
      {"delete", 51},
      {"esc", 53},
      {"escape", 53},
      {"left", 123},
      {"right", 124},
      {"down", 125},
      {"up", 126},
      {"shift", 56},
      {"control", 59},
      {"ctrl", 59},
      {"alt", 58},
      {"option", 58},
      {"meta", 55},
      {"cmd", 55},
      {"command", 55},
  };
  auto it = map.find(k);
  if (it == map.end()) {
    throw std::runtime_error("Unsupported key name: " + keyName);
  }
  return it->second;

#elif defined(_WIN32)
  if (k.size() == 1) {
    char ch = k[0];
    if (ch >= 'a' && ch <= 'z') {
      return static_cast<NativeKeyCode>('A' + (ch - 'a'));
    }
    if (ch >= '0' && ch <= '9') {
      return static_cast<NativeKeyCode>('0' + (ch - '0'));
    }
  }
  static const std::unordered_map<std::string, NativeKeyCode> map = {
      {"enter", VK_RETURN},
      {"return", VK_RETURN},
      {"tab", VK_TAB},
      {"space", VK_SPACE},
      {"backspace", VK_BACK},
      {"delete", VK_BACK},
      {"esc", VK_ESCAPE},
      {"escape", VK_ESCAPE},
      {"left", VK_LEFT},
      {"right", VK_RIGHT},
      {"up", VK_UP},
      {"down", VK_DOWN},
      {"shift", VK_LSHIFT},
      {"control", VK_LCONTROL},
      {"ctrl", VK_LCONTROL},
      {"alt", VK_LMENU},
      {"meta", VK_LWIN},
      {"win", VK_LWIN},
  };
  auto it = map.find(k);
  if (it == map.end()) {
    throw std::runtime_error("Unsupported key name: " + keyName);
  }
  return it->second;
#endif
}

struct ParsedKey {
  bool isNativeCode = false;
  uint32_t nativeCode = 0;
  std::string name;
};

template <typename Fn>
Napi::Value RunSafely(const Napi::CallbackInfo& info, Fn&& fn) {
  Napi::Env env = info.Env();
  try {
    return fn(env);
  } catch (const Napi::Error& e) {
    e.ThrowAsJavaScriptException();
  } catch (const std::exception& e) {
    Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
  }
  return env.Undefined();
}

ParsedKey ParseKeyArg(const Napi::CallbackInfo& info, size_t index) {
  Napi::Env env = info.Env();
  if (info.Length() <= index) {
    throw Napi::TypeError::New(env, "Key is required");
  }
  if (info[index].IsString()) {
    ParsedKey k;
    k.isNativeCode = false;
    k.name = info[index].ToString().Utf8Value();
    return k;
  }
  if (info[index].IsNumber()) {
    ParsedKey k;
    k.isNativeCode = true;
    double v = info[index].ToNumber().DoubleValue();
    if (v < 0) {
      throw Napi::TypeError::New(env, "Key code must be non-negative");
    }
    k.nativeCode = static_cast<uint32_t>(v);
    return k;
  }
  throw Napi::TypeError::New(env, "Key must be a string or a number");
}

void KeyDownUpNoModifiers(const ParsedKey& key, bool down) {
  NativeKeyCode kc;
  if (key.isNativeCode) {
    kc = static_cast<NativeKeyCode>(key.nativeCode);
  } else {
    kc = KeyNameToNativeKeyCode(key.name);
  }
  PostKeyEvent(kc, down);
}

void KeyTapWithModifiers(const ParsedKey& key, const Modifiers& mods) {
  auto postModifier = [&](const char* name, bool down) {
    NativeKeyCode kc = KeyNameToNativeKeyCode(name);
    PostKeyEvent(kc, down);
  };

  if (mods.shift) postModifier("shift", true);
  if (mods.ctrl) postModifier("ctrl", true);
  if (mods.alt) postModifier("alt", true);
  if (mods.meta) postModifier("meta", true);

  NativeKeyCode main;
  if (key.isNativeCode) {
    main = static_cast<NativeKeyCode>(key.nativeCode);
  } else {
    main = KeyNameToNativeKeyCode(key.name);
  }
  PostKeyEvent(main, true);
  PostKeyEvent(main, false);

  if (mods.meta) postModifier("meta", false);
  if (mods.alt) postModifier("alt", false);
  if (mods.ctrl) postModifier("ctrl", false);
  if (mods.shift) postModifier("shift", false);
}

Napi::Value KeyDown(const Napi::CallbackInfo& info) {
  return RunSafely(info, [&](Napi::Env env) {
    ParsedKey key = ParseKeyArg(info, 0);
    if (info.Length() >= 2) {
      throw Napi::TypeError::New(env, "Usage: keyDown(key)");
    }
    KeyDownUpNoModifiers(key, true);
    return env.Undefined();
  });
}

Napi::Value KeyUp(const Napi::CallbackInfo& info) {
  return RunSafely(info, [&](Napi::Env env) {
    ParsedKey key = ParseKeyArg(info, 0);
    if (info.Length() >= 2) {
      throw Napi::TypeError::New(env, "Usage: keyUp(key)");
    }
    KeyDownUpNoModifiers(key, false);
    return env.Undefined();
  });
}

Napi::Value KeyTap(const Napi::CallbackInfo& info) {
  return RunSafely(info, [&](Napi::Env env) {
    ParsedKey key = ParseKeyArg(info, 0);
    Modifiers mods = (info.Length() >= 2) ? ParseOptionalModifiers(env, info[1]) : Modifiers{};
    if (info.Length() >= 3) {
      throw Napi::TypeError::New(env, "Usage: keyTap(key, [modifiers])");
    }
    KeyTapWithModifiers(key, mods);
    return env.Undefined();
  });
}

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
  return RunSafely(info, [&](Napi::Env env) {
    auto pt = ParseOptionalPoint(info);
    PostMouseClick(MouseButton::Left, pt, 1);
    return env.Undefined();
  });
}

Napi::Value LeftDoubleClick(const Napi::CallbackInfo& info) {
  return RunSafely(info, [&](Napi::Env env) {
    auto pt = ParseOptionalPoint(info);
    PostMouseClick(MouseButton::Left, pt, 2);
    return env.Undefined();
  });
}

Napi::Value RightClick(const Napi::CallbackInfo& info) {
  return RunSafely(info, [&](Napi::Env env) {
    auto pt = ParseOptionalPoint(info);
    PostMouseClick(MouseButton::Right, pt, 1);
    return env.Undefined();
  });
}

Napi::Value GetMousePos(const Napi::CallbackInfo& info) {
  return RunSafely(info, [&](Napi::Env env) {
    Point pt = GetCurrentMouseLocation();
    Napi::Object obj = Napi::Object::New(env);
    obj.Set("x", Napi::Number::New(env, pt.x));
    obj.Set("y", Napi::Number::New(env, pt.y));
    return obj;
  });
}

Napi::Value MoveMouse(const Napi::CallbackInfo& info) {
  return RunSafely(info, [&](Napi::Env env) {
    Point pt = ParseRequiredPoint(info);
    MoveMouseTo(pt);
    return env.Undefined();
  });
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("leftClick", Napi::Function::New(env, LeftClick));
  exports.Set("leftDoubleClick", Napi::Function::New(env, LeftDoubleClick));
  exports.Set("rightClick", Napi::Function::New(env, RightClick));
  exports.Set("getMousePos", Napi::Function::New(env, GetMousePos));
  exports.Set("moveMouse", Napi::Function::New(env, MoveMouse));
  exports.Set("keyDown", Napi::Function::New(env, KeyDown));
  exports.Set("keyUp", Napi::Function::New(env, KeyUp));
  exports.Set("keyTap", Napi::Function::New(env, KeyTap));
  return exports;
}

} // namespace

NODE_API_MODULE(crobot, Init)
