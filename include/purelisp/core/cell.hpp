#ifndef INCLUDED_PURELISP_CORE_CELL_HPP
#define INCLUDED_PURELISP_CORE_CELL_HPP


#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

#include <purelisp/core/tokenizer.hpp>


namespace purelisp
{
  class cell
    : public std::vector<cell>
  {
  public:
    enum class type { list, atom } state;

    using value_type = std::string;
    value_type value;

    // TODO コンテナの再選定
    using scope_type = boost::container::flat_map<std::string, cell>;
    scope_type closure;

  public:
    cell(type state = type::list, const std::string& value = "")
      : state {state}, value {value}
    {}

    template <typename InputIterator,
              typename = typename std::enable_if<
                                    std::is_nothrow_constructible<
                                      decltype(value), typename std::remove_reference<InputIterator>::type::value_type
                                    >::value
                                  >::type>
    cell(InputIterator&& first, InputIterator&& last)
      : state {type::list}
    {
      if (std::distance(first, last) != 0)
      {
        if (*first == "(")
        {
          while (++first != last && *first != ")")
          {
            emplace_back(first, last);
          }
        }
        else
        {
          *this = {type::atom, *first};
        }
      }
    }

    explicit cell(const tokenizer& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

    cell(tokenizer&& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

    bool operator!=(const cell& rhs) const noexcept
    {
      if (std::size(*this) != std::size(rhs) || (*this).state != rhs.state || (*this).value != rhs.value)
      {
        return true;
      }

      static auto zip_begin = [](auto&&... args) constexpr
      {
        return boost::make_zip_iterator(boost::make_tuple(std::begin(args)...));
      };

      static auto zip_end = [](auto&&... args) constexpr
      {
        return boost::make_zip_iterator(boost::make_tuple(std::end(args)...));
      };

      for(auto iter {zip_begin(*this, rhs)}; iter != zip_end(*this, rhs); ++iter)
      {
        if (boost::get<0>(*iter) != boost::get<1>(*iter))
        {
          return true;
        }
      }

      return false;
    }

    bool operator==(const cell& rhs)
    {
      return !(*this != rhs);
    }

    friend auto operator<<(std::ostream& os, const cell& expr)
      -> std::ostream&
    {
      switch (expr.state)
      {
      case type::list:
        os <<  '(';
        for (const auto& each : expr)
        {
          os << each << (&each != &expr.back() ? " " : "");
        }
        return os << ')';

      default:
        return os << expr.value;
      }
    }
  };
} // namespace purelisp


#endif // INCLUDED_PURELISP_CORE_CELL_HPP

