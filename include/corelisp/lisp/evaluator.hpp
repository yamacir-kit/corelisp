#ifndef INCLUDED_CORELISP_LISP_EVALUATOR_HPP
#define INCLUDED_CORELISP_LISP_EVALUATOR_HPP


#include <functional>
#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include <corelisp/lisp/vectored_cons_cells.hpp>


namespace lisp
{
  class evaluator
    : public std::unordered_map<
               typename vectored_cons_cells::value_type,
               std::function<vectored_cons_cells& (vectored_cons_cells&, vectored_cons_cells::scope_type&)>
             >
  {
    static inline vectored_cons_cells::scope_type root_env_
    {
      {"true", std::make_shared<vectored_cons_cells>(vectored_cons_cells::type::atom, "true")},
      {"false", std::make_shared<vectored_cons_cells>()}
    };

    vectored_cons_cells buffer_;

  public:
    decltype(auto) operator()(const vectored_cons_cells::value_type& s, vectored_cons_cells::scope_type& env = root_env_)
    {
      return operator()(vectored_cons_cells {tokenize(s)}, env);
    }

    auto operator()(vectored_cons_cells&& expr, vectored_cons_cells::scope_type& env = root_env_)
      -> vectored_cons_cells&
    {
      std::swap(buffer_, expr);
      return operator()(buffer_, env);
    }

    auto operator()(vectored_cons_cells& expr, vectored_cons_cells::scope_type& env = root_env_)
      -> vectored_cons_cells& try
    {
      switch (expr.state)
      {
      case vectored_cons_cells::type::atom:
        if (auto iter {env.find(expr.value)}; iter != std::end(env))
        {
          return *(iter->second);
        }
        else return expr;

      case vectored_cons_cells::type::list:
        if (auto iter {find(expr.at(0).value)}; iter != std::end(*this))
        {
          return (iter->second)(expr, env);
        }
        else switch (auto& proc {(*this)(expr[0], env)}; proc.state)
        {
        case vectored_cons_cells::type::list:
          {
            // ステップ１：
            // ラムダ式が定義時に保存した当時の環境（クロージャ）を取り出す
            vectored_cons_cells::scope_type scope {proc.closure};

            // ステップ２：
            // 実引数と仮引数の対応を保存する
            // このとき定義時に同名の変数が存在していた場合は上書きする
            // なぜなら引数のほうが内側のスコープであるためである
            for (std::size_t index {0}; index < std::size(proc.at(1)); ++index)
            {
              scope[proc.at(1).at(index).value] = std::make_shared<vectored_cons_cells>((*this)(expr.at(index + 1), env));
            }

            // ステップ３：
            // 今現在の環境をスコープへ収める
            // このとき定義済みの変数が存在していた場合は上書きしない
            // より外側のスコープの変数であるためである
            for (const auto& each : env)
            {
              scope.emplace(each);
            }

            return (*this)(proc.at(2), scope);
          }

        case vectored_cons_cells::type::atom:
          return (*this)(*(env.at(proc.value)), env);
        }
      }

      std::exit(boost::exit_failure);
    }
    catch (const std::exception& ex)
    {
      std::cerr << "(error " << ex.what() << " \e[31m" << expr << "\e[0m) -> " << std::flush;
      return false_;
    }
  } static evaluate;
} // namespace lisp


#endif // INCLUDED_CORELISP_LISP_EVALUATOR_HPP

