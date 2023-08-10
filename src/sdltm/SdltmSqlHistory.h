#pragma once
#include <QComboBox>
#include <QDateTime>


class SdltmSqlHistory
{
public:
	void SetCombo(QComboBox* combo);

	struct Item {
		QString Sql;
		QString Description;
		QDateTime AtTime() const { return _atTime; }
		QString Tooltip() const { return _tooltip;  }
		QString NormalizedSql() const { return _normalizedSql; }
		QString Text() const;

		Item(const QString& sql, const QString& description);
	private:
		QDateTime _atTime = QDateTime::currentDateTime();
		// normalized, by keeps enters
		QString _tooltip;
		// normalized, no enters -- just so that if user simply removes spaces from an sql, i won't actually create a new entry
		QString _normalizedSql;
	};

	void Add(const QString& sql, const QString& description );
	void Set(const QString& sql, const QString& description);

	bool IgnoreUpdate() const { return _ignoreUpdate; }

	QString GetCurrentSql() const;

private:
	QComboBox* _historyCombo = nullptr;
	std::vector<Item> _items;
	bool _ignoreUpdate = false;
};

