#include "log/log.hpp"

int main() {
#ifdef ASYNC_LOG  //异步日志
  what::Log::Log::GetInstance()->Init("server_log", what::Log::Verbosity::Verbosity_LOG, 100);
#else  //同步日志
  what::Log::Log::GetInstance()->Init("server_log", what::Log::Verbosity::Verbosity_LOG, 0);
#endif
}