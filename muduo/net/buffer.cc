#include "muduo/net/buffer.h"
#include <sys/uio.h>

using muduo::net::Buffer;

Buffer::Buffer(size_t initial_size)
        : buffer_(kCheapPrepend + initial_size),
          read_idx_(kCheapPrepend),
          write_idx_(kCheapPrepend) {
}


void Buffer::makeSpace(size_t len) {
    /**
     * 需要搬运readable数据
     * 将 (xxx代表已经取走的数据)
     * | kCheapPrepend | xxxxxx |  readable |  writeable |
     * 变为
     * | kCheapPrepend | readable |       writeable      |
     */
    if (kCheapPrepend < read_idx_) {
        //将readable 部分数据搬运到xxx的位置
        auto begin = buffer_.begin();
        std::copy(begin + read_idx_,
                  begin + write_idx_,
                  begin + kCheapPrepend);
        read_idx_ = kCheapPrepend;
        write_idx_ = read_idx_ + readableBytes();
    }

    /**
     * 如果writeable不够，扩容
     */
    if (len > writableBytes()) {
        buffer_.resize(write_idx_ + len);   //注意，resize会导致vector二倍去将capacity加倍
    }
}

void Buffer::ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}

void Buffer::swap(Buffer &rhs) {
    using std::swap;
    swap(write_idx_, rhs.write_idx_);
    swap(read_idx_, rhs.read_idx_);
    buffer_.swap(rhs.buffer_);
}

void Buffer::shrink(size_t reserve) {
    /**
     * 为什么要一个新建临时的buffer
     * 这样可以保证prependable的空间回到初始kCheapPrepend，并且shrink后的readable + writable空间至少有kInitialSize
     */
    Buffer other;
    other.ensureWritableBytes(readableBytes() + reserve);
    other.append(buffer_.data() + read_idx_, readableBytes());
    swap(other);
}

void Buffer::append(const char *data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, buffer_.begin() + write_idx_);
    write_idx_ += len;
}

void Buffer::append(const void *data, size_t len) {
    append(static_cast<const char *>(data), len);
}

void Buffer::append(const std::string &str) {
    append(str.data(), str.size());
}

void Buffer::retrieveAll() {
    read_idx_ = kCheapPrepend;
    write_idx_ = kCheapPrepend;
}

void Buffer::retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
        read_idx_ += len;
    } else {
        retrieveAll();        //两个指针归位kCheapPrepend
    }
}

std::string Buffer::retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string tmp(buffer_.data() + read_idx_, len);
    retrieve(len);
    return tmp;
}

std::string Buffer::retrieveAllAsString() {
    return retrieveAsString(readableBytes());
}

ssize_t Buffer::readFd(int fd, int *savedErrno) {
    //栈上临时空间
    char extrabuf[65536];


    /**
     * 申请两段分离式IO空间
     * 第一段是buffer自己的空间，第二段是栈上临时空间
     */
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = buffer_.data() + write_idx_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    //读进iovec
    const ssize_t n = ::readv(fd, vec, 2);

    if (n < 0) {
        *savedErrno = errno;
    }
        //如果只读进buffer
    else if (n <= writable) {
        write_idx_ += n;
    } else {
        write_idx_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

void Buffer::prepend(const void *data, size_t len) {
    assert(len <= prependableBytes());
    read_idx_ -= len;
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, buffer_.data() + read_idx_);
}



