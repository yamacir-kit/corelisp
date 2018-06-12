#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <purelisp/core/cell.hpp>
#include <purelisp/core/evaluator.hpp>
#include <purelisp/core/tokenizer.hpp>
#include <purelisp/function/arithmetic.hpp>


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  using namespace purelisp;
  using namespace boost::multiprecision;

  using value_type = int; // boost::multiprecision::cpp_dec_float_100;
  evaluate["+"]  = arithmetic::function<value_type, std::plus> {};
  evaluate["-"]  = arithmetic::function<value_type, std::minus> {};
  evaluate["*"]  = arithmetic::function<value_type, std::multiplies> {};
  evaluate["/"]  = arithmetic::function<value_type, std::divides> {};
  evaluate["="]  = arithmetic::function<value_type, std::equal_to> {};
  evaluate["<"]  = arithmetic::function<value_type, std::less> {};
  evaluate["<="] = arithmetic::function<value_type, std::less_equal> {};
  evaluate[">"]  = arithmetic::function<value_type, std::greater> {};
  evaluate[">="] = arithmetic::function<value_type, std::greater_equal> {};

  std::vector<std::string> tests
  {
    "(define fib (lambda (n) (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))",
    "(define tarai (lambda (x y z) (if (<= x y) y (tarai (tarai (- x 1) y z) (tarai (- y 1) z x) (tarai (- z 1) x y)))))",
    "(define map (lambda (func e) (if (eq e false) false (cons (func (car e)) (map func (cdr e))))))",
    "(define x (quote (1 2 3 4 5)))"
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

