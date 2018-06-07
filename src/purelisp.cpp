#ifndef NDEBUG
#define VISUALIZE_DEFORMATION_PROCESS
#endif // NDEBUG


#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include <purelisp/core/cell.hpp>
#include <purelisp/core/evaluator.hpp>
#include <purelisp/core/tokenizer.hpp>


#define VISUALIZE_PROCESSING_TIME
#ifdef VISUALIZE_PROCESSING_TIME
#include <chrono>
#endif // VISUALIZE_PROCESSING_TIME


namespace purelisp
{
  // class evaluator
  //   : public std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>>
  // {
  //   static inline cell::scope_type dynamic_scope_ {
  //     {"true", cell {cell::type::atom, "true"}},
  //   };
  //
  //   cell buffer_;
  //
  // public:
  //   evaluator()
  //     : std::unordered_map<std::string, std::function<cell& (cell&, cell::scope_type&)>> {
  //         {"quote",  &purelisp::evaluator::quote},
  //         {"lambda", &purelisp::evaluator::lambda},
  //         {"eq",     &purelisp::evaluator::eq}
  //       }
  //   {
  //     (*this)["cond"] = [this](auto& expr, auto& scope)
  //       -> decltype(auto)
  //     {
  //       // return (*this)((*this)(expr.at(1), scope) != scope["nil"] ? expr.at(2) : expr.at(3), scope);
  //       return (*this)((*this)(expr.at(1), scope) != nil ? expr.at(2) : expr.at(3), scope);
  //     };
  //
  //     (*this)["define"] = [this](auto& expr, auto& scope)
  //       -> decltype(auto)
  //     {
  //       scope.emplace(expr.at(1).value, (*this)(expr.at(2), scope));
  //       return expr;
  //     };
  //
  //     (*this)["atom"] = [this](auto& expr, auto& scope)
  //       -> decltype(auto)
  //     {
  //       const auto& buffer {(*this)(expr.at(1), scope)};
  //       // return buffer.state != cell::type::atom && std::size(buffer) != 0 ? scope["nil"] : scope["true"];
  //       return buffer.state != cell::type::atom && std::size(buffer) != 0 ? nil : scope["true"];
  //     };
  //
  //     (*this)["cons"] = [this](auto& expr, auto& scope)
  //       -> decltype(auto)
  //     {
  //       cell buffer {};
  //
  //       buffer.push_back((*this)(expr.at(1), scope));
  //
  //       for (const auto& each : (*this)(expr.at(2), scope))
  //       {
  //         buffer.push_back(each);
  //       }
  //
  //       return expr = std::move(buffer);
  //     };
  //
  //     (*this)["car"] = [this](auto& expr, auto& scope)
  //       -> decltype(auto)
  //     {
  //       return (*this)(expr.at(1), scope).at(0);
  //     };
  //
  //     (*this)["cdr"] = [this](auto& expr, auto& scope)
  //       -> decltype(auto)
  //     {
  //       auto buffer {(*this)(expr.at(1), scope)};
  //       // return expr = (std::size(buffer) != 0 ? buffer.erase(std::begin(buffer)), std::move(buffer) : scope["nil"]);
  //       return expr = (std::size(buffer) != 0 ? buffer.erase(std::begin(buffer)), std::move(buffer) : nil);
  //     };
  //   }
  //
  //   decltype(auto) operator()(const std::string& s, cell::scope_type& scope = dynamic_scope_)
  //   {
  //     return operator()(cell {s}, scope);
  //   }
  //
  //   cell& operator()(cell&& expr, cell::scope_type& scope = dynamic_scope_)
  //   {
  //     std::swap(buffer_, expr);
  //     return operator()(buffer_, scope);
  //   }
  //
  //   cell& operator()(cell& expr, cell::scope_type& scope = dynamic_scope_) try
  //   {
  //     #ifdef VISUALIZE_DEFORMATION_PROCESS
  //     BOOST_SCOPE_EXIT_ALL(this) { rewrite_expression(); };
  //     #endif // VISUALIZE_DEFORMATION_PROCESS
  //
  //     switch (expr.state)
  //     {
  //     case cell::type::atom:
  //       if (auto iter {scope.find(expr.value)}; iter != std::end(scope))
  //       {
  //         return (*iter).second;
  //       }
  //       else
  //       {
  //         return expr;
  //       }
  //
  //     case cell::type::list:
  //       if (auto iter {find(expr.at(0).value)}; iter != std::end(*this)) // special forms
  //       {
  //         return (*iter).second(expr, scope);
  //       }
  //
  //       // switch (*std::begin(expr) = {(*this)(*std::begin(expr), scope)}; std::begin(expr)->state)
  //       // switch (expr[0] = {(*this)(*std::begin(expr), scope)}; expr[0].state)
  //       switch (expr[0] = (*this)(expr[0], scope); expr[0].state) // こいつはムーブしたらダメ
  //       {
  //       case cell::type::list:
  //         for (std::size_t index {0}; index < std::size(expr[0].at(1)); ++index)
  //         {
  //           // expr.closure[expr[0][1].at(index).value] = (*this)(expr.at(index + 1), scope);
  //           expr.closure.emplace(expr[0][1].at(index).value, (*this)(expr.at(index + 1), scope));
  //         }
  //
  //         for (const auto& each : scope) // TODO 既存要素を上書きしないことの確認
  //         {
  //           expr.closure.emplace(each);
  //         }
  //
  //         // return expr = {(*this)(expr[0].at(2), expr.closure)};
  //         return (*this)(expr[0].at(2), expr.closure);
  //
  //       case cell::type::atom:
  //         for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
  //         {
  //           // *iter = {(*this)(*iter, scope)};
  //           *iter = (*this)(*iter, scope);
  //         }
  //
  //         // return expr = {(*this)(scope.at(std::begin(expr)->value), scope)};
  //         return (*this)(scope.at(expr[0].value), scope);
  //       }
  //     }
  //
  //     std::exit(boost::exit_failure);
  //   }
  //   catch (const std::exception& ex)
  //   {
  //     #ifndef NDEBUG
  //     std::cerr << "(runtime_error " << ex.what() << " \e[31m" << expr << "\e[0m) -> " << std::flush;
  //     #endif // NDEBUG
  //     // return scope["nil"];
  //     return nil;
  //   }
  //
  // protected:
  //   static cell& quote(cell& expr, cell::scope_type&) noexcept(false)
  //   {
  //     return expr.at(1);
  //   }
  //
  //   // TODO 静的スコープモードと動的スコープモードを切り替えられるように
  //   static cell& lambda(cell& expr, cell::scope_type& scope) noexcept(false)
  //   {
  //     expr.closure = scope;
  //     return expr;
  //   }
  //
  //   static cell& eq(cell& expr, cell::scope_type& scope) noexcept(false)
  //   {
  //     // return expr.at(1) != expr.at(2) ? scope["nil"] : scope["true"];
  //     return expr.at(1) != expr.at(2) ? nil : scope["true"];
  //   }
  //
  // private:
  //   #ifdef VISUALIZE_DEFORMATION_PROCESS
  //   void rewrite_expression() const
  //   {
  //     static struct winsize window_size;
  //     ioctl(STDERR_FILENO, TIOCGWINSZ, &window_size);
  //
  //     static std::size_t previous_row {0};
  //     if (0 < previous_row)
  //     {
  //       for (auto row {previous_row}; 0 < row; --row)
  //       {
  //         std::cerr << "\r\e[K\e[A";
  //       }
  //     }
  //
  //     std::stringstream ss {};
  //     ss << buffer_;
  //
  //     previous_row = std::size(ss.str()) / window_size.ws_col;
  //
  //     if (std::size(ss.str()) % window_size.ws_col == 0)
  //     {
  //       --previous_row;
  //     }
  //
  //     auto serialized {ss.str()};
  //
  //     if (auto column {std::size(ss.str()) / window_size.ws_col}; window_size.ws_row < column)
  //     {
  //       serialized.erase(0, (column - window_size.ws_row) * window_size.ws_col);
  //     }
  //
  //     std::cerr << "\r\e[K" << serialized << std::flush;
  //     std::this_thread::sleep_for(std::chrono::milliseconds {10});
  //   }
  //   #endif // VISUALIZE_DEFORMATION_PROCESS
  // } static evaluate;


  template <typename T, template <typename...> typename BinaryOperator,
            typename = typename std::enable_if<
                                  std::is_constructible<
                                    T, typename std::add_lvalue_reference<cell::value_type>::type
                                  >::value
                                >::type,
            typename = typename std::enable_if<
                                  std::is_same< // TODO なんか気に入らない
                                    decltype(boost::lexical_cast<cell::value_type>(std::declval<T>())),
                                    cell::value_type
                                  >::value
                                >::type>
  class numeric_procedure
  {
  public:
    cell& operator()(cell& expr, cell::scope_type& scope)
    {
      std::vector<T> args {};

      for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
      {
        args.emplace_back(evaluate(*iter, scope).value);
      }

      const auto buffer {std::accumulate(
        std::begin(args) + 1, std::end(args), args.front(), BinaryOperator<T> {}
      )};

      if constexpr (std::is_same<typename BinaryOperator<T>::result_type, T>::value)
      {
        return expr = {cell::type::atom, boost::lexical_cast<cell::value_type>(buffer)};
      }
      else
      {
        // return expr = (buffer != 0 ? scope["true"] : scope["nil"]);
        // return buffer != 0 ? scope["true"] : scope["nil"];
        return buffer != 0 ? scope["true"] : nil;
      }
    }
  };


  template <typename T>
  class numeric_type
  {
    std::string data_;

  public:
    using value_type = T;
    value_type value;

  public:
    numeric_type(value_type value)
      : data_ {boost::lexical_cast<decltype(data_)>(value)},
        value {value}
    {}

    numeric_type(const std::string& data)
      : data_ {data},
        value {boost::lexical_cast<value_type>(data_)}
    {}

    const auto& str() const noexcept
    {
      return data_;
    }

    // XXX 多分こいつのおかげでlexical-castable
    operator value_type() const noexcept
    {
      return value;
    }
  };
} // namespace lisp


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  using namespace purelisp;
  using namespace boost::multiprecision;

  evaluate["+"]  = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::plus> {};
  evaluate["-"]  = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::minus> {};
  evaluate["*"]  = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::multiplies> {};
  evaluate["/"]  = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::divides> {};
  evaluate["="]  = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::equal_to> {};
  evaluate["<"]  = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::less> {};
  evaluate["<="] = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::less_equal> {};
  evaluate[">"]  = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::greater> {};
  evaluate[">="] = numeric_procedure<cpp_dec_float_100 /* numeric_type<int> */, std::greater_equal> {};

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
    #ifdef VISUALIZE_PROCESSING_TIME
    const auto begin {std::chrono::high_resolution_clock::now()};
    #endif // VISUALIZE_PROCESSING_TIME

    #ifndef VISUALIZE_DEFORMATION_PROCESS
    std::cout <<
    #endif // VISUALIZE_DEFORMATION_PROCESS
                 evaluate(buffer);

    std::cerr <<
    #ifdef VISUALIZE_PROCESSING_TIME
                 " in " << std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::high_resolution_clock::now() - begin
                           ).count()
              << "msec" <<
    #endif // VISUALIZE_PROCESSING_TIME
                 "\n\n";
  }

  return boost::exit_success;
}

