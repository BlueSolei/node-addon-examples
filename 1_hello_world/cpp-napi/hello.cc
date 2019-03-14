#include <napi.h>
#include <tuple>

#include "hello.h"

std::string World() { return "world"; }
std::string Hello() { return "hello"; }

auto Wrap(const std::string &str) {
  return [&](Napi::Env &env) { return Napi::String::New(env, str); };
}

auto Wrap(std::string (*fn)()) {
  return [fn](const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    return Wrap(fn())(env);
  };
}

auto JSType = [](auto method) {
  return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(method)); };
};

auto JS = [](const char *identifierName, auto identifier) {
  return std::make_tuple(
      identifierName, [=](Napi::Env &env) { return JSType(identifier)(env); });
};

struct Export {
  Napi::Env& env;
  Napi::Object& exports;

  template <class Metadata> Export &operator()(Metadata &&metadata) {
    exports.Set(Napi::String::New(env, std::get<0>(metadata)),
                std::get<1>(metadata)(env));
    return *this;
  }

  operator Napi::Object&() const { return exports; }
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return Export{env, exports}(JS("hello", Hello))(JS("world", World));
}

NODE_API_MODULE(hello, Init)
