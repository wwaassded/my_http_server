#include "../../include/log/log.hpp"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <thread>

namespace what::Log
{

  static const int MAX_BUFFER_QUEUE_SIZE = 8;
  static const int VERBOSITY_NAME_LEN = 9;

  auto Log::GetInstance() -> Log *
  {
    static Log log;
    return &log;
  }

  // TODO
  Log::Log() : is_aync(false), block_queue(nullptr) {}
  Log::~Log()
  {
    free(start_time);
    if (file != nullptr)
      fclose(file);
  }

  void Log::Init(const char *file_name, Verbosity max_verbosity, int max_queue_size)
  {
    max_verbosity_into_file = max_verbosity;
    block_queue = std::make_unique<Block_queue<std::string>>(max_queue_size);
    is_aync = (max_queue_size != 0);
    time_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    auto current_time = time(NULL);
    auto tm = localtime(&current_time);
    start_time = malloc(sizeof(struct tm));
    memcpy(start_time, tm, sizeof(struct tm));
    memset(dir_name, '\0', DIR_NAME_LEN);
    memset(this->file_name, '\0', FILE_NAME_LEN);
    if (file_name != nullptr)
    { //*用户提供了日志文件的路径
      const char *last_slash_position = strrchr(file_name, '/');
      char file_path[DIR_NAME_LEN + FILE_NAME_LEN + 1];
      file_path[DIR_NAME_LEN + FILE_NAME_LEN] = '\0';
      auto time_cal = reinterpret_cast<struct tm *>(start_time);

      if (last_slash_position != nullptr)
      {
        snprintf(dir_name,
                 DIR_NAME_LEN < (last_slash_position - file_name) ? DIR_NAME_LEN : (last_slash_position - file_name),
                 "%s", file_name);
        snprintf(this->file_name, FILE_NAME_LEN, "%s", last_slash_position + 1);
        snprintf(file_path, sizeof(file_path), "%s/%s_%04d-%02d-%02d", dir_name, this->file_name,
                 time_cal->tm_year + 1900, time_cal->tm_mon + 1, time_cal->tm_mday);
        if (!create_log_dir(file_path))
        {
          exit(EXIT_FAILURE);
        }
      }
      else
      {
        snprintf(file_path, sizeof(file_path), "%s/%s_%04d-%02d-%02d", dir_name, this->file_name,
                 time_cal->tm_year + 1900, time_cal->tm_mon + 1, time_cal->tm_mday);
      }
      file = fopen(file_path, "a");
      if (file == nullptr)
      {
        LOG(ERROR, "failed to open %s", file_name);
        exit(EXIT_FAILURE);
      }
    }
    //* 异步日志 flush线程
    if (this->is_aync)
    {
      std::thread(&Log::flush_thread_function, this).detach();
    }
  }

  void Log::Log_everywhere(Verbosity verbosity, const char *file_path, unsigned int line, const char *format, ...)
  {
    // INFO:    2024-
    // WARNING:
    char level[VERBOSITY_NAME_LEN];
    level[VERBOSITY_NAME_LEN - 1] = '\0';
    switch (verbosity)
    {
    case Verbosity::Verbosity_LOG:
    {
      snprintf(level, VERBOSITY_NAME_LEN, "LOG:");
      break;
    }
    case Verbosity::Verbosity_INFO:
    {
      snprintf(level, VERBOSITY_NAME_LEN, "INFO:");
      break;
    }
    case Verbosity::Verbosity_WARNING:
    {
      snprintf(level, VERBOSITY_NAME_LEN, "WARNING:");
      break;
    }
    case Verbosity::Verbosity_ERROR:
    {
      snprintf(level, VERBOSITY_NAME_LEN, "ERROR:");
      break;
    }
    case Verbosity::Verbosity_FATAL:
    {
      snprintf(level, VERBOSITY_NAME_LEN, "FATAL:");
      break;
    }
    default:
    {
      LOG(ERROR, "unknown verbosity name!");
    }
    }

    // 这个过程可能都会需要加锁

    this->locker.lock();

    auto time_seed = time(NULL);
    struct tm curr_tm;
    localtime_r(&time_seed, &curr_tm);
    // 我们希望对日志进行 按照天数进行分类
    if (curr_tm.tm_mday != reinterpret_cast<struct tm *>(start_time)->tm_mday)
    {
      memcpy(start_time, &curr_tm, sizeof(struct tm));
      assert(file != nullptr);
      flush();
      fclose(file);
      // 重新使用新的文件
    }

    this->locker.unlock();
  }

  auto Log::create_log_dir(char *tmp_file) -> bool
  {
    assert(tmp_file != nullptr);
    if (tmp_file[0] == '\0')
      return false;
    char *p = strchr(tmp_file, '/');
    for (; *p; p = strchr(p + 1, '/'))
    {
      *p = '\0';
      if (!mkdir(tmp_file, 0755))
      {
        LOG(ERROR, "failed to create log directory");
        return false;
      }
      *p = '/';
    }
    return true;
  }

  void Log::flush_thread_function()
  {
    if (!is_aync)
      return;
    std::string str;
    for (;;)
    { // TODO 需要一种退出的方式
      block_queue->Pop(str);
      fputs(str.c_str(), file);
    }
  }

  void Log::flush()
  {
    locker.lock();
    fflush(file);
    locker.unlock();
  }

} // namespace what::Log