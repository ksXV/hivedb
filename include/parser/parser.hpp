#pragma once

#include <data_types/integer.hpp>
#include <data_types/real.hpp>
#include <data_types/varchar.hpp>
#include <memory>
#include <parser/lexer.hpp>
#include <parser/tokens.hpp>
#include <sstream>
#include <stdexcept>
#include <storage_engine/special_types.hpp>
#include <string_view>
#include <type_traits>
#include <variant>

#define DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(name) \
  name() = default;                                      \
  name(const name &) = delete;                           \
  name &operator=(const name &) = delete;                \
  name(name &&) = default;                               \
  name &operator=(name &&) = default;                    \
  ~name() override = default;

namespace hivedb {

template <typename... Ts>
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
      const fetched_data_map &fetched_data, std::size_t idx) const override {
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
        }
        return clm_value;
      } else {
        return value;
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
    if (op == token_type::substract) s << "-";
    rhs->prettyPrint(s);
  }
  void retriveColumns(std::vector<std::string_view> &columns) override {
    rhs->retriveColumns(columns);
  }

  [[nodiscard]]
  inline std::variant<float, int, std::string_view, std::vector<values>>
  solveExprs(std::variant<float, int, std::string_view, std::vector<values>>
                 &expr) const {
    static auto applyOperator = []<typename T>(T &value) { value = -value; };

    static auto handleStrings = [](std::string_view) {
      throw std::invalid_argument(
          "Cannot <insert unary operation here> strings!");
    };

    static auto handleMultipleValues = [](std::vector<values> &value) {
      if (value.size() != 1)
        throw std::invalid_argument(
            "Cannot <insert unary operation here> multiple values!");
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

struct binary_expr final : public exprs {
  std::unique_ptr<exprs> lhs;
  token_type op{};
  std::unique_ptr<exprs> rhs;

  void prettyPrint(std::stringstream &s) const override {
    s << "binary: ( ";
    lhs->prettyPrint(s);
    if (op == token_type::add) s << " + ";
    if (op == token_type::substract) s << " - ";
    if (op == token_type::divide) s << " / ";
    if (op == token_type::star) s << " * ";
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
    static auto handleSameType = [this]<typename T>
      requires somewhat_mathable<T, T>
    (T & l, T & r) -> void {
      if (op == token_type::add) {
        l += r;
        return;
      }
      if (op == token_type::substract) {
        l -= r;
        return;
      }
      if (op == token_type::divide) {
        l /= r;
        return;
      }
      if (op == token_type::star) {
        l *= r;
        return;
      }
    };

    static auto handleDifferentTypes = [this]<typename T, typename U>
      requires different_types_somewhat_mathable<T, U>
    (T & l, U & r) -> void {
      if (op == token_type::add) {
        l += static_cast<T>(r);
        return;
      }
      if (op == token_type::substract) {
        l -= static_cast<T>(r);
        return;
      }
      if (op == token_type::divide) {
        l /= static_cast<T>(r);
        return;
      }
      if (op == token_type::star) {
        l *= static_cast<T>(r);
        return;
      }
    };

    static auto handleEitherString = []<typename T, typename U>
      requires is_string_view<T, U>
    (T &, U &) -> void {
      throw std::invalid_argument("Cannot whatever strings.");
    };

    static auto handleEitherVector1 = [this]<typename T, typename U>
      requires is_value_and_vector_of_values<T, U>
    (T & l, U & r) -> void {
      if (r.size() != 1)
        throw std::invalid_argument(
            "Cannot <binary expression> something something.");

      values &rv = r[0];

      static auto handleNumericTypes = [&l, this]<typename V>(V rh) {
        using W = std::decay_t<decltype(l)>;
        if constexpr (std::is_same_v<W, V>) {
          if (op == token_type::add) {
            l += rh;
            return;
          }
          if (op == token_type::substract) {
            l -= rh;
            return;
          }
          if (op == token_type::divide) {
            l /= rh;
            return;
          }
          if (op == token_type::star) {
            l *= rh;
            return;
          }
        } else if constexpr (std::is_same_v<W, W>) {
          if (op == token_type::add) {
            l += static_cast<W>(rh);
            return;
          }
          if (op == token_type::substract) {
            l -= static_cast<W>(rh);
            return;
          }
          if (op == token_type::divide) {
            l /= static_cast<W>(rh);
            return;
          }
          if (op == token_type::star) {
            l *= static_cast<W>(rh);
            return;
          }
        } else {
          static_assert(false, "RAAAAAAAAAAAAAAAAAAAAA");
        }
      };

      std::visit(
          overload{handleNumericTypes,
                   [](std::string_view) {
                     throw std::invalid_argument("Cannot whatever strings.");
                   }},
          rv);
    };

    static auto handleEitherVector2 = [this]<typename T, typename U>
      requires is_vector_of_values_and_value<T, U>
    (T & l, U r) -> void {
      if (l.size() != 1)
        throw std::invalid_argument(
            "Cannot <binary expression> something something.");
      values &lv = l[0];
      static auto handleNumericTypes = [&r, this]<typename V>(V lh) {
        using W = std::decay_t<decltype(lh)>;
        if constexpr (std::is_same_v<W, V>) {
          if (op == token_type::add) {
            lh += r;
            return;
          }
          if (op == token_type::substract) {
            lh -= r;
            return;
          }
          if (op == token_type::divide) {
            lh /= r;
            return;
          }
          if (op == token_type::star) {
            lh *= r;
            return;
          }
        } else if constexpr (std::is_same_v<W, W>) {
          if (op == token_type::add) {
            lh += static_cast<W>(r);
            return;
          }
          if (op == token_type::substract) {
            lh -= static_cast<W>(r);
            return;
          }
          if (op == token_type::divide) {
            lh /= static_cast<W>(r);
            return;
          }
          if (op == token_type::star) {
            lh *= static_cast<W>(r);
            return;
          }
        } else {
          static_assert(false, "RAAAAAAAAAAAAAAAAAAAAA");
        }
      };
      std::visit(
          overload{handleNumericTypes,
                   [](std::string_view) {
                     throw std::invalid_argument("Cannot whatever strings.");
                   }},
          lv);
    };

    std::visit(
        overload{
            handleSameType,
            handleDifferentTypes,
            handleEitherString,
            handleEitherVector1,
            handleEitherVector2,
            [](std::vector<values> &l, std::vector<values> &r) -> void {
              if (l.size() != 1 && r.size() != 1)
                throw std::invalid_argument(
                    "Cannot <binary expression> something something.");
              std::visit(
                  overload{
                      handleSameType,
                      handleDifferentTypes,
                      handleEitherString,
                  },
                  l[0], r[0]);
            },
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
        << " Null?: " << (c.can_be_null ? "yes" : "no");
      s << "\n";
    }
  }

  void retriveColumns(std::vector<std::string_view> &) override {
    throw std::invalid_argument("INVALID CALL!");
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<values>> execute()
      const override {
    throw std::invalid_argument("INVALID CALL!");
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<exprs::values>>
  execute(const fetched_data_map &, std::size_t) const override {
    throw std::invalid_argument("INVALID CALL!");
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(create_tbl_expr)
};

struct insert_expr final : public exprs {
  std::string_view tblName;
  std::vector<std::variant<std::string_view, int, float>> values;
  std::vector<std::string_view> columns;

  void prettyPrint(std::stringstream &s) const override {
    s << "insert into: " << tblName << "values: \n";
    for (std::size_t i = 0; i < columns.size(); ++i) {
      s << columns[i] << " value -> ";
      std::visit([&s](auto &&v) { s << v; }, values[i]);
      s << "\n";
    }
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<exprs::values>>
  execute() const override {
    throw std::invalid_argument("INVALID CALL!");
  }

  [[nodiscard]]
  std::variant<float, int, std::string_view, std::vector<exprs::values>>
  execute(const fetched_data_map &, std::size_t) const override {
    throw std::invalid_argument("INVALID CALL!");
  }

  void retriveColumns(std::vector<std::string_view> &) override {
    throw std::invalid_argument("INVALID CALL!");
  }

  DEFAULT_CONSTRUCT_REMOVE_COPY_DEFAULT_MOVE(insert_expr)
};

struct select_expr final : public exprs {
  std::vector<std::unique_ptr<exprs>> innerExpr;
  std::string_view tblName;

  // TODO: Replace this later
  std::unique_ptr<exprs> whereExpr;

  void prettyPrint(std::stringstream &s) const override {
    s << "select: (";

    for (const auto &expr : innerExpr) {
      expr->prettyPrint(s);
      s << ", ";
    }

    s << ")";
    s << " from table: " << (tblName.length() ? tblName : "NULL");
    s << "\n";
  };

  void retriveColumns(std::vector<std::string_view> &columnsTofetch) override {
    for (auto &exp : innerExpr) {
      exp->retriveColumns(columnsTofetch);
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
                    "An expression cannot return more than 1 value!");
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
                    "An expression cannot return more than 1 value!");
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
