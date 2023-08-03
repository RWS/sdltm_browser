#pragma once
#include <map>
#include <QString>

class QString;

class SdltmUpdateCache
{
public:
	enum class ValueType {
		Source, Target,
	};
	void Add(int translationUnitId, const QString& xmlText, const QString & friendlyText, ValueType valueType);
	void Clear();

	bool Has(int translationUnitId, ValueType valueType) const;

	bool TryGet(int translationUnitId, ValueType valueType, const QString* & value, const QString*& friendlyText) const;
	const QString& GetXml(int translationUnitId, ValueType valueType) const;
	const QString& GetFriendlyText(int translationUnitId, ValueType valueType) const;
private:
	struct Key {
		friend bool operator<(const Key& lhs, const Key& rhs) {
			if (lhs.TranslationUnitId < rhs.TranslationUnitId)
				return true;
			if (rhs.TranslationUnitId < lhs.TranslationUnitId)
				return false;
			return lhs.Type < rhs.Type;
		}

		friend bool operator<=(const Key& lhs, const Key& rhs) {
			return !(rhs < lhs);
		}

		friend bool operator>(const Key& lhs, const Key& rhs) {
			return rhs < lhs;
		}

		friend bool operator>=(const Key& lhs, const Key& rhs) {
			return !(lhs < rhs);
		}

		int TranslationUnitId;
		ValueType Type;
	};

	struct Value {
		QString XmlText;
		QString FriendlyText;
	};

	std::map<Key, Value> _cache;
};

