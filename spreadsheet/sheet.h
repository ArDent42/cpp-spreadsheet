#pragma once

#include "cell.h"
#include "common.h"
#include <vector>
#include <functional>
#include <memory>

class Cell;

class Sheet: public SheetInterface {
public:
	~Sheet();

	void SetCell(Position pos, std::string text) override;

	const CellInterface* GetCell(Position pos) const override;
	CellInterface* GetCell(Position pos) override;

	void ClearCell(Position pos) override;

	Size GetPrintableSize() const override;
	void Resize();

	void PrintValues(std::ostream &output) const override;
	void PrintTexts(std::ostream &output) const override;

private:
	Size GetSize() const;
	void Print(std::ostream &output, const std::string &type) const;
	std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
};
