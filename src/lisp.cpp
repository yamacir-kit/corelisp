#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/cstdlib.hpp>


#define debug(...) std::cerr << "(\e[32mdebug \e[31m" << expression << " \e[36m" << __LINE__ << "\e[0m) -> ";
#define error(...) std::cerr << "(\e[32merror \e[31m" << expression << " \e[36m" << __LINE__ << "\e[0m)" << std::endl; std::exit(boost::exit_failure);


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
          category = expression_category::list;
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

    virtual ~cell() = default;

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


  template <typename Function>
  class procedure
    : public Function,
      public cell
  {
  public:
    template <typename... Ts>
    procedure(Ts&&... args)
      : Function {std::forward<Ts>(args)...},
        cell {cell::expression_category::atom, "procedure"}
    {}

    virtual ~procedure() = default;
  };


  // プロシージャ定義のためのヘルパ関数。引数には関数オブジェクトを与える。
  template <typename... Ts>
  constexpr auto make_procedure(const std::string& token, Ts&&... args)
    -> procedure<typename std::decay<Ts>::type...>
  {
    return {std::forward<Ts>(args)...};
  }


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


  template <typename Cell>
  auto& evaluate(Cell&& expression, std::map<std::string, lisp::cell>& scope)
  {
    using category = lisp::cell::expression_category;

    switch (expression.category)
    {
    case category::atom:
      debug();
      try
      {
        std::stod(expression.value);
      }
      catch (const std::logic_error&)
      {
        return scope[expression.value];
      }
      break;

    case category::list:
      if (std::empty(expression))
      {
        debug();
        expression.category = category::atom;
        expression.value = "nil";
        break;
      }

      if (expression[0].value == "quote")
      {
        debug();
        return std::size(expression) < 2 ? scope["nil"] : expression[1];
      }
      else if (expression[0].value == "cond")
      {
        debug();
        while (std::size(expression) < 4)
        {
          expression.emplace_back();
        }
        return evaluate(evaluate(expression[1], scope).value == "true" ? expression[2] : expression[3], scope);
      }
      else if (expression[0].value == "define")
      {
        debug();
        return scope[expression[1].value] = evaluate(expression[2], scope);
      }
      else if (expression[0].value == "lambda")
      {
        debug();
        expression.closure = scope;
        return expression;
      }

      debug();
      // XXX ここで組み込みプロシージャに対してユーザ定義変数か否かの問い合わせが発生している
      expression[0] = evaluate(expression[0], scope);

      switch (expression[0].category)
      {
      case category::list:
        debug();
        for (std::size_t index {0}; index < std::size(expression[0][1]); ++index)
        {
          expression.closure[expression[0][1][index].value] = evaluate(expression[index + 1], scope);
        }

        if (expression[0][0].value == "lambda")
        {
          return evaluate(expression[0][2], expression.closure);
        }
        else error();

      case category::atom:
        debug();

        using function_type = std::function<cell& (cell&, cell::scope_type&)>;
        if (auto* proc {dynamic_cast<procedure<function_type>*>(&expression[0])}; proc != nullptr)
        {
          return (*proc)(expression, scope);
        }
        else error();

        // if (expression[0].value == "+")
        // {
        //   double buffer {0.0};
        //   for (auto iter {std::begin(expression) + 1}; iter != std::end(expression); ++iter)
        //   {
        //     buffer += std::stod(evaluate(*iter, expression.closure).value);
        //   }
        //
        //   expression.category = category::atom;
        //   expression.value = std::to_string(buffer);
        //
        //   return expression;
        // }
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
    {"false", {category::atom, "false"}}
  };

  scope["+"] = lisp::make_procedure("+", [](auto& expression, auto& scope) -> decltype(auto)
  {
    debug();
    double buffer {0.0};
    for (auto iter {std::begin(expression) + 1}; iter != std::end(expression); ++iter)
    {
      buffer += std::stod(evaluate(*iter, expression.closure).value);
    }
    debug();

    expression.category = lisp::cell::expression_category::atom;
    expression.value = std::to_string(buffer);

    debug();

    return expression;
  });

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    const auto tokens {lisp::tokenize(buffer)};
    std::cout << "[" << std::size(history) << "]> " << lisp::evaluate(lisp::cell {std::begin(tokens), std::end(tokens)}, scope) << std::endl;
  }

  return boost::exit_success;
}

