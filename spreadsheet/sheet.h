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
	mutable Cell empty_cell_{ *this };

	//увеличиваем длину строк
	void IncreaseSize(Size pos);

	CellInterface* GetCellCommon(Position pos) const;
	//обрабатывает cells_count
	void IncrementCellsCount(Size pos);
	void DecrementCellsCount(Size pos);
	void FittCellsCount();

	//бросает ошибку если позиция не валидна
	void ValidatePosition(Position pos) const;
	//печатает таблицу
	void PrintData(std::ostream& output, bool as_text) const;
	// Изначально в таблице нет циклических зависимостей,
	// поэтому достаточно проверить, что в новой формуле нет зависимостей от новой
	// ячейки(pos)
	void CheckCycle(const Position& pos, std::string text);
	void CheckCycleInner(const Position& pos, CellsOrder& cells, CellsOrder& cheked_cells);
};

std::ostream& operator<<(std::ostream& output, CellInterface::Value value);