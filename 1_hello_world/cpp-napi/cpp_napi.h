
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

auto JS = [](const char *identifierName, auto identifier) {
    return std::make_tuple(identifierName, JSType(identifier));
};

struct Export
{
    Napi::Env &env;
    Napi::Object &exports;

    template <class Metadata>
    Export &operator()(Metadata &&metadata)
    {
        exports.Set(Napi::String::New(env, std::get<0>(metadata)),
                    std::get<1>(metadata)(env));
        return *this;
    }

    operator Napi::Object &() const { return exports; }
};