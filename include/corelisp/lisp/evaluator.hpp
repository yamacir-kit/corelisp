#ifndef INCLUDED_CORELISP_LISP_EVALUATOR_HPP
#define INCLUDED_CORELISP_LISP_EVALUATOR_HPP


#include <functional>
#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <boost/cstdlib.hpp>

#include <corelisp/lisp/vectored_cons_cells.hpp>


namespace lisp
{
  class evaluator
    : public std::unordered_map<
               typename vectored_cons_cells::value_type,
               std::function<vectored_cons_cells& (vectored_cons_cells&, vectored_cons_cells::scope_type&)>
             >
  {
    using cells_type = vectored_cons_cells;
    using value_type = typename cells_type::value_type;
    using scope_type = typename cells_type::scope_type;

    static inline scope_type env_
    {
      {"true",  std::make_shared<cells_type>("true")},
      {"false", std::make_shared<cells_type>()}
    };

  public:
    decltype(auto) operator()(const value_type& s, scope_type& env = env_)
    {
      return operator()(cells_type {tokenize(s)}, env);
    }

    auto operator()(cells_type&& e, scope_type& env = env_)
      -> cells_type&
    {
      auto buffer {e};
      return operator()(buffer, env);
    }

    auto operator()(cells_type& e, scope_type& env = env_)
      -> cells_type& try
    {
      if (e.is_atom())
      {
        if (auto iter {env.find(e.value)}; iter != std::end(env))
        {
          return *(iter->second);
        }
        else return e;
      }
      else
      {
        if (auto iter {find(e.at(0).value)}; iter != std::end(*this))
        {
          return (iter->second)(e, env);
        }
        else if (auto& proc {(*this)(e[0], env)}; proc.is_atom())
        {
          return (*this)(*(env.at(proc.value)), env);
        }
        else
        {
          scope_type scope {proc.closure};

          for (std::size_t index {0}; index < std::size(proc.at(1)); ++index)
          {
            scope[proc.at(1).at(index).value] = std::make_shared<cells_type>((*this)(e.at(index + 1), env));
          }

          for (const auto& each : env) // XXX shared_ptrをコピーせずに直接構築してる説あり
          {
            scope.emplace(each);
          }

          return (*this)(proc.at(2), scope);
        }
      }

      std::exit(boost::exit_failure);
    }
    catch (const std::exception& ex)
    {
      std::cerr << "(error " << ex.what() << " in expression \e[31m" << e << "\e[0m) -> " << std::flush;
      return false_;
    }
  } static evaluate;
} // namespace lisp


#endif // INCLUDED_CORELISP_LISP_EVALUATOR_HPP

