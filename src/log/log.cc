#include "log.hpp"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <cassert>
#include <chrono>

namespace what::Log {

auto Log::GetInstance() -> Log * {
  static Log log = Log();
  return &log;
}

void Log::Init(const char *file_name, Verbosity max_verbosity, int flush_ms) {
  max_verbosity_into_file = max_verbosity;
  log_flush_internal_ms = flush_ms;
  is_aync == log_flush_internal_ms != 0;
  memset(dir_name, '\0', DIR_NAME_LEN);
  memset(this->file_name, '\0', FILE_NAME_LEN);
  if (file_name != nullptr) {  //*用户提供了日志文件的路径
    const char *last_slash_position = strrchr(file_name, '/');
    if (last_slash_position != nullptr) {
      snprintf(dir_name,
               DIR_NAME_LEN < (last_slash_position - file_name) ? DIR_NAME_LEN : (last_slash_position - file_name),
               "%s", file_name);
      snprintf(this->file_name, FILE_NAME_LEN, "%s", last_slash_position + 1);
      char file_path[DIR_NAME_LEN + FILE_NAME_LEN + 1];
      file_path[DIR_NAME_LEN + FILE_NAME_LEN] = '\0';
      snprintf(file_path, sizeof(file_path), "%s/%s", dir_name, this->file_name);
      if (!create_log_dir(file_path)) {
        exit(EXIT_FAILURE);
      }
    } else {
      snprintf(this->file_name, FILE_NAME_LEN, "%s", file_name);
    }
    file = fopen(file_name, "a");
    if (file == nullptr) {
      LOG(ERROR, "failed to open %s", file_name);
      exit(EXIT_FAILURE);
    }
  }
  time_since_epoch =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  auto current_time = time(NULL);
  auto tm = localtime(&current_time);
  start_time = malloc(sizeof(struct tm));
  memcpy(start_time, tm, sizeof(struct tm));
}

auto Log::create_log_dir(char *tmp_file) -> bool {
  assert(tmp_file != nullptr);
  if (tmp_file[0] == '\0') return false;
  char *p = strchr(tmp_file, '/');
  for (; *p; p = strchr(p + 1, '/')) {
    *p = '\0';
    if (!mkdir(tmp_file, 0755)) {
      LOG(ERROR, "failed to create log directory");
      return false;
    }
    *p = '/';
  }
  return true;
}

}  // namespace what::Log