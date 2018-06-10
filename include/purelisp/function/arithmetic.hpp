#ifndef INCLUDED_PURELISP_FUNCTION_ARITHMETIC_HPP
#define INCLUDED_PURELISP_FUNCTION_ARITHMETIC_HPP


#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>

#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <purelisp/core/cell.hpp>
#include <purelisp/core/evaluator.hpp>


namespace purelisp::arithmetic
{
  template <typename T, template <typename...> typename BinaryOperator,
            typename = typename std::enable_if<
                                  std::is_constructible<
                                    T, typename std::add_lvalue_reference<cell::value_type>::type
                                  >::value
                                >::type,
            typename = typename std::enable_if<
                                  std::is_same< // TODO なんか気に入らない
                                    decltype(boost::lexical_cast<cell::value_type>(std::declval<T>())),
                                    cell::value_type
                                  >::value
                                >::type>
  class function
  {
    cell buffer_;

  public:
    cell& operator()(cell& expr, cell::scope_type& scope)
    {
      std::vector<T> args {};

      // TODO ここで評価するのか、呼ぶ前に評価するのか考えなおすこと
      for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
      {
        args.emplace_back(evaluate(*iter, scope).value);
      }

      const auto buffer {std::accumulate(
        std::begin(args) + 1, std::end(args), args.front(), BinaryOperator<T> {}
      )};

      // TODO comparison functions を分離
      if constexpr (std::is_same<typename BinaryOperator<T>::result_type, T>::value)
      {
        return buffer_ = {cell::type::atom, boost::lexical_cast<cell::value_type>(buffer)};
      }
      else
      {
        return buffer != 0 ? true_ : false_;
      }
    }
  };


  template <typename T>
  class [[deprecated]] numeric_type
  {
    std::string data_;

  public:
    using value_type = T;
    value_type value;

  public:
    numeric_type(value_type value)
      : data_ {boost::lexical_cast<decltype(data_)>(value)},
        value {value}
    {}

    numeric_type(const std::string& data)
      : data_ {data},
        value {boost::lexical_cast<value_type>(data_)}
    {}

    const auto& str() const noexcept
    {
      return data_;
    }

    // XXX 多分こいつのおかげでlexical-castable
    operator value_type() const noexcept
    {
      return value;
    }
  };
} // namespace purelisp::arithmetic


#endif // INCLUDED_PURELISP_FUNCTION_ARITHMETIC_HPP

