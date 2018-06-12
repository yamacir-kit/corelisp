#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#ifndef NDEBUG
#define VISUALIZE_DEFORMATION_PROCESS
#endif // NDEBUG

#include <purelisp/core/cell.hpp>
#include <purelisp/core/evaluator.hpp>
#include <purelisp/core/tokenizer.hpp>
#include <purelisp/function/arithmetic.hpp>


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  using namespace purelisp;
  using namespace boost::multiprecision;

  evaluate["+"]  = arithmetic::function</* cpp_dec_float_100 */ int, std::plus> {};
  evaluate["-"]  = arithmetic::function</* cpp_dec_float_100 */ int, std::minus> {};
  evaluate["*"]  = arithmetic::function</* cpp_dec_float_100 */ int, std::multiplies> {};
  evaluate["/"]  = arithmetic::function</* cpp_dec_float_100 */ int, std::divides> {};
  evaluate["="]  = arithmetic::function</* cpp_dec_float_100 */ int, std::equal_to> {};
  evaluate["<"]  = arithmetic::function</* cpp_dec_float_100 */ int, std::less> {};
  evaluate["<="] = arithmetic::function</* cpp_dec_float_100 */ int, std::less_equal> {};
  evaluate[">"]  = arithmetic::function</* cpp_dec_float_100 */ int, std::greater> {};
  evaluate[">="] = arithmetic::function</* cpp_dec_float_100 */ int, std::greater_equal> {};

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
                 evaluate(buffer);

    std::cerr << " in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - begin
                 ).count()
              << "msec\n\n";
  }

  return boost::exit_success;
}

