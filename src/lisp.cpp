#include <bits/stdc++.h>

#include <boost/tti/has_member_function.hpp>
#include <boost/cstdlib.hpp>


// ラムダの実装がトンチンカン


namespace type_traits
{
  BOOST_TTI_HAS_MEMBER_FUNCTION(pop)
  BOOST_TTI_HAS_MEMBER_FUNCTION(pop_front)
  BOOST_TTI_HAS_MEMBER_FUNCTION(pop_back)
}


namespace utility
{
  template <typename T,
            typename = typename std::enable_if<
                                  std::disjunction<
                                    type_traits::has_member_function_pop<
                                      void (T::*)(void)
                                    >,
                                    type_traits::has_member_function_pop_front<
                                      void (T::*)(void)
                                    >,
                                    type_traits::has_member_function_pop_back<
                                      void (T::*)(void)
                                    >
                                  >::value
                                >::type>
  class pop_extended
    : public T
  {
  public:
    template <typename = typename std::enable_if<
                                    type_traits::has_member_function_pop_front<
                                      void (T::*)(void)
                                    >::value
                                  >::type,
              typename = typename std::enable_if<
                                    std::is_move_constructible<
                                      typename std::remove_reference<T>::type::value_type
                                    >::value
                                  >::type>
    auto pop_front() noexcept
    {
      const typename T::value_type buffer {std::move(T::front())};
      T::pop_front();
      return buffer;
    }
  };
}


namespace lisp
{
  template <typename CharType>
  class cell
    : public std::vector<lisp::cell<CharType>>
      // public std::unordered_map<std::basic_string<CharType>, lisp::cell<CharType>>
  {
  public:
    enum class lexical_category
    {
      list, symbol, number, lambda
    } category;

    using char_type = CharType;
    std::basic_string<char_type> value;

    // class closure
    //   : public std::unordered_map<std::basic_string<char_type>, lisp::cell<char_type>>
    // {
    // public:
    //   using base_type = std::unordered_map<std::basic_string<char_type>, lisp::cell<char_type>>;
    //
    //   template <template <typename...> typename StandardContainer>
    //   closure(const StandardContainer<lisp::cell<char_type>>& params,
    //           const StandardContainer<lisp::cell<char_type>>& args)
    //   {
    //     if (std::size(params) < std::size(args))
    //     {
    //       std::cerr << "  fatal error. line " << __LINE__ << std::endl;
    //
    //       while (std::size(params) < std::size(args))
    //       {
    //         params.emplace_back({});
    //       }
    //     }
    //     else if (std::size(args) != std::size(params))
    //     {
    //       std::cerr << "  fatal error. line " << __LINE__ << std::endl;
    //
    //       while (std::size(args) < std::size(params))
    //       {
    //         args.emplace_back({});
    //       }
    //     }
    //
    //     for (auto param {std::begin(params)}, arg {std::begin(args)}; param != std::end(params); ++param, ++arg)
    //     {
    //       (*this)[param->value] = *args;
    //     }
    //   }
    // } closure;

  public:
    cell(lexical_category category = lexical_category::symbol, const std::basic_string<char_type>& value = "null")
      : category {category},
        value {value}
    {
      std::cerr << std::endl;
      std::cerr << "construction: " << std::endl;
      description(2);
    }

    template <typename T,
              typename = typename std::enable_if<
                                    type_traits::has_member_function_pop_front<
                                      typename std::remove_reference<T>::type::value_type (std::remove_reference<T>::type::*)(void)
                                    >::value
                                  >::type,
              typename = typename std::enable_if<
                                    std::is_constructible<
                                      std::basic_string<char_type>,
                                      typename std::remove_reference<T>::type::value_type
                                    >::value
                                  >::type>
    cell(T&& tokens)
    {
      std::cerr << std::endl;
      std::cerr << "construction: " << std::endl;

      static const std::basic_regex<char_type> number_regex {"^([+-]?[\\d]+\\.?[\\d]*)$"};

      if (std::empty(tokens))
      {
        std::cerr << "  empty tokens passed. therefore, the tokens parsed as \"null\"." << std::endl;

        category = lexical_category::symbol;
        value = "null";

        description(2);

        return;
      }

      std::cerr << "  tokens: ";
      for (const auto& each : tokens)
      {
        std::cerr << "\e[1;31m" << each << "\e[0m" << (&each != &tokens.back() ? ", " : "\n");
      }

      if (auto token {tokens.pop_front()};
          std::cerr << "  check if token is open parenthesis... ",
          token == "(")
      {
        std::cerr << "match" << std::endl;

        category = lexical_category::list;
        value = "null";

        description(2);

        while (tokens.front() != ")")
        {
          if (tokens.back() != ")")
          {
            std::cerr << "  unterminated expression detected, inserted close parenthesis." << std::endl;
            tokens.emplace_back(")");
          }

          std::vector<lisp::cell<char_type>>::emplace_back(tokens);
        }

        tokens.pop_front();
      }
      else if (std::cerr << "no match" << std::endl
                         << "  check if token is number... ",
               std::regex_match(token, number_regex))
      {
        std::cerr << "match" << std::endl;

        category = lexical_category::number;
        value = token;

        description(2);
      }
      else
      {
        std::cerr << "no match" << std::endl;
        std::cerr << "  therefore, this token must be symbol." << std::endl;

        category = lexical_category::symbol;
        value = token;

        description(2);
      }
    }

    friend auto operator<<(std::basic_ostream<char_type>& ostream, lisp::cell<char_type> expression)
      -> std::basic_ostream<char_type>&
    {
      switch (expression.category)
      {
      case lisp::cell<char_type>::lexical_category::list:
        ostream << "(";
        for (const auto& each : expression)
        {
          ostream << each << (&each != &expression.back() ? " " : ")");
        }
        break;

      case lisp::cell<char_type>::lexical_category::lambda:
        ostream << "lambda::" << expression.value;
        break;

      case lisp::cell<char_type>::lexical_category::symbol:
        ostream << "symbol::" << expression.value;
        break;

      case lisp::cell<char_type>::lexical_category::number:
        ostream << "number::" << expression.value;
        break;
      }

      return ostream;
    }

    friend auto to_string(lexical_category category)
      -> std::basic_string<char_type>
    {
      switch (category)
      {
      case lexical_category::symbol:
        return {"symbol"};

      case lexical_category::number:
        return {"number"};

      case lexical_category::list:
        return {"list"};

      case lexical_category::lambda:
        return {"lambda"};

      default:
        throw std::logic_error {"lisp::cell<char_type>::to_string() - unimplemented category"};
      }
    }

  private:
    void description(typename std::basic_string<char_type>::size_type indent_size = 0) const
    {
      const std::basic_string<char_type> indent (indent_size, ' ');
      std::cerr << indent << "category: " << to_string(category) << ", value: " << value << std::endl;
    }
  };


  std::unordered_map<std::string, lisp::cell<char>> global_scope {};


  template <typename CharType>
  auto tokenize(std::basic_string<CharType> raw_code)
    -> utility::pop_extended<std::list<std::basic_string<CharType>>>
  {
    std::cerr << std::endl
              << "tokenize:" << std::endl;

    utility::pop_extended<std::list<std::basic_string<CharType>>> tokens {};

    auto is_paren = [](auto c) constexpr
    {
      return c == '(' || c == ')';
    };

    auto find_graph = [&](auto iter) constexpr
    {
      return std::find_if(iter, std::end(raw_code), [](auto c) { return std::isgraph(c); });
    };

    for (auto iter {find_graph(std::begin(raw_code))}; iter != std::end(raw_code); iter = find_graph(iter))
    {
      tokens.emplace_back(
        iter,
        is_paren(*iter) ? iter + 1
                        : std::find_if(iter, std::end(raw_code), [&](auto c)
                          {
                            return is_paren(c) || std::isspace(c);
                          })
      );

      iter += std::size(tokens.back());
    }

    std::cerr << "  ";
    for (const auto& each : tokens)
    {
      std::cerr << "\e[1;31m" << each << "\e[0m" << (&each != &tokens.back() ? ", " : "\n");
    }

    return tokens;
  }

  template <typename CharType>
  auto evaluate(lisp::cell<CharType>& expression)
    -> lisp::cell<CharType>
  {
    using category = typename lisp::cell<CharType>::lexical_category;

    std::cerr << std::endl;
    std::cerr << "evaluation:" << std::endl;
    std::cerr << "  \e[31m" << expression << "\e[0m" << std::endl;

    switch (expression.category)
    {
    case category::symbol:
      if (global_scope.find(expression.value) != std::end(global_scope))
      {
        return global_scope.at(expression.value);
      }
      else // これ要る？
      {
        std::cerr << "  undefined symbol \"" << expression.value << "\" requested. return null." << std::endl;
        return {};
      }

    case category::number:
      return expression;

    case category::list:
      if (std::empty(expression))
      {
        return {};
      }

      if (expression[0].value == "quote")
      {
        if (2 < std::size(expression))
        {
          std::cerr << "  invalid expression size detected. therefore, index 2 or later will be ignored." << std::endl;
        }
        return expression[1];
      }
      else if (expression[0].value == "cond")
      {
        while (std::size(expression) < 4)
        {
          std::cerr << "  argument omission detected. unserted null." << std::endl;
        }
        return evaluate(
                 evaluate(expression[1]).value == "true"
                   ? expression[2]
                   : expression[3]
               );
      }
      else if (expression[0].value == "define")
      {
        return global_scope[expression[1].value] = evaluate(expression[2]);
      }
      else if (expression[0].value == "lambda")
      {
        expression.category = category::lambda;
        return expression;
      }
      break;

    case category::lambda:
      {
        auto lambda {evaluate(expression[0])};

        for (auto param {std::begin(lambda[1])}, binded {std::begin(expression) + 1};
             param != std::end(lambda[1]) && binded != std::end(expression);
             ++param, ++binded)
        {
          global_scope[param->value] = evaluate(*binded);
        }

        return evaluate(lambda[2]);
      }
    }

    std::cerr << "  fatal error. line " << __LINE__ << std::endl;
    std::exit(boost::exit_failure);
  }
} // namespace lisp


auto main(int argc, char** argv)
  -> int
{
  const std::vector<std::string> args {argv, argv + argc};

  std::list<std::string> history {};

  for (std::string buffer {}; std::cin.good(); history.push_back(buffer))
  {
    std::cout << "[" << std::size(history) << "]< \e[0;34m" << std::flush;
    std::getline(std::cin, buffer);
    std::cout << "\e[0m" << std::flush;

    lisp::cell<char> expression {lisp::tokenize(buffer)};

    auto evaluated {lisp::evaluate(expression)};

    std::cerr << std::endl;
    std::cout << "[" << std::size(history) << "]> \e[0;34m" << evaluated << "\e[0m" << std::endl;
  }

  return boost::exit_success;
}

