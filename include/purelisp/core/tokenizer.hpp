#ifndef INCLUDED_PURELISP_CORE_TOKENIZER_HPP
#define INCLUDED_PURELISP_CORE_TOKENIZER_HPP


#include <algorithm>
#include <iterator>
#include <locale>
#include <string>
#include <utility>
#include <vector>


namespace purelisp::core
{
  class tokenizer
    : public std::vector<std::string>
  {
    static auto tokenize_(const std::string& s)
      -> std::vector<std::string>
    {
      std::vector<std::string> buffer {};

      for (auto iter {find_token_begin(std::begin(s), std::end(s))}; iter != std::end(s); iter = find_token_begin(iter, std::end(s)))
      {
        buffer.emplace_back(iter, is_round_brackets(*iter) ? iter + 1 : find_token_end(iter, std::end(s)));
        iter += std::size(buffer.back());
      }

      return buffer; // copy elision
    }

  public:
    template <typename... Ts>
    tokenizer(Ts&&... args)
      : std::vector<std::string> {std::forward<Ts>(args)...}
    {}

    tokenizer(const std::string& s)
      : std::vector<std::string> {tokenize_(s)} // copy elision
    {}

    auto& operator()(const std::string& s)
    {
      return *this = std::move(tokenize_(s));
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
    template <typename CharType>
    static constexpr auto is_round_brackets(CharType c) noexcept
      -> bool
    {
      return c == '(' || c == ')';
    }

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
} // namespace purelisp::core


#endif // INCLUDED_PURELISP_CORE_TOKENIZER_HPP

