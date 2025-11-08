#include <catch_amalgamated.hpp>
#include <b_plus_tree/b_plus_tree.hpp>
#include <disk/disk_manager.hpp>
#include <exception>
#include <misc/config.hpp>
#include <iostream>
#include <random>

struct int_key {
    std::int64_t key;
    std::int64_t a,b,c,d,e,f,g,h,j,i,k,l,m,n,o;
    std::int64_t aa,bb,cc,dd,ee,ff,gg,hh,jj,ii,kk,ll,mm,nn,oo,pp;

    int_key(std::int64_t x): key(x) {}; // NOLINT

    [[nodiscard]]
    constexpr auto operator<=>(const int_key& rhs) const noexcept = default;

    [[nodiscard]]
    constexpr bool operator!=(const int_key& rhs) const {
        return key != rhs.key;
    }

    [[nodiscard]] std::string to_string() const {
        return std::to_string(key);
    }

    int_key& operator++() {
        this->key++;
        return *this;
    }

    static int_key invalid_key() {
       return {-99};
    };

    static int_key* deserialize(char* buffer) {
        return reinterpret_cast<int_key*>(buffer);
    }
};

namespace std {
    template<>
    struct hash<int_key>
    {
        std::size_t operator()(const auto& s) const noexcept
        {
            std::hash<std::int64_t> temp;
            return temp(s.key);
        }
    };
}

std::ostream& operator<<(std::ostream& os, const int_key& rhs) {
    os << rhs.key;
    return os;
}

TEST_CASE("b_plus_tree trivial leaf test", "[b_plus_tree]") {
    hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key> tree{
        hivedb::INVALID_PAGE_ID,
        15,
        ""
    };

    try {
    std::random_device rnd_device;
    std::mt19937 mersenne_engine{rnd_device()};

    std::vector<int_key> vec(50, 0);
    std::iota(vec.begin(), vec.end(), int_key{0});
    std::shuffle(vec.begin(), vec.end(), mersenne_engine);


    std::vector<std::pair<int_key, int_key>> values;
    int idx = 0;
    for (const auto& element: vec) {
        auto temp = int_key{idx++};
        REQUIRE(tree.insert(element, temp));
        spdlog::info(fmt::format("TESTS: Emplacing {} and {} in values", element.key, temp.key));
        values.emplace_back(element, temp);
    }

    for (const auto& element: vec) {
        int_key found_value{int_key::invalid_key()};
        tree.find(element, found_value);
        spdlog::info(fmt::format("TESTS: Trying to search for {} and {}", element.key, found_value.key));
        if (!(std::find(values.cbegin(), values.cend(), std::pair{element, found_value}) != values.cend())) {
           tree.dump_contents();
           FAIL(":(");
        }
    }

    } catch (const std::exception& err) {
        tree.dump_contents();
        FAIL(err.what());
    }
}

// TEST_CASE("b_plus_tree create another root test", "[b_plus_tree_new_root]") {
//     try {
//     std::random_device rnd_device;
//     std::mt19937 mersenne_engine {rnd_device()};

//     std::vector<int_key> vec(300, 0);
//     std::iota(vec.begin(), vec.end(), int_key{0});
//     std::shuffle(vec.begin(), vec.end(), mersenne_engine);

//     // std::vector<int_key> vec{207,257,171,1,69,184,179,181,94,12,299,266,68,116,151,26,87,164,196,202,106,72,285,91,0,44,98,112,117,279,108,66,9,56,260,242,29,201,19,95,237,20,231,67,278,64,194,152,264,215,82,110,267,160,10,23,13,115,263,292,50,4,88,182,80,177,174,243,58,273,262,43,223,122,38,148,281,71,62,248,144,213,83,75,214,297,79,24,283,238,40,146,234,192,21,143,218,265,251,57,255,210,53,93,268,175,8,149,290,81,209,140,167,96,247,86,65,90,133,137,49,55,32,232,60,70,54,277,27,156,150,162,16,211,34,105,100,286,190,195,48,92,228,161,46,145,229,212,11,17,221,128,291,230,78,259,239,15,220,51,141,233,138,244,295,28,294,271,14,282,103,254,153,204,135,126,165,99,97,73,63,47,41,22,224,61,280,186,36,200,163,35,183,168,33,272,222,121,89,31,118,39,123,170,252,172,18,187,180,142,216,127,37,246,2,74,275,173,104,155,169,102,120,136,241,240,225,276,205,288,139,298,158,208,113,166,206,154,7,269,197,274,227,124,189,107,284,6,193,236,235,199,191,30,114,119,261,198,226,270,203,159,5,287,217,125,293,76,249,176,130,59,258,84,185,250,289,157,296,256,45,109,147,131,77,25,219,253,101,42,134,111,129,132,85,178,3,188,245,52};

//     hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key> tree{
//         hivedb::INVALID_PAGE_ID,
//         10,
//         ""
//     };

//     std::vector<std::pair<int_key, int_key>> values;
//     int idx = 0;
//     for (const auto& element: vec) {
//         auto temp = int_key{idx++};
//         REQUIRE(tree.insert(element, temp));
//         spdlog::info("TESTS: Emplacing {} and {} in values", element.key, temp.key);
//         values.emplace_back(element, temp);
//     }

//     for (const auto& element: vec) {
//         int_key found_value{int_key::invalid_key()};
//         tree.find(element, found_value);

//         spdlog::info("TESTS: Trying to search for {} and {}", element.key, found_value.key);
//         REQUIRE(std::find(values.cbegin(), values.cend(), std::pair{element, found_value}) != values.cend());
//     }

//     } catch (const std::exception& err) {
//         FAIL(err.what());
//     }
// }

// TEST_CASE("b_plus_tree insert into inner node", "[b_plus_tree_insert_inner_node]") {
//     try {
//     std::random_device rnd_device;
//     std::mt19937 mersenne_engine {rnd_device()};

//     std::vector<int_key> vec(900, 0);
//     std::iota(vec.begin(), vec.end(), int_key{0});
//     std::shuffle(vec.begin(), vec.end(), mersenne_engine);

//     // std::vector<int_key> vec{
//     //     258,49,354,214,380,66,33,10,473,411,224,55,115,339,429,182,57,356,455,148,446,323,279,268,459,236,274,286,331,111,297,190,466,149,284,245,54,447,109,191,275,202,359,308,69,325,138,485,124,61,480,79,162,208,4,363,46,335,384,185,68,261,453,18,499,177,495,259,426,83,25,97,76,183,146,262,407,194,334,250,282,361,195,71,91,415,70,314,360,342,270,235,64,63,200,28,462,265,85,225,170,108,264,98,173,88,0,326,498,371,483,254,324,5,167,53,403,246,231,128,479,381,8,73,436,272,180,156,35,481,300,327,77,454,72,125,338,51,207,366,100,410,271,239,295,188,348,494,228,44,393,59,322,105,30,260,22,48,123,19,368,86,351,193,343,17,118,395,469,372,329,21,443,96,358,438,157,330,131,206,442,307,2,344,491,396,346,362,99,60,476,423,276,212,414,417,328,75,367,205,296,165,413,280,9,169,477,451,408,41,294,186,84,234,302,379,301,27,365,457,237,102,482,112,184,374,370,398,52,465,20,437,458,400,6,401,450,58,117,467,181,175,238,67,143,140,13,472,222,87,174,478,137,293,132,40,201,257,189,364,347,464,333,244,336,159,147,288,444,399,82,305,197,223,460,449,135,461,219,47,497,155,104,227,42,309,249,142,136,95,221,176,80,318,211,420,89,116,397,493,428,152,3,349,391,320,468,475,253,158,448,92,126,120,404,385,56,332,26,418,39,488,386,278,220,405,484,439,310,179,406,251,416,233,93,154,256,298,487,166,409,435,113,470,433,306,23,32,210,192,452,153,269,139,203,392,337,412,427,240,199,34,134,127,103,321,290,247,217,486,345,387,388,171,232,36,340,319,215,357,29,198,90,242,229,107,151,434,419,196,285,1,145,204,252,421,492,402,316,248,161,431,178,168,129,164,489,38,31,230,11,255,14,209,441,74,37,456,394,213,273,106,62,304,187,277,172,445,12,299,114,121,226,119,289,16,43,160,163,474,216,369,389,383,243,377,471,440,65,263,110,150,353,463,50,94,375,45,432,424,281,130,101,218,422,241,78,7,313,317,355,15,287,430,341,382,133,373,425,266,378,24,311,496,315,490,141,292,376,291,81,144,312,352,122,283,390,350,267,303
//     // };

//     hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key> tree{
//         hivedb::INVALID_PAGE_ID,
//         10,
//         ""
//     };


//     std::vector<std::pair<int_key, int_key>> values;
//     int idx = 0;
//     for (const auto& element: vec) {
//         auto temp = int_key{idx++};
//         REQUIRE(tree.insert(element, temp));
//         spdlog::info("TESTS: Emplacing {} and {} in values", element.key, temp.key);
//         values.emplace_back(element, temp);
//     }


//     for (const auto& element: vec) {
//         int_key found_value{int_key::invalid_key()};
//         tree.find(element, found_value);
//         spdlog::info("TESTS: Trying to search for {} and {}", element.key, found_value.key);
//         if (found_value == int_key::invalid_key()) {
//             tree.dump_contents();
//         }
//         REQUIRE(std::find(values.cbegin(), values.cend(), std::pair{element, found_value}) != values.cend());
//     }

//     } catch (const std::exception& err) {
//         FAIL(err.what());
//     }
// }

// TEST_CASE("b_plus_tree split inner node", "[b_plus_tree_insert_and_split_inner_node]") {
//     try {
//     std::random_device rnd_device;
//     std::mt19937 mersenne_engine {rnd_device()};

//     std::vector<int_key> vec(64000, 0);
//     std::iota(vec.begin(), vec.end(), int_key{0});
//     std::shuffle(vec.begin(), vec.end(), mersenne_engine);

//     // std::vector<int_key> vec{
//     //     258,49,354,214,380,66,33,10,473,411,224,55,115,339,429,182,57,356,455,148,446,323,279,268,459,236,274,286,331,111,297,190,466,149,284,245,54,447,109,191,275,202,359,308,69,325,138,485,124,61,480,79,162,208,4,363,46,335,384,185,68,261,453,18,499,177,495,259,426,83,25,97,76,183,146,262,407,194,334,250,282,361,195,71,91,415,70,314,360,342,270,235,64,63,200,28,462,265,85,225,170,108,264,98,173,88,0,326,498,371,483,254,324,5,167,53,403,246,231,128,479,381,8,73,436,272,180,156,35,481,300,327,77,454,72,125,338,51,207,366,100,410,271,239,295,188,348,494,228,44,393,59,322,105,30,260,22,48,123,19,368,86,351,193,343,17,118,395,469,372,329,21,443,96,358,438,157,330,131,206,442,307,2,344,491,396,346,362,99,60,476,423,276,212,414,417,328,75,367,205,296,165,413,280,9,169,477,451,408,41,294,186,84,234,302,379,301,27,365,457,237,102,482,112,184,374,370,398,52,465,20,437,458,400,6,401,450,58,117,467,181,175,238,67,143,140,13,472,222,87,174,478,137,293,132,40,201,257,189,364,347,464,333,244,336,159,147,288,444,399,82,305,197,223,460,449,135,461,219,47,497,155,104,227,42,309,249,142,136,95,221,176,80,318,211,420,89,116,397,493,428,152,3,349,391,320,468,475,253,158,448,92,126,120,404,385,56,332,26,418,39,488,386,278,220,405,484,439,310,179,406,251,416,233,93,154,256,298,487,166,409,435,113,470,433,306,23,32,210,192,452,153,269,139,203,392,337,412,427,240,199,34,134,127,103,321,290,247,217,486,345,387,388,171,232,36,340,319,215,357,29,198,90,242,229,107,151,434,419,196,285,1,145,204,252,421,492,402,316,248,161,431,178,168,129,164,489,38,31,230,11,255,14,209,441,74,37,456,394,213,273,106,62,304,187,277,172,445,12,299,114,121,226,119,289,16,43,160,163,474,216,369,389,383,243,377,471,440,65,263,110,150,353,463,50,94,375,45,432,424,281,130,101,218,422,241,78,7,313,317,355,15,287,430,341,382,133,373,425,266,378,24,311,496,315,490,141,292,376,291,81,144,312,352,122,283,390,350,267,303
//     // };

//     hivedb::b_plus_tree<hivedb::disk_manager_mock, int_key, int_key, int_key> tree{
//         hivedb::INVALID_PAGE_ID,
//         10,
//         ""
//     };


//     std::vector<std::pair<int_key, int_key>> values;
//     int idx = 0;
//     for (const auto& element: vec) {
//         auto temp = int_key{idx++};
//         REQUIRE(tree.insert(element, temp));
//         spdlog::info("TESTS: Emplacing {} and {} in values", element.key, temp.key);
//         values.emplace_back(element, temp);
//     }


//     for (const auto& element: vec) {
//         int_key found_value{int_key::invalid_key()};
//         tree.find(element, found_value);
//         spdlog::info("TESTS: Trying to search for {}", element.key);
//         spdlog::info("TESTS: FOUND {}", found_value.key);
//         // if (found_value == int_key::invalid_key()) {
//         //     tree.dump_contents();
//         // }
//         REQUIRE(std::find(values.cbegin(), values.cend(), std::pair{element, found_value}) != values.cend());
//     }

//     } catch (const std::exception& err) {
//         FAIL(err.what());
//     }
// }
