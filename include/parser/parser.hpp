#pragma once

#include <fmt/core.h>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <exception>
#include <memory>
#include <parser/tokens.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <data_types/data_types.hpp>
#include <data_types/integer.hpp>
#include <data_types/real.hpp>
#include <data_types/varchar.hpp>
#include <storage_engine/special_types.hpp>

namespace hivedb {

#define DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(class_name) \
  class_name() = default;                                     \
  class_name(const class_name &) = delete;                    \
  class_name &operator=(const class_name &) = delete;         \
  class_name(class_name &&) = default;                        \
  class_name &operator=(class_name &&) = default;

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};

template <typename T, typename U>
concept different_types_somewhat_mathable =
    requires(T a, U b) { a += static_cast<T>(b); } && !std::is_same_v<T, U> &&
    std::is_arithmetic_v<T> && std::is_arithmetic_v<U>;

template <typename T, typename U>
concept somewhat_mathable = requires(T a, T b) { a += b; } &&
                            std::is_same_v<T, U> && std::is_arithmetic_v<T>;

template <typename T, typename U>
concept is_string_view =
    std::is_same_v<T, std::string_view> || std::is_same_v<U, std::string_view>;

struct exprs {
  virtual void prettyPrint(std::stringstream &s) const = 0;
  virtual void retriveColumns(std::vector<std::string_view> &) = 0;

  using values = std::variant<float, int, std::string_view>;

  [[nodiscard]]
  virtual std::variant<float, int, std::string_view, std::vector<values>>
  execute() const = 0;

  [[nodiscard]]
  virtual std::variant<float, int, std::string_view, std::vector<values>>
  execute(const fetched_data_map &, std::size_t) const = 0;

  virtual ~exprs() = default;
};

template <typename T>
concept is_value = std::is_arithmetic_v<T>;

template <typename T, typename U>
concept is_vector_of_values_and_value =
    (std::is_same_v<T, std::vector<exprs::values>> && is_value<U>);

template <typename T, typename U>
concept is_value_and_vector_of_values =
    (is_value<T> && std::is_same_v<U, std::vector<exprs::values>>);

template <typename T>
struct literal_expr final : public exprs {
  T value;
  bool isIdentifier;

  explicit literal_expr(T &&v, bool isIdntf)
      : value(v), isIdentifier(isIdntf) {};

  void prettyPrint(std::stringstream &s) const override {
    s << "( literal: " << value << " )";
  }

  void retriveColumns(std::vector<std::string_view> &columns) override {
    if constexpr (std::is_same_v<T, std::string>) {
      if (isIdentifier) {
        columns.push_back(value);
      }
    }
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    return value;
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute(
      [[maybe_unused]] const fetched_data_map &fetched_data,
      [[maybe_unused]] std::size_t idx) const override {
    if constexpr (std::is_same_v<T, std::string>) {
      if (isIdentifier) {
        const fetched_columns &clm = fetched_data.find(value)->second[idx];
        std::variant<float, int, std::string_view, std::vector<values>>
            clm_value;
        switch (clm.dt) {
          case data_types::integer:
            clm_value = integer::deserialize(clm.ptr);
            break;
          case data_types::real:
            clm_value = real::deserialize(clm.ptr);
            break;
          case data_types::varchar:
            clm_value = varchar::deserialize(clm.ptr);
            break;
          default:
            throw std::invalid_argument("Unknown data type encountered during column deserialization");
        }

        return clm_value;
      } else {
        return std::string_view{value};
      }
    } else {
      return value;
    }
  }
};

struct unary_expr final : public exprs {
  token_type op{};
  std::unique_ptr<exprs> rhs{};

  void prettyPrint(std::stringstream &s) const override {
    if (op == token_type::bang) s << "!";
    if (op == token_type::_not) s << "NOT ";
    if (op == token_type::substract) s << "-";
    rhs->prettyPrint(s);
  }
  void retriveColumns(std::vector<std::string_view> &columns) override {
    rhs->retriveColumns(columns);
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>>
  solveExprs(std::variant<float, int, std::string_view, std::vector<values>>
                 &expr) const {
    const auto applyOperator = [this]<typename T>(T &value) {
      if (op == token_type::bang || op == token_type::_not) {
        if constexpr (std::is_integral_v<T>) {
          value = (value == 0 ? 1 : 0);
        } else if constexpr (std::is_floating_point_v<T>) {
          value = (value == 0.0f ? 1.0f : 0.0f);
        }
      } else if (op == token_type::substract) {
        value = -value;
      }
    };

    static auto handleStrings = [](std::string_view) {
      throw std::invalid_argument(
          "Cannot apply unary operator to string value");
    };

    const auto handleMultipleValues = [this, &applyOperator](std::vector<values> &value) {
      if (value.size() != 1)
        throw std::invalid_argument(
            "Unary operator requires a single scalar value, but got multiple values");
      std::visit(overload{applyOperator, handleStrings}, value[0]);
    };

    std::visit(overload{applyOperator, handleStrings, handleMultipleValues},
               expr);

    return expr;
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    auto expr = rhs->execute();
    return solveExprs(expr);
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute(
      const fetched_data_map &fetched_values, std::size_t idx) const override {
    auto expr = rhs->execute(fetched_values, idx);
    return solveExprs(expr);
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(unary_expr)
};

struct grouping_expr final : public exprs {
  std::unique_ptr<exprs> expr;

  void prettyPrint(std::stringstream &s) const override {
    s << "( grouping: ";
    expr->prettyPrint(s);
    s << ")";
  };

  void retriveColumns(std::vector<std::string_view> &columns) override {
    expr->retriveColumns(columns);
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    return expr->execute();
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute(
      const fetched_data_map &fetched_values, std::size_t idx) const override {
    return expr->execute(fetched_values, idx);
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(grouping_expr)
};

struct wildcard_expr final : public exprs {
  void prettyPrint(std::stringstream &s) const override {
    s << "*";
  }

  void retriveColumns(std::vector<std::string_view> &columns) override {
    columns.emplace_back("*");
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    return std::string_view{"*"};
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute(
      [[maybe_unused]] const fetched_data_map &,
      [[maybe_unused]] std::size_t) const override {
    return std::string_view{"*"};
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(wildcard_expr)
};

struct binary_expr final : public exprs {
  std::unique_ptr<exprs> lhs;
  token_type op{};
  std::unique_ptr<exprs> rhs;

  void prettyPrint(std::stringstream &s) const override {
    s << "binary: ( ";
    lhs->prettyPrint(s);
    if (op == token_type::add) s << " + ";
    else if (op == token_type::substract) s << " - ";
    else if (op == token_type::divide) s << " / ";
    else if (op == token_type::star) s << " * ";
    else if (op == token_type::equal) s << " = ";
    else if (op == token_type::not_equal) s << " != ";
    else if (op == token_type::less) s << " < ";
    else if (op == token_type::greater) s << " > ";
    else if (op == token_type::less_equal) s << " <= ";
    else if (op == token_type::greater_equal) s << " >= ";
    else if (op == token_type::_and) s << " AND ";
    else if (op == token_type::_or) s << " OR ";
    rhs->prettyPrint(s);
    s << ") ";
  };

  void retriveColumns(std::vector<std::string_view> &columns) override {
    lhs->retriveColumns(columns);
    rhs->retriveColumns(columns);
  }

  inline std::variant<float, int, std::string_view, std::vector<values>>
  solveExprs(
      std::variant<float, int, std::string_view, std::vector<values>> &leftExpr,
      std::variant<float, int, std::string_view, std::vector<values>>
          &rightExpr) const {
    if (auto *lv = std::get_if<std::vector<values>>(&leftExpr)) {
      if (lv->size() != 1) {
        throw std::invalid_argument(
            "Binary operation requires scalar operands, but operand contains multiple values");
      }
      std::variant<float, int, std::string_view, std::vector<values>> lScalar;
      std::visit([&](auto &&val) { lScalar = val; }, (*lv)[0]);
      leftExpr = solveExprs(lScalar, rightExpr);
      return leftExpr;
    }

    if (auto *rv = std::get_if<std::vector<values>>(&rightExpr)) {
      if (rv->size() != 1) {
        throw std::invalid_argument(
            "Binary operation requires scalar operands, but operand contains multiple values");
      }
      std::variant<float, int, std::string_view, std::vector<values>> rScalar;
      std::visit([&](auto &&val) { rScalar = val; }, (*rv)[0]);
      return solveExprs(leftExpr, rScalar);
    }

    const auto handleMathAndComparison = [&leftExpr, this]<typename T, typename U>(const T &l, const U &r) {
      if constexpr (std::is_same_v<T, std::vector<values>> || std::is_same_v<U, std::vector<values>>) {
        return;
      } else if constexpr (std::is_same_v<T, std::string_view> && std::is_same_v<U, std::string_view>) {
        if (op == token_type::equal) {
          leftExpr = (l == r ? 1 : 0);
        } else if (op == token_type::not_equal) {
          leftExpr = (l != r ? 1 : 0);
        } else if (op == token_type::less) {
          leftExpr = (l < r ? 1 : 0);
        } else if (op == token_type::less_equal) {
          leftExpr = (l <= r ? 1 : 0);
        } else if (op == token_type::greater) {
          leftExpr = (l > r ? 1 : 0);
        } else if (op == token_type::greater_equal) {
          leftExpr = (l >= r ? 1 : 0);
        } else {
          throw std::invalid_argument("Cannot apply binary arithmetic operations to string values");
        }
      } else if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<U, std::string_view>) {
        if (op == token_type::equal) {
          leftExpr = 0;
        } else if (op == token_type::not_equal) {
          leftExpr = 1;
        } else {
          throw std::invalid_argument("Cannot compare or perform arithmetic between string and numeric values");
        }
      } else if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) {
        if (op == token_type::add) {
          if constexpr (std::is_same_v<T, float> || std::is_same_v<U, float>) {
            leftExpr = static_cast<float>(l) + static_cast<float>(r);
          } else {
            leftExpr = static_cast<int>(l) + static_cast<int>(r);
          }
        } else if (op == token_type::substract) {
          if constexpr (std::is_same_v<T, float> || std::is_same_v<U, float>) {
            leftExpr = static_cast<float>(l) - static_cast<float>(r);
          } else {
            leftExpr = static_cast<int>(l) - static_cast<int>(r);
          }
        } else if (op == token_type::star) {
          if constexpr (std::is_same_v<T, float> || std::is_same_v<U, float>) {
            leftExpr = static_cast<float>(l) * static_cast<float>(r);
          } else {
            leftExpr = static_cast<int>(l) * static_cast<int>(r);
          }
        } else if (op == token_type::divide) {
          if constexpr (std::is_same_v<T, float> || std::is_same_v<U, float>) {
            leftExpr = static_cast<float>(l) / static_cast<float>(r);
          } else {
            leftExpr = static_cast<int>(l) / static_cast<int>(r);
          }
        } else if (op == token_type::equal) {
          leftExpr = (l == r ? 1 : 0);
        } else if (op == token_type::not_equal) {
          leftExpr = (l != r ? 1 : 0);
        } else if (op == token_type::less) {
          leftExpr = (l < r ? 1 : 0);
        } else if (op == token_type::less_equal) {
          leftExpr = (l <= r ? 1 : 0);
        } else if (op == token_type::greater) {
          leftExpr = (l > r ? 1 : 0);
        } else if (op == token_type::greater_equal) {
          leftExpr = (l >= r ? 1 : 0);
        } else if (op == token_type::_and) {
          leftExpr = ((l != 0 && r != 0) ? 1 : 0);
        } else if (op == token_type::_or) {
          leftExpr = ((l != 0 || r != 0) ? 1 : 0);
        }
      }
    };

    std::visit(
        [&](const auto &l, const auto &r) {
          handleMathAndComparison(l, r);
        },
        leftExpr, rightExpr);

    return leftExpr;
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute(
      const fetched_data_map &fetched_values, std::size_t idx) const override {
    auto leftExpr = lhs->execute(fetched_values, idx);
    auto rightExpr = rhs->execute(fetched_values, idx);

    return solveExprs(leftExpr, rightExpr);
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    auto leftExpr = lhs->execute();
    auto rightExpr = rhs->execute();

    return solveExprs(leftExpr, rightExpr);
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(binary_expr)
};

struct table_column {
  std::string_view name;
  std::string_view type;
  bool can_be_null;
};

struct create_tbl_expr final : public exprs {
  std::string_view tblName;
  std::vector<table_column> tblColumns;

  void prettyPrint(std::stringstream &s) const override {
    s << "Table name: " << tblName << "\n";
    for (auto &c : tblColumns) {
      s << "Name: " << c.name << " Type: " << c.type
        << " Can be null: " << (c.can_be_null ? "true" : "false") << "\n";
    }
  };

  void retriveColumns(std::vector<std::string_view> &) override {
    throw std::invalid_argument("Statement cannot be directly evaluated; must be executed by storage engine");
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    throw std::invalid_argument("Statement cannot be directly evaluated; must be executed by storage engine");
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<exprs::values>>
  execute(const fetched_data_map &, std::size_t) const override {
    throw std::invalid_argument("Statement cannot be directly evaluated; must be executed by storage engine");
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(create_tbl_expr)
};

struct insert_expr final : public exprs {
  std::string_view tblName;
  std::vector<std::string_view> columns;
  std::vector<std::variant<std::string_view, int, float>> values;

  void prettyPrint(std::stringstream &s) const override {
    s << "Table name: " << tblName << "\n";
    for (auto &c : columns) {
      s << "Column: " << c << " \n";
    }

    for (auto &v : values) {
      std::visit(
          overload{
              [&s](int v) { s << "Value: " << v << " \n"; },
              [&s](float v) { s << "Value: " << v << " \n"; },
              [&s](std::string_view v) { s << "Value: " << v << " \n"; },
          },
          v);
    }
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<exprs::values>>
  execute() const override {
    throw std::invalid_argument("Statement cannot be directly evaluated; must be executed by storage engine");
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<exprs::values>>
  execute(const fetched_data_map &, std::size_t) const override {
    throw std::invalid_argument("Statement cannot be directly evaluated; must be executed by storage engine");
  }

  void retriveColumns(std::vector<std::string_view> &) override {
    throw std::invalid_argument("Statement cannot be directly evaluated; must be executed by storage engine");
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(insert_expr)
};

struct select_expr final : public exprs {
  std::vector<std::unique_ptr<exprs>> innerExpr;
  std::string_view tblName;

  std::unique_ptr<exprs> whereExpr;

  [[nodiscard]] bool hasWildcard() const noexcept {
    for (const auto &expr : innerExpr) {
      if (dynamic_cast<const wildcard_expr *>(expr.get()) != nullptr) {
        return true;
      }
    }
    return false;
  }

  void prettyPrint(std::stringstream &s) const override {
    s << "select: (";

    for (const auto &expr : innerExpr) {
      expr->prettyPrint(s);
      s << ", ";
    }

    s << ")";
    s << " from table: " << (tblName.length() ? tblName : "NULL");
    if (whereExpr) {
      s << " where: ";
      whereExpr->prettyPrint(s);
    }
    s << "\n";
  };

  void retriveColumns(std::vector<std::string_view> &columnsTofetch) override {
    for (auto &exp : innerExpr) {
      exp->retriveColumns(columnsTofetch);
    }
    if (whereExpr) {
      whereExpr->retriveColumns(columnsTofetch);
    }
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<exprs::values>>
  execute(const fetched_data_map &fetchedValues, std::size_t i) const override {
    if (innerExpr.size() == 1) {
      return innerExpr[0]->execute(fetchedValues, i);
    }

    std::vector<values> v;
    for (const auto &expr : innerExpr) {
      std::visit(
          [&v](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::vector<values>>) {
              if (arg.size() != 1)
                throw std::invalid_argument(
                    "Scalar sub-expression evaluated to multiple values");
              v.push_back(arg[0]);
            } else {
              v.push_back(arg);
            }
          },
          expr->execute(fetchedValues, i));
    }

    return v;
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    if (innerExpr.size() == 1) {
      return innerExpr[0]->execute();
    }
    std::vector<values> v;
    for (const auto &expr : innerExpr) {
      std::visit(
          [&v](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::vector<values>>) {
              if (arg.size() != 1)
                throw std::invalid_argument(
                    "Scalar sub-expression evaluated to multiple values");
              v.push_back(arg[0]);
            } else {
              v.push_back(arg);
            }
          },
          expr->execute());
    }

    return v;
  }

  [[nodiscard]]
  std::vector<std::string_view> retriveColumnsToBeFetched() const {
    std::vector<std::string_view> columns;

    for (const auto &expr : innerExpr) {
      expr->retriveColumns(columns);
    }
    if (whereExpr) {
      whereExpr->retriveColumns(columns);
    }

    return columns;
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(select_expr)
};

class parser {
 private:
  std::vector<token> m_tokens;

  std::size_t m_cursor{0};

  [[nodiscard]]
  inline const token &peek() const noexcept;
  [[nodiscard]]
  inline const token &previous() const noexcept;
  [[nodiscard]]
  inline const token &current() const noexcept;

  inline const token &advance() noexcept;

  template <std::same_as<token_type>... T>
  bool match(T...) noexcept;

  [[nodiscard]]
  inline bool isDone() const noexcept;
  [[nodiscard]]
  inline bool check(token_type) const noexcept;

  inline void consume(token_type, std::string_view);

  [[nodiscard]]
  inline std::unique_ptr<exprs> expr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> commaExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> binaryExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> logicalOrExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> logicalAndExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> comparisonExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> additiveExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> multiplicativeExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> unaryExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> primaryExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> selectExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> createExpr();

  [[nodiscard]]
  inline std::unique_ptr<exprs> insertExpr();

 public:
  explicit parser(const std::vector<token> &t);

  std::unique_ptr<exprs> parse();

  parser(const parser &) = delete;
  parser &operator=(const parser &) = delete;

  parser(parser &&) = default;
  parser &operator=(parser &&) = default;

  ~parser() = default;
};

}  // namespace hivedb
