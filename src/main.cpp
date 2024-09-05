#include <cstdio>
#include <iostream>
#include <limits>
#include <string>

int main() {
    std::cout << "WELCOME TO HIVE!!!!" << std::endl;
    std::string input{};
    while (true) {
        std::cout << "> ";
        if (std::getline(std::cin, input, ';')) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            //do stuff
            input.clear();
        }
    }
}
