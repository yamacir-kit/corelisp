#include <bits/stdc++.h>

#include <boost/cstdlib.hpp>
#include <boost/tti/has_member_function.hpp>


namespace type_traits
{
  BOOST_TTI_HAS_MEMBER_FUNCTION(pop_front);
}


namespace lisp
{
  auto tokenize(const std::string& code)
    -> std::list<std::string>
  {
    std::cerr << "\ntokenize:" << std::endl;

    auto seek = [&](auto iter) constexpr
    {
      return std::find_if(iter, std::end(code), [](auto c) { return std::isgraph(c); });
    };

    std::list<std::string> tokens {};
    for (auto iter {seek(std::begin(code))}; iter != std::end(code); iter += std::size(tokens.back()), iter = seek(iter))
    {
      tokens.emplace_back(iter, *iter == '(' or *iter == ')' ? iter + 1 : std::find_if(iter, std::end(code), [](auto c) { return c == '(' or c == ')' or std::isspace(c); }));
    }

    std::cerr << "  ";
    for (const auto& each : tokens)
    {
      std::cerr << "\e[1;31m" << each << "\e[0m" << (&each != &tokens.back() ? ", " : "\n");
    }

    return tokens;
  };

  class cell
    : public std::vector<lisp::cell>
  {
    using base_type = std::vector<lisp::cell>;

  public: // data members
    enum class lexical_category
    {
      list,
      atom, symbol,
            number,
      special,
      lambda
    } category;

    // コイツはそもそも設計を間違ってる気がする
    class scope
      : public std::list<scope> // 特にここ
    {
      using base_type = std::list<scope>;

      static inline std::size_t depth {0}; // デバッグ用

    public:
      // 別にプライベートでも良いはずだけどデバッグ用に露出させてる
      std::map<std::string, lisp::cell> local_scope;

      decltype(auto) operator[](const std::string& symbol)
      {
        return local_scope[symbol];
      }

      // eval の仕様が悪いかも知れないんだが、書き換え可能な形で返すのはクソ感漂ってる
      lisp::cell& find(const std::string& symbol)
      {
        std::cerr << "\nsearch:" << std::endl;
        std::cerr << "  target symbol: " << symbol << std::endl;
        std::cerr << "  target scope depth: " << depth << std::endl;

        static lisp::cell null {};

        if (std::cerr << "  searching local scope... ", local_scope.find(symbol) != std::end(local_scope))
        {
          std::cerr << "found." << std::endl;
          return local_scope.at(symbol);
        }
        else
        {
          std::cerr << "not found." << std::endl;

          if (!std::empty(*this))
          {
            std::cerr << "  search outer scope." << std::endl;
          }

          for (auto&& each : *this)
          {
            if (auto&& found {each.find(symbol)}; found.value != "null")
            {
              return found;
            }
          }
        }

        return null;
      }
    } closure;

    std::string value;

  public: // constructors
    cell(lexical_category category = lexical_category::symbol, const std::string& value = "null")
      : category {category}, value {value}
    {
      // std::cerr << "\nconstruction:" << std::endl;
      // std::cerr << "  cateory: " << category << ", value: " << value << std::endl;
    }

    cell(const std::string& code)
      : cell {tokenize(code)}
    {}

    // 受け取ったコンテナを破壊的に変更するクソ仕様
    // イテレータを受け取って非破壊的に走査する仕様で作り直すべき
    //
    // 既知のバグ：閉じカッコがひとつ足りない時に死ぬ
    template <typename T,
              typename = typename std::enable_if<
                                    type_traits::has_member_function_pop_front<
                                      void (std::remove_reference<T>::type::*)(void)
                                    >::value
                                  >::type,
              typename = typename std::enable_if<
                                    std::is_move_constructible<
                                      // std::string,
                                      typename std::remove_reference<T>::type::value_type
                                    >::value
                                  >::type>
    cell(T&& tokens)
    {
      std::cerr << "\nconstruction:" << std::endl;

      if (std::empty(tokens))
      {
        std::cerr << "  empty tokens passed. therefore, this expression parsed as \"null\"." << std::endl;
        std::cerr << "  category: " << (category = lexical_category::symbol) << ", value: " << (value = "null") << std::endl;
        return;
      }

      std::cerr << "  tokens: ";
      for (const auto& each : tokens)
      {
        std::cerr << "\e[1;31m" << each << "\e[0m" << (&each != &tokens.back() ? ", " : "\n");
      }

      const auto token {std::move(tokens.front())};
      tokens.pop_front();

      if (std::cerr << "  check if front token is open parenthesis... ", token == "(")
      {
        std::cerr << "match." << std::endl;
        std::cerr << "  therefore, this expression must be list." << std::endl;
        std::cerr << "  category: " << (category = lexical_category::list) << ", value: " << (value = "evaluation result of this list") << std::endl;

        if (std::empty(tokens))
        {
          std::cerr << "  unterminated expression detected, inserted close parenthesis." << std::endl;
          tokens.emplace_back(")");
        }
        else if (tokens.back() != ")")
        {
          std::cerr << "  unterminated expression detected, inserted close parenthesis." << std::endl;
          tokens.emplace_back(")");
        }

        std::cerr << "  emplace tokens to this list until close parenthesis..." << std::endl;
        while (tokens.front() != ")")
        {
          emplace_back(tokens);
        }

        // if (std::empty(tokens))
        // {
        //   std::cerr << "  unterminated expression detected, inserted close parenthesis." << std::endl;
        //   tokens.emplace_back(")");
        // }

        tokens.pop_front();
      }
      else
      {
        std::cerr << "no match." << std::endl;
        std::cerr << "  therefore, this expression must be atom." << std::endl;
        std::cerr << "  category: " << (category = lexical_category::atom) << ", value: " << (value = token) << std::endl;
      }
    }

    // ~cell()
    // {
    // #ifndef NDEBUG
    //   std::cerr << "\ndesctuction:" << std::endl;
    //   std::cerr << "  category: " << category << ", value: " << value << std::endl;
    // #endif // NDEBUG
    // }

  public: // friend member functions
    friend auto evaluate(lisp::cell& expression, lisp::cell::scope& scope)
      -> lisp::cell
    {
      std::cerr << "\nevaluation:" << std::endl;
      std::cerr << "  expression: \e[31m" << expression << "\e[0m" << std::endl;
      std::cerr << "  lexical_category: " << expression.category << std::endl;

      static const std::regex number {"^([+-]?[\\d]+\\.?[\\d]*)$"};

      // セルの構築時にはリストかアトムかのみを判別し、eval によって詳細なレキシカルカテゴリが確定する
      // 引数のレキシカルカテゴリを書き換えるため、二度目に渡した時はパースが早くなる、のだろうか

      switch (expression.category)
      {
      case lexical_category::atom:
        if (std::cerr << "  check if value of the atom is number... ";
            std::regex_match(expression.value, number))
        {
          std::cout << "match." << std::endl;
          std::cout << "  therefore, this expression evaluated as number." << std::endl;
          expression.category = lexical_category::number;
        }
        else
        {
          std::cout << "no match." << std::endl;
          std::cout << "  therefore, this expression evaluated as symbol." << std::endl;
          expression.category = lexical_category::symbol;
        }
        return evaluate(expression, scope);

      case lexical_category::number:
        return expression;

      case lexical_category::symbol:
        return scope.find(expression.value);

      case lexical_category::list:
        if (std::empty(expression))
        {
          return {};
        }
        else // リストの先頭は定義済みオペレータのシンボルでなければならない
        {
          // TODO 特殊形式は特殊形式で分離しておきたい
          if (expression[0].value == "quote")
          {
            return std::size(expression) < 2 ? lisp::cell {} : expression[1];
          }
          else if (expression[0].value == "cond")
          {
            while (std::size(expression) < 4)
            {
              expression.emplace_back();
            }
            return evaluate(evaluate(expression[1], scope).value == "true" ? expression[2] : expression[3], scope);
          }
          else if (expression[0].value == "define")
          {
            // (define pi 3.14)
            //   -> スコープに pi の名前で評価した数値を格納
            //
            // (define power (lambda (n) (* n n)))
            //   -> スコープに power の名前で評価したラムダを格納
            return scope[expression[1].value] = evaluate(expression[2], scope);
          }
          else if (expression[0].value == "lambda")
          {
            // (lambda (n) (* n n))
            //   -> レキシカルカテゴリをリストから特殊形式であるラムダに変更
            //   -> このリストのクロージャに今のスコープを登録（このラムダの定義時に視界内にあったシンボル）
            expression.category = lexical_category::lambda;
            expression.closure = scope;
            return expression;
          }
          else
          {
            std::cerr << "  no match to any special form." << std::endl;
            return evaluate(scope.find(expression[0].value), scope);
          }
        }
        break;

      case lexical_category::lambda: // special form に変更すべき
        {
          // // (define power (lambda (n) (* n n)))
          // // ってのが定義してあったとしたら、power ってシンボルからリスト (lambda (n) (* n n)) が引っ張り出せる
          // // この時点ではラムダはラムダとして認識されていない
          //
          // // ここに来てるってことは
          // // (power 2)
          // // って式の頭を評価して、power ってシンボルからラムダが返ってきてて、
          // // ((lambda (n) (* n n)) 2)
          // // こうなってる
          // lisp::cell procedure {evaluate(expression[0], scope)}; // これ要る？
          //
          // // んで、ラムダの部分が評価の対象になって、先頭のシンボルが lambda であることが判明し、
          // // レキシカルカテゴリがリストからラムダに修正され、最終的にここに到達している
          //
          // // ラムダを評価するにあたって引数 n を評価して、本体の中の n にはこれを参照させないといけない
          // lisp::cell::scope evaluated_arguments {};
          // for (const auto& parameter : procedure[1])
          // {
          //   evaluated_arguments[parameter] =
          // }
          //
          // evaluated_arguments.push_back(scope);
          //
          // // expression[2] -> ラムダの定義本体
          // // expression[1] -> 引数のリスト
          // return evaluate(expression[2], evaluated_arguments);
        }
        break;

      default:
        std::cerr << "  fatal error. return null." << std::endl;
      }

      return {};
    }

  public: // operators
    friend auto operator<<(std::ostream& ostream, lexical_category category)
      -> std::ostream&
    {
      switch (category)
      {
      case lexical_category::list:
        ostream << "list";
        break;

      case lexical_category::atom:
        ostream << "atom";
        break;

      case lexical_category::symbol:
        ostream << "symbol";
        break;

      case lexical_category::number:
        ostream << "number";
        break;

      case lexical_category::special:
        ostream << "special";
        break;

      case lexical_category::lambda:
        ostream << "lambda";
        break;
      }

      return ostream;
    }

    friend auto operator<<(std::ostream& ostream, const lisp::cell& expression)
      -> std::ostream&
    {
      switch (expression.category)
      {
      case lexical_category::list:
      case lexical_category::lambda:
        ostream << "(";
        for (const auto& each : expression)
        {
          ostream << each << (&each != &expression.back() ? " " : ")");
        }
        break;

      default:
      // #ifndef NDEBUG
      //   ostream << expression.category << "::";
      // #endif
        ostream << expression.value;
        break;
      }

      return ostream;
    }
  };
}


auto main(int argc, char** argv)
  -> int
{
  const std::vector<std::string> args {argv + 1, argv + argc};

  for (auto iter {std::begin(args)}; iter != std::end(args); ++iter)
  {
    // show type information
  }

  lisp::cell::scope global_scope {};

  std::vector<std::string> history {};
  for (std::string buffer {}; std::cout << "[" << std::size(history) << "]< \e[0;34m", std::getline(std::cin, buffer), std::cout << "\e[0m"; history.push_back(buffer))
  {
    lisp::cell expression {buffer};
    const auto evaluated {evaluate(expression, global_scope)};
    std::cout << "\n[" << std::size(history) << "]> \e[0;34m" << evaluated << "\e[0m" << std::endl;
  }

  return boost::exit_success;
}

