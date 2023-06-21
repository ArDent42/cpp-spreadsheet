#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <algorithm>

using namespace std::literals;

Sheet::~Sheet() {
}

void Sheet::SetCell(Position pos, std::string text) {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}
	sheet_.resize(std::max(pos.row + 1, static_cast<int>(sheet_.size())));
	sheet_[pos.row].resize(std::max(pos.col + 1, static_cast<int>(sheet_[pos.row].size())));
	auto temp_cell = std::move(sheet_[pos.row][pos.col]);
	sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this);
	try {
		sheet_[pos.row][pos.col]->Set(text);
	} catch (CircularDependencyException &cd) {
		sheet_[pos.row][pos.col] = std::move(temp_cell);
		throw cd;
	}
	Resize();
}

const CellInterface* Sheet::GetCell(Position pos) const {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}
	Size size = GetPrintableSize();
	if (pos.row < size.rows && pos.col < size.cols) {
		return sheet_[pos.row][pos.col].get();
	}
	return nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}
	Size size = GetPrintableSize();
	if (pos.row < size.rows && pos.col < size.cols) {
		return sheet_[pos.row][pos.col].get();
	}
	return nullptr;
}

void Sheet::ClearCell(Position pos) {
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid position");
	}
	Size size = GetPrintableSize();
	if (pos.row < size.rows && pos.col < size.cols) {
		if (sheet_[pos.row][pos.col]) {
			sheet_[pos.row][pos.col]->Clear();
		}
		Resize();
	}
}

Size Sheet::GetPrintableSize() const {
	Size res;
	res.rows = sheet_.size();
	if (sheet_.size() == 0) {
		res.cols = 0;
	} else {
		res.cols = sheet_.front().size();
	}
	return res;
}

Size Sheet::GetSize() const {
	Size res;
	for (size_t row = 0; row < sheet_.size(); ++row) {
		for (size_t col = 0; col < sheet_[row].size(); ++col) {
			if (sheet_[row][col] && !sheet_[row][col]->GetText().empty()) {
				res.rows = row + 1;
				res.cols = std::max(res.cols, static_cast<int>(col + 1));
			}
		}
	}
	return res;
}

void Sheet::Resize() {
	Size size = GetSize();
	sheet_.resize(size.rows);
	for (int row = 0; row < size.rows; ++row) {
		sheet_[row].resize(size.cols);
		for (size_t col = 0; col < sheet_[row].size(); ++col) {
			if (!sheet_[row][col]) {
				sheet_[row][col] = std::move(std::make_unique<Cell>(*this));
				sheet_[row][col]->Set("");
			}
		}
	}
}

void Sheet::PrintValues(std::ostream &output) const {
	Print(output, "values");
}

void Sheet::PrintTexts(std::ostream &output) const {
	Print(output, "text");
}

void Sheet::Print(std::ostream &output, const std::string &type) const {
	auto size = GetPrintableSize();
	for (int row = 0; row < size.rows; ++row) {
		for (int col = 0; col < size.cols; ++col) {
			if (col > 0) {
				output << '\t';
			}
			if (sheet_[row][col]->GetText().empty()) {
				continue;
			}
			if (type == "values") {
				const auto res = sheet_[row][col]->GetValue();
				if (const double *pval = std::get_if<double>(&res)) {
					output << *pval;
				} else if (const std::string *pval = std::get_if<std::string>(&res)) {
					output << *pval;
				} else {
					output << *std::get_if<FormulaError>(&res);
				}
			} else {
				output << sheet_[row][col]->GetText();
			}
		}
		output << '\n';
	}
}

std::unique_ptr<SheetInterface> CreateSheet() {
	return std::make_unique<Sheet>();
}
