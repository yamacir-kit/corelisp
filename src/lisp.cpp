#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include <boost/cstdlib.hpp>


namespace lisp
{
  class cell
    : public std::vector<cell>
  {
  public:
    enum class expression_category { list, atom } category;

    std::string value;

    using scope_type = std::map<std::string, lisp::cell>;
    scope_type closure;

  public:
    cell(expression_category category = expression_category::list, const std::string& value = "")
      : category {category}, value {value}
    {}

    template <typename InputIterator>
    cell(InputIterator&& first, InputIterator&& last)
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
          *this = {expression_category::atom, *first};
        }
      }
    }

  public:
    friend auto operator<<(std::ostream& ostream, const lisp::cell& expression)
      -> std::ostream&
    {
      switch (expression.category)
      {
      case expression_category::list:
        ostream <<  "(";
        for (const auto& each : expression)
        {
          ostream << each << (&each != &expression.back() ? " " : "");
        }
        return ostream << ")";

      default:
        return ostream << expression.value;
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
                           return isparen(c) or std::isspace(c);
                         })
      );
    }

    return tokens;
  }

  template <typename CellType>
  auto& evaluate(CellType&& expression, std::map<std::string, lisp::cell>& scope)
  {
    using category = lisp::cell::expression_category;

    switch (expression.category)
    {
    case category::atom:
      try
      {
        std::stod(expression.value);
      }
      catch (const std::logic_error&) // 数値への変換に失敗する、つまり数字ではないシンボル
      {
        return scope[expression.value];
      }
      break;

    case category::list:
      if (std::empty(expression)) // 空のリストは nil に評価される
      {
        expression.category = category::atom;
        expression.value = "nil";
        break;
      }

      if (expression[0].value == "quote")
      {
        return std::size(expression) < 2 ? scope["nil"] : expression[1];
      }
      else if (expression[0].value == "cond")
      {
        while (std::size(expression) < 4)
        {
          expression.emplace_back();
        }
        return evaluate(evaluate(expression[1], scope).value == "true" ? expression[2] : expression[3], scope);
      }
      else if (expression[0].value == "define")
      {
        return scope[expression[1].value] = evaluate(expression[2], scope);
      }
      else if (expression[0].value == "lambda")
      {
        return expression;
      }

      expression[0] = evaluate(expression[0], scope);
      expression.closure = scope;

      switch (expression[0].category)
      {
      case category::list:
        for (auto parameter {std::begin(expression[0][1])}, argument {std::begin(expression) + 1};
             parameter != std::end(expression[0][1]) && argument != std::end(expression);
             ++parameter, ++argument)
        {
          expression.closure[(*parameter).value] = evaluate(*argument, scope);
        }

        if (expression[0][0].value == "lambda")
        {
          return evaluate(expression[0][2], expression.closure);
        }
        break;

      case category::atom:
        if (expression[0].value == "+")
        {
          double buffer {0.0};
          for (auto iter {std::begin(expression) + 1}; iter != std::end(expression); ++iter)
          {
            buffer += std::stod(evaluate(*iter, expression.closure).value);
          }

          expression.category = category::atom;
          expression.value = std::to_string(buffer);

          return expression;
        }
        break;
      }
      break;
    }

    return expression;
  }
} // namespace lisp


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  using category = lisp::cell::expression_category;
  lisp::cell::scope_type scope
  {
    {"nil",   {category::atom, "nil"}},
    {"true",  {category::atom, "true"}},
    {"false", {category::atom, "false"}},
    {"+",     {category::atom, "+"}}
  };

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    const auto tokens {lisp::tokenize(buffer)};
    lisp::cell expression {std::begin(tokens), std::end(tokens)};

    std::cout << "[" << std::size(history) << "]> " << lisp::evaluate(expression, scope) << std::endl;
  }

  return boost::exit_success;
}


