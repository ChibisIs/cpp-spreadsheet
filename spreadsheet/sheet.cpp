#include "sheet.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (!cells_.count(pos)) {
        cells_[pos] = std::make_unique<Cell>(*this);
    }
    cells_[pos]->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    auto it = cells_.find(pos);
    return it == cells_.end() ? nullptr : it->second.get();
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(static_cast<const Sheet*>(this)->GetCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    auto it = cells_.find(pos);
    if (it != cells_.end()) {
        it->second->Clear();
        cells_.erase(it);
    }
}

Size Sheet::GetPrintableSize() const {
    int max_row = 0;
    int max_col = 0;

    for (const auto& [pos, cell] : cells_) {
        if (!cell->GetText().empty()) {
            max_row = std::max(max_row, pos.row + 1);
            max_col = std::max(max_col, pos.col + 1);
        }
    }

    return { max_row, max_col };
}

void Sheet::PrintValues(std::ostream& output) const {
    auto size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            const CellInterface* cell = GetCell({ row, col });
            if (cell) {
                std::visit([&output](const auto& value) { output << value; }, cell->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    auto size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) {
                output << '\t';
            }
            const CellInterface* cell = GetCell({ row, col });
            if (cell) {
                output << cell->GetText();
            }
        }
        output << '\n';
    }
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");

    const auto cell = cells_.find(pos);
    if (cell == cells_.end()) {
        return nullptr;
    }

    return cells_.at(pos).get();
}

Cell* Sheet::GetCellPtr(Position pos) {
    return const_cast<Cell*>(
        static_cast<const Sheet&>(*this).GetCellPtr(pos));
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}