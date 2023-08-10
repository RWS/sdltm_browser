#include "SdltmSqlHistory.h"

namespace {
	const int MAX_ITEM_TEXT = 150;
	const int MAX_ITEM_COUNT = 100;
}

void SdltmSqlHistory::SetCombo(QComboBox* combo) {
	_historyCombo = combo;
}


QString SdltmSqlHistory::Item::Text() const {
	QString text = _normalizedSql;
	if (text.size() >= MAX_ITEM_TEXT)
		text = "... " + text.right(MAX_ITEM_TEXT);

	if (Description != "")
		text = Description + ": " + text;
	else
		text = "Query: " + text;
	return text;
}

SdltmSqlHistory::Item::Item(const QString& sql, const QString& description) {
	// split into lines, trim lines, and then readd them
	Sql = sql;
	Description = description;

	auto lines = sql.split("\r");
	for (auto& line : lines)
		line = line.trimmed();

	for (const auto & line :lines) {
		if (line == "")
			continue;

		if (_tooltip != "")
			_tooltip += "\r\n";
		if (_normalizedSql != "")
			_normalizedSql += " ";

		_tooltip += line;
		_normalizedSql += line;
	}
}


void SdltmSqlHistory::Add(const QString& sql, const QString& description) {
	assert(_historyCombo != nullptr);

	// look for existing item
	// add to the top
	Item newItem(sql, description);

	auto existing = std::find_if(_items.begin(), _items.end(), [=](const Item& i) { return i.NormalizedSql() == newItem.NormalizedSql(); });
	if (existing != _items.end()) {
		// already in history
		auto idx = existing - _items.begin();
		if (idx == _historyCombo->currentIndex()) {
			Set(sql, description);
			return;
		}
		_historyCombo->removeItem(idx);
		_items.erase(existing);
	} else {
		// it's new
	}

	_items.insert(_items.begin(), newItem);

	_ignoreUpdate = true;
	_historyCombo->insertItem(0, newItem.Text());
	_historyCombo->setItemData(0, newItem.Tooltip(), Qt::ToolTipRole);
	_historyCombo->setCurrentIndex(0);

	if (_items.size() > MAX_ITEM_COUNT) {
		_historyCombo->removeItem(_items.size() - 1);
		_items.erase(_items.end() - 1);
	}
	_ignoreUpdate = false;
}

// the difference between add and set: set always updates the selected index
void SdltmSqlHistory::Set(const QString& sql, const QString& description) {
	if (_items.empty()) {
		Add(sql, description);
		return;
	}

	_ignoreUpdate = true;
	auto idx = _historyCombo->currentIndex();
	_items[idx] = Item(sql, description);
	_historyCombo->setItemText(idx, _items[idx].Text());
	_historyCombo->setItemData(idx, _items[idx].Tooltip(), Qt::ToolTipRole);
	_ignoreUpdate = false;
}

QString SdltmSqlHistory::GetCurrentSql() const {
	auto idx = _historyCombo->currentIndex();
	return idx >= 0 ? _items[idx].Sql : "";
}
