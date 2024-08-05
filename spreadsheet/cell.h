#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
    void InvalidateAllCache(bool flag);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    bool HasCircularDependency(const Impl& impl) const;

    Sheet& sheet_;
    std::unique_ptr<Impl> impl_;
    std::unordered_set<Cell*> dependent_cells_;
    std::unordered_set<Cell*> referenced_cells_;
};