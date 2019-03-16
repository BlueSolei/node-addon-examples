
#pragma once

#include <napi.h>
#include <string>
#include <type_traits>

#include "function_traits.h"

template <typename T>
struct is_function_pointer
{
    static const bool value =
        std::is_function<T>::value ||
        (std::is_pointer<T>::value &&
         std::is_function<typename std::remove_pointer<T>::type>::value) ||
        (std::is_reference<T>::value &&
         std::is_function<typename std::remove_reference<T>::type>::value);
};

template <class F, class... Args>
constexpr F for_each_arg(F f, Args &&... args)
{
    std::initializer_list<int>{((void)f(std::forward<Args>(args)), 0)...};
    return f;
}

template <class Tuple, class F, std::size_t... I>
constexpr F for_each_impl(Tuple &&t, F &&f, std::index_sequence<I...>)
{
    return (void)std::initializer_list<int>{
               (std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))), 0)...},
           f;
}

template <class Tuple, class F>
constexpr F for_each(Tuple &&t, F &&f)
{
    return for_each_impl(
        std::forward<Tuple>(t), std::forward<F>(f),
        std::make_index_sequence<
            std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

template <class Tuple, class F, std::size_t... I>
constexpr F for_each_impl_index(Tuple &&t, F &&f, std::index_sequence<I...>)
{
    return (void)std::initializer_list<int>{(
               std::forward<F>(f)(I, std::get<I>(std::forward<Tuple>(t))), 0)...},
           f;
}

template <class Tuple, class F>
constexpr F for_each_index(Tuple &&t, F &&f)
{
    return for_each_impl_index(
        std::forward<Tuple>(t), std::forward<F>(f),
        std::make_index_sequence<
            std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

template <class Tuple, class F, std::size_t... I>
constexpr F for_each_impl_type(F &&f, std::index_sequence<I...>)
{
    return (void)std::initializer_list<int>{
               (f.operator()<I, std::tuple_element_t<I, Tuple>>(), 0)...},
           f;
}

template <class Tuple, class F>
constexpr F for_each_type(F &&f)
{
    return for_each_impl_type(
        std::forward<F>(f),
        std::make_index_sequence<
            std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

template <typename Tuple, typename Predicate>
constexpr size_t find_if(Tuple &&tuple, Predicate pred)
{
    size_t index = std::tuple_size<std::remove_reference_t<Tuple>>::value;
    size_t currentIndex = 0;
    bool found = false;
    for_each(tuple, [&](auto &&value) {
        if (!found && pred(value))
        {
            index = currentIndex;
            found = true;
        }
        ++currentIndex;
    });
    return index;
}

template <class CppType>
struct Cpp2JS;

template <class T, std::enable_if_t<std::is_arithmetic<T>::value, T> = 0>
struct Cpp2JS<T>
{
    using CppType = T;
    using JSCppType = double;
    using JSType = Napi::Number;

    static JSType Create(Napi::Env &env, CppType value)
    {
        return JSType::New(env, static_cast<JSCppType>(value));
    }
    static bool SameTypeAs(Napi::Value value) { return value.IsNumber(); }
    static CppType ToCpp(Napi::Value value) { return value.As<JSType>(); }
    static auto JSTypeName() { return "Number"; }
}

auto
Wrap(const std::string &str)
{
    return [&](Napi::Env &env) { return Cpp2JS<std::string>::Create(env, str); };
}

template <class Number,
          std::enable_if_t<std::is_arithmetic<Number>::value, Number> = 0>
auto Wrap(Number value)
{
    return [&](Napi::Env &env) { return Cpp2JS<Number>::Create(env, value); };
}

auto Wrap(std::string (*fn)())
{
    return [fn](const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        return Wrap(fn())(env);
    };
}

using Fn = double (*)(double, double);
auto Wrap(Fn fn)
{
    using FnTraits = function_traits<Fn>;
    using FnArgs = FnTraits::arguments;

    auto jsFunc = [fn](const Napi::CallbackInfo &info) -> Napi::Value {
        Napi::Env env = info.Env();

        if (info.Length() != FnTraits::arity)
        {
            Napi::TypeError::New(env, "Wrong number of arguments. got " +
                                          std::to_string(info.Length()) +
                                          ", expected " +
                                          std::to_string(FnTraits::arity) + ".")
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        bool badParamType = false;
        Args args;
        for_each(args, [&](auto &&arg, auto index) {
            using CppType = std::decay(decltype(arg));
            using JSType = Cpp2JS<CppType>;

            if (badParamType)
                return;

            bool jsTypeOK = JSType::SameTypeAs(info[index]);
            if (jsTypeOK)
            {
                arg = JSType::ToCpp(info[index]);
            }
            else
            {
                badParamType = true;
                Napi::TypeError::New(env, "Bad param type. Argument no. " +
                                              std::to_string(ElementInfo::Index) +
                                              " is of type '" + typeof(CppType).name() +
                                              "', expected type was '" +
                                              JSType::JSTypeName() + "'")
                    .ThrowAsJavaScriptException();
            }
        });
        if (badParamType)
            return env.Null();
        else
            return Wrap(invoke_fn(fn, args))(env);
    };

    return jsFunc;
};
}

template <class Func, std::enable_if_t<is_function_pointer<Func>::value> = 0>
auto JSType(Func fn)
{
    return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(fn)); };
};

auto JSType(std::string (*fn)())
{
    return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(fn)); };
};

auto JSType(double (*fn)(double, double))
{
    return [=](Napi::Env &env) { return Napi::Function::New(env, Wrap(fn)); };
};

class Module
{
  public:
    Module(Napi::Env &env, Napi::Object &exports)
        : m_env(env), m_exports(exports) {}

    template <class Identifier>
    Module &Register(const char *identifierName, Identifier identifier)
    {
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