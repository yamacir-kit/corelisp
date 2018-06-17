#include <chrono>
#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <purelisp/core/cell.hpp>
#include <purelisp/core/evaluator.hpp>
#include <purelisp/core/tokenizer.hpp>
#include <purelisp/function/arithmetic.hpp>


auto define_origin_functions = [&]()
{
  using namespace purelisp;

  evaluate["quote"] = [](auto& expr, auto&)
    -> decltype(auto)
  {
    return expr.at(1);
  };

  evaluate["atom"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    const auto& buffer {evaluate(expr.at(1), scope)};
    return buffer.state != cell::type::atom && std::size(buffer) != 0 ? false_ : true_; // XXX これ正しい？
  };

  evaluate["eq"] = [](auto& expr, auto&)
    -> decltype(auto)
  {
    return  expr.at(1) != expr.at(2) ? false_ : true_;
  };

  evaluate["car"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    return evaluate(expr.at(1), scope).at(0);
  };

  evaluate["cdr"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    auto buffer {evaluate(expr.at(1), scope)};
    return expr = (std::size(buffer) != 0 ? buffer.erase(std::begin(buffer)), std::move(buffer) : false_);
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

  evaluate["cond"] = [&](auto& expr, auto& scope)
    -> cell&
  {
    for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
    {
      if (evaluate(iter->at(0), scope) != false_)
      {
        return evaluate(iter->at(1), scope);
      }
    }

    return false_; // TODO
  };

  evaluate["lambda"] = [](auto& expr, auto& scope)
    -> decltype(auto)
  {
    expr.closure = scope;
    return expr;
  };

  evaluate["define"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    scope.emplace(expr.at(1).value, std::make_shared<cell>(evaluate(expr.at(2), scope)));
    return expr[2];
  };
};


auto define_scheme_functions = [&]()
{
  using namespace purelisp;

  define_origin_functions();

  evaluate["if"] = [&](auto& expr, auto& scope)
    -> decltype(auto)
  {
    return evaluate(evaluate(expr.at(1), scope) != false_ ? expr.at(2) : expr.at(3), scope);
  };

  using value_type =  boost::multiprecision::cpp_int; // boost::multiprecision::cpp_dec_float_100;
  evaluate["+"]  = arithmetic::function<value_type, std::plus> {};
  evaluate["-"]  = arithmetic::function<value_type, std::minus> {};
  evaluate["*"]  = arithmetic::function<value_type, std::multiplies> {};
  evaluate["/"]  = arithmetic::function<value_type, std::divides> {};
  evaluate["="]  = arithmetic::function<value_type, std::equal_to> {};
  evaluate["<"]  = arithmetic::function<value_type, std::less> {};
  evaluate["<="] = arithmetic::function<value_type, std::less_equal> {};
  evaluate[">"]  = arithmetic::function<value_type, std::greater> {};
  evaluate[">="] = arithmetic::function<value_type, std::greater_equal> {};
};


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  for (auto iter {std::begin(args)}; iter != std::end(args); ++iter) [&]()
  {
    std::match_results<std::string::const_iterator> results {};

    for (const auto& each : std::vector<std::string> {"-h", "--help"})
    {
      if (std::regex_match(*iter, std::regex {each}))
      {
        std::cout << "TODO" << std::endl;
        std::exit(boost::exit_success);
      }
    }

    for (const auto& each : std::vector<std::string> {"^-s(.*)$", "^--syntax=(.*)$"})
    {
      if (std::regex_match(*iter, results, std::regex {each}) && results.ready())
      {
        static std::unordered_map<std::string, std::function<void (void)>> syntaxes
        {
          {"origin", define_origin_functions},
          {"scheme", define_scheme_functions}
        };

        if (auto syntax {syntaxes.find(results[1])}; syntax != std::end(syntaxes))
        {
          return (*syntax).second();
        }
        else
        {
          std::cerr << "[error] unexpected syntax specified: \e[31m\"" << *iter << "\"\e[0m" << std::endl;
          std::exit(boost::exit_failure);
        }
      }
    }

    std::cerr << "[error] unexpected option specified: \e[31m\"" << *iter << "\"\e[0m" << std::endl;
    std::exit(boost::exit_failure);
  }();

  using namespace purelisp;

  std::vector<std::string> tests
  {
    "(define fib (lambda (n) (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))",
    "(define tarai (lambda (x y z) (if (<= x y) y (tarai (tarai (- x 1) y z) (tarai (- y 1) z x) (tarai (- z 1) x y)))))",
    "(define map (lambda (func e) (if (eq e false) false (cons (func (car e)) (map func (cdr e))))))",
    "(define x (quote (1 2 3 4 5)))",
    "(define factorial (lambda (n) (cond ((< n 0) false) ((<= n 1) 1) (true (* n (factorial (- n 1)))))))"
  };

  for (const auto& each : tests)
  {
    evaluate(each);
  }

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    const auto begin {std::chrono::high_resolution_clock::now()};

    std::cout << evaluate(buffer)
              << " in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - begin
                 ).count()
              << "msec\n\n";
  }

  return boost::exit_success;
}

