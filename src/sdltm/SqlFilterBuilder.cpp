#include "SqlFilterBuilder.h"

#include "SdltmUtil.h"

void SqlFilterBuilder::AddAnd(const QString& filter, int IndentLevel)
{
	Item item = { filter, true, IndentLevel };
	_items.push_back(item);
}

void SqlFilterBuilder::AddOr(const QString& filter, int IndentLevel)
{
	Item item = { filter, false, IndentLevel };
	_items.push_back(item);
}

namespace 
{
	void AppendSql(QString& sql, const QString& subQuery, bool isAnd, bool prependEnter = false)
	{
		if (sql != "") {
			if (prependEnter)
				sql += "\r\n            ";
			sql += isAnd ? " AND " : " OR ";
		}
		sql += "(" + subQuery + ")";
	}


}


QString SqlFilterBuilder::Get() const
{
	std::vector<Item> items = _items;

	while (items.size() > 1) {
		auto maxLevel = std::max_element(items.begin(), items.end(), [](const Item& a, const Item & b) { return a.IndentLevel < b.IndentLevel; })->IndentLevel;
		items = ProcessIndentLevel(items, maxLevel);
	}
	return items.size() > 0 ? items[0].SqlFilter : "";
}

// processes all Items with this indent level
std::vector<SqlFilterBuilder::Item> SqlFilterBuilder::ProcessIndentLevel(const std::vector<Item>& items, int level)
{
	std::vector<Item> result;
	bool needsParanthesis = false;
	int appendCount = 0;
	for(const auto & item : items) {
		bool append = false;
		if (item.IndentLevel != level)
			append = true;
		else {
			auto isFirst = result.empty() || result.back().IndentLevel != level;
			if (isFirst)
				append = true;
		}

		if (append) {
			if (needsParanthesis)
				result.back().SqlFilter = "(" + result.back().SqlFilter + ")";
			needsParanthesis = false;
			appendCount = 0;
			result.push_back(item);
		} else {
			// append to the last one
			needsParanthesis = true;
			if (appendCount == 0)
				result.back().SqlFilter = "(" + result.back().SqlFilter + ")";
			++appendCount;
			AppendSql(result.back().SqlFilter, item.SqlFilter, result.back().IsAnd);
		}
	}

	int prevLevel = -1;
	for (const auto& item : result)
		if (item.IndentLevel != level && item.IndentLevel > prevLevel)
			prevLevel = item.IndentLevel;

	if (prevLevel > -1)
		for (int i = 0; i < result.size(); ++i) {
			auto& item = result[i];
			if (item.IndentLevel == level) {
				item.IndentLevel = prevLevel;
				// the idea - the lower the indent level, the more important a sub-query is
				// so if I merge sub-query a with sub-query b, but b has a lower Indent level, b will dictate
				// if using AND or OR
				if (i < result.size() - 1 && result[i + 1].IndentLevel == prevLevel)
					item.IsAnd = result[i + 1].IsAnd;
			}
		}
	return result;
}

// test the SqlFilterBuilder class
void TestSqlFilterBuilder() {
	SqlFilterBuilder sql;
	sql.AddAnd("a == 1", 1);
	sql.AddAnd("b > 2", 2);
	sql.AddAnd("b < 5", 2);
	sql.AddOr("c >= 10", 3);
	sql.AddOr("c <= 20", 3);
	sql.AddOr("c == 200", 3);
	sql.AddAnd("d == 1", 1);
	sql.AddOr("e < 100", 2);
	sql.AddOr("e > 10", 2);
	assert(sql.Get() == "(a == 1) AND (((b > 2) AND (b < 5) AND (((c >= 10) OR (c <= 20) OR (c == 200))))) AND (d == 1) AND ((e < 100) OR (e > 10))");

	SqlFilterBuilder sql2;
	sql2.AddOr("a == 10", 2);
	sql2.AddOr("a == 20", 2);
	sql2.AddOr("a == 30", 2);
	sql2.AddAnd("b > 5", 1);
	sql2.AddOr("c == 0", 2);
	sql2.AddOr("c == 1", 2);
	sql2.AddAnd("d == 1", 5);
	sql2.AddAnd("d == 2", 5);
	sql2.AddAnd("d == 3", 5);
	sql2.AddOr("e == 10", 20);
	sql2.AddOr("e == 11", 20);
	sql2.AddOr("e == 12", 20);
	sql2.AddOr("e == 13", 20);
	sql2.AddAnd("f == 14", 10);
	sql2.AddAnd("f == 15", 10);
	sql2.AddAnd("f == 16", 10);
	sql2.AddAnd("f == 17", 10);
	assert(sql2.Get() == "(((a == 10) OR (a == 20) OR (a == 30))) AND (b > 5) AND ((c == 0) OR (c == 1) OR ((d == 1) AND (d == 2) AND (d == 3) AND ((((e == 10) OR (e == 11) OR (e == 12) OR (e == 13))) AND (f == 14) AND (f == 15) AND (f == 16) AND (f == 17))))");

	SqlFilterBuilder sql3;
	sql3.AddAnd("a == 5", 10);
	sql3.AddAnd("a == 6", 10);
	sql3.AddAnd("a == 7", 10);
	sql3.AddOr("b == 1", 5);
	sql3.AddOr("b == 2", 5);
	sql3.AddOr("b == 3", 5);
	sql3.AddAnd("c == 10", 1);
	sql3.AddAnd("c == 11", 1);
	sql3.AddAnd("c == 13", 1);
	sql3.AddOr("d == 20", 15);
	sql3.AddOr("d == 21", 15);
	sql3.AddAnd("e == 30", 20);
	sql3.AddAnd("e == 31", 20);
	sql3.AddAnd("f == 50", 3);
	sql3.AddAnd("f == 51", 3);
	sql3.AddAnd("f == 52", 3);
	sql3.AddAnd("f == 53", 3);
	assert(sql3.Get() == "(((((a == 5) AND (a == 6) AND (a == 7))) OR (b == 1) OR (b == 2) OR (b == 3))) AND (c == 10) AND (c == 11) AND (c == 13) AND ((((d == 20) OR (d == 21) OR (((e == 30) AND (e == 31))))) AND (f == 50) AND (f == 51) AND (f == 52) AND (f == 53))");
}
