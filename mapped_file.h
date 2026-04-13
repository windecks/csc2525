#pragma once

#include <string>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

enum class mode { read, write };

template<mode m>
class MappedFile {
public:
    explicit MappedFile(const std::string &path, const off_t size = 0) {
        fd_ = open(path.c_str(), m == mode::read ? O_RDONLY : O_RDWR | O_CREAT | O_TRUNC);
        if (fd_ == -1) {
            perror(("Error opening file: " + path).c_str());
            return;
        }

        if constexpr (m == mode::write) {
            if (ftruncate(fd_, size) == -1) {
                perror(("Error truncating file: " + path + "to size: " + std::to_string(size)).c_str());
            }
        }

        struct stat sb{};
        if (fstat(fd_, &sb) == -1) {
            perror(("Error getting file size: " + path).c_str());
            close(fd_);
            fd_ = -1;
            return;
        }

        if ((size_ = sb.st_size) == 0 || (m == mode::write && size_ != size))
            return;

        if constexpr (m == mode::read) {
            data_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
            if (data_ != MAP_FAILED)
                madvise(data_, size_, MADV_SEQUENTIAL);
        } else {
            data_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        }

        if (data_ == MAP_FAILED) {
            perror(("Error mapping file: " + path).c_str());
            close(fd_);
            fd_ = -1;
            data_ = nullptr;
        }
    }

    MappedFile(const MappedFile &) = delete;
    MappedFile &operator=(const MappedFile &) = delete;
    MappedFile(MappedFile &&) = delete;
    MappedFile &operator=(MappedFile &&) = delete;

    [[nodiscard]] bool is_valid() const { return fd_ != -1 && data_ != nullptr; }
    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] const char *data() const { return static_cast<const char *>(data_); }
    [[nodiscard]] char *data() {
        if constexpr (m == mode::read)
            static_assert("can't modify in read mode");

        return static_cast<char *>(data_);
    }

    ~MappedFile() {
        if (data_) {
            if constexpr (m == mode::write)
                msync(data_, size_, MS_SYNC);
            munmap(data_, size_);
        }
        if (fd_ != -1)
            close(fd_);
    }

private:
    int fd_ = -1;
    void *data_ = nullptr;
    off_t size_ = 0;
};
