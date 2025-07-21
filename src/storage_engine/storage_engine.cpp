#include "storage_engine/special_types.hpp"
#include <data_types/integer.hpp>
#include <data_types/real.hpp>
#include <data_types/varchar.hpp>
#include <cassert>

#include <iostream>
#include <stdexcept>
#include <storage_engine/storage_engine.hpp>
#include <data_types/data_types.hpp>
#include <string>
#include <type_traits>
#include <unordered_map>

//remove this later
struct myint {
  std::byte value;
};

template <> struct fmt::formatter<myint> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  auto format(myint n, format_context& ctx) const {
    return fmt::format_to(ctx.out(), "z: {0:x} {0}", n.value);
  }
};

//^^^^^^^^^^^^^^^
namespace hivedb{
    void table::insertData(std::variant<std::string_view, int, float> value) {
        std::visit([this](auto&& v){
           using T = std::decay_t<decltype(v)>;
           if constexpr (std::is_same_v<float, T>) {
              real::serialize(v, data);
           } else if constexpr (std::is_same_v<int, T>) {
              integer::serialize(v, data);
           } else if constexpr (std::is_same_v<std::string_view, T>) {
              varchar::serialize(v, data, strings);
           } else {
              static_assert(false, "RAHHHHHHH");
           }
        }, value);
    }


    auto table::fetch(const std::vector<column_to_fetch>& clms) {
        fetched_data_map fetchedData{};

        std::size_t idx = 0;
        while (idx < data.size()) {
          for (const auto& clm: clms) {
              std::byte* ptr = &data[idx+clm.offset];
              auto it = fetchedData.find(clm.name);
              if (it == fetchedData.end()) {
                std::vector<fetched_columns> x{{ptr, clm.dt}};
                fetchedData.emplace(clm.name, std::move(x));
              } else {
                it->second.push_back({ptr, clm.dt});
              }
          }

          idx += rowSize;
        }

        return fetchedData;
   }

   void storage_engine::createTable(create_tbl_expr* expr) {
       assert(expr != nullptr);

       table new_table{};
       for (const auto& column : expr->tblColumns) {
           if (!isTypeValid(column.type)) throw std::invalid_argument("Invalid type detected! Wtf is: " + std::string(column.type));
           new_table.columns.emplace_back(std::string(column.name), fromString(column.type));
           new_table.rowSize += findOffset(fromString(column.type));
       }

       tables.insert({std::string(expr->tblName), std::move(new_table)});

       fmt::println("OK! Created table with name {}", expr->tblName);
   }

   void storage_engine::queryDataFromTable(select_expr* expr) {
       assert(expr != nullptr);

       auto columnsToFetch = expr->retriveColumnsToBeFetched();
       if (columnsToFetch.empty()) {
           if (!expr->tblName.empty()) {
               auto it = tables.find(std::string(expr->tblName));
               if (it == tables.end()) throw std::invalid_argument("Table: " + std::string(expr->tblName) + " doesn't exist!");
           }
           auto value = expr->execute();
           std::visit(overload{
               [](const std::vector<exprs::values>& v) {
                   for (const auto& vl : v) {
                       std::visit(overload{
                           [](auto&& val) {std::cout << val << ", ";},
                       }, vl);
                   }
                   std::cout << '\b';
                   std::cout << '\b';
                   std::cout << "  ";
                   std::cout << std::endl;
               },
               [](int v) {std::cout << "\n" << v << std::endl;},
               [](float v) {std::cout << "\n" << v << std::endl;},
               [](std::string_view v) {std::cout << "\n" << v << std::endl;}
           }, value);

           return;
       }
       auto it = tables.find(std::string(expr->tblName));
       if (it == tables.end()) throw std::invalid_argument("Table: " + std::string(expr->tblName) + " doesn't exist!");
       auto& [tblName, tbl] = *it;

       std::vector<column_to_fetch> dataToFetch;
       for (const auto& column : columnsToFetch) {
           bool found = false;
           std::size_t offset = 0;
           for (const auto& [name, dt]: tbl.columns) {
               if (name == column)  {
                  found = true;
                  column_to_fetch clmn{name, offset, dt};
                  dataToFetch.push_back(clmn);
                  break;
               }
               offset += findOffset(dt);
           }
           if (!found) throw std::invalid_argument("Couldn't find the column " + std::string(column) + "in table named" + tblName);
      }

      const auto fetchedData = tbl.fetch(dataToFetch);
      const auto maxIndex = fetchedData.begin()->second.size();

      for (std::size_t i = 0; i < maxIndex; ++i) {
          auto value = expr->execute(fetchedData, i);
          std::visit(overload{
                     [](const std::vector<exprs::values>& v) {
                         for (const auto& vl : v) {
                             std::visit(overload{
                                 [](auto&& val) {std::cout << val << ", ";},
                             }, vl);
                         }
                         std::cout << '\b';
                         std::cout << '\b';
                         std::cout << "  ";
                         std::cout << std::endl;
                     },
                     [](int v) {std::cout << "\n" << v << std::endl;},
                     [](float v) {std::cout << "\n" << v << std::endl;},
                     [](std::string_view v) {std::cout << "\n" << v << std::endl;}
                 }, value);
          std::cout << std::endl;
      }
   }

   void storage_engine::insertIntoTable(insert_expr* expr) {
      assert(expr != nullptr);
      auto it = tables.find(std::string(expr->tblName));
      if (it == tables.end()) throw std::invalid_argument("Table: " + std::string(expr->tblName) + " doesn't exist!");

      auto& [tblName, tbl] = *it;
      for (const auto exprColumn : expr->columns) {
          bool found = false;
          for (const auto& pair : tbl.columns) {
              if (pair.first == exprColumn)  {
                 found = true;
                 break;
              }
          }
          if (!found) throw std::invalid_argument("Couldn't find the column " + std::string(exprColumn) + "in table named" + tblName);
     }

     for (const auto& [name, dt]: tbl.columns) {
         std::size_t i = 0;
         bool found = false;
         for (; i < expr->columns.size(); ++i) {
             if (expr->columns[i] == name) {
                 found = true;
                 break;
             };
        }
        if (found) {
           tbl.insertData(expr->values[i]);
        } else {
           tbl.insertData({0});
        }
    }

    //for debug purposes:
    int x{0};
    for (auto b : tbl.data) {
        fmt::print("byte {}:  {}\n", x++, myint{b});
    }
  }


}
