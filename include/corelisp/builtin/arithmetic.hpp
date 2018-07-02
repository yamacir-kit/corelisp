#ifndef INCLUDED_CORELISP_BUILTIN_ARITHMETIC_HPP
#define INCLUDED_CORELISP_BUILTIN_ARITHMETIC_HPP


#include <iterator>
#include <numeric>
#include <type_traits>
#include <utility>

#include <boost/lexical_cast.hpp>

#include <corelisp/lisp/evaluator.hpp>
#include <corelisp/lisp/vectored_cons_cells.hpp>


namespace builtin
{
  template <typename T>
  using add_const_lvalue_reference
    = typename std::add_const<typename std::add_lvalue_reference<T>::type>;


  // XXX 全体的に雑すぎる
  template <typename T, template <typename...> typename BinaryOperator
  , typename = typename std::enable_if<
                          std::is_same<
                            decltype(
                              boost::lexical_cast<T>(
                                std::declval<
                                  typename add_const_lvalue_reference<
                                    lisp::vectored_cons_cells::value_type
                                  >::type
                                >()
                              )
                            ),
                            T
                          >::value
                        >::type
  , typename = typename std::enable_if<
                          std::is_same< // TODO なんか気に入らない
                            decltype(
                              boost::lexical_cast<
                                lisp::vectored_cons_cells::value_type
                              >(std::declval<T>())
                            ),
                            lisp::vectored_cons_cells::value_type
                          >::value
                        >::type>
  class arithmetic
  {
    using cells_type = lisp::vectored_cons_cells;
    using value_type = typename cells_type::value_type;
    using scope_type = typename cells_type::scope_type;

    cells_type buffer_;

  public:
    auto operator()(cells_type& expr, scope_type& scope)
      -> cells_type&
    {
      std::vector<T> args {};

      for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
      {
        args.emplace_back(boost::lexical_cast<T>(lisp::evaluate(*iter, scope).value));
      }

      const auto buffer {std::accumulate(
        std::begin(args) + 1, std::end(args), args.front(), BinaryOperator<T> {}
      )};

      // TODO comparison functions を分離
      if constexpr (std::is_same<typename BinaryOperator<T>::result_type, T>::value)
      {
        return buffer_ = {boost::lexical_cast<value_type>(buffer)};
      }
      else
      {
        return buffer != 0 ? lisp::true_value : lisp::false_value;
      }
    }
  };
} // namespace builtin


#endif // INCLUDED_CORELISP_BUILTIN_ARITHMETIC_HPP

