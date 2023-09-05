#pragma once
#include <functional>
#include <map>
#include <QDateTime>
#include <QString>
#include <vector>



struct SimpleXmlNode {
	QString Name;
	QString Value;
	std::map<QString, QString> Attributes;
	static std::function<QDateTime(const QString&)> ReadDate;

	bool HasAttribute(const QString& name) const;

	void ReadAttribute(const QString& name, QDateTime& value);
	void ReadAttribute(const QString& name, QString& value);
	void ReadAttribute(const QString& name, int& value);

	static SimpleXmlNode Parse(const QString& xml);
};

