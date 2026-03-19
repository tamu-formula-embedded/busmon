#ifndef _F_STR_
#define _F_STR_

#include <array>
#include <cstring>

// TODO MORE STUFF BRO

template <std::size_t N>
class FixedString
{
  std::array<char, N> data_{};
  std::size_t len_{0};

public:
    FixedString() = default;

    FixedString(const char* str) : len_{std::min(std::strlen(str), N - 1)}
    {
        std::memcpy(data_.data(), str, len_);
        data_[len_] = '\0';
    }

    FixedString(const FixedString&) = default;
    FixedString& operator=(const FixedString&) = default;

    // cross-size copy constructor
    template <std::size_t M>
    FixedString(const FixedString<M>& other) : len_{std::min(other.size(), N - 1)}
    {
        std::memcpy(data_.data(), other.c_str(), len_);
        data_[len_] = '\0';
    }

    // cross-size copy assign
    template <std::size_t M>
    FixedString& operator=(const FixedString<M>& other)
    {
        len_ = std::min(other.size(), N - 1);
        std::memcpy(data_.data(), other.c_str(), len_);
        data_[len_] = '\0';
        return *this;
    }

    const char* c_str() const {
        return data_.data();
    }
    std::size_t size()  const {
        return len_;
    }
    static constexpr std::size_t capacity() {
        return N - 1;
    }

    char  operator[](std::size_t i) const {
        return data_[i];
    }
    char& operator[](std::size_t i) {
        return data_[i];
    }
};
#endif