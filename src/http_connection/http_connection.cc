#include <string.h>
#include "../../include/http_connection/http_connection.hpp"

void Http_Connection::Init(int fd, sockaddr_in *address)
{
    __client_fd = fd;
    __client_address = *address;
    init(); // 初始化connection的初始状态 初始化状态机
}

void Http_Connection::init()
{
    status = STATUS::GET_REQUEST_LINE;
    method = METHOD::GET;
    __read_idx = 0;
    __checked_idx = 0;
    __start_line_idx = 0;
    memset(__read_buffer, '\0', READ_BUFFER_LEN);
    memset(__write_buffer, '\0', WRITE_BUFFER_LEN);
    memset(__file_name, '\0', FILE_NAME_LEN);
}

bool Http_Connection::Read_Once()
{
    ssize_t bytes = 0;
    // 采用 边沿触发 每次处理时一定要全部读完所有的数据 如果水平出发则无所谓
    while (true)
    {
        bytes = recv(__client_fd, __read_buffer + __read_idx, READ_BUFFER_LEN - __read_idx, 0);
        if (bytes < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            return false;
        }
        else if (bytes == 0)
        {
            return false;
        }
        __read_idx += bytes;
    }
    return true;
}

//* http 以\r\n 作为分界符
LINE_STATUS Http_Connection::parse_line() // 解析request_line 和 request_head
{
    char tmp = '@';
    for (; __checked_idx < __read_idx; ++__checked_idx)
    {
        tmp = __read_buffer[__checked_idx];
        if (tmp == '\r')
        {
            if (__checked_idx + 1 == __read_idx)
                return LINE_STATUS::LINE_OPEN;
            if (__read_buffer[__checked_idx + 1] == '\n')
            {
                __read_buffer[__checked_idx++] = '\0';
                __read_buffer[__checked_idx++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
        else if (tmp == '\n')
        {
            if (__checked_idx > 1 && __read_buffer[__checked_idx - 1] == '\r')
            {
                __read_buffer[__checked_idx - 1] = '\0';
                __read_buffer[__checked_idx++] = '\0';
                return LINE_STATUS::LINE_OK;
            }
            return LINE_STATUS::LINE_BAD;
        }
    }
    return LINE_STATUS::LINE_OPEN;
}

HTTP_REQUEST_STATUS Http_Connection::run_read()
{
    LINE_STATUS line_status = LINE_STATUS::LINE_OK;
    HTTP_REQUEST_STATUS ret = HTTP_REQUEST_STATUS::NO_REQUEST;
    while ((status == STATUS::GET_REQUEST_CONTINENT && line_status == LINE_STATUS::LINE_OK) || ((line_status = parse_line()) == LINE_STATUS::LINE_OK))
    {
    }
    return ret;
}

void Http_Connection::Run()
{
    HTTP_REQUEST_STATUS ret = run_read();
    if (ret == HTTP_REQUEST_STATUS::NO_REQUEST) // 当前解析尚未完成 添加到 epoll 中继续等待
    {
        poller->ReAdd_fd(__client_fd, EPOLLIN, true); // 实际上我们只支持边沿触发😁
        return;
    }
    // TODO: 进行对客户端的相应
}