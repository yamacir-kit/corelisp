#ifndef INCLUDED_PURELISP_DEBUG_REWRITE_EXPRESSION_HPP
#define INCLUDED_PURELISP_DEBUG_REWRITE_EXPRESSION_HPP


#include <chrono>
#include <sstream>
#include <thread>

#include <boost/scope_exit.hpp>

#include <unistd.h>
#include <sys/ioctl.h>


namespace purelisp { inline namespace debug
{
  [[deprecated]] void rewrite_expression(cell& expr)
  {
    static struct winsize window_size;
    ioctl(STDERR_FILENO, TIOCGWINSZ, &window_size);

    static std::size_t previous_row {0};
    if (0 < previous_row)
    {
      for (auto row {previous_row}; 0 < row; --row)
      {
        std::cerr << "\r\e[K\e[A";
      }
    }

    std::stringstream ss {};
    ss << expr;

    previous_row = std::size(ss.str()) / window_size.ws_col;

    if (std::size(ss.str()) % window_size.ws_col == 0)
    {
      --previous_row;
    }

    auto serialized {ss.str()};

    if (auto column {std::size(ss.str()) / window_size.ws_col}; window_size.ws_row < column)
    {
      serialized.erase(0, (column - window_size.ws_row) * window_size.ws_col);
    }

    std::cerr << "\r\e[K" << serialized << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds {10});
  }
}} // purelisp::debug


#endif // INCLUDED_PURELISP_DEBUG_REWRITE_EXPRESSION_HPP

