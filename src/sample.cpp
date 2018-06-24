#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/multiprecision/gmp.hpp>

// ユーザが使うときはインクルード文自体を名前空間でラップして使うのが良さそう
// 特にユーティリティライブラリ部分
#include <corelisp/lisp/evaluator.hpp>
#include <corelisp/lisp/tokenizer.hpp>
#include <corelisp/lisp/vectored_cons_cells.hpp>
#include <corelisp/builtin/arithmetic.hpp>


auto define_builtins = [&]()
{
  using namespace lisp;

  evaluate["quote"] = [&](auto& e, auto&) noexcept
    -> decltype(auto)
  {
    return std::size(e) != 2 ? false_value : e[1];
  };

  evaluate["atom"] = [&](auto& e, auto& env)
    -> decltype(auto)
  {
    return evaluate(e.at(1), env).is_atom() ? true_value : false_value;
  };

  evaluate["eq"] = [](auto& e, auto&) // XXX EQって可変長じゃなくても良かったっけ
    -> decltype(auto)
  {
    return  e.at(1) != e.at(2) ? false_value : true_value;
  };

  evaluate["car"] = [&](auto& expr, auto& scope) // TODO クソ
    -> decltype(auto)
  {
    return evaluate(expr.at(1), scope).at(0);
  };

  evaluate["cdr"] = [&](auto& expr, auto& scope) // TODO クソ
    -> decltype(auto)
  {
    auto buffer {evaluate(expr.at(1), scope)};
    return expr = (std::size(buffer) != 0 ? buffer.erase(std::begin(buffer)), std::move(buffer) : false_value);
  };

  evaluate["cons"] = [&](auto& expr, auto& scope) // TODO クソ
    -> decltype(auto)
  {
    vectored_cons_cells buffer {};

    buffer.push_back(evaluate(expr.at(1), scope));

    for (const auto& each : evaluate(expr.at(2), scope))
    {
      buffer.push_back(each);
    }

    return expr = std::move(buffer);
  };

  evaluate["cond"] = [&](auto& e, auto& env)
    -> vectored_cons_cells&
  {
    for (auto iter {std::begin(e) + 1}; iter != std::end(e); ++iter)
    {
      if (evaluate(iter->at(0), env) != false_value)
      {
        return evaluate(iter->at(1), env);
      }
    }

    return false_value; // TODO
  };

  evaluate["lambda"] = [](auto& expr, auto& scope)
    -> decltype(auto)
  {
    expr.closure = scope;
    return expr;
  };

  evaluate["define"] = [&](auto& e, auto& env) noexcept
    -> decltype(auto)
  {
    return std::size(e) != 3 ? false_value : (env.emplace(e[1].value, evaluate(e[2], env).share()), e[2]);
  };

  evaluate["if"] = [&](auto& e, auto& env) noexcept
    -> decltype(auto)
  {
    return std::size(e) != 4 ? false_value : evaluate(evaluate(e[1], env) != false_value ? e[2] : e[3], env);
  };

  using value_type = boost::multiprecision::mpf_float;
  evaluate["+"]  = builtin::arithmetic<value_type, std::plus> {};
  evaluate["-"]  = builtin::arithmetic<value_type, std::minus> {};
  evaluate["*"]  = builtin::arithmetic<value_type, std::multiplies> {};
  evaluate["/"]  = builtin::arithmetic<value_type, std::divides> {};
  evaluate["="]  = builtin::arithmetic<value_type, std::equal_to> {};
  evaluate["<"]  = builtin::arithmetic<value_type, std::less> {};
  evaluate["<="] = builtin::arithmetic<value_type, std::less_equal> {};
  evaluate[">"]  = builtin::arithmetic<value_type, std::greater> {};
  evaluate[">="] = builtin::arithmetic<value_type, std::greater_equal> {};
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

    std::cerr << "[error] unexpected option specified: \e[31m\"" << *iter << "\"\e[0m" << std::endl;
    std::exit(boost::exit_failure);
  }();

  define_builtins();

  std::vector<std::string> tests
  {
    "(define fib (lambda (n) (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))",
    "(define tarai (lambda (x y z) (if (<= x y) y (tarai (tarai (- x 1) y z) (tarai (- y 1) z x) (tarai (- z 1) x y)))))",
    // "(define map (lambda (f e) (if (eq? e false) false (cons (f (car e)) (map f (cdr e))))))",
    "(define x (quote (1 2 3 4 5)))",
    "(define factorial (lambda (n) (cond ((< n 0) false) ((<= n 1) 1) (true (* n (factorial (- n 1)))))))"
  };

  for (const auto& each : tests)
  {
    lisp::evaluate(each);
  }

  // std::fstream fstream {"../tests.scm", std::ios_base::in};
  //
  // std::string library {"(quote ("};
  //
  // for (std::string buffer {}; std::getline(fstream, buffer);)
  // {
  //   library += buffer;
  //
  //   for (const auto& each : buffer)
  //   {
  //     std::this_thread::sleep_for(std::chrono::milliseconds {10});
  //     std::cout << each << std::flush;
  //   }
  //
  //   std::cout << std::endl;
  // }
  //
  // library += "))";
  //
  // std::cout << "[debug] library evaluation "
  //           << (lisp::evaluate(library) != lisp::false_value ? "succeeded" : "failed")
  //           << "\n\n";

  for (std::string buffer {}; std::cout << ">> ", std::getline(std::cin, buffer);)
  {
    using namespace std::chrono;

    const auto begin {high_resolution_clock::now()};

    std::cout << lisp::evaluate(buffer)
              << " in "
              << duration_cast<milliseconds>(high_resolution_clock::now() - begin).count()
              << "msec\n\n";
  }

  return boost::exit_success;
}

