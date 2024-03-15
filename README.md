# http server

***简单概述***

1. linux下的C++轻量级 Web Server
2. 采用 epoll(ET触发) + 线程池的 非阻塞同步 多线程reactor网络模型
3. log_what异步日志库 高效完成日志的记录
4. 基于有序链表 + alarm 实现的定时器 对超时为响应的链接做出及时的处理
5. 采用有限状态机的理念完成对于http请求的解析以及 回复应答报文 现支持 (HEAD，GET，POST)等方法
6. 数据方面采用 mysql连接池管理空闲的mysql链接 并在查询时使用redis进行缓存
7. 服务器为每一个链接设置cookie 并保存session 将session通过json库序列化之后与cookie形成键值对在redis中缓存

***架构介绍***

1. 主线程通过 epoll 监听多个client 的 socket 以及链接所使用的socket，主线程用于处理新连接 以及client读写任务的分发
2. 线程池接收到主线程分发的任务后 有工作线程取出并执行 解析http请求的主逻辑

~still working on it~
