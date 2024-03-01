#pragma once

#include <stdexcept>

extern "C" {
#include <termios.h>
#include <unistd.h>
}

class NoncanonicalMode {
public:
  NoncanonicalMode() {
    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
      throw std::runtime_error("tcgetattr failed");
    }
    termios newt = oldt;
    newt.c_lflag &= ~(ICANON);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) != 0) {
      throw std::runtime_error("tcsetattr failed");
    }
  }

  ~NoncanonicalMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  }

private:
  termios oldt;
};

