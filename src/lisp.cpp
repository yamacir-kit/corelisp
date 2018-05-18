#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <boost/cstdlib.hpp>


#define debug(...) std::cerr << "(\e[32mdebug \e[31m" << expr << " \e[36m" << __LINE__ << "\e[0m) -> ";
#define error(...) std::cerr << "(\e[32merror \e[31m" << expr << " \e[36m" << __LINE__ << "\e[0m)" << std::endl; std::exit(boost::exit_failure);


namespace lisp
{
  class cell
    : public std::vector<cell>
  {
  public:
    enum class expr_category { list, atom } category;

    std::string value;

    using scope_type = std::map<std::string, lisp::cell>;
    scope_type closure;

  public:
    cell(expr_category category = expr_category::list, const std::string& value = "")
      : category {category}, value {value}
    {}

    template <typename InputIterator>
    cell(InputIterator&& first, InputIterator&& last)
      : category {expr_category::list}
    {
      if (std::distance(first, last) != 0)
      {
        if (*first == "(")
        {
          while (++first != last && *first != ")")
          {
            emplace_back(first, last);
          }
        }
        else
        {
          *this = {expr_category::atom, *first};
        }
      }
    }

  public:
    friend auto operator<<(std::ostream& ostream, const cell& expr)
      -> std::ostream&
    {
      switch (expr.category)
      {
      case expr_category::list:
        ostream <<  "(";
        for (const auto& each : expr)
        {
          ostream << each << (&each != &expr.back() ? " " : "");
        }
        return ostream << ")";

      default:
        return ostream << expr.value;
      }
    }
  };


  auto tokenize(const std::string& code)
  {
    std::vector<std::string> tokens {};

    auto isparen = [](auto c) constexpr
    {
      return c == '(' or c == ')';
    };

    auto find_graph = [&](auto iter) constexpr
    {
      return std::find_if(iter, std::end(code), [](auto c) { return std::isgraph(c); });
    };

    for (auto iter {find_graph(std::begin(code))};
         iter != std::end(code);
         iter = find_graph(iter += std::size(tokens.back())))
    {
      tokens.emplace_back(
        iter,
        isparen(*iter) ? iter + 1
                       : std::find_if(iter, std::end(code), [&](auto c)
                         {
                           return isparen(c) || std::isspace(c);
                         })
      );
    }

    return tokens;
  }


  template <typename Cell>
  auto evaluate(Cell&& expr, std::map<std::string, cell>& scope)
    -> typename std::remove_reference<Cell>::type&
  {
    using category = cell::expr_category;

    switch (expr.category)
    {
    case category::atom:
      // 未定義変数はセルをオウム返しする。
      // 非数値かつ未定義変数がプロシージャに投入された時の例外処理を追加すること
      return scope.find(expr.value) != std::end(scope) ? scope[expr.value] : expr;

    case category::list:
      // 空のリストはNILに評価される
      if (std::empty(expr))
      {
        return expr = {category::atom, "nil"};
      }

      if (expr[0].value == "quote")
      {
        return std::size(expr) < 2 ? scope["nil"] : expr[1];
      }

      if (expr[0].value == "cond")
      {
        while (std::size(expr) < 4)
        {
          expr.emplace_back();
        }
        return evaluate(evaluate(expr[1], scope).value == "true" ? expr[2] : expr[3], scope);
      }

      if (expr[0].value == "define")
      {
        return scope[expr[1].value] = evaluate(expr[2], scope);
      }

      if (expr[0].value == "lambda")
      {
        return expr;
      }

      expr[0] = evaluate(expr[0], scope);
      expr.closure = scope;

      switch (expr[0].category)
      {
      case category::list:
        for (std::size_t index {0}; index < std::size(expr[0][1]); ++index)
        {
          expr.closure[expr[0][1][index].value] = evaluate(expr[index + 1], scope);
        }

        if (expr[0][0].value == "lambda")
        {
          return evaluate(expr[0][2], expr.closure);
        }
        else error();

      case category::atom:
        for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
        {
          *iter = evaluate(*iter, scope);
        }

        if (expr[0].value == "+") try
        {
          const auto buffer {std::accumulate(std::begin(expr) + 1, std::end(expr), 0.0,
            [](auto lhs, auto rhs)
            {
              return lhs + std::stod(rhs.value);
            }
          )};

          expr.category = category::atom;
          expr.value = std::to_string(buffer);

          return expr;
        }
        catch (std::invalid_argument&)
        {
          std::cerr << "An undefined symbol or a non-numeric token is passed to procedure \"+\"." << std::endl;
          return scope["nil"];
        }

        error();
      }
    }

    return expr;
  }
} // namespace lisp


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  lisp::cell::scope_type scope {};

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    const auto tokens {lisp::tokenize(buffer)};
    std::cout << lisp::evaluate(lisp::cell {std::begin(tokens), std::end(tokens)}, scope) << std::endl;
  }

  return boost::exit_success;
}

