
#pragma once

#include <napi.h>
#include <string>

auto Wrap(const std::string &str)
{
    return [&](Napi::Env &env) { return Napi::String::New(env, str); };
}

auto Wrap(std::string (*fn)())
{
    return [fn](const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        return Wrap(fn())(env);
    };
}

auto JSType = [](auto method) {
    return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(method)); };
};

class Module
{
  public:
    Module(Napi::Env &env,
           Napi::Object &exports) : m_env(env), m_exports(exports) {}

    template <class Identifier>
    Module &Register(const char *identifierName, Identifier &&identifier)
    {
        auto jsTypeFactory = JSType(std::forward<Identifier>(identifier));
        m_exports.Set(Napi::String::New(m_env, identifierName),
                      jsTypeFactory(m_env));
        return *this;
    }

    operator Napi::Object &() { return m_exports; }

  private:
    Napi::Env &m_env;
    Napi::Object &m_exports;
};