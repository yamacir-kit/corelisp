#ifndef INCLUDED_PURELISP_CORE_CELL_HPP
#define INCLUDED_PURELISP_CORE_CELL_HPP


#include <iterator>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

#include <purelisp/core/tokenizer.hpp>


namespace purelisp { inline namespace core
{
  template <typename... Ts>
  inline constexpr auto zip_begin(Ts&&... args)
  {
    using namespace boost;
    return make_zip_iterator(boost::make_tuple(std::begin(args)...));
  }

  template <typename... Ts>
  inline constexpr auto zip_end(Ts&&... args)
  {
    using namespace boost;
    return make_zip_iterator(boost::make_tuple(std::end(args)...));
  }

  class cell
    : public std::vector<cell>
  {
  public:
    enum class type { list, atom } state;

    using value_type = std::string;
    value_type value;

    using scope_type = std::unordered_map<std::string, std::shared_ptr<cell>>;
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
        if (*first != "(")
        {
          *this = {type::atom, *first};
        }
        else while (++first != last && *first != ")")
        {
          emplace_back(first, last);
        }
      }
    }

    template <template <typename...> typename SequenceContainer>
    explicit cell(const SequenceContainer<value_type>& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

    template <template <typename...> typename SequenceContainer>
    cell(SequenceContainer<value_type>&& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

    bool operator!=(const cell& rhs) const noexcept
    {
      if (std::size(*this) != std::size(rhs) || (*this).state != rhs.state || (*this).value != rhs.value)
      {
        return true;
      }

      for(auto iter {zip_begin(*this, rhs)}; iter != zip_end(*this, rhs); ++iter)
      {
        if (boost::get<0>(*iter) != boost::get<1>(*iter))
        {
          return true;
        }
      }

      return false;
    }

    bool operator==(const cell& rhs) const noexcept
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
  } static true_ {cell::type::atom, "true"}, false_;
}} // namespace purelisp::core


#endif // INCLUDED_PURELISP_CORE_CELL_HPP

