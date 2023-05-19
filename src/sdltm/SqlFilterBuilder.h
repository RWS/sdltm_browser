#pragma once
#include <QString>
#include <vector>

class SqlFilterBuilder
{
public:
	void AddAnd(const QString& filter, int IndentLevel);
	void AddOr(const QString& filter, int IndentLevel);
	QString Get() const;
private:
	struct Item
	{
		QString SqlFilter;
		bool IsAnd;
		int IndentLevel;
	};

	std::vector<Item> _items;
};

