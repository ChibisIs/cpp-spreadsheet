#pragma once

#include "cell.h"
#include "common.h"

#include <unordered_map>
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

    const Cell* GetCellPtr(Position pos) const;
    Cell* GetCellPtr(Position pos);
private:
    // Хеш-функция для Position
    struct PositionHasher {
        size_t operator()(const Position& pos) const {
            return std::hash<std::string>()(pos.ToString());
        }
    };

    // Компаратор для Position
    struct PositionEqual {
        bool operator()(const Position& lhs, const Position& rhs) const {
            return lhs == rhs;
        }
    };
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher, PositionEqual> cells_;
};