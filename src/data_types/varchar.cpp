#include <data_types/varchar.hpp>
#include <stdexcept>

namespace hivedb {
        varchar::varchar(std::string_view str) {
            if (str.size() > 255) {
               throw std::invalid_argument("cannot have varchar bigger than 255");
            }

            data = str;
        }
}
