#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据  Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 */
// 从文件描述符fd读取数据到Buffer，saveErrno保存错误码
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    // 1. 栈上分配64KB临时缓冲区，速度极快，无需malloc
    char extrabuf[65536] = {0};

    // 2. 定义2个iovec结构体，用于readv分散读（最多两块内存）
    struct iovec vec[2];

    // 3. 获取Buffer当前的尾部可写空间大小
    const size_t writable = writableBytes();

    // 4. 第一块内存：Buffer自身的可写区
    vec[0].iov_base = begin() + writerIndex_; // 起始地址 = Buffer起始 + 写指针
    vec[0].iov_len = writable;                // 长度 = 可写空间

    // 5. 第二块内存：栈上临时缓冲区（Buffer写满后用这里）
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf; // 长度64K

    // 6. 决定使用几块内存：Buffer可写空间 <64K → 用2块；否则用1块
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;

    // 7. 调用Linux readv分散读，一次读取所有数据
    const ssize_t n = ::readv(fd, vec, iovcnt);

    // 8. 错误处理：读取失败，保存错误码
    if (n < 0)
    {
        *saveErrno = errno;
    }
    // 9. 情况1：读取的数据量 ≤ Buffer可写空间（数据全写在Buffer里）
    else if (n <= writable)
    {
        writerIndex_ += n; // 只需要移动写指针即可
    }
    // 10. 情况2：数据量超大，Buffer写满，剩余数据写入了extrabuf
    else
    {
        writerIndex_ = buffer_.size(); // 标记Buffer已满
        // 将extrabuf中的剩余数据，追加到Buffer（自动扩容）
        append(extrabuf, n - writable);
    }

    // 11. 返回实际读取的总字节数
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}