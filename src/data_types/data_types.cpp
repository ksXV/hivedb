#include <data_types/integer.hpp>
#include <data_types/real.hpp>
#include <data_types/varchar.hpp>
#include <data_types/data_types.hpp>
#include <stdexcept>
#include <string_view>
#include <cassert>


namespace hivedb {
   std::string_view toString(data_types dt) {
      switch (dt) {
        case data_types::integer: return "integer";
        case data_types::varchar: return "varchar";
        case data_types::real: return "real";
        default: return "???";
      }
   }

   data_types fromString(std::string_view dt) {
       assert(isTypeValid(dt));

       if (dt == "integer") return data_types::integer;
       if (dt == "varchar") return data_types::varchar;
       if (dt == "real") return data_types::real;
       throw std::invalid_argument("RAHHHHHHHHHHHHHHHHHHH! Invalid data type.");
   }

   bool isTypeValid(std::string_view dt) {
       if (dt == "integer") return true;
       if (dt == "varchar") return true;
       if (dt == "real") return true;
       return false;
   }
   std::size_t findOffset(data_types dt) {
       switch (dt) {
         case data_types::integer: return integer::size;
         case data_types::varchar: return varchar::size;
         case data_types::real: return real::size;
         default: return 0;
       }
   }
}
