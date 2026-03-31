#pragma once

#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义
/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size

/*
+-------------------+------------------+------------------+
|  前置区(prepend)  |   可读缓冲区     |   可写缓冲区     |
|  (预留8字节)      |  (已写未读数据)  |  (空闲空间)      |
+-------------------+------------------+------------------+
↑                   ↑                  ↑
0              readerIndex          writerIndex
*/

class Buffer
{
public:
    /*
    32 位系统占 4字节，64 位系统占 8字节；
    */
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char *peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer  释放指定长度的有效数据（读一部分）
    // onMessage string <- Buffer  注释含义：业务层从Buffer取数据
    void retrieve(size_t len)
    {
        // 情况1：要释放的长度 < 有效数据总长度 → 只释放一部分
        if (len < readableBytes())
        {
            // 读指针向后移动len，标记这len字节数据已读、可覆盖
            readerIndex_ += len;
        }
        // 情况2：要释放的长度 ≥ 有效数据 → 直接释放全部
        else // 等价于 len == readableBytes()
        {
            retrieveAll();
        }
    }

    // 释放所有有效数据（读完全部）
    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }

    // 从Buffer读取len长度数据，转为string返回，并释放缓冲区对应空间
    std::string retrieveAsString(size_t len)
    {
        // 从Buffer可读区，取len字节数据，构造string
        std::string result(peek(), len);

        // 数据已经拷贝到string，释放Buffer中的这部分数据
        retrieve(len);

        // 返回构造好的字符串
        return result;
    }

    // buffer_.size() - writerIndex_    len
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }

    // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char *beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin()
    {
        // it.operator*()
        return &*buffer_.begin(); // vector底层数组首元素的地址，也就是数组的起始地址
    }
    const char *begin() const
    {
        return &*buffer_.begin();
    }

    /*
    总空闲空间不足 → 直接扩容（vector resize）
    总空闲空间足够 → 数据前移整理内存（不扩容，复用空闲空间）
    */
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readalbe = readableBytes();
            std::copy(
                begin() + readerIndex_, // 原数据起始：可读区开头
                begin() + writerIndex_, // 原数据结束：可读区结尾
                begin() + kCheapPrepend // 目标位置：8字节头部之后
            );
            readerIndex_ = kCheapPrepend;           // 读指针回到8字节位置
            writerIndex_ = readerIndex_ + readalbe; // 写指针 = 读指针+有效数据长度
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};