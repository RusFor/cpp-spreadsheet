#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:

    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> data_;
    std::vector<uint32_t> cells_count_by_row_;
    std::vector<uint32_t> cells_count_by_col_;
	
    void IncreaseSize(Size pos);

    void IncrementCellsCount(Size pos);
    void DecrementCellsCount(Size pos);

    void FittCellsCount();

    Size ValidatePosition(Position pos) const;

    void PrintData(std::ostream& output, bool as_text) const;
    //+
    void ChekCycle(Position pos, std::string text) const;
};

std::ostream& operator<<(std::ostream& output, CellInterface::Value value);
