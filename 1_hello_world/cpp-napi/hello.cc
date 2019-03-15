#include <napi.h>
#include <tuple>

#include "hello.h"

#include "cpp_napi.h"

std::string World() { return "world"; }
std::string Hello() { return "hello"; }

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  return Export{env, exports}(JS("hello", Hello))(JS("world", World));
}

NODE_API_MODULE(hello, Init)
