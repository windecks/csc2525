#pragma once

#include <string>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class MappedFile {
public:
    explicit MappedFile(const std::string &path) {
        fd_ = open(path.c_str(), O_RDONLY);
        if (fd_ == -1) {
            perror(("Error opening file: " + path).c_str());
            return;
        }

        struct stat sb{};
        if (fstat(fd_, &sb) == -1) {
            perror(("Error getting file size: " + path).c_str());
            close(fd_);
            fd_ = -1;
            return;
        }

        if ((size_ = sb.st_size) == 0)
            return;

        data_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
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

    [[nodiscard]] bool is_valid() const { return fd_ != -1 && (size_ == 0 || data_ != nullptr); }
    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] const char *data() const { return static_cast<const char *>(data_); }

    ~MappedFile() {
        if (data_)
            munmap(data_, size_);
        if (fd_ != -1)
            close(fd_);
    }

private:
    int fd_ = -1;
    void *data_ = nullptr;
    size_t size_ = 0;
};
