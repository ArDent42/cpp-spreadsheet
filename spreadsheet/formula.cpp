#include "formula.h"
#include "sheet.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category) :
		category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
	return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
	return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
	switch (category_) {
	case Category::Ref:
		return "#REF!";
	case Category::Value:
		return "#VALUE!";
	case Category::Div0:
		return "#DIV/0!";
	}
	return "";
}

std::ostream& operator<<(std::ostream &output, FormulaError fe) {
	return output << fe.ToString();
}

namespace {

class Formula: public FormulaInterface {
public:
	explicit Formula(std::string expression) :
			ast_(ParseFormulaAST(expression)) {
	}
	Value Evaluate(const SheetInterface &sheet) const override {
		try {
			auto get_value = [&sheet](Position pos) -> double {
				if (!pos.IsValid()) {
					throw FormulaError(FormulaError::Category::Ref);
				}
				const auto *cell = sheet.GetCell(pos);
				if (cell == nullptr) {
					return 0.0;
				}
				const auto value = cell->GetValue();
				if (const auto *pval = std::get_if<double>(&value)) {
					return *pval;
				}
				if (const auto *pval = std::get_if<std::string>(&value)) {
					try {
						return std::stod(*pval);
					} catch (std::logic_error &ex) {
						throw FormulaError(FormulaError::Category::Value);
					}
				}
				if (std::get_if<FormulaError>(&value)) {
					throw FormulaError(FormulaError::Category::Value);
				}
				return 0;
			};
			return ast_.Execute(get_value);
		} catch (const FormulaError &fe) {
			return fe;
		}
	}
	std::string GetExpression() const override {
		std::ostringstream oss;
		ast_.PrintFormula(oss);
		return oss.str();
	}

	std::vector<Position> GetReferencedCells() const override {
		std::vector<Position> cells;
		for (auto cell : ast_.GetCells()) {
			if (cell.IsValid()) {
				cells.push_back(cell);
			}
		}
		cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
		return cells;
	}

private:
	FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
	return std::make_unique<Formula>(std::move(expression));
}
