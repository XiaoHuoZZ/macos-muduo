/**
 * 框架内部buffer
 * 用于暂存数据
 */

#ifndef MACOS_MUDUO_BUFFER_H
#define MACOS_MUDUO_BUFFER_H

#include <vector>

namespace muduo::net {

    class Buffer {
    private:
        std::vector<char> buffer_;
        size_t read_idx_;        //指向可读区域
        size_t write_idx_;        //指向可写区域
    public:
        static constexpr size_t kCheapPrepend = 8;       //定义buffer prependable 部分初始大小
        static constexpr size_t kInitialSize = 1024;     //定义buffer writeable 部分初始大小

        /**
         * 创建buffer
         * @param initial_size  初始化大小
         */
        explicit Buffer(size_t initial_size = kInitialSize);

        /**
         * 目前buffer可读数据长度
         * @return
         */
        size_t readableBytes() const { return write_idx_ - read_idx_; }

        /**
         * 目前buffer可写数据长度
         * @return
         */
        size_t writableBytes() const { return buffer_.size() - write_idx_; }

        /**
         * 目前buffer前置部分可插入数据长度
         * @return
         */
        size_t prependableBytes() const { return read_idx_; }

        void swap(Buffer &rhs);

        /**
         * 尽可能读取fd里面的所有数据
         * @param fd
         * @param savedErrno
         * @return
         */
        ssize_t readFd(int fd, int* savedErrno);

        void append(const char* data, size_t len);

        void append(const void* data, size_t len);

        void append(const std::string& str);

        /**
         * 确保能写入len字节数据
         * @param len
         */
        void ensureWritableBytes(size_t len);

        /**
         * 进行内部腾挪/扩容
         * 并确保writable >= len
         * @param len
         */
        void makeSpace(size_t len);

        /**
         * 将writable收缩到reserve大小
         * @param reserve
         */
        void shrink(size_t reserve);

        /**
         * 取出所有数据
         */
        void retrieveAll();

        /**
         * 取出len字节的数据
         * @param len
         */
        void retrieve(size_t len);

        /**
         * 取出所有可读数据作为string
         * @return
         */
        std::string retrieveAllAsString();

        /**
         * 取出len字节数据作为string
         * @param len
         * @return
         */
        std::string retrieveAsString(size_t len);
    };

}

#endif //MACOS_MUDUO_BUFFER_H
