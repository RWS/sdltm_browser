#include "SqlFilterBuilder.h"

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
		if (sql != "")
		{
			if (prependEnter)
				sql += "\r\n            ";
			sql += isAnd ? " AND " : " OR ";
		}
		sql += "(" + subQuery + ")";
	}

}
QString SqlFilterBuilder::Get() const
{
	// FIXME at this time, I don't care about the indent - I should use a stack to add/delete brackets
	// + I should validate the indents, just in case they're incorrect, like 0, 1, 3, 2, 0 -- the 2 should become 1
	std::vector<int> indents;

	QString sql;
	for (const auto& item : _items)
		AppendSql(sql, item.SqlFilter, item.IsAnd, true);
	return sql;
}
