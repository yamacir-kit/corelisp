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
    lisp::vectored_cons_cells buffer_;

  public:
    auto operator()(lisp::vectored_cons_cells& expr, lisp::vectored_cons_cells::scope_type& scope)
      -> lisp::vectored_cons_cells&
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
        return buffer_ = {lisp::vectored_cons_cells::type::atom, boost::lexical_cast<lisp::vectored_cons_cells::value_type>(buffer)};
      }
      else
      {
        return buffer != 0 ? lisp::true_ : lisp::false_;
      }
    }
  };
} // namespace builtin


#endif // INCLUDED_CORELISP_BUILTIN_ARITHMETIC_HPP

