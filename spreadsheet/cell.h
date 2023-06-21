#pragma once

#include <string>
#include <functional>
#include <unordered_set>
#include "common.h"
#include "formula.h"
#include "sheet.h"

class Sheet;

class Cell: public CellInterface {
private:
	class Impl;
	class EmptyImpl;
	class TextImpl;
	class FormulaImpl;
	std::unique_ptr<Impl> impl_;
	Sheet &sheet_;
	std::unordered_set<Cell*> in_refs_;
	std::unordered_set<Cell*> out_refs_;
public:
	Cell(Sheet &sheet_);
	~Cell();
	void Set(std::string text);
	void Clear();
	Value GetValue() const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;
	bool IsReferenced() const;

private:
	void ClearAllCache();
	void UpdateRefs();
	bool CheckCircleDependency(const Impl &impl) const;
	bool CheckCircleDependency(const CellInterface *next_cell) const;

};
