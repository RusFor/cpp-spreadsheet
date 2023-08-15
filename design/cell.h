#pragma once

#include "common.h"
#include "formula.h"


class Cell : public CellInterface {
private: 
    class Impl {
    public:
        Impl() = default;
        virtual CellInterface::Value GetValue() = 0;
        virtual std::string GetText() = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual ~Impl() = default;
    private:
    };

    class EmptyImpl final : public Impl {
    public:
        EmptyImpl() = default;
        ~EmptyImpl() = default;
        CellInterface::Value GetValue() override;
        std::string GetText() override;
        std::vector<Position> GetReferencedCells() const override;
    private:
    };

    class TextImpl final : public Impl {
    public:
        TextImpl(std::string text) : text_(text) {};
        ~TextImpl() = default;
        CellInterface::Value GetValue() override;
        std::string GetText() override;
        std::vector<Position> GetReferencedCells() const override;
    private:
        std::string text_{};
    };

    class FormulaImpl final : public Impl {
    public:
        FormulaImpl(const SheetInterface& sheet, std::string expression)
            : sheet_(sheet), formula_(ParseFormula(expression)) {};
        ~FormulaImpl() = default;
        CellInterface::Value GetValue() override;
        std::string GetText() override;
        std::vector<Position> GetReferencedCells() const override;
    private:
        const SheetInterface& sheet_;
        std::unique_ptr<FormulaInterface> formula_;
    };

public:
    Cell(const SheetInterface& sheet) : sheet_(sheet) {};
    Cell() = delete;
    ~Cell() {};

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    //+
    std::vector<Position> GetReferencedCells() const override;
    void InvalidateCache();

private:
    const SheetInterface& sheet_; //+
    std::unique_ptr<Impl> impl_{ nullptr };
    std::optional<double> cache_{ std::nullopt }; //+
    std::vector<Position> referenced_cells_; //+
    std::set<Position> depended_cells_; //+
    bool is_processing{ false }; //+

};
