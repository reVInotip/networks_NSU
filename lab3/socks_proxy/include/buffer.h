#include <memory>
#include <cstring>
#pragma once

class Buffer final {
    private:
        std::shared_ptr<unsigned char[]> buffer_;
        size_t buffer_capacity_;
    
    public:
        size_t buffer_size_;
    
    public:
        Buffer(size_t cap): buffer_ {new unsigned char [cap]}, buffer_capacity_ {cap} {}
        void clear() {buffer_size_ = 0;}
        size_t capacity() {return buffer_capacity_;}
        void fill(unsigned char data) {memset(buffer_.get(), data, buffer_capacity_);}
        unsigned char *data() {return buffer_.get();}
        bool empty() {return buffer_size_ == 0;}
    
    public:
        unsigned char operator[](unsigned index) const {
            return buffer_.get()[index];
        }

        unsigned char& operator[](unsigned index) {
            buffer_size_ = index + 1 > buffer_size_ ? index + 1 : buffer_size_;
            return buffer_.get()[index];
        }
};