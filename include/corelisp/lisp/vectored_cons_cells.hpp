#ifndef INCLUDED_CORELISP_LISP_VECTORED_CONS_CELLS_HPP
#define INCLUDED_CORELISP_LISP_VECTORED_CONS_CELLS_HPP


#include <iterator>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <corelisp/lisp/tokenizer.hpp>
#include <corelisp/utility/zip_iterator.hpp>


namespace lisp
{
  class vectored_cons_cells
    : public std::vector<vectored_cons_cells>
  {
  public: // data members
    enum class type { list, atom } state; // TODO deprecated

    using value_type = std::string;
    value_type value;

    using scope_type = std::unordered_map<std::string, std::shared_ptr<vectored_cons_cells>>;
    scope_type closure;

  public: // constructors
    vectored_cons_cells(type state = type::list, const std::string& value = "")
      : state {state}, value {value}
    {}

    template <typename InputIterator,
              typename = typename std::enable_if<
                                    std::is_nothrow_constructible<
                                      decltype(value), typename std::remove_reference<InputIterator>::type::value_type
                                    >::value
                                  >::type>
    vectored_cons_cells(InputIterator&& first, InputIterator&& last)
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
    explicit vectored_cons_cells(const SequenceContainer<value_type>& tokens)
      : vectored_cons_cells {std::begin(tokens), std::end(tokens)}
    {}

    template <template <typename...> typename SequenceContainer>
    vectored_cons_cells(SequenceContainer<value_type>&& tokens)
      : vectored_cons_cells {std::begin(tokens), std::end(tokens)}
    {}

  public: // operators
    bool operator!=(const vectored_cons_cells& rhs) const noexcept
    {
      if (std::size(*this) != std::size(rhs) || (*this).state != rhs.state || (*this).value != rhs.value)
      {
        return true;
      }

      for(auto iter {utility::zip_begin(*this, rhs)}; iter != utility::zip_end(*this, rhs); ++iter)
      {
        if (boost::get<0>(*iter) != boost::get<1>(*iter))
        {
          return true;
        }
      }

      return false;
    }

    bool operator==(const vectored_cons_cells& rhs) const noexcept
    {
      return !(*this != rhs);
    }

    friend auto operator<<(std::ostream& os, const vectored_cons_cells& e)
      -> std::ostream&
    {
      switch (e.state)
      {
      case type::list:
        os <<  '(';
        for (const auto& each : e)
        {
          os << each << (&each != &e.back() ? " " : "");
        }
        return os << ')';

      default:
        return os << e.value;
      }
    }
  } static true_ {vectored_cons_cells::type::atom, "true"}, false_;
}


#endif // INCLUDED_CORELISP_LISP_VECTORED_CONS_CELLS_HPP

