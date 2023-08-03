#include "SdltmUpdateCache.h"

#include <stdexcept>

void SdltmUpdateCache::Add(int translationUnitId, const QString& xmlText, const QString& friendlyText, ValueType valueType) {
	Key key = { translationUnitId, valueType };
	Value value = { xmlText , friendlyText };
	_cache[key] = value;
}

void SdltmUpdateCache::Clear() {
	_cache.clear();
}

bool SdltmUpdateCache::Has(int translationUnitId, ValueType valueType) const {
	const QString* value = nullptr;
	const QString* friendly = nullptr;
	return TryGet(translationUnitId, valueType, value, friendly);
}

bool SdltmUpdateCache::TryGet(int translationUnitId, ValueType valueType, const QString*& xmlText, const QString*& friendlyText) const {
	Key key = { translationUnitId, valueType };
	auto it = _cache.find(key);
	if (it != _cache.end()) {
		xmlText = &it->second.XmlText;
		friendlyText = &it->second.FriendlyText;
		return true;
	}
	xmlText = nullptr;
	friendlyText = nullptr;
	return false;
}

const QString& SdltmUpdateCache::GetXml(int translationUnitId, ValueType valueType) const {
	const QString* value = nullptr;
	const QString* friendly = nullptr;
	if (TryGet(translationUnitId, valueType, value, friendly))
		return *value;
	else
		throw std::runtime_error("item not in cache");
}

const QString& SdltmUpdateCache::GetFriendlyText(int translationUnitId, ValueType valueType) const {
	const QString* value = nullptr;
	const QString* friendly = nullptr;
	if (TryGet(translationUnitId, valueType, value, friendly))
		return *friendly;
	else
		throw std::runtime_error("item not in cache");
}
