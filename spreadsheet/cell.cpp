#include "cell.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <string>

void Cell::Set(Position pos, std::string text) {
	InvalidateCache(pos);
	if (text.size() == 0) {
		impl_.reset(new EmptyImpl());
	}
	else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
		// 1. Saves a copy of the current pointer old_ptr = current_ptr.
		// 2. Overwrites the current pointer with the argument current_ptr = ptr.
		// 3. If the old pointer was non - empty, deletes the previously managed
		// object if (old_ptr) get_deleter()(old_ptr).
		impl_.reset(new FormulaImpl{ sheet_, std::string(text.begin() + 1, text.end()) });
		SetDependencies(pos);
	}
	else {
		impl_.reset(new TextImpl{ text });
	}
}

void Cell::Clear() {
	impl_.reset();
}

Cell::Value Cell::GetValue() const {
	Cell::Value result;
	if (cache_.has_value()) {
		result = cache_.value();
	}
	else {
		result = impl_->GetValue();
		cache_ = std::visit(
			[](auto&& arg) -> std::optional<double> {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, double>) {
					return arg;
				}
				else if constexpr (std::is_same_v<T, std::string>) {
					if (std::regex_match(
						arg,
						std::regex(
							R"(^([+-]?(?:[[:d:]]+\.?|[[:d:]]*\.[[:d:]]+))(?:[Ee][+-]?[[:d:]]+)?$)"))) {
						return std::stod(arg);
					}
					else {
						return std::nullopt;
					}
				}
				else {
					return std::nullopt;
				}
			},
			result);
	}
	return result;
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

CellsOrder Cell::GetReferencedCells() const {
	return impl_.get() == nullptr ? CellsOrder{} : impl_->GetReferencedCells();
}

void Cell::InvalidateCache(Position pos) {
	if (cache_.has_value()) {
		cache_.reset();
		for (Position p : depended_cells_) {
			Cell* cell{ dynamic_cast<Cell*>(sheet_.GetCell(p)) };
			if (cell != nullptr) {
				cell->InvalidateCache(pos);
				cell->DeleteDependence(pos);
			}
		}
	}
}

void Cell::AddDependece(Position pos)
{
	depended_cells_.insert(pos);
}

void Cell::DeleteDependence(Position pos)
{
	depended_cells_.erase(pos);
}

void Cell::SetDependencies(Position pos)
{
	for (Position p : referenced_cells_) {
		Cell* cell{ dynamic_cast<Cell*>(sheet_.GetCell(p)) };
		cell->AddDependece(pos);
	}
}

CellInterface::Value Cell::EmptyImpl::GetValue() {
	return std::string{};
}

std::string Cell::EmptyImpl::GetText() {
	return std::string{};
}

CellsOrder Cell::EmptyImpl::GetReferencedCells() const {
	return {};
}

Cell::Type Cell::EmptyImpl::GetType() const {
	return Cell::Type::Empty;
}

CellInterface::Value Cell::TextImpl::GetValue() {
	bool apostraf{ text_.front() == '\'' ? true : false };
	return apostraf ? std::string{text_.begin() + 1, text_.end()} : text_;
}

std::string Cell::TextImpl::GetText() {
	return text_;
}

CellsOrder Cell::TextImpl::GetReferencedCells() const {
	return {};
}

Cell::Type Cell::TextImpl::GetType() const {
	return Cell::Type::Text;
}

CellInterface::Value Cell::FormulaImpl::GetValue() {
	FormulaInterface::Value value{formula_.get()->Evaluate(sheet_)};
	return std::visit([](auto&& arg) -> CellInterface::Value { return arg; },
		value);
}

std::string Cell::FormulaImpl::GetText() {
	return '=' + formula_.get()->GetExpression();
}

CellsOrder Cell::FormulaImpl::GetReferencedCells() const {
	return formula_.get()->GetReferencedCells();
}

Cell::Type Cell::FormulaImpl::GetType() const {
	return Cell::Type::Formula;
}
