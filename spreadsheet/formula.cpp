#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category) : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
  return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
  return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
  if (category_ == Category::Div0) {
    return "#DIV/0!"sv;
  } else if (category_ == Category::Ref) {
    return "#REF!"sv;
  } else if (category_ == Category::Value) {
    return "#VALUE!"sv;
  }
  return "error:("sv;
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
  return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
 public:
  // Реализуйте следующие методы:
  explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
    try {
    } catch (...) {
      throw FormulaException("");
    }
  }

  Value Evaluate(const SheetInterface& sheet) const override {
    Value result;
    try {
      result = ast_.Execute(sheet);
    } catch (FormulaError& exeption) {
      result = exeption;
    }
    return result;
  }

  std::string GetExpression() const override {
    std::stringstream output;
    ast_.PrintFormula(output);
    return output.str();
  }

  CellsOrder GetReferencedCells() const override {
    std::forward_list<Position> cells{ast_.GetCells()};
    CellsOrder result;
    for (Position pos : cells) {
      result.push_back(pos);
    }
    ClearVector(result);
    return result;
  }

 private:
  FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("");
    }
}