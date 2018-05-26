#include <algorithm> // std::find_if
#include <functional> // std::function, std::plus, std::minus, std::multiplies, std::divides
#include <iostream>
#include <iterator> // std::begin, std::end, std::size, std::empty, std::distance
#include <locale> // std::isgraph, std::isspace
#include <map>
#include <numeric> // std::accumulate
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/cstdlib.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>


namespace lisp
{
  // トークナイザを関数オブジェクトにした理由は正直、特に無い
  class tokenizer
    : public std::vector<std::string> // 多分ベクタじゃない方が効率が良い
  {
  public:
    // 関数として事前に実体化しておくオブジェクト`tokenize`のために用意
    tokenizer() = default;

    // `cell`型を文字列から構築する際に暗黙に呼んでもらうために用意
    template <typename... Ts>
    tokenizer(Ts&&... args)
    {
      operator()(std::forward<Ts>(args)...);
    }

    auto& operator()(const std::string& s)
    {
      if (!std::empty(*this)) // この条件要らない説
      {
        clear();
      }

      // メンバ関数名が長いせいで読みにくいのが気に入らない
      for (auto iter {find_token_begin(std::begin(s), std::end(s))}; iter != std::end(s); iter = find_token_begin(iter, std::end(s)))
      {
        emplace_back(iter, is_round_brackets(*iter) ? iter + 1 : find_token_end(iter, std::end(s)));
        iter += std::size(back()); // この行を上手いこと上の行の処理に組み込みたい
      }

      return *this;
    }

    template <typename T>
    static constexpr bool is_round_brackets(T&& c) // `std::isparen`があれば
    {
      return c == '(' || c == ')';
    }

    // デバッグ用のストリーム出力演算子オーバーロード
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

    // トップレベルは右辺値参照でイテレータを受けることになるが、
    // 再帰呼出しの過程でイテレータを左辺値参照で渡していく必要があるため
    template <typename InputIterator>
    cell(InputIterator&& first, InputIterator&& last)
      : state {type::list}
    {
      if (std::distance(first, last) != 0) // サイズゼロのリストはNIL
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

    // 文字列を明示的にトークナイズしてASTを構築するときのためコンストラクタ。
    explicit cell(const tokenizer& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

    // 文字列から暗黙に構築したトークン列を経由してASTを構築するときのためのコンストラクタ。
    cell(tokenizer&& tokens)
      : cell {std::begin(tokens), std::end(tokens)}
    {}

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

    // デバッグ用のシンタックスハイライタ
    // 指定された文字列部分を強調表示するエスケープシーケンスを混ぜ込んだセルの文字列を返す
    auto highlight(const std::string& target)
    {
      std::stringstream sstream {}, pattern {};
      sstream << *this;
      pattern << "^(.*)(" << escape_regex_specials(target) << ")(.*)$";
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
    static inline cell::scope_type dynamic_scope {};

  public:
    using signature = std::function<cell& (cell&, cell::scope_type&)>;

    cell& operator()(const std::string& s, cell::scope_type& scope = dynamic_scope)
    {
      cell expr {s};
      return operator()(expr, scope);
    }

    cell& operator()(cell& expr, cell::scope_type& scope = dynamic_scope)
    {
      switch (expr.state)
      {
      case cell::type::atom:
        return scope.find(expr.value) != std::end(scope) ? scope[expr.value] : expr;

      case cell::type::list:
        // 空のリストはNILに評価される。
        if (std::empty(expr))
        {
          return expr = {cell::type::atom, "nil"};
        }

        // ラムダか組み込みプロシージャの別名である可能性しか残ってないので、一先ずCAR部分を評価にかける。
        // 既知のシンボルであった場合はバインドされた式が返ってくる。
        // 未知のシンボルであった場合はシンボル名がオウム返しされてくる。
        std::cerr << expr.highlight(expr[0]) << std::endl;
        switch (expr[0] = (*this)(expr[0], scope); expr[0].state)
        {
        case cell::type::list:
          if (expr[0][0].value == "lambda") // TODO この条件式要らない説
          {
            if (std::size(expr[0][1]) != std::size(expr) - 1)
            {
              std::cerr << "(error " << expr << " \"argument size is unmatched with parameter size\")" << std::endl;
              return scope["nil"];
            }

            // クロージャ（ラムダ定義時のスコープ）へ仮引数名をキーに実引数を評価して格納する。
            for (std::size_t index {0}; index < std::size(expr[0][1]); ++index)
            {
              std::cerr << expr.highlight(expr[index + 1]) << std::endl;
              expr.closure[expr[0][1][index].value] = (*this)(expr[index + 1], scope);
            }

            // ラムダ本体の部分を評価する。仮引数名と評価済み実引数の対応はクロージャが知っている。
            std::cerr << expr.highlight(expr[0][2]) << std::endl;
            return (*this)(expr[0][2], expr.closure);
          }
          else // ここに来るということは多分ラムダのタイポか何か。
          {
            std::cerr << "(error " << expr << " \"folloing expression isn't a lambda\")" << std::endl;
            return scope["nil"];
          }

        case cell::type::atom:
          // 特殊形式および組み込みプロシージャの探索と評価。
          if ((*this).find(expr[0].value) != std::end(*this))
          {
            // 引数をどのように評価するかは形式毎に異なるため、評価前の式を渡す。
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
  } static evaluate;


  template <template <typename...> typename BinaryOperaor, typename T>
  class numeric_procedure
  {
  public:
    template <typename... Ts>
    using binary_operator = BinaryOperaor<Ts...>;

    using value_type = T;

    cell& operator()(cell& expr, cell::scope_type& scope) try
    {
      std::vector<value_type> buffer {};

      for (auto iter {std::begin(expr) + 1}; iter != std::end(expr); ++iter)
      {
        std::cerr << expr.highlight(*iter) << std::endl;
        *iter = evaluate(*iter, scope);
        buffer.emplace_back(iter->value);
      }

      const auto result {std::accumulate(
        std::begin(buffer) + 1, std::end(buffer), buffer.front(), binary_operator<value_type> {}
      )};

      return expr = {cell::type::atom, result.str()};
    }
    catch (std::exception& exception)
    {
      std::cerr << "(error " << expr << " \"" << exception.what() << "\")" << std::endl;
      return expr = scope["nil"];
    }
  };
} // namespace lisp


int main(int argc, char** argv)
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  {
    using namespace lisp;
    using namespace boost::multiprecision;

    evaluate["quote"] = [](auto& expr, auto& scope)
      -> decltype(auto)
    {
      return std::size(expr) < 2 ? scope["nil"] : expr[1];
    };

    evaluate["cond"] = [&](auto& expr, auto& scope)
      -> decltype(auto)
    {
      while (std::size(expr) < 4)
      {
        expr.emplace_back();
      }
      return evaluate(
               (std::cerr << expr.highlight(expr[1]) << std::endl, evaluate(expr[1], scope).value != "nil")
                 ? (std::cerr << expr.highlight(expr[2]) << std::endl, expr[2])
                 : (std::cerr << expr.highlight(expr[3]) << std::endl, expr[3]),
               scope
             );
    };

    evaluate["define"] = [&](auto& expr, auto& scope)
      -> decltype(auto)
    {
      while (std::size(expr) < 3)
      {
        expr.emplace_back();
      }
      std::cerr << expr.highlight(expr[2]) << std::endl;
      return scope[expr[1].value] = evaluate(expr[2], scope);
    };

    evaluate["lambda"] = [](auto& expr, auto& scope)
      -> decltype(auto)
    {
      expr.closure = scope;

      // for (const auto& each : expr.closure)
      // {
      //   std::cerr << "  \e[32mclosure[" << each.first << "] = " << each.second << "\e[0m" << std::endl;
      // }

      return expr;
    };

    evaluate["+"] = numeric_procedure<std::plus, cpp_dec_float_100> {};
    evaluate["-"] = numeric_procedure<std::minus, cpp_dec_float_100> {};
    evaluate["*"] = numeric_procedure<std::multiplies, cpp_dec_float_100> {};
    evaluate["/"] = numeric_procedure<std::divides, cpp_dec_float_100> {};
  }

  {
    const std::vector<std::string> prelude
    {
      "(define cons (lambda (a d) (lambda (f) (f a d))))",

      "(define car (lambda (ad) (ad (lambda (a d) a))))",
      "(define cdr (lambda (ad) (ad (lambda (a d) d))))",
    };

    for (auto iter {std::begin(prelude)}; iter != std::end(prelude); ++iter)
    {
      std::cerr << "prelude[" << std::distance(std::begin(prelude), iter) << "]< " << *iter << std::endl;
      std::cerr << lisp::evaluate(*iter) << "\n\n";
    }
  }

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< ", std::getline(std::cin, buffer); history.push_back(buffer))
  {
    std::cout << lisp::evaluate(buffer) << "\n\n";
  }

  return boost::exit_success;
}

