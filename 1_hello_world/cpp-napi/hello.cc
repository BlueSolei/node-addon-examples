#include <napi.h>

#include "hello.h"

#include "cpp_napi.h"

std::string World() { return "world"; }
std::string Hello() { return "hello"; }
double Add(double a, double b) { return a + b; }

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  auto module = Module(env, exports)
                    .Register("hello", Hello)
                    .Register("world", World)
                    .Register("add", Add);
  return module;
}

NODE_API_MODULE(hello, Init)
