#pragma once

#include <bitset>
#include <cstdint>

namespace hivedb {
    struct numeric {
        private:
           std::bitset<12> m_sexp;
           std::bitset<53> m_prec;
        public:
            explicit numeric(std::uint8_t precision);

            numeric& operator++();
            numeric operator++(int);

            numeric& operator+(const numeric& rhs);
            numeric& operator*(const numeric& rhs);
            numeric& operator/(const numeric& rhs);
            numeric& operator-(const numeric& rhs);

            friend numeric operator+(const numeric& lhs, const numeric& rhs);
            friend numeric operator*(const numeric& lhs, const numeric& rhs);
            friend numeric operator/(const numeric& lhs, const numeric& rhs);
            friend numeric operator-(const numeric& lhs, const numeric& rhs);
    };
}
