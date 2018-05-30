#include <algorithm> // std::find_if
#include <functional> // std::function, std::plus, std::minus, std::multiplies, std::divides
#include <iostream>
#include <iterator> // std::begin, std::end, std::size, std::empty, std::distance
#include <locale> // std::isgraph, std::isspace
#include <map>
#include <numeric> // std::accumulate
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
        if ((*this)[index] != rhs[index])
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
      pattern << "^(.*?[\\s|\\(]?)(" << escape_regex_specials(target) << ")([\\s|\\)].*)$";
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

  protected:
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
  };


  class evaluator
    : public std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>>
  {
    static inline cell::scope_type dynamic_scope {
      {"true", cell {cell::type::atom, "true"}}
    };

  public:
    using signature = std::function<cell& (cell&, cell::scope_type&)>;

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
        // TODO this block is removal if use std::vector<T>::at() insted of std::vector<T>::operator[]()
        if (std::empty(expr))
        {
          return expr = {};
        }

        if ((*this).find(expr[0].value) != std::end(*this))
        {
          highwrite(expr[0]);
          const cell buffer {(*this)[expr[0].value](expr, scope)};
          return expr = std::move(buffer);
        }

        highwrite(expr[0]);
        switch (const auto buffer {(*this)(expr[0], scope)}; (expr[0] = std::move(buffer)).state)
        {
        case cell::type::list:
          if (expr[0][0].value == "lambda")
          {
            if (std::size(expr[0][1]) != std::size(expr) - 1)
            {
              std::cerr << "(error " << expr << " \"argument size is unmatched with parameter size\")" << std::endl;
              return scope["nil"];
            }

            for (std::size_t index {0}; index < std::size(expr[0][1]); ++index)
            {
              highwrite(expr[index + 1]);
              expr.closure[expr[0][1][index].value] = (*this)(expr[index + 1], scope);
            }

            highwrite(expr[0][2]);
            return (*this)(expr[0][2], expr.closure);
          }
          else
          {
            std::cerr << "(error " << expr << " \"folloing expression isn't a lambda\")" << std::endl;
            return scope["nil"];
          }

        case cell::type::atom:
          if ((*this).find(expr[0].value) != std::end(*this))
          {
            highwrite(expr[0]);
            return (*this).at(expr[0].value)(expr, scope);
          }
          else
          {
            std::cerr << "(error " << expr << " \"unknown procedure\")" << std::endl;
            return scope["nil"];
          }
        }
      }

      std::cerr << "(error " << expr << " \"unexpected conditional break\")" << std::endl;
      std::exit(boost::exit_failure);
    }
    catch (const std::out_of_range&)
    {
      return expr = scope["nil"];
    }

    cell& replace_by_buffered_evaluation(cell& expr, cell::scope_type& scope = dynamic_scope)
    {
      const auto buffer {(*this)(expr, scope)};
      return expr = std::move(buffer);
    }
  } static evaluate;


  template <template <typename...> typename BinaryOperaor, typename T>
  class numeric_procedure
  {
  public:
    template <typename... Ts>
    using binary_operator = BinaryOperaor<Ts...>;

    using value_type = T;

    cell& operator()(cell& expr, cell::scope_type& scope) // try
    {
      std::vector<value_type> args {};

      for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
      {
        highwrite(*iter);
        args.emplace_back(evaluate.replace_by_buffered_evaluation(*iter, scope).value);
      }

      highwrite(expr.at(1));
      const auto result {std::accumulate(
        std::begin(args) + 1, std::end(args), args.front(), binary_operator<value_type> {}
      )};

      return expr = {cell::type::atom, result.str()};
    }
  };
} // namespace lisp


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  {
    using namespace lisp;
    using namespace boost::multiprecision;

    evaluate["quote"] = [](auto& expr, auto&)
      -> decltype(auto)
    {
      return highwrite(expr.at(1)), expr.at(1);
    };

    evaluate["cond"] = [&](auto& expr, auto& scope)
      -> decltype(auto)
    {
      highwrite(expr.at(1));
      return evaluate(
               evaluate(expr.at(1), scope).value != "nil"
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

      for (const auto& each : expr.closure)
      {
        std::cerr << "  \e[32m[" << each.first << "] = " << each.second << "\e[0m" << std::endl;
      }

      return expr;
    };

    evaluate["atom"] = [&](auto& expr, auto& scope)
      -> decltype(auto)
    {
      highwrite(expr.at(1));
      const auto buffer {evaluate(expr.at(1), scope)};
      expr[1] = std::move(buffer);

      return expr[1].state != cell::type::atom && std::size(expr.at(1)) != 0 ? scope["nil"] : scope["true"];
    };

    evaluate["eq"] = [&](auto& expr, auto& scope)
      -> decltype(auto)
    {
      return expr.at(1) != expr.at(2) ? scope["nil"] : scope["true"];
    };

    evaluate["+"] = numeric_procedure<std::plus, cpp_dec_float_100> {};
    evaluate["-"] = numeric_procedure<std::minus, cpp_dec_float_100> {};
    evaluate["*"] = numeric_procedure<std::multiplies, cpp_dec_float_100> {};
    evaluate["/"] = numeric_procedure<std::divides, cpp_dec_float_100> {};
  }

  {
    const std::vector<std::pair<std::string, std::string>> prelude
    {
      {"(quote a)", "a"},
      {"(quote (a b c))", "(a b c)"},

      {"(atom (quote a))", "true"},
      {"(atom (quote (a b c))", "()"},
      {"(atom (quote ()))", "true"},
      {"(atom (atom (quote a)))", "true"},
      {"(atom (quote (atom (quote a))))", "()"},

      {"(eq (quote a) (quote a))", "true"},
      {"(eq (quote a) (quote b))", "()"},
      {"(eq (quote ()) (quote ()))", "true"},

      {"(define car (lambda (e) ((lambda (a d) a) e)))", "(lambda (e) ((lambda (a d) a) e))"},
      {"(car (quote (a b c)))", "a"},

      {"(define cdr (lambda (e) ((lambda (a d) d) e)))", "(lambda (e) ((lambda (a d) d) e))"},
      {"(cdr (quote (a b c)))", "(b c)"},

      // {"(define cons (lambda (a d) (lambda (f) (f a d))))", "(lambda (a d) (lambda (f) (f a d)))"},
      // {"(cons (quote a) (quote (b c)))", "(a b c)"},
      // {"(cons (quote a) (cons (quote b) (cons (quote c) (quote ()))))", "(a b c)"},
      // {"(car (cons (quote a) (quote (b c))))", "a"},
      // {"(cdr (cons (quote a) (quote (b c))))", "(b c)"},

      // {"(cond ((eq (quote a) (quote b)) (quote first)) ((atom (quote a)) (quote second)))", "second"}
    };

    for (auto iter {std::begin(prelude)}; iter != std::end(prelude); ++iter)
    {
      std::cerr << "prelude[" << std::distance(std::begin(prelude), iter) << "]< " << (*iter).first << std::endl;

      std::stringstream buffer {};
      buffer << lisp::evaluate((*iter).first);

      std::cerr << buffer.str() << " -> \e[33m(selfcheck " << (buffer.str() != (*iter).second ? "failed" : "passed") << ")\e[0m\n\n";
    }
  }

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    std::cout << lisp::evaluate(buffer) << "\n\n";
  }

  return boost::exit_success;
}

