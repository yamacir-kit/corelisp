#ifndef INCLUDED_CORELISP_UTILITY_SUBRANGE_VECTOR_HPP
#define INCLUDED_CORELISP_UTILITY_SUBRANGE_VECTOR_HPP


#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>


namespace utility
{
  // サブレンジベクタ。`std::string_view`に設計が似ているが値の書き換えを制限していない点で異なる
  template <typename T>
  class subrange_vector
  {
  public: // member types
    using size_type = typename std::vector<T>::size_type;
    using difference_type = typename std::vector<T>::difference_type;
    using value_type = typename std::vector<T>::value_type;
    using allocator_type  = typename std::vector<T>::allocator_type;

    using       reference = typename std::vector<T>::reference;
    using const_reference = typename std::vector<T>::const_reference;

    using       pointer = typename std::vector<T>::const_pointer;
    using const_pointer = typename std::vector<T>::const_pointer;

    using       iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    using       reverse_iterator = typename std::vector<T>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;

  private: // private attributes
    const_pointer data_;
    size_type size_;

  public: // constructors and destructors
    constexpr subrange_vector() noexcept
      : data_ {nullptr},
        size_ {0}
    {}

    constexpr subrange_vector(const subrange_vector<T>& view) noexcept
      : data_ {view.data_},
        size_ {view.size_}
    {}

    template <typename Allocator>
    subrange_vector(const std::vector<T, Allocator>& vector) noexcept
      : data_ {vector.data()},
        size_ {vector.size()}
    {}

    constexpr subrange_vector(const_pointer data, size_type size) noexcept
      : data_ {data},
        size_ {size}
    {}

  public: // member accesses
    constexpr auto size() const noexcept
    {
      return size_;
    }

    constexpr auto max_size() const noexcept
    {
      return size_;
    }

    constexpr bool empty() const noexcept
    {
      return !size_;
    }

    constexpr auto operator[](size_type pos) noexcept
      -> reference
    {
      return data_[pos];
    }

    constexpr auto at(size_type pos) const noexcept(false)
      -> const_reference
    {
      return size_ <= pos ? throw std::out_of_range {""} : data_[pos];
    }

    constexpr auto data() const noexcept
      -> const_pointer
    {
      return data_;
    }

    constexpr auto front() const noexcept
      -> const_reference
    {
      return data_[0];
    }

    constexpr auto back() const noexcept
      -> const_reference
    {
      return data_[size_ - 1];
    }

  public: // operations
    void clear() noexcept
    {
      *this = subrange_vector {};
    }

    void swap(subrange_vector<T>& rhs) noexcept
    {
      std::swap(data_, rhs.data_);
      std::swap(size_, rhs.size_);
    }

    template <auto NothrowChangable = true>
    auto remove_prefix() noexcept(NothrowChangable)
    {
      if constexpr (NothrowChangable)
      {
        ++data_;
        --size_;
        return *this;
      }
      else
      {
        if (0 < (*this).size())
        {
          ++data_;
          --size_;
          return *this;
        }
        else
        {
          throw std::out_of_range {"subrange_vector::remove_prefix() - out_of_range"};
        }
      }
    }

    template <auto NothrowChangable = true>
    auto remove_suffix() noexcept(NothrowChangable)
    {
      if constexpr (NothrowChangable)
      {
        --size_;
        return *this;
      }
      else
      {
        if (!(*this).empty())
        {
          --size_;
          return *this;
        }
        else
        {
          throw std::out_of_range {"subrange_vector::remove_prefix() - out_of_range"};
        }
      }
    }

  public: // iterators
    constexpr auto begin() const noexcept
      -> const_iterator
    {
      return (*this).cbegin();
    }

    constexpr auto end() const noexcept
      -> const_iterator
    {
      return (*this).cend();
    }

    constexpr auto cbegin() const noexcept
      -> const_iterator
    {
      return const_iterator {data_};
    }

    constexpr auto cend() const noexcept
      -> const_iterator
    {
      return const_iterator {data_ + size_};
    }

    auto rbegin() const noexcept
      -> const_reverse_iterator
    {
      return (*this).crbegin();
    }

    auto rend() const noexcept
      -> const_reverse_iterator
    {
      return (*this).crend();
    }

    auto crbegin() const noexcept
      -> const_reverse_iterator
    {
      return {(*this).end()};
    }

    auto crend() const noexcept
      -> const_reverse_iterator
    {
      return {(*this).begin()};
    }

  public: // operators
    auto& operator=(const subrange_vector<T>& rhs) noexcept
    {
      data_ = rhs.data_;
      size_ = rhs.size_;
      return *this;
    }

    template <typename Allocator>
    explicit operator std::vector<T, Allocator>() const
    {
      return {(*this).begin(), (*this).end()};
    }

    friend auto operator==(subrange_vector<T>& lhs, subrange_vector<T>& rhs)
    {
      return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
    }

    friend auto operator!=(subrange_vector<T>& lhs, subrange_vector<T>& rhs)
    {
      return !(lhs == rhs);
    }

    friend auto operator<(subrange_vector<T>& lhs, subrange_vector<T>& rhs)
    {
      return std::lexicographical_compare(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
    }

    friend auto operator>(subrange_vector<T>& lhs, subrange_vector<T>& rhs)
    {
      return !(lhs < rhs);
    }

    friend auto operator<=(subrange_vector<T>& lhs, subrange_vector<T>& rhs)
    {
      return (lhs < rhs) or (lhs == rhs);
    }

    friend auto operator>=(subrange_vector<T>& lhs, subrange_vector<T>& rhs)
    {
      return (lhs > rhs) or (lhs == rhs);
    }
  };
} // namespace utility


#endif // INCLUDED_CORELISP_UTILITY_SUBRANGE_VECTOR_HPP

