#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <iterator>
#include <locale>
#include <map>
#include <numeric>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>


#define highwrite(CELL) std::cerr << expr.highlight(CELL) << " " << __LINE__ << std::endl


namespace lisp::regex
{
  auto escape_regex_specials(const std::string& s)
    -> std::string
  {
    auto buffer {s};

    static const std::vector<std::string> regex_specials // 多分足りない
    {
      "\\", "$", "(", ")", "*", "+", ".", "[", "]", "?", "^", "{", "}", "|"
    };

    for (const auto& special : regex_specials)
    {
      for (auto pos {buffer.find(special)}; pos != std::string::npos; pos = buffer.find(special, pos + 2))
      {
        buffer.replace(pos, 1, std::string {"\\"} + special);
      }
    }

    return buffer;
  }
} // namespace lisp::regex


namespace lisp
{
  class tokenizer
    : public std::vector<std::string>
  {
  public:
    tokenizer() = default;

    template <typename... Ts>
    tokenizer(Ts&&... args)
    {
      (*this).operator()(std::forward<Ts>(args)...);
    }

    auto& operator()(const std::string& s)
    {
      if (!std::empty(*this))
      {
        clear();
      }

      for (auto iter {find_token_begin(std::begin(s), std::end(s))}; iter != std::end(s); iter = find_token_begin(iter, std::end(s)))
      {
        emplace_back(iter, is_round_brackets(*iter) ? iter + 1 : find_token_end(iter, std::end(s)));
        iter += std::size(back());
      }

      return *this;
    }

    template <typename T>
    static constexpr bool is_round_brackets(T&& c)
    {
      return c == '(' || c == ')';
    }

    friend auto operator<<(std::ostream& os, tokenizer& tokens)
      -> std::ostream&
    {
      for (const auto& each : tokens)
      {
        os << each << (&each != &tokens.back() ? ", " : "");
      }
      return os;
    }

  protected:
    template <typename InputIterator>
    static constexpr auto find_token_begin(InputIterator first, InputIterator last)
      -> InputIterator
    {
      return std::find_if(first, last, [](auto c)
             {
               return std::isgraph(c);
             });
    }

    template <typename InputIterator>
    static constexpr auto find_token_end(InputIterator first, InputIterator last)
      -> InputIterator
    {
      return std::find_if(first, last, [](auto c)
             {
               return is_round_brackets(c) || std::isspace(c);
             });
    }
  } static tokenize;

  class cell
    : public std::vector<cell>
  {
  public:
    enum class type { list, atom } state;

    std::string value;

    using scope_type = std::map<std::string, lisp::cell>;
    scope_type closure;

  public:
    cell(type state = type::list, const std::string& value = "")
      : state {state}, value {value}
    {}

    template <typename InputIterator>
    cell(InputIterator&& first, InputIterator&& last)
      : state {type::list}
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
          *this = {type::atom, *first};
        }
      }
    }

    explicit cell(const tokenizer& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

    cell(tokenizer&& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

    bool operator!=(const cell& rhs)
    {
      if (std::size(*this) != std::size(rhs) || (*this).state != rhs.state || (*this).value != rhs.value)
      {
        return true;
      }

      for (std::size_t index {0}; index < std::size(*this); ++index)
      {
        if ((*this)[index] != rhs[index]) // (std::size(*this) == std::size(rhs)) == true
        {
          return true;
        }
      }

      return false;
    }

    bool operator==(const cell& rhs)
    {
      return !((*this) != rhs);
    }

    friend auto operator<<(std::ostream& ostream, const cell& expr)
      -> std::ostream&
    {
      switch (expr.state)
      {
      case type::list:
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

    auto highlight(const std::string& target)
    {
      std::stringstream sstream {}, pattern {};
      sstream << *this;
      pattern << "^(.*?[\\s|\\(]?)(" << regex::escape_regex_specials(target) << ")([\\s|\\)].*)$";
      return std::regex_replace(sstream.str(), std::regex {pattern.str()}, "$1\e[31m$2\e[0m$3");
    }

    decltype(auto) highlight(const cell& expr)
    {
      if (expr.state != type::atom)
      {
        std::stringstream sstream {};
        sstream << expr;
        return highlight(sstream.str());
      }
      else
      {
        return highlight(expr.value);
      }
    }
  };


  class evaluator
    : public std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>>
  {
    static inline cell::scope_type dynamic_scope {
      {"true", cell {cell::type::atom, "true"}}
    };

  public:
    decltype(auto) operator()(const std::string& s, cell::scope_type& scope = dynamic_scope)
    {
      return operator()(cell {s}, scope);
    }

    template <typename C,
              typename = typename std::enable_if<
                                    std::is_same<
                                      typename std::remove_cv<
                                        typename std::remove_reference<C>::type
                                      >::type,
                                      cell
                                    >::value
                                  >::type>
    C operator()(C&& expr, cell::scope_type& scope = dynamic_scope) try
    {
      switch (expr.state)
      {
      case cell::type::atom:
        return scope.find(expr.value) != std::end(scope) ? scope[expr.value] : expr;

      case cell::type::list:
        if ((*this).find(expr.at(0).value) != std::end(*this))
        {
          highwrite(expr[0]);
          const cell buffer {(*this)[expr[0].value](expr, scope)};
          return expr = std::move(buffer);
        }

        highwrite(expr[0]);
        switch (replace_by_buffered_evaluation(expr[0], scope).state)
        {
        case cell::type::list:
          for (std::size_t index {0}; index < std::size(expr[0].at(1)); ++index)
          {
            highwrite(expr.at(index + 1));
            expr.closure[expr[0][1].at(index).value] = (*this)(expr.at(index + 1), scope);
          }

          for (const auto& each : scope) // TODO 既存要素を上書きしないことの確認
          {
            expr.closure.emplace(each);
          }

          highwrite(expr[0].at(2));
          return (*this)(expr[0].at(2), expr.closure);

        case cell::type::atom:
          for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
          {
            highwrite(*iter);
            (*this).replace_by_buffered_evaluation(*iter, scope);
          }

          highwrite(expr[0]);
          return (*this)(scope.at(expr[0].value), scope);
        }
      }

      std::exit(boost::exit_failure);
    }
    catch (const std::exception& ex)
    {
      std::cerr << "(runtime_error " << ex.what() << " \e[31m" << expr << "\e[0m) -> " << std::flush;
      return expr = scope["nil"];
    }

    cell& replace_by_buffered_evaluation(cell& expr, cell::scope_type& scope = dynamic_scope)
    {
      const auto buffer {(*this)(expr, scope)};
      return expr = std::move(buffer);
    }
  } static evaluate;


  template <typename T, template <typename...> typename BinaryOperator>
  class numeric_procedure
  {
  public:
    cell& operator()(cell& expr, cell::scope_type& scope)
    {
      std::vector<T> args {};

      for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
      {
        highwrite(*iter);
        args.emplace_back(evaluate.replace_by_buffered_evaluation(*iter, scope).value);
      }

      highwrite(expr.at(1));
      const auto result {std::accumulate(std::begin(args) + 1, std::end(args), args.front(), BinaryOperator<T> {})};

      if constexpr (std::is_same<typename BinaryOperator<T>::result_type, T>::value)
      {
        return expr = {cell::type::atom, result.str()};
      }
      else
      {
        // return expr = {cell::type::atom, result != 0 ? "true" : "nil"};
        return expr = (result != 0 ? scope.at("true") : scope["nil"]);
      }
    }
  };
} // namespace lisp


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  using namespace lisp;
  using namespace boost::multiprecision;

  evaluate["quote"] = [](auto& expr, auto&)
    -> decltype(auto)
  {
    highwrite(expr.at(1));
    return expr.at(1);
  };

  evaluate["cond"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    highwrite(expr.at(1));
    return evaluate(
             evaluate(expr.at(1), scope) != scope["nil"]
               ? (highwrite(expr.at(2)), expr.at(2))
               : (highwrite(expr.at(3)), expr.at(3)),
             scope
           );
  };

  evaluate["define"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    highwrite(expr.at(2));
    return scope[expr.at(1).value] = evaluate(expr.at(2), scope);
  };

  evaluate["lambda"] = [](auto& expr, auto& scope)
    -> decltype(auto)
  {
    expr.closure = scope;
    return expr;
  };

  evaluate["atom"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    highwrite(expr.at(1));
    evaluate.replace_by_buffered_evaluation(expr.at(1), scope);

    return expr[1].state != cell::type::atom && std::size(expr.at(1)) != 0 ? scope["nil"] : scope["true"];
  };

  evaluate["eq"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    return expr.at(1) != expr.at(2) ? scope["nil"] : scope["true"];
  };

  evaluate["cons"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    cell buffer {};

    highwrite(expr.at(1));
    buffer.push_back(evaluate(expr.at(1), scope));

    highwrite(expr.at(2));
    for (const auto& each : evaluate(expr.at(2), scope))
    {
      buffer.push_back(each);
    }

    return expr = std::move(buffer);
  };

  evaluate["car"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    highwrite(expr.at(1));
    const auto buffer {evaluate(expr.at(1), scope).at(0)};
    return expr = std::move(buffer);
  };

  evaluate["cdr"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    highwrite(expr.at(1));
    auto buffer {evaluate(expr.at(1), scope)};
    buffer.erase(std::begin(buffer));
    return expr = std::move(buffer);
  };

  evaluate["+"]  = numeric_procedure<cpp_dec_float_100, std::plus> {};
  evaluate["-"]  = numeric_procedure<cpp_dec_float_100, std::minus> {};
  evaluate["*"]  = numeric_procedure<cpp_dec_float_100, std::multiplies> {};
  evaluate["/"]  = numeric_procedure<cpp_dec_float_100, std::divides> {};
  evaluate["="]  = numeric_procedure<cpp_dec_float_100, std::equal_to> {};
  evaluate["<"]  = numeric_procedure<cpp_dec_float_100, std::less> {};
  evaluate["<="] = numeric_procedure<cpp_dec_float_100, std::less_equal> {};
  evaluate[">"]  = numeric_procedure<cpp_dec_float_100, std::greater> {};
  evaluate[">="] = numeric_procedure<cpp_dec_float_100, std::greater_equal> {};

  std::vector<std::string> prelude
  {
    "(define fib (lambda (n) (cond (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))",
    "(fib 10)"
  };

  for (const auto& each : prelude)
  {
    std::cerr << lisp::evaluate(each) << "\n\n";
  }

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    const auto begin {std::chrono::high_resolution_clock::now()};
    std::cout << lisp::evaluate(buffer) << std::flush;
    std::cerr << " in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - begin
                 ).count()
              << "ms" << "\n\n";
  }

  return boost::exit_success;
}

