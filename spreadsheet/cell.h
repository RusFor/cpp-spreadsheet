#pragma once
#include <memory>
#include <optional>
#include <set>

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
 public:
  enum class Type {
    Empty,
    Text,
    Formula,
  };

 private:
  class Impl {
   public:
    Impl() = default;
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue() = 0;
    virtual std::string GetText() = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual Type GetType() const = 0;

   private:
  };

  class EmptyImpl final : public Impl {
   public:
    EmptyImpl() = default;
    ~EmptyImpl() = default;
    CellInterface::Value GetValue() override;
    std::string GetText() override;
    std::vector<Position> GetReferencedCells() const override;
    Type GetType() const override;

   private:
  };

  class TextImpl final : public Impl {
   public:
    TextImpl(std::string text) : text_(text){};
    ~TextImpl() = default;
    CellInterface::Value GetValue() override;
    std::string GetText() override;
    std::vector<Position> GetReferencedCells() const override;
    Type GetType() const override;

   private:
    std::string text_{};
  };

  class FormulaImpl final : public Impl {
   public:
    FormulaImpl(const SheetInterface& sheet, std::string expression)
        : sheet_(sheet), formula_(ParseFormula(expression)){};
    ~FormulaImpl() = default;
    CellInterface::Value GetValue() override;
    std::string GetText() override;
    std::vector<Position> GetReferencedCells() const override;
    Type GetType() const override;

   private:
    const SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
  };

 public:
  Cell(SheetInterface& sheet) : sheet_(sheet){};
  Cell() = delete;
  ~Cell(){};

  void Set(Position pos, std::string text);
  void Clear();

  Value GetValue() const override;
  std::string GetText() const override;
  //+
  std::vector<Position> GetReferencedCells() const override;
  void InvalidateCache(Position pos);
  void AddDependece(Position pos);
  void DeleteDependence(Position pos);

 private:
  SheetInterface& sheet_;  //+
  std::unique_ptr<Impl> impl_{nullptr};
  mutable std::optional<double> cache_{std::nullopt};  //+
  std::vector<Position> referenced_cells_;             //+
  std::set<Position> depended_cells_;                  //+
  //bool is_processing{false};                         //+
  void SetDependencies(Position pos);
};