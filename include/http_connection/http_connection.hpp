#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include "../thread_pool/thread_pool_continent_base.hpp"
#include "../socket_op.hpp"

#define FILE_NAME_LEN 200
#define READ_BUFFER_LEN 2048
#define WRITE_BUFFER_LEN 1024

// http请求的方法
enum class METHOD
{
    GET = 0,
    POST,
    HEAD,
    DELETE,
    CONNECT,
    PUT,
    OPTIONS,
    TRACE,
    PATCH,
};

enum HTTP_REQUEST_STATUS
{
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURCE,
    FORBIDDEN_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};

enum class STATUS
{
    GET_REQUEST_LINE = 0,
    GET_REQUEST_HEADER,
    GET_REQUEST_CONTINENT,
};

enum class LINE_STATUS
{
    LINE_OK = 0,
    LINE_BAD,
    LINE_OPEN,
};

class Http_Connection : public Thread_Pool_Continent_Base
{
public:
    void Run() override; // 执行http主逻辑的执行函数

    // 对外接口
public:
    Http_Connection() = default;
    ~Http_Connection() = default;

    void Init(int fd, sockaddr_in *address);

    bool Read_Once();

    // 私有函数
private:
    void init();
    HTTP_REQUEST_STATUS run_read();
    //! unsafe
    inline char *get_line() { return __read_buffer + __start_line_idx; } //*调用该函数前应该确保 \r\n已经被替换成为 \0\0 否则会导致难以预料的错误
    LINE_STATUS parse_line();
    HTTP_REQUEST_STATUS parse_request_line(char *);

    // 确定链接
private:
    int __client_fd;
    sockaddr_in __client_address;
    static Epoll *poller;

    // 读写缓冲
private:
    uint32_t __read_idx;
    uint32_t __checked_idx;
    uint32_t __start_line_idx;
    char __read_buffer[READ_BUFFER_LEN];
    char __write_buffer[WRITE_BUFFER_LEN];
    char __file_name[FILE_NAME_LEN];

    // 状态机
private:
    METHOD method;
    STATUS status;
};

#endif