#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <parser/lexer.hpp>
#include <parser/parser.hpp>
#include <storage_engine/storage_engine.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string trim(std::string_view s) {
  auto start = s.begin();
  while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) {
    ++start;
  }
  auto end = s.end();
  do {
    --end;
  } while (std::distance(start, end) > 0 &&
           std::isspace(static_cast<unsigned char>(*end)));
  return std::string(start, end + 1);
}

void printResultTable(const hivedb::query_result &result) {
  if (result.column_names.empty() && result.rows.empty()) {
    std::cout << "(empty set)\n";
    return;
  }

  std::vector<std::size_t> widths;
  widths.reserve(result.column_names.size());
  for (const auto &col : result.column_names) {
    widths.push_back(col.empty() ? 8 : col.size());
  }

  std::vector<std::vector<std::string>> str_rows;
  str_rows.reserve(result.rows.size());

  for (const auto &row : result.rows) {
    std::vector<std::string> str_row;
    str_row.reserve(row.size());
    for (std::size_t j = 0; j < row.size(); ++j) {
      std::string val_str;
      std::visit(
          hivedb::overload{
              [&val_str](int v) { val_str = std::to_string(v); },
              [&val_str](float v) {
                val_str = std::to_string(v);
                while (val_str.size() > 1 && val_str.back() == '0') {
                  val_str.pop_back();
                }
                if (!val_str.empty() && val_str.back() == '.') {
                  val_str.push_back('0');
                }
              },
              [&val_str](std::string_view v) { val_str = std::string(v); },
              [&val_str](const auto &) { val_str = "<unsupported>"; }},
          row[j]);

      if (j < widths.size()) {
        widths[j] = std::max(widths[j], val_str.size());
      } else {
        widths.push_back(val_str.size());
      }
      str_row.push_back(std::move(val_str));
    }
    str_rows.push_back(std::move(str_row));
  }

  for (std::size_t i = 0; i < widths.size(); ++i) {
    std::string name = (i < result.column_names.size() && !result.column_names[i].empty())
                           ? result.column_names[i]
                           : "?column?";
    widths[i] = std::max(widths[i], name.size());
  }

  auto print_separator = [&widths]() {
    std::cout << "+";
    for (std::size_t w : widths) {
      std::cout << std::string(w + 2, '-') << "+";
    }
    std::cout << "\n";
  };

  print_separator();

  std::cout << "|";
  for (std::size_t i = 0; i < widths.size(); ++i) {
    std::string name = (i < result.column_names.size() && !result.column_names[i].empty())
                           ? result.column_names[i]
                           : "?column?";
    std::cout << " " << name << std::string(widths[i] - name.size() + 1, ' ') << "|";
  }
  std::cout << "\n";

  print_separator();

  for (const auto &row : str_rows) {
    std::cout << "|";
    for (std::size_t i = 0; i < widths.size(); ++i) {
      std::string cell = (i < row.size()) ? row[i] : "";
      std::cout << " " << cell << std::string(widths[i] - cell.size() + 1, ' ') << "|";
    }
    std::cout << "\n";
  }

  print_separator();
  std::cout << result.rows.size() << " row(s) in set\n";
}

}  // namespace

int main(int argc, char **argv) {
  std::cout << "========================================\n";
  std::cout << "  Welcome to HiveDB Interactive REPL!   \n";
  std::cout << "  Type SQL ending with ';' or '.exit'   \n";
  std::cout << "========================================\n\n";

  std::unique_ptr<hivedb::storage_engine> engine;
  try {
    if (argc > 1) {
      std::cout << "Opening database: " << argv[1] << "\n";
      engine = std::make_unique<hivedb::storage_engine>(argv[1]);
    } else {
      std::cout << "No database file provided. Using temporary database.\n";
      engine = std::make_unique<hivedb::storage_engine>();
    }
  } catch (const std::exception &e) {
    std::cerr << "Failed to initialize storage engine: " << e.what() << "\n";
    return 1;
  }

  std::string accumulated_input;
  std::string line;

  while (true) {
    if (accumulated_input.empty()) {
      std::cout << "hive> ";
    } else {
      std::cout << "   -> ";
    }

    if (!std::getline(std::cin, line)) {
      std::cout << "\nBye!\n";
      break;
    }

    std::string trimmed_line = trim(line);
    if (accumulated_input.empty()) {
      if (trimmed_line == ".exit" || trimmed_line == ".quit" || trimmed_line == "exit" || trimmed_line == "quit") {
        std::cout << "Bye!\n";
        break;
      }
      if (trimmed_line == ".help") {
        std::cout << "Commands:\n";
        std::cout << "  .exit, .quit  Exit REPL\n";
        std::cout << "  .help         Show this help message\n";
        std::cout << "SQL statements must terminate with a semicolon ';'\n\n";
        continue;
      }
      if (trimmed_line.empty()) {
        continue;
      }
    }

    accumulated_input += line + " ";

    std::size_t semicolon_pos = accumulated_input.find(';');
    while (semicolon_pos != std::string::npos) {
      std::string statement = trim(accumulated_input.substr(0, semicolon_pos));
      accumulated_input = accumulated_input.substr(semicolon_pos + 1);

      if (!statement.empty()) {
        try {
          hivedb::lexer lex(statement);
          hivedb::parser p(lex.getTokens());
          auto ast = p.parse();

          if (!ast) {
            std::cerr << "Error: parsed empty statement\n";
          } else if (auto *create_stmt = dynamic_cast<hivedb::create_tbl_expr *>(ast.get())) {
            engine->createTable(create_stmt);
            std::cout << "Table created successfully.\n";
          } else if (auto *insert_stmt = dynamic_cast<hivedb::insert_expr *>(ast.get())) {
            engine->insertIntoTable(insert_stmt);
            std::cout << "1 row inserted.\n";
          } else if (auto *select_stmt = dynamic_cast<hivedb::select_expr *>(ast.get())) {
            auto result = engine->executeSelect(select_stmt);
            printResultTable(result);
          } else {
            std::cerr << "Unsupported statement type.\n";
          }
        } catch (const std::exception &e) {
          std::cerr << "Error: " << e.what() << "\n";
        }
      }

      semicolon_pos = accumulated_input.find(';');
    }

    if (trim(accumulated_input).empty()) {
      accumulated_input.clear();
    }
  }

  return 0;
}
