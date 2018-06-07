#include <chrono>
#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/cstdlib.hpp>

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

  evaluate["+"]  = numeric_procedure<cpp_dec_float_100, std::plus> {};
  evaluate["-"]  = numeric_procedure<cpp_dec_float_100, std::minus> {};
  evaluate["*"]  = numeric_procedure<cpp_dec_float_100, std::multiplies> {};
  evaluate["/"]  = numeric_procedure<cpp_dec_float_100, std::divides> {};
  evaluate["="]  = numeric_procedure<cpp_dec_float_100, std::equal_to> {};
  evaluate["<"]  = numeric_procedure<cpp_dec_float_100, std::less> {};
  evaluate["<="] = numeric_procedure<cpp_dec_float_100, std::less_equal> {};
  evaluate[">"]  = numeric_procedure<cpp_dec_float_100, std::greater> {};
  evaluate[">="] = numeric_procedure<cpp_dec_float_100, std::greater_equal> {};

  std::vector<std::string> tests
  {
    "(define fib (lambda (n) (cond (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))",
    "(define tarai (lambda (x y z) (cond (<= x y) y (tarai (tarai (- x 1) y z) (tarai (- y 1) z x) (tarai (- z 1) x y)))))",
    "(define map (lambda (func e) (cond (eq e nil) nil (cons (func (car e)) (map func (cdr e))))))",
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

