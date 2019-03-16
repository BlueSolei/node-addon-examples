
#pragma once

#include <napi.h>
#include <string>
#include <type_traits>

template <typename T> struct is_function_pointer {
  static const bool value =
      std::is_function<T>::value ||
      (std::is_pointer<T>::value &&
       std::is_function<typename std::remove_pointer<T>::type>::value) ||
      (std::is_reference<T>::value &&
       std::is_function<typename std::remove_reference<T>::type>::value);
};

auto Wrap(const std::string &str) {
  return [&](Napi::Env &env) { return Napi::String::New(env, str); };
}

auto Wrap(double value) {
  return [&](Napi::Env &env) { return Napi::Number::New(env, value); };
}

template <class Number,
          std::enable_if_t<std::is_arithmetic<Number>::value, Number> = 0>
auto Wrap(Number value) {
  return Wrap(static_cast<double>(value));
}

auto Wrap(std::string (*fn)()) {
  return [fn](const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    return Wrap(fn())(env);
  };
}

auto Wrap(double (*fn)(double, double)) {
  return [fn](const Napi::CallbackInfo &info) -> Napi::Value {
    Napi::Env env = info.Env();

    if (info.Length() < 2) {
      Napi::TypeError::New(env, "Wrong number of arguments")
          .ThrowAsJavaScriptException();
      return env.Null();
    }

    if (!info[0].IsNumber() || !info[1].IsNumber()) {
      Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
      return env.Null();
    }

    double arg0 = info[0].As<Napi::Number>().DoubleValue();
    double arg1 = info[1].As<Napi::Number>().DoubleValue();
    return Wrap(fn(arg0, arg1))(env);
  };
}

template <class Func, std::enable_if_t<is_function_pointer<Func>::value> = 0>
auto JSType(Func fn) {
  return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(fn)); };
};

auto JSType(std::string (*fn)()) {
  return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(fn)); };
};

auto JSType(double (*fn)(double, double)) {
  return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(fn)); };
};

class Module {
public:
  Module(Napi::Env &env, Napi::Object &exports)
      : m_env(env), m_exports(exports) {}

  template <class Identifier>
  Module &Register(const char *identifierName, Identifier identifier) {
    auto jsTypeFactory = JSType(identifier);
    m_exports.Set(Napi::String::New(m_env, identifierName),
                  jsTypeFactory(m_env));
    return *this;
  }

  operator Napi::Object &() { return m_exports; }

private:
  Napi::Env &m_env;
  Napi::Object &m_exports;
};