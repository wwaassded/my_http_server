#ifndef LOG_HPP
#define LOG_HPP

#include <stdarg.h>
#include <stdio.h>
#include <memory>
#include "thread_safe_queue.hpp"

namespace what::Log {

enum class Verbosity {
  Verbosity_FATAL = -4,
  Verbosity_ERROR,
  Verbosity_WARNING,
  Verbosity_INFO,
  Verbosity_LOG,
};

class Log {
 public:
  static auto GetInstance() -> Log *;

  //! 我们应该尽量保证 file_name not null, 成员函数内部可能会有某些assert要求 FILE* not null
  void Init(const char *file_name, Verbosity max_verbosity, int max_queue_size);

 private:
  const static int DIR_NAME_LEN = 128;
  const static int FILE_NAME_LEN = 128;

  Log();
  ~Log();

  char dir_name[DIR_NAME_LEN];
  char file_name[FILE_NAME_LEN];

  void flush_thread_function();

  Verbosity max_verbosity_into_file;  //写入到文件中的最大 verbosity

  FILE *file{nullptr};  // 日志文件对应的文件指针
  bool is_aync{false};
  std::mutex locker;
  __int64_t time_since_epoch;
  void *start_time;  //! malloc

  std::unique_ptr<Block_queue<std::string>> block_queue;

  void Log_everywhere(Verbosity, const char *file_path, unsigned int line, const char *format, ...);

  auto create_log_dir(char *dir) -> bool;

  void flush();
};

#define VLOG(verbosity, ...) what::Log::Log::GetInstance()->Log_everywhere(verbosity, __VA_ARGS__);

#define LOG(verbosity, ...) VLOG(what::Log::Verbosity::Verbosity_##verbosity, __FILE__, __LINE__, __VA_ARGS__)

}  // namespace what::Log

#endif