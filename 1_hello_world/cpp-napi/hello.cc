#include <napi.h>

#include "hello.h"

#include "cpp_napi.h"

std::string World() { return "world"; }
std::string Hello() { return "hello"; }

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  //  return Module(env, exports).Register<Hello>("hello").Register<World>("world");
  return Module(env, exports).Register<std::string, Hello>("hello").Register<std::string, World>("world");
}

NODE_API_MODULE(hello, Init)
