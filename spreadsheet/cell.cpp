#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <variant>

class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const { return {}; }
    virtual bool HasCache() { return true; }
    virtual void InvalidateCache() {}

protected:
    Value value_;
    std::string text_;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    CellInterface::Value GetValue() const override {
        return value_;
    }

    std::string GetText() const override {
        return text_;
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl(std::string_view text)
    {
        text_ = text;
        if (text[0] == '\'') text = text.substr(1);
        value_ = std::string(text);
    }

    CellInterface::Value GetValue() const override {
        return value_;
    }

    std::string GetText() const override {
        return text_;
    }
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl(std::string formula, const SheetInterface& sheet)
        : sheet_(sheet), formula_(ParseFormula(std::move(formula)))
    {
        
    }

    CellInterface::Value GetValue() const override {
        if (!cache_) {
            cache_ = formula_->Evaluate(sheet_);
        }

        auto value = formula_->Evaluate(sheet_);
        try {
            return std::get<double>(value);
        }
        catch (...) {
            return std::get<FormulaError>(value);
        }
    }

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    }

    void InvalidateCache() override
    {
        cache_.reset();
    }

    bool HasCache() {
        return cache_.has_value();
    }

    std::vector<Position> GetReferencedCells() const {
        return formula_->GetReferencedCells();
    }

private:
    mutable std::optional<FormulaInterface::Value> cache_;
    const SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> formula_;
};

bool Cell::HasCircularDependency(const Impl& new_impl) const
{
    // Проверяем, есть ли вообще зависимости
    if (new_impl.GetReferencedCells().empty()) {
        return false;
    }
    std::unordered_set<const Cell*> referenced;
    for (const auto& pos : new_impl.GetReferencedCells()) {
        const Cell* ref_cell = sheet_.GetCellPtr(pos);
        if (ref_cell) {
            referenced.insert(ref_cell);
        }
    }

    std::unordered_set<const Cell*> visited;
    std::stack<const Cell*> to_visit;
    to_visit.push(this);
    while (!to_visit.empty()) {
        const Cell* current = to_visit.top();
        to_visit.pop();
        visited.insert(current);

        if (referenced.find(current) != referenced.end()) return true;

        for (const Cell* incoming : current->dependent_cells_) {
            if (visited.find(incoming) == visited.end()) to_visit.push(incoming);
        }
    }

    return false;
}

void Cell::InvalidateAllCache(bool flag = false) {
    if (impl_->HasCache() || flag) {
        impl_->InvalidateCache();

        for (Cell* dep : dependent_cells_) {
            dep->InvalidateAllCache();
        }
    }
}

Cell::Cell(Sheet& sheet)
    : sheet_(sheet),
    impl_(std::make_unique<EmptyImpl>())
{}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;

    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    }
    else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
        impl = std::make_unique<FormulaImpl>(std::move(text.substr(1)), sheet_);
    }
    else {
        impl = std::make_unique<TextImpl>(std::move(text));
    }
    if (HasCircularDependency(*impl)) {
        throw CircularDependencyException("");
    }
    impl_ = std::move(impl);

    for (auto c_ptr : referenced_cells_) {
        c_ptr->dependent_cells_.erase(this);
    }
    referenced_cells_.clear();

    for (const auto& pos : impl_->GetReferencedCells()) {
        Cell* ref = sheet_.GetCellPtr(pos);
        if (!ref) {
            sheet_.SetCell(pos, "");
            ref = sheet_.GetCellPtr(pos);
        }
        referenced_cells_.insert(ref);
        ref->dependent_cells_.insert(this);
    }
    InvalidateAllCache(true);
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

CellInterface::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const
{
    return !dependent_cells_.empty();
}
