#ifndef INCLUDED_PURELISP_CORE_EVALUATOR_HPP
#define INCLUDED_PURELISP_CORE_EVALUATOR_HPP


#include <functional>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <stdexcept>

#include <purelisp/core/cell.hpp>


#ifdef VISUALIZE_DEFORMATION_PROCESS
#include <chrono>
#include <sstream>
#include <thread>

#include <boost/scope_exit.hpp>

#include <unistd.h>
#include <sys/ioctl.h>
#endif // VISUALIZE_DEFORMATION_PROCESS


namespace purelisp
{
  class evaluator
    : public std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>>
  {
    static inline cell::scope_type dynamic_scope_ {
      {"true", cell {cell::type::atom, "true"}},
    };

    cell buffer_;

  public:
    evaluator()
      : std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>> {
          {"quote",  &purelisp::evaluator::quote},
          {"lambda", &purelisp::evaluator::lambda},
          {"eq",     &purelisp::evaluator::eq}
        }
    {
      (*this)["cond"] = [this](auto& expr, auto& scope)
        -> decltype(auto)
      {
        return (*this)((*this)(expr.at(1), scope) != nil ? expr.at(2) : expr.at(3), scope);
      };

      (*this)["define"] = [this](auto& expr, auto& scope)
        -> decltype(auto)
      {
        scope.emplace(expr.at(1).value, (*this)(expr.at(2), scope));
        return expr;
      };

      (*this)["atom"] = [this](auto& expr, auto& scope)
        -> decltype(auto)
      {
        const auto& buffer {(*this)(expr.at(1), scope)};
        return buffer.state != cell::type::atom && std::size(buffer) != 0 ? nil : scope["true"];
      };

      (*this)["cons"] = [this](auto& expr, auto& scope)
        -> decltype(auto)
      {
        cell buffer {};

        buffer.push_back((*this)(expr.at(1), scope));

        for (const auto& each : (*this)(expr.at(2), scope))
        {
          buffer.push_back(each);
        }

        return expr = std::move(buffer);
      };

      (*this)["car"] = [this](auto& expr, auto& scope)
        -> decltype(auto)
      {
        return (*this)(expr.at(1), scope).at(0);
      };

      (*this)["cdr"] = [this](auto& expr, auto& scope)
        -> decltype(auto)
      {
        auto buffer {(*this)(expr.at(1), scope)};
        return expr = (std::size(buffer) != 0 ? buffer.erase(std::begin(buffer)), std::move(buffer) : nil);
      };
    }

    decltype(auto) operator()(const std::string& s, cell::scope_type& scope = dynamic_scope_)
    {
      return operator()(cell {s}, scope);
    }

    cell& operator()(cell&& expr, cell::scope_type& scope = dynamic_scope_)
    {
      std::swap(buffer_, expr);
      return operator()(buffer_, scope);
    }

    cell& operator()(cell& expr, cell::scope_type& scope = dynamic_scope_) try
    {
      #ifdef VISUALIZE_DEFORMATION_PROCESS
      BOOST_SCOPE_EXIT_ALL(this) { rewrite_expression(buffer_); };
      #endif // VISUALIZE_DEFORMATION_PROCESS

      switch (expr.state)
      {
      case cell::type::atom:
        if (auto iter {scope.find(expr.value)}; iter != std::end(scope))
        {
          return (*iter).second;
        }
        else
        {
          return expr;
        }

      case cell::type::list:
        if (auto iter {find(expr.at(0).value)}; iter != std::end(*this)) // special forms
        {
          return (*iter).second(expr, scope);
        }

        switch (expr[0] = (*this)(expr[0], scope); expr[0].state) // こいつはムーブしたらダメ
        {
        case cell::type::list:
          for (std::size_t index {0}; index < std::size(expr[0].at(1)); ++index)
          {
            expr.closure.emplace(expr[0][1].at(index).value, (*this)(expr.at(index + 1), scope));
          }

          for (const auto& each : scope) // TODO 既存要素を上書きしないことの確認
          {
            expr.closure.emplace(each);
          }

          return (*this)(expr[0].at(2), expr.closure);

        case cell::type::atom:
          for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
          {
            *iter = (*this)(*iter, scope);
          }

          return (*this)(scope.at(expr[0].value), scope);
        }
      }

      std::exit(boost::exit_failure);
    }
    catch (const std::exception& ex)
    {
      #ifndef NDEBUG
      std::cerr << "(runtime_error " << ex.what() << " \e[31m" << expr << "\e[0m) -> " << std::flush;
      #endif // NDEBUG
      return nil;
    }

  protected:
    static cell& quote(cell& expr, cell::scope_type&) noexcept(false)
    {
      return expr.at(1);
    }

    // TODO 静的スコープモードと動的スコープモードを切り替えられるように
    static cell& lambda(cell& expr, cell::scope_type& scope) noexcept(false)
    {
      expr.closure = scope;
      return expr;
    }

    static cell& eq(cell& expr, cell::scope_type& scope) noexcept(false)
    {
      return expr.at(1) != expr.at(2) ? nil : scope["true"];
    }

  private:
  } static evaluate;


  #ifdef VISUALIZE_DEFORMATION_PROCESS
  void rewrite_expression(cell& expr) const
  {
    static struct winsize window_size;
    ioctl(STDERR_FILENO, TIOCGWINSZ, &window_size);

    static std::size_t previous_row {0};
    if (0 < previous_row)
    {
      for (auto row {previous_row}; 0 < row; --row)
      {
        std::cerr << "\r\e[K\e[A";
      }
    }

    std::stringstream ss {};
    ss << expr;

    previous_row = std::size(ss.str()) / window_size.ws_col;

    if (std::size(ss.str()) % window_size.ws_col == 0)
    {
      --previous_row;
    }

    auto serialized {ss.str()};

    if (auto column {std::size(ss.str()) / window_size.ws_col}; window_size.ws_row < column)
    {
      serialized.erase(0, (column - window_size.ws_row) * window_size.ws_col);
    }

    std::cerr << "\r\e[K" << serialized << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds {10});
  }
  #endif // VISUALIZE_DEFORMATION_PROCESS
} // namespace purelisp


#endif // INCLUDED_PURELISP_CORE_EVALUATOR_HPP

