#ifndef INCLUDED_PURELISP_CORE_EVALUATOR_HPP
#define INCLUDED_PURELISP_CORE_EVALUATOR_HPP


#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <corelisp/lisp/cell.hpp>


namespace purelisp { inline namespace core
{
  class evaluator
    : public std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>>
  {
    static inline cell::scope_type root_env
    {
      {"true", std::make_shared<cell>(cell::type::atom, "true")},
      {"false", std::make_shared<cell>()}
    };

    cell buffer_;

  public:
    decltype(auto) operator()(const std::string& s, cell::scope_type& env = root_env)
    {
      return operator()(cell {tokenize(s)}, env);
    }

    cell& operator()(cell&& expr, cell::scope_type& env = root_env)
    {
      std::swap(buffer_, expr);
      return operator()(buffer_, env);
    }

    cell& operator()(cell& expr, cell::scope_type& env = root_env) try
    {
      switch (expr.state)
      {
      case cell::type::atom:
        if (auto iter {env.find(expr.value)}; iter != std::end(env))
        {
          return *(iter->second);
        }
        else return expr;

      case cell::type::list:
        if (auto iter {find(expr.at(0).value)}; iter != std::end(*this))
        {
          return (iter->second)(expr, env);
        }
        else switch (auto& proc {(*this)(expr[0], env)}; proc.state)
        {
        case cell::type::list:
          {
            // ステップ１：
            // ラムダ式が定義時に保存した当時の環境（クロージャ）を取り出す
            cell::scope_type scope {proc.closure};

            // ステップ２：
            // 実引数と仮引数の対応を保存する
            // このとき定義時に同名の変数が存在していた場合は上書きする
            // なぜなら引数のほうが内側のスコープであるためである
            for (std::size_t index {0}; index < std::size(proc.at(1)); ++index)
            {
              scope[proc.at(1).at(index).value] = std::make_shared<cell>((*this)(expr.at(index + 1), env));
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

        case cell::type::atom:
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
}} // namespace purelisp::core


#endif // INCLUDED_PURELISP_CORE_EVALUATOR_HPP

