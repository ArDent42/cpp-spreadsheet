#include "cell.h"
#include "formula.h"
#include "common.h"
#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl {
public:
	virtual ~Impl() = default;
	virtual Value GetValue() const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const {
		return {};
	}
	virtual bool IsCache() const {
		return true;
	}
	virtual void ClearCache() {
	}
};

class Cell::EmptyImpl: public Impl {
public:
	Value GetValue() const override {
		return 0.0;
	}
	std::string GetText() const override {
		return "";
	}
};

class Cell::TextImpl: public Impl {
public:
	explicit TextImpl(std::string text) :
			text_(text) {
	}
	Value GetValue() const override {
		if (text_.front() == ESCAPE_SIGN) {
			return text_.substr(1);
		}
		return text_;
	}
	std::string GetText() const override {
		return text_;
	}
private:
	std::string text_;
};

class Cell::FormulaImpl: public Impl {
public:
	explicit FormulaImpl(std::string expression, const SheetInterface &sheet) :
			sheet_(sheet) {
		if (expression.empty() || expression[0] != FORMULA_SIGN) {

			throw std::logic_error("");
		}
		formula_ = ParseFormula(expression.substr(1));
	}
	Value GetValue() const override {
		if (!cache_value_) {
			cache_value_ = formula_->Evaluate(sheet_);
		}
		return std::visit([](const auto &val) {
			return Value(val);
		}, *cache_value_);
	}
	std::string GetText() const override {
		return FORMULA_SIGN + formula_->GetExpression();
	}
	std::vector<Position> GetReferencedCells() const override {
		return formula_->GetReferencedCells();
	}
	bool IsCache() const override {
		return cache_value_.has_value();
	}
	void ClearCache() override {
		cache_value_.reset();
	}
private:
	const SheetInterface &sheet_;
	std::unique_ptr<FormulaInterface> formula_;
	mutable std::optional<FormulaInterface::Value> cache_value_;
};

Cell::Cell(Sheet &sheet) :
		impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {
}

Cell::~Cell() {
}

void Cell::Set(std::string text) {
	std::unique_ptr<Impl> impl;
	if (text.empty()) {
		impl = std::make_unique<EmptyImpl>();
	} else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
		impl = std::make_unique<FormulaImpl>(text, sheet_);
	} else {
		impl = std::make_unique<TextImpl>(text);
	}
	if (CheckCircleDependency(*impl)) {
		throw CircularDependencyException("");
	}
	impl_ = std::move(impl);
	UpdateRefs();
	ClearAllCache();
}

void Cell::UpdateRefs() {
	for (Cell *cell : out_refs_) {
		cell->in_refs_.erase(this);
	}
	out_refs_.clear();
	for (const auto &pos : impl_->GetReferencedCells()) {
		Cell *out = dynamic_cast<Cell*>(sheet_.GetCell(pos));
		out_refs_.insert(out);
		if (out) {
			out->in_refs_.insert(this);
		}
	}

}

bool Cell::CheckCircleDependency(const Impl &impl) const {
	for (const auto &pos : impl.GetReferencedCells()) {
		auto out = sheet_.GetCell(pos);
//		Cell *out = dynamic_cast<Cell*>(sheet_.GetCell(pos));
		if (out) {
			if (CheckCircleDependency(out)) {
				return true;
			}
		}
	}
	return false;
}

bool Cell::CheckCircleDependency(const CellInterface *cell) const {
	if (this == cell) {
		return true;
	}
	if (cell->GetReferencedCells().empty()) {
		return false;
	}
	for (const Position &pos : cell->GetReferencedCells()) {
		auto next_cell = sheet_.GetCell(pos);
		if (!next_cell) {
			continue;
		}
		if (CheckCircleDependency(next_cell)) {
			return true;
		}
	}
	return false;
}

void Cell::Clear() {
	impl_ = std::move(std::make_unique<Cell::EmptyImpl>());
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

void Cell::ClearAllCache() {
	if (impl_->IsCache()) {
		impl_->ClearCache();
		for (Cell *in : in_refs_) {
			in->ClearAllCache();
		}
	}
}
