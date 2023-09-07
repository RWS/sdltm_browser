#include "ImportTmxToSql.h"

#include <QTextStream>

#include "BulkSqlExecute.h"
#include "CustomFieldsTmxLoader.h"
#include "FileDialog.h"
#include "SdltmSqlUtil.h"
#include "SdltmUtil.h"
#include "SimpleXml.h"

namespace  {
	QDateTime TmxToDate(const QString & s) {
		return QDateTime::fromString(s, "yyyyMMddTHHmmssZ");
	}

	QString DateToSql(QDateTime dt) {
		return dt.toString("yyyy-MM-dd HH:mm:ss");
	}

	QString GetAttributeTableName(SdltmFieldMetaType fieldType) {
		switch (fieldType) {
		case SdltmFieldMetaType::Int: assert(false);  break;
		case SdltmFieldMetaType::Double:assert(false); break;

		case SdltmFieldMetaType::MultiText:
		case SdltmFieldMetaType::Text: return "string_attributes";
		case SdltmFieldMetaType::Number: return "numeric_attributes";
		case SdltmFieldMetaType::CheckboxList:
		case SdltmFieldMetaType::List: return "picklist_attributes";
		case SdltmFieldMetaType::DateTime: return "date_attributes";
		default:;
		}
		return "";
	}

	QString StringValueToSql(SdltmFieldMetaType fieldType, const QString& text, const QDateTime& date, int comboValue) {
		switch (fieldType) {
		case SdltmFieldMetaType::Int: assert(false); break;
		case SdltmFieldMetaType::Double: assert(false); break;
		case SdltmFieldMetaType::Text:
		case SdltmFieldMetaType::MultiText:
			return "'" + EscapeXmlAndSql(text) + "'";
		case SdltmFieldMetaType::Number:
			return text;

		case SdltmFieldMetaType::List:
			return QString::number(comboValue);
		case SdltmFieldMetaType::CheckboxList: break;

		case SdltmFieldMetaType::DateTime:
			return "datetime('" + date.toString(Qt::ISODate) + "')";
		default:;
		}
		assert(false);
		return "";
	}

	void GetSqlsToAddCustomField(int translationUnitId, const CustomFieldValue& newValue, std::vector<QString>& sqls) {
		auto tuIdStr = QString::number(translationUnitId);
		auto attributeIdStr = QString::number(newValue.Field().ID);
		if (!newValue.IsEmpty())
			switch (newValue.Field().FieldType) {
			case SdltmFieldMetaType::Int:
			case SdltmFieldMetaType::Double:
			case SdltmFieldMetaType::Number:
			case SdltmFieldMetaType::Text:
			case SdltmFieldMetaType::DateTime: {
				auto tableName = GetAttributeTableName(newValue.Field().FieldType);
				auto fieldValue = StringValueToSql(newValue.Field().FieldType, newValue.Text, newValue.Time, newValue.ComboId());
				sqls.push_back("INSERT INTO " + tableName + "(translation_unit_id,attribute_id,value) VALUES(" + QString::number(translationUnitId) + "," + attributeIdStr + "," + fieldValue + ");");
			}
											 break;

			case SdltmFieldMetaType::MultiText: {
				for (const auto& text : newValue.MultiText) {
					sqls.push_back("INSERT INTO string_attributes(translation_unit_id,attribute_id,value) VALUES(" + QString::number(translationUnitId) + "," + attributeIdStr + ",'" + EscapeXmlAndSql(text) + "');");
				}
			}
											  break;

			case SdltmFieldMetaType::List: {
				auto tableName = GetAttributeTableName(newValue.Field().FieldType);
				auto fieldValue = StringValueToSql(newValue.Field().FieldType, newValue.Text, newValue.Time, newValue.ComboId());
				sqls.push_back("INSERT INTO " + tableName + "(translation_unit_id,picklist_value_id) VALUES(" + QString::number(translationUnitId) + "," + fieldValue + ");");
			}
										 break;
			case SdltmFieldMetaType::CheckboxList: {
				// insert them one by one
				auto attributeIds = newValue.CheckboxIds();
				for (const auto& attributeId : attributeIds) {
					sqls.push_back("INSERT INTO picklist_attributes(translation_unit_id,picklist_value_id) VALUES(" + QString::number(translationUnitId) + "," + QString::number(attributeId) + ");");
				}
			}
												 break;
			default: assert(false); break;
			}
	}

}

ImportTmxToSql::ImportTmxToSql(const QString& fileName, DBBrowserDB& db, CustomFieldService& cfs)
	: _importFileName(fileName)
	, _db(db)
	, _customFieldService(cfs)
{
	SimpleXmlNode::ReadDate = TmxToDate;
}

namespace {
	struct TagOrText {
		// if non empty, it's a text
		QString Text;

		// bpt, ept, ph
		QString TagType;

		int Anchor = -1;
		int AlignmentAnchor = -1;
		// note:tag ID can also contain a prefix: TextPlaceholder or LockedContent
		QString TagID ;

		bool IsTextPlaceholder = false;
		bool IsLockedContent = false;

		QString TextEquivalent ;
		// if true, this tag should be ignored
		bool Ignore = false;

		bool IsStartTag() const { return TagType == "bpt"; }
		bool IsEndTag() const { return TagType == "ept"; }

		static TagOrText ParseTag(const QString & tagXml) {
			TagOrText tag;
			auto xml = SimpleXmlNode::Parse(tagXml);
			tag.TagType = xml.Name;
			xml.ReadAttribute("i", tag.Anchor);
			xml.ReadAttribute("x", tag.AlignmentAnchor);
			xml.ReadAttribute("type", tag.TagID);

			if (tag.TagID.startsWith("x-TextPlaceholder")) {
				tag.IsTextPlaceholder = true;
				tag.TagID = tag.TagID.mid(17);
			} else if (tag.TagID.startsWith("x-LockedContent")) {
				tag.IsLockedContent = true;
				tag.TagID = tag.TagID.mid(15);
			}

			return tag;
		}
	};

	struct ParseTu {
		ParseTu(const CustomFieldsTmxLoader& customFieldsLoader) : _customFieldsLoader(customFieldsLoader) {
		}

		void ParseStart(const QString & line) {
			Clear();
			auto xml = SimpleXmlNode::Parse(line);
			xml.ReadAttribute("creationdate", _creationDate);
			xml.ReadAttribute("creationid", _creationUser);
			xml.ReadAttribute("changedate", _changeDate);
			xml.ReadAttribute("changeid", _changeUser);
			xml.ReadAttribute("lastusagedate", _lastUseDate);
		}

		void ParseLine(const QString & line) {
			if (line.startsWith("<prop "))
				ParseProperty(line);
			else if (line.startsWith("<tuv "))
				ParseTuv(line);
			else if (line.startsWith("</tuv>"))
				_isSource = false;
			else
				ParseSeg(line);
		}

		void ParseEnd() {
			ParseEnd(_sourceSegment);
			ParseEnd(_targetSegment);

			if (_creationDate.isNull())
				_creationDate = QDateTime::currentDateTime();
			if (_changeDate.isNull())
				_changeDate = QDateTime::currentDateTime();
			if (_lastUseDate.isNull())
				_lastUseDate = QDateTime::currentDateTime();
		}


		std::vector<QString> GetInsertSql(int translationId) const {
			std::vector<QString> sqls;
			auto source = ToXmlSegment(_sourceSegment, _sourceLanguage);
			auto target = ToXmlSegment(_targetSegment, _targetLanguage);

			QString sqlPrefix , sqlSuffix ;
			AddFieldToSql(sqlPrefix, sqlSuffix, "id", translationId);
			AddFieldToSql(sqlPrefix, sqlSuffix, "creation_date", _creationDate);
			AddFieldToSql(sqlPrefix, sqlSuffix, "creation_user", _creationUser);
			AddFieldToSql(sqlPrefix, sqlSuffix, "change_date", _changeDate);
			AddFieldToSql(sqlPrefix, sqlSuffix, "change_user", _changeUser);
			AddFieldToSql(sqlPrefix, sqlSuffix, "last_used_date", _lastUseDate);
			AddFieldToSql(sqlPrefix, sqlSuffix, "insert_date", QDateTime::currentDateTime());
			AddFieldToSql(sqlPrefix, sqlSuffix, "last_used_user", _lastUser);
			AddFieldToSql(sqlPrefix, sqlSuffix, "guid", "");
			AddFieldToSql(sqlPrefix, sqlSuffix, "source_segment", source);
			AddFieldToSql(sqlPrefix, sqlSuffix, "target_segment", target);
			AddFieldToSql(sqlPrefix, sqlSuffix, "source_hash", 1);
			AddFieldToSql(sqlPrefix, sqlSuffix, "target_hash", 1);
			AddFieldToSql(sqlPrefix, sqlSuffix, "translation_memory_id", 1);
			AddFieldToSql(sqlPrefix, sqlSuffix, "usage_counter", 0);

			auto sql = "INSERT INTO translation_units(" + sqlPrefix + ") values(" + sqlSuffix + ")";
			sqls.push_back(sql);

			for (const auto& cf : _customFields)
				GetSqlsToAddCustomField(translationId, cf.second, sqls);

			return sqls;
		}

	private:
		static void AddFieldToSql(QString& prefix, QString & suffix, const QString & fieldName, const QString & fieldValue) {
			if (prefix != "")
				prefix += ",";
			if (suffix != "")
				suffix += ",";

			prefix += fieldName;
			suffix += "'" + EscapeSqlString(fieldValue) + "'";
		}
		static void AddFieldToSql(QString& prefix, QString& suffix, const QString& fieldName, const QDateTime& fieldValue) {
			if (prefix != "")
				prefix += ",";
			if (suffix != "")
				suffix += ",";

			prefix += fieldName;
			suffix += "'" + DateToSql(fieldValue) + "'";
		}
		static void AddFieldToSql(QString& prefix, QString& suffix, const QString& fieldName, int fieldValue) {
			if (prefix != "")
				prefix += ",";
			if (suffix != "")
				suffix += ",";

			prefix += fieldName;
			suffix += QString::number(fieldValue);
		}



		void ParseEnd(std::vector<TagOrText>& tags) {
			for (int i = 0; i < tags.size(); ++i)
				if (tags[i].IsTextPlaceholder || tags[i].IsLockedContent) {
					auto endI = i + 1;
					if (i < tags.size() - 1 && tags[i + 1].Text != "") {
						tags[i + 1].Ignore = true;
						tags[i].TextEquivalent = tags[i + 1].Text;
						endI = i + 2;
					}
					if (endI < tags.size() && tags[endI].IsEndTag())
						tags[endI].Ignore = true;
				}
		}

		QString ToTag(const TagOrText & tag, const QString & tagType) const {
			if (tag.IsTextPlaceholder || tag.IsLockedContent) {
			}
			QString xml = "<Tag>";
			xml += "<Type>" + tagType + "</Type>";
			if (tag.Anchor >= 0)
				xml += "<Anchor>" + QString::number(tag.Anchor) + "</Anchor>";
			if (tag.AlignmentAnchor >= 0)
				xml += "<AlignmentAnchor>" + QString::number(tag.AlignmentAnchor) + "</AlignmentAnchor>";
			if (tag.TagID != "")
				xml += "<TagID>" + tag.TagID + "</TagID>";

			if (tag.TextEquivalent != "")
				xml += "<TextEquivalent>" + EscapeXml(tag.TextEquivalent) + "</TextEquivalent>";

			xml += "<CanHide>true</CanHide>";
			xml += "</Tag>";
			return xml;
		}

		QString ToXmlSegment(const std::vector<TagOrText> & tags, const QString & language) const {
			// look for TextEquivalent
			QString xml = "<Segment xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><Elements>";

			std::vector<TagOrText> startTags;
			for(const auto &tag : tags) {
				if (tag.Text != "")
					xml += "<Text><Value>" + EscapeXml(tag.Text) + "</Value></Text>";
				else {
					if (tag.Ignore)
						continue;

					if (tag.IsTextPlaceholder || tag.IsLockedContent) {
						xml += ToTag(tag, tag.IsTextPlaceholder ? "TextPlaceholder" : "LockedContent");
					} else {
						// Start or End or Standalone
						if (tag.IsStartTag())
							startTags.push_back(tag);

						if (tag.IsStartTag())
							xml += ToTag(tag, "Start");
						else if (tag.IsEndTag())
							xml += ToTag(!startTags.empty() ? startTags.back() : tag, "End");
						else
							xml += ToTag(tag, "Standalone");

						if (tag.IsEndTag())
							startTags.pop_back();
					}
				}
			}

			xml += "</Elements><CultureName>" + language + "</CultureName></Segment>";
			return xml;
		}
	

		void UpdateCustomFieldValue(CustomFieldValue &value, const QString & str) {
			switch (value.Field().FieldType) {
			case SdltmFieldMetaType::Int:
			case SdltmFieldMetaType::Double:
			case SdltmFieldMetaType::Number:
				value.Text = QString::number(str.toInt());
				break;
			case SdltmFieldMetaType::DateTime:
				value.Time = TmxToDate(str);
				break;
			case SdltmFieldMetaType::Text:
				value.Text = str;
				break;
			case SdltmFieldMetaType::MultiText:
				value.MultiText.push_back(str);
				break;
			case SdltmFieldMetaType::List: {
				auto index = value.Field().StringValueToIndex(str);
				value.ComboIndex = index;
			}
				break;
			case SdltmFieldMetaType::CheckboxList: {
				auto index = value.Field().StringValueToIndex(str);
				if (index >= 0)
					value.CheckboxIndexes[index] = true;
			}
				break;
			default: ;
			}
		}

		void ParseProperty(const QString & line) {
			auto xml = SimpleXmlNode::Parse(line);
			if (xml.HasAttribute("type")) {
				auto type = xml.Attributes["type"];
				type = type.mid(2); // remove "x-"
				if (type == "LastUsedBy")
					_lastUser = xml.Value;
				else if (type.contains(':')) {
					auto idx = type.indexOf(':');
					type = type.left(idx);
					auto customField = _customFieldsLoader.TryGetCustomField(type);
					if (customField != nullptr) {
						auto id = customField->ID;
						if (_customFields.find(id) == _customFields.end())
							_customFields.insert(std::make_pair(id, CustomFieldValue(*customField)));
						auto& fieldValue = _customFields.find(id)->second;
						UpdateCustomFieldValue(fieldValue, xml.Value);
					}
				}
			}
		}

		void ParseTuv(const QString & line) {
			auto xml = SimpleXmlNode::Parse(line);
			xml.ReadAttribute("xml:lang", _isSource ? _sourceLanguage : _targetLanguage);
		}

		void ParseSeg(const QString & line) {
			_cachedSeg += line;
			if (_cachedSeg.contains("</seg>")) {
				ParseFullSeg(_isSource ? _sourceSegment : _targetSegment);
				_cachedSeg = "";
			}
		}

		void ParseFullSeg(std::vector<TagOrText> & segment) {
			_cachedSeg = _cachedSeg.trimmed();
			// remove <seg> and </seg>
			_cachedSeg = _cachedSeg.mid(5, _cachedSeg.size() - 11);
			auto idx = 0;
			while (idx >= 0) {
				auto idxNext = _cachedSeg.indexOf("<", idx);
				if (idxNext >= 0) {
					auto idxEndOfNext = _cachedSeg.indexOf("/>", idxNext);
					if (idxEndOfNext < 0)
						break; // malformed
					auto text = _cachedSeg.mid(idx, idxNext - idx);
					if (text.trimmed() != "")
						segment.push_back({ text });
					idxEndOfNext += 2;
					auto xml = _cachedSeg.mid(idxNext, idxEndOfNext - idxNext);
					segment.push_back(TagOrText::ParseTag(xml));
					idx = idxEndOfNext;
				} else {
					auto end = _cachedSeg.mid(idx);
					if (end.trimmed() != "")
						segment.push_back({ end });
					break;
				}
			}
		}

		void Clear() {
			_creationDate = _changeDate = _lastUseDate = QDateTime();
			_creationUser = _changeUser = _lastUser = "";
			_insideSeg = false;
			_isSource = true;
			_sourceLanguage = _targetLanguage = "";
			_customFields.clear();
			_cachedSeg = "";
			_sourceSegment.clear();
			_targetSegment.clear();
		}

		const CustomFieldsTmxLoader & _customFieldsLoader;

		QDateTime _creationDate, _changeDate, _lastUseDate;
		QString _creationUser, _changeUser, _lastUser;

		bool _insideSeg;

		// note : at this time, I assume we only have 2 tuv's - source and target (even though the TMX allows for any number)
		bool _isSource = true;
		QString _sourceLanguage, _targetLanguage;

		QString _cachedSeg;

		std::map<int, CustomFieldValue> _customFields;

		std::vector<TagOrText> _sourceSegment, _targetSegment;

	};
}

void ImportTmxToSql::Import() {
	// first, parse the header for custom properties
	CustomFieldsTmxLoader customFieldsLoader(_importFileName, _db, _customFieldService);
	customFieldsLoader.SaveToDb();

	int nextTranslationId = GetNextTranslationUnitId(_db);

	// I need to also parse the header
	QFile file(_importFileName);
	file.open(QIODevice::ReadOnly);

	QTextStream stream(&file);
	stream.setCodec("UTF-8");

	ParseTu parseTu(customFieldsLoader);
	BulkSqlExecute bulkExecute(_db);
	while (!stream.atEnd()) {
		auto line = stream.readLine();
		if (!_headerParsed) {
			if (line.contains("<header ")) {
				_headerParsed = true;
				auto xml = SimpleXmlNode::Parse(line);
				xml.ReadAttribute("srclang", _sourceLanguage);
				xml.ReadAttribute("creationdate", _creationDate);
				xml.ReadAttribute("creationid", _creationUser);
			}
		}

		if (!_headerEndReached)
			if (line.contains("</header>"))
				_headerEndReached = true;

		if (!_headerEndReached)
			continue;

		line = line.trimmed();
		if (line.startsWith("<tu ")) {
			parseTu.ParseStart(line);
		}
		else if (line.startsWith("</tu>")) {
			// save this TU into our db. -- bulkExecute
			parseTu.ParseEnd();
			auto sqls = parseTu.GetInsertSql(nextTranslationId);
			for (const auto& sql : sqls)
				bulkExecute.AddSql(sql);

			++nextTranslationId;
		}
		else
			parseTu.ParseLine(line);
	}
}
