#ifndef INCLUDED_PURELISP_CORE_EVALUATOR_HPP
#define INCLUDED_PURELISP_CORE_EVALUATOR_HPP


#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <purelisp/core/cell.hpp>


namespace purelisp { inline namespace core
{
  class evaluator
    : public std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>>
  {
    static inline cell::scope_type dynamic_scope_ {
      {"true", true_}, {"false", false_}
    };

    cell buffer_;

  public:
    evaluator()
      : std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>> {
          {"quote",  &purelisp::core::evaluator::quote},
          {"lambda", &purelisp::core::evaluator::lambda},
          {"eq",     &purelisp::core::evaluator::eq}
        }
    {
      (*this)["if"] = [this](auto& expr, auto& scope)
        -> decltype(auto)
      {
        return (*this)((*this)(expr.at(1), scope) != false_ ? expr.at(2) : expr.at(3), scope);
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
        return buffer.state != cell::type::atom && std::size(buffer) != 0 ? false_ : true_;
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
        return expr = (std::size(buffer) != 0 ? buffer.erase(std::begin(buffer)), std::move(buffer) : false_);
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

        switch (auto& function {(*this)(expr[0], scope)}; function.state)
        {
        case cell::type::list:
          {
            cell::scope_type closure {};

            for (std::size_t index {0}; index < std::size(function.at(1)); ++index)
            {
              closure[function.at(1).at(index).value] = (*this)(expr.at(index + 1), scope);
            }

            for (const auto& each : scope)
            {
              closure.emplace(each);
            }

            return (*this)(function.at(2), closure);
          }

        case cell::type::atom:
          return (*this)(scope.at(function.value), scope);
        }
      }

      std::exit(boost::exit_failure);
    }
    catch (const std::exception& ex)
    {
      std::cerr << "(error " << ex.what() << " \e[31m" << expr << "\e[0m) -> " << std::flush;
      return false_;
    }

  protected:
    static cell& quote(cell& expr, cell::scope_type&) noexcept(false)
    {
      return expr.at(1);
    }

    static cell& lambda(cell& expr, cell::scope_type& scope) noexcept(false)
    {
      expr.closure = scope;
      return expr;
    }

    static cell& eq(cell& expr, cell::scope_type&) noexcept(false)
    {
      return expr.at(1) != expr.at(2) ? false_ : true_;
    }
  } static evaluate;
}} // namespace purelisp::core


#endif // INCLUDED_PURELISP_CORE_EVALUATOR_HPP

