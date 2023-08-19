#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() { }

void Sheet::SetCell(Position pos, std::string text)
{
	ValidatePosition(pos); // если не выбросило исключения ОК
	CheckCycle(pos, text); // если не выбросило исключения ОК
	Size s_pos{ static_cast<size_t>(pos.row), static_cast<size_t>(pos.col) };
	IncreaseSize(s_pos); // увиличиваем размер и размер печатной области если больше
	IncrementCellsCount(s_pos); // если ячейка пуста то увеличиваем на 1 поле по
	// индексу в cells_count_by_[]
	if (data_.at(pos.row).at(pos.col).get() == nullptr) {
		data_.at(pos.row).at(pos.col).reset(new Cell(*this));
	}
	data_.at(pos.row).at(pos.col)->Set(pos, text);
}

const CellInterface* Sheet::GetCell(Position pos) const
{
	return GetCellCommon(pos);
}
CellInterface* Sheet::GetCell(Position pos)
{
	return  GetCellCommon(pos);
}

void Sheet::ClearCell(Position pos)
{
	ValidatePosition(pos); // делаем проверку (выбрасывает исключение)
	Size s_pos{ static_cast<size_t>(pos.row), static_cast<size_t>(pos.col) };
	if (data_.size() > s_pos.rows && data_.at(s_pos.rows).size() > s_pos.cols) {
		DecrementCellsCount(s_pos);
		data_.at(s_pos.rows).at(s_pos.cols).reset();
	}
}

Size Sheet::GetPrintableSize() const
{
	return { cells_count_by_row_.size(), cells_count_by_col_.size() };
}

void Sheet::PrintValues(std::ostream& output) const
{
	PrintData(output, false);
}
void Sheet::PrintTexts(std::ostream& output) const
{
	PrintData(output, true);
}

// Проверяем что позиция ячейки входит в размер таблицы,
// иначе увеличиваем до этой позиции
void Sheet::IncreaseSize(Size pos)
{
	if (data_.size() <= pos.rows) {
		data_.resize(pos.rows + 1);
	}
	if (data_.at(pos.rows).size() <= pos.cols) {
		data_.at(pos.rows).resize(pos.cols + 1);
	}
}

CellInterface* Sheet::GetCellCommon(Position pos) const
{
	ValidatePosition(pos); // делаем проверку (выбрасывает исключение)
	Size s_pos{ static_cast<size_t>(pos.row), static_cast<size_t>(pos.col) };
	if (data_.size() > s_pos.rows && data_.at(s_pos.rows).size() > s_pos.cols) {
		auto& cell_ptr{ data_.at(s_pos.rows).at(s_pos.cols) };
		return cell_ptr.get();
	}
	return nullptr;
}

void Sheet::IncrementCellsCount(Size pos)
{
	if (cells_count_by_row_.size() <= pos.rows) {
		cells_count_by_row_.resize(pos.rows + 1);
	}
	if (cells_count_by_col_.size() <= pos.cols) {
		cells_count_by_col_.resize(pos.cols + 1);
	}
	// ячейка пуста
	if (data_.at(pos.rows).at(pos.cols).get() == nullptr) {
		++cells_count_by_row_.at(pos.rows);
		++cells_count_by_col_.at(pos.cols);
	}
}

void Sheet::DecrementCellsCount(Size pos)
{
	// ячейка не пуста
	if (data_.at(pos.rows).at(pos.cols).get() != nullptr) {
		--cells_count_by_row_.at(pos.rows);
		--cells_count_by_col_.at(pos.cols);
	}
	FittCellsCount();
}

void Sheet::FittCellsCount()
{
	auto zero{ [](uint32_t num) { return num == 0; } };
	auto it_row{ std::find_if_not(
		cells_count_by_row_.rbegin(), cells_count_by_row_.rend(), zero) };
	auto it_col{ std::find_if_not(
		cells_count_by_col_.rbegin(), cells_count_by_col_.rend(), zero) };

	cells_count_by_row_.resize(
		std::distance(cells_count_by_row_.begin(), it_row.base()));
	cells_count_by_col_.resize(
		std::distance(cells_count_by_col_.begin(), it_col.base()));
}

void Sheet::ValidatePosition(Position pos) const
{
	if (!pos.IsValid()) {
		throw InvalidPositionException("");
	}
}

void Sheet::PrintData(std::ostream& output, bool as_text) const
{
	auto rows{ cells_count_by_row_ };
	auto cols{ cells_count_by_col_ };
	for (size_t i{}; i < rows.size(); ++i) {
		bool first{ true };
		for (size_t j{}; j < cols.size(); ++j) {
			if (first && rows.at(i) == 0) {
				output << '\t';
				break;
			}
			if (rows.at(i) == 0) {
				output << std::string(cols.size() - j, '\t');
				break;
			}
			if (!first) {
				output << '\t';
			}
			else {
				first = false;
			}
			auto& cell{ data_.at(i).at(j) };
			if (cols.at(j) == 0 || cell.get() == nullptr) {
				continue;
			}
			output << (as_text ? cell->GetText() : cell->GetValue());
			--rows.at(i);
			--cols.at(j);
		}
		output << '\n';
	}
}

void Sheet::CheckCycle(const Position& pos, const std::string& text)
{
	Cell temp_cell(*this);
	try {
		temp_cell.Set(pos, text);
	}
	catch (...) {
		throw FormulaException("");
	}
	CellsOrder temp_cells{ temp_cell.GetReferencedCells() };
	if (!temp_cells.size()) {
		return;
	}
	CellsOrder checked_cells;
	checked_cells.reserve(temp_cells.size());

	CheckCycleInner(pos, temp_cells, checked_cells);
}

void Sheet::CheckCycleInner(const Position& pos, CellsOrder& cells, CellsOrder& cheked_cells)
{
	CellsOrder diff;
	std::set_difference(cells.begin(),
		cells.end(),
		cheked_cells.begin(),
		cheked_cells.end(),
		std::inserter(diff, diff.begin()));

	if (std::binary_search(diff.begin(), diff.end(), pos)) {
		throw CircularDependencyException("");
	}
	// заполняем вектор всеми новыми векторами
	CellsOrder new_cells;
	for (const Position& p : diff) {
		// проверку
		if (p.IsValid()) {
			const CellInterface* cell_ptr{ GetCell(p) };
			if (cell_ptr == nullptr) {
				SetCell(p, "");
				continue;
			}
			CellsOrder temp_cells{ cell_ptr->GetReferencedCells() };
			new_cells.reserve(new_cells.size() + temp_cells.size());
			new_cells.insert(new_cells.end(),
				std::make_move_iterator(temp_cells.begin()),
				std::make_move_iterator(temp_cells.end()));
			// очищать temp_cells нет необходимости
		}
	}
	// единственный способ выйти
	if (!new_cells.size()) {
		return;
	}
	// сортируем и очищаем от дубликатов
	ClearVector(new_cells);

	CellsOrder new_cheked_cells;
	new_cheked_cells.reserve(diff.size() + cheked_cells.size());
	// теперь new_cheked_cells содержит оба проверенных вектора без повторов!
	std::set_union(cheked_cells.begin(),
		cheked_cells.end(),
		diff.begin(),
		diff.end(),
		std::inserter(new_cheked_cells, new_cheked_cells.begin()));

	// Вызываем себя же с новыми контейнерами
	CheckCycleInner(pos, new_cells, new_cheked_cells);
}

std::ostream& operator<<(std::ostream& output, CellInterface::Value value)
{
	std::visit([&output](auto&& arg) { output << arg; }, value);
	return output;
}

std::unique_ptr<SheetInterface> CreateSheet()
{
	return std::make_unique<Sheet>();
}
