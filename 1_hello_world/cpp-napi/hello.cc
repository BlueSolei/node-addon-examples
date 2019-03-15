#include <napi.h>

#include "hello.h"

#include "cpp_napi.h"

std::string World() { return "world"; }
std::string Hello() { return "hello"; }

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return Module(env, exports).Register("hello", Hello).Register("world", World);
}

NODE_API_MODULE(hello, Init)
