#include <napi.h>

#include <ApplicationServices/ApplicationServices.h>

#include <optional>
#include <stdexcept>

namespace {

CGPoint GetCurrentMouseLocation() {
  CGEventRef event = CGEventCreate(nullptr);
  if (!event) {
    return CGPointMake(0, 0);
  }
  CGPoint location = CGEventGetLocation(event);
  CFRelease(event);
  return location;
}

void PostMouseClick(CGMouseButton button, const std::optional<CGPoint>& maybeLocation) {
  CGPoint location = maybeLocation.has_value() ? *maybeLocation : GetCurrentMouseLocation();

  CGEventType downType = (button == kCGMouseButtonLeft) ? kCGEventLeftMouseDown : kCGEventRightMouseDown;
  CGEventType upType = (button == kCGMouseButtonLeft) ? kCGEventLeftMouseUp : kCGEventRightMouseUp;

  CGEventRef down = CGEventCreateMouseEvent(nullptr, downType, location, button);
  if (!down) {
    throw std::runtime_error("Failed to create mouse down event");
  }
  CGEventRef up = CGEventCreateMouseEvent(nullptr, upType, location, button);
  if (!up) {
    CFRelease(down);
    throw std::runtime_error("Failed to create mouse up event");
  }

  CGEventPost(kCGHIDEventTap, down);
  CGEventPost(kCGHIDEventTap, up);

  CFRelease(down);
  CFRelease(up);
}

void MoveMouseTo(CGPoint location) {
  // Warp immediately.
  CGWarpMouseCursorPosition(location);
  // Re-associate hardware mouse movement with cursor position.
  CGAssociateMouseAndMouseCursorPosition(true);

  // Also post a move event so observers/UI update consistently.
  CGEventRef move = CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, location, kCGMouseButtonLeft);
  if (!move) {
    throw std::runtime_error("Failed to create mouse move event");
  }
  CGEventPost(kCGHIDEventTap, move);
  CFRelease(move);
}

std::optional<CGPoint> ParseOptionalPoint(const Napi::CallbackInfo& info) {
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
    return CGPointMake(x, y);
  }

  if (info.Length() >= 2 && info[0].IsNumber() && info[1].IsNumber()) {
    double x = info[0].ToNumber().DoubleValue();
    double y = info[1].ToNumber().DoubleValue();
    return CGPointMake(x, y);
  }

  throw Napi::TypeError::New(info.Env(), "Usage: fn(), fn(x, y) or fn({x, y})");
}

CGPoint ParseRequiredPoint(const Napi::CallbackInfo& info) {
  if (info.Length() == 1 && info[0].IsObject()) {
    Napi::Object obj = info[0].As<Napi::Object>();
    if (!obj.Has("x") || !obj.Has("y")) {
      throw Napi::TypeError::New(info.Env(), "Point object must have x and y");
    }
    double x = obj.Get("x").ToNumber().DoubleValue();
    double y = obj.Get("y").ToNumber().DoubleValue();
    return CGPointMake(x, y);
  }

  if (info.Length() >= 2 && info[0].IsNumber() && info[1].IsNumber()) {
    double x = info[0].ToNumber().DoubleValue();
    double y = info[1].ToNumber().DoubleValue();
    return CGPointMake(x, y);
  }

  throw Napi::TypeError::New(info.Env(), "Usage: fn(x, y) or fn({x, y})");
}

Napi::Value LeftClick(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  try {
    auto pt = ParseOptionalPoint(info);
    PostMouseClick(kCGMouseButtonLeft, pt);
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
    PostMouseClick(kCGMouseButtonRight, pt);
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
    CGPoint pt = GetCurrentMouseLocation();
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
    CGPoint pt = ParseRequiredPoint(info);
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
