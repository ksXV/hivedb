#include <cstdio>
#include <iostream>
#include <limits>
#include <parser/parser.hpp>
#include <stdexcept>
#include <string>

int main() {
  std::cout << "WELCOME TO HIVE!!!!" << std::endl;
  std::string input{};
  try {
    while (true) {
      std::cout << "> ";
      if (std::getline(std::cin, input, ';')) {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << input;
        input.clear();
      } else {
        throw std::runtime_error("CIN FUCKED UP !!!");
      }
    }
  } catch (std::runtime_error &err) {
    std::cerr << "SOMETHING BAD HAPPENED !!!!!!!!!!" << "\n\n\n";
    std::cerr << err.what() << "\n";
    return 1;
  }
}
