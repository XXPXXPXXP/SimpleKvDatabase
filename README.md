# Simple Kv datas
这是使用epoll的一个分支，写的很菜，请大佬勿喷。  
客户端的设置都放在`settings.h`里面了，有需要可以直接修改。  
在Linux arm64上面开发的，不知道在其他架构上会不会有bug。
> 当前分支重构了整个模型，在高并发的时候会有较少的性能开销，并且也能保证连接接收的及时性

**具体架构：**  

