#ifndef NDEBUG
#define VISUALIZE_DEFORMATION_PROCESS
#endif // NDEBUG

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <iterator>
#include <locale>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#ifdef VISUALIZE_DEFORMATION_PROCESS
#include <chrono>
#include <sstream>
#include <thread>

#include <boost/scope_exit.hpp>

#include <unistd.h>
#include <sys/ioctl.h>
#endif // VISUALIZE_DEFORMATION_PROCESS


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
      clear();

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

    friend auto operator<<(std::ostream& os, const cell& expr)
      -> std::ostream&
    {
      switch (expr.state)
      {
      case type::list:
        os <<  '(';
        for (const auto& each : expr)
        {
          os << each << (&each != &expr.back() ? " " : "");
        }
        return os << ')';

      default:
        return os << expr.value;
      }
    }
  };


  class evaluator
    : public std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>>
  {
    static inline cell::scope_type dynamic_scope {
      {"true", cell {cell::type::atom, "true"}}
    };

    cell buffer_;

    #ifdef VISUALIZE_DEFORMATION_PROCESS
    static inline std::size_t previous_row {0};
    struct winsize window_size;
    #endif // VISUALIZE_DEFORMATION_PROCESS

  public:
    decltype(auto) operator()(const std::string& s, cell::scope_type& scope = dynamic_scope)
    {
      return operator()(cell {s}, scope);
    }

    cell& operator()(cell&& expr, cell::scope_type& scope = dynamic_scope)
    {
      buffer_ = expr;
      return operator()(buffer_, scope);
    }

    cell& operator()(cell& expr, cell::scope_type& scope = dynamic_scope) try
    {
      #ifdef VISUALIZE_DEFORMATION_PROCESS
      BOOST_SCOPE_EXIT_ALL(this)
      {
        ioctl(STDERR_FILENO, TIOCGWINSZ, &window_size);

        std::stringstream ss {};
        ss << buffer_;

        if (0 < previous_row)
        {
          for (auto row {previous_row}; 0 < row; --row)
          {
            std::cerr << "\r\e[K\e[A";
          }
        }

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
      };
      #endif // VISUALIZE_DEFORMATION_PROCESS

      switch (expr.state)
      {
      case cell::type::atom:
        return scope.find(expr.value) != std::end(scope) ? scope[expr.value] : expr;

      case cell::type::list:
        if ((*this).find(expr.at(0).value) != std::end(*this))
        {
          const cell buffer {(*this)[expr[0].value](expr, scope)};
          return expr = std::move(buffer);
        }

        switch (replace_by_buffered_evaluation(expr[0], scope).state)
        {
        case cell::type::list:
          {
            for (std::size_t index {0}; index < std::size(expr[0].at(1)); ++index)
            {
              expr.closure[expr[0][1].at(index).value] = (*this)(expr.at(index + 1), scope);
            }

            for (const auto& each : scope) // TODO 既存要素を上書きしないことの確認
            {
              expr.closure.emplace(each);
            }

            const auto buffer {(*this)(expr[0].at(2), expr.closure)};
            return expr = std::move(buffer);
          }

        case cell::type::atom:
          {
            for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
            {
              (*this).replace_by_buffered_evaluation(*iter, scope);
            }

            const auto buffer {(*this)(scope.at(expr[0].value), scope)};
            return expr = std::move(buffer);
          }
        }
      }

      std::exit(boost::exit_failure);
    }
    catch (const std::exception& ex)
    {
      #ifndef NDEBUG
      std::cerr << "(runtime_error " << ex.what() << " \e[31m" << expr << "\e[0m) -> " << std::flush;
      #endif // NDEBUG
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
        args.emplace_back(evaluate.replace_by_buffered_evaluation(*iter, scope).value);
      }

      const auto result {std::accumulate(std::begin(args) + 1, std::end(args), args.front(), BinaryOperator<T> {})};

      if constexpr (std::is_same<typename BinaryOperator<T>::result_type, T>::value)
      {
        return expr = {cell::type::atom, result.str()};
      }
      else
      {
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
    return expr.at(1);
  };

  evaluate["cond"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    return evaluate(evaluate(expr.at(1), scope) != scope["nil"] ? expr.at(2) : expr.at(3), scope);
  };

  evaluate["define"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
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

    buffer.push_back(evaluate(expr.at(1), scope));

    for (const auto& each : evaluate(expr.at(2), scope))
    {
      buffer.push_back(each);
    }

    return expr = std::move(buffer);
  };

  evaluate["car"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    const auto buffer {evaluate(expr.at(1), scope).at(0)};
    return expr = std::move(buffer);
  };

  evaluate["cdr"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
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

  std::vector<std::string> predefined
  {
    "(define fib (lambda (n) (cond (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))",
    "(define tarai (lambda (x y z) (cond (<= x y) y (tarai (tarai (- x 1) y z) (tarai (- y 1) z x) (tarai (- z 1) x y)))))"
  };

  for (const auto& each : predefined)
  {
    lisp::evaluate(each);
    #ifdef VISUALIZE_DEFORMATION_PROCESS
    std::cerr << "\r\e[K";
    #endif // VISUALIZE_DEFORMATION_PROCESS
  }

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    const auto begin {std::chrono::high_resolution_clock::now()};

    #ifndef VISUALIZE_DEFORMATION_PROCESS
    std::cout <<
    #endif // VISUALIZE_DEFORMATION_PROCESS
                 lisp::evaluate(buffer);

    std::cerr << " in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - begin
                 ).count()
              << "ms" << "\n\n";
  }

  return boost::exit_success;
}

