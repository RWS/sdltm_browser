#include "ExportSqlToTmx.h"

#include <QDomDocument>
#include <QIODevice>
#include <sqlite3.h>

#include "FileDialog.h"
#include "SdltmSqlUtil.h"
#include "SdltmUtil.h"

ExportSqlToTmx::ExportSqlToTmx(const QString& fileName, DBBrowserDB& db, CustomFieldService& cfs)
	: _db(&db)
	, _fieldValues(db, cfs)
	, _exportFileName(fileName)
	, _fields(cfs.GetFields()) 
{

}

void ExportSqlToTmx::Export(const QString& sql) {
	QFile file(_exportFileName);
	if (file.open(QIODevice::WriteOnly)) {
		file.write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<tmx version=\"1.4\">\r\n");
		WriteHeader(file);
		file.write("  <body>\r\n");
		WriteBody(file, sql);
		file.write("  </body>\r\n</tmx>");
	}
}

namespace  {
	QString DateToTmx(QDateTime dt) {
		// FIXME
		// creationdate="20230407T055447Z"
		return dt.toString("yyyyMMddThhmmssZ");
		//return dt.toString(Qt::ISODate);
	}

	QString FieldTypeToTmx(SdltmFieldMetaType fieldType) {
		switch (fieldType) {
		case SdltmFieldMetaType::Int:
		case SdltmFieldMetaType::Double:
		case SdltmFieldMetaType::Number:
			return "Integer";
		case SdltmFieldMetaType::Text:
			return "SingleString";
		case SdltmFieldMetaType::MultiText:
			return "MultipleString";
			break;
		case SdltmFieldMetaType::List:
			return "SinglePicklist";
		case SdltmFieldMetaType::CheckboxList:
			return "MultiplePicklist";
		case SdltmFieldMetaType::DateTime:
			return "DateTime";
		default: assert(false);
		}
		return "";
	}
}

void ExportSqlToTmx::WriteHeader(QFile& file) {
	auto forceWait = true;
	auto pDb = _db->get("export sql - header", forceWait);

	QString sql = "select id, name, source_language, creation_user, creation_date from translation_memories";
	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
	QString header = "<header>\r\n";
	QString name = "";
	if (status == SQLITE_OK) {
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			auto id = ReadSqlIntField(stmt, 0);
			name = ReadSqlStringField(stmt, 1);
			auto sourceLanguage = ReadSqlStringField(stmt, 2);
			auto creationUser = ReadSqlStringField(stmt, 3);
			auto creationDate = ReadSqlDateTimeField(stmt, 4);
			header = "  <header creationtool=\"SDL Language Platform\" creationtoolversion=\"8.1\" o-tmf=\"SDL TM8 Format\" datatype=\"xml\" segtype=\"sentence\" ";
			header += " adminlang=\"" + sourceLanguage + "\" srclang=\"" + sourceLanguage + "\"";
			header += " creationdate=\"" + DateToTmx(creationDate) + "\" creationid=\"" + creationUser+ "\">\r\n";
		}
		sqlite3_finalize(stmt);
	}
	file.write(header.toUtf8());

	for (const auto & cf : _fields) {
		QString field = "    <prop type=\"x-" + cf.FieldName + ":";
		switch (cf.FieldType) {
		case SdltmFieldMetaType::Int:
		case SdltmFieldMetaType::Double:
		case SdltmFieldMetaType::Number:
			field += "Integer\"></prop>";
			break;
		case SdltmFieldMetaType::Text:
			field += "SingleString\"></prop>";
			break;
		case SdltmFieldMetaType::MultiText:
			field += "MultipleString\"></prop>";
			break;
		case SdltmFieldMetaType::List: {
			field += "SinglePicklist\">";
			QString list;
			for (const auto & value : cf.Values) {
				if (list != "")
					list += ",";
				list += EscapeXml(value) ;
			}
			field += list;
			field += "</prop>";
		}
			break;
		case SdltmFieldMetaType::CheckboxList: {
			field += "MultiplePicklist\">";
			QString list;
			for (const auto& value : cf.Values) {
				if (list != "")
					list += ",";
				list += EscapeXml(value);
			}
			field += list;
			field += "</prop>";
		}
			break;
		case SdltmFieldMetaType::DateTime:
			field += "DateTime\"></prop>";
			break;
		default: ;
		}
		field += "\r\n";
		file.write(field.toUtf8());
	}

	file.write("    <prop type=\"x-Recognizers\">RecognizeAll</prop>\r\n");
	file.write("    <prop type=\"x-IncludesContextContent\">True</prop>\r\n");
	file.write(("    <prop type=\"x-TMName\">" + EscapeXml(name) + "</prop>\r\n").toUtf8());
	file.write("    <prop type=\"x-TokenizerFlags\">DefaultFlags</prop>\r\n");
	file.write("    <prop type=\"x-WordCountFlags\">DefaultFlags</prop>\r\n");
	file.write("  </header>\r\n");
}

void ExportSqlToTmx::WriteBody(QFile& file, const QString& sql) {

	auto forceWait = true;
	auto pDb = _db->get("export sql", forceWait);

	sqlite3_stmt* stmt;
	int status = sqlite3_prepare_v2(pDb.get(), sql.toStdString().c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
	if (status == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			// ID, source segment, target segment, creation date, creation id , change date, change id, lastusage date, last usage user
			WriteTuHeader(file, stmt);

			WriteSegment(file, ReadSqlStringField(stmt, 1));
			WriteSegment(file, ReadSqlStringField(stmt, 2));

			WriteTuEnd(file);
		}
		sqlite3_finalize(stmt);
	}
}

namespace  {
	void WriteStringAttribute(QFile & file, const QString & name, const QString & value) {
		if (value.isNull())
			return;
		auto attr = " " + name + "=\"" + EscapeXml(value) + "\"";
		file.write(attr.toUtf8());
	}
	void WriteDateAttribute(QFile& file, const QString& name, const QDateTime value) {
		if (value.isNull())
			return;
		auto attr = " " + name + "=\"" + DateToTmx(value) + "\"";
		file.write(attr.toUtf8());
	}
	void WriteProperty(QFile & file, const QString & name, const QString & value) {
		auto prop = "      <prop type=\"" + name + "\">" + EscapeXml(value) + "</prop>\r\n";
		file.write(prop.toUtf8());
	}
}


// <tu creationdate="20230407T055447Z" creationid="GLOBAL\jtorjo" changedate="20230518T084224Z" changeid="GLOBAL\jtorjo" lastusagedate="20230407T055447Z">
void ExportSqlToTmx::WriteTuHeader(QFile& file, sqlite3_stmt* stmt) {
	// ID, source segment, target segment, creation date, creation id , change date, change id, lastusage date, last usage user

	auto creationDate = ReadSqlDateTimeField(stmt, 3);
	auto creationId = ReadSqlStringField(stmt, 4);
	auto changeDate = ReadSqlDateTimeField(stmt, 5);
	auto changeId = ReadSqlStringField(stmt, 6);
	auto lastDate = ReadSqlDateTimeField(stmt, 7);
	auto lastId = ReadSqlStringField(stmt, 8);

	file.write("    <tu");
	WriteDateAttribute(file, "creationdate", creationDate);
	WriteStringAttribute(file, "creationid", creationId);

	WriteDateAttribute(file, "changedate", changeDate);
	WriteStringAttribute(file, "changeid", changeId);
	WriteDateAttribute(file, "lastusagedate", lastDate);
	file.write(">\r\n");

	WriteProperty(file, "x-LastUsedBy", lastId);
	WriteProperty(file, "x-Context", "0, 0");
	WriteProperty(file, "x-Origin", "TM");
	// FIXME this can be any of Draft, Translated, RejectedTranslation, ApprovedTranslation, RejectedSignOff, ApprovedSignOff, 
	WriteProperty(file, "x-ConfirmationLevel", "Translated");

	auto id = sqlite3_column_int(stmt, 0);
	auto values = _fieldValues.TryGetCustomValues(id);
	if (!values->IsEmpty()) {
		for(const auto & value : values->Numbers) {
			const auto& field = _fieldValues.GetCustomField(value.CustomFieldID);
			WriteProperty(file, "x-" + field.FieldName + ":" + FieldTypeToTmx(field.FieldType), QString::number(value.Value));
		}
		for (const auto& value : values->Dates) {
			const auto& field = _fieldValues.GetCustomField(value.CustomFieldID);
			WriteProperty(file, "x-" + field.FieldName + ":" + FieldTypeToTmx(field.FieldType), DateToTmx(value.Value));
		}
		for (const auto& value : values->Texts) {
			const auto& field = _fieldValues.GetCustomField(value.CustomFieldID);
			WriteProperty(file, "x-" + field.FieldName + ":" + FieldTypeToTmx(field.FieldType), value.Text);
		}
		// <prop type = "x-Titi Multi:MultipleString">cucu</prop>
		// <prop type = "x-Titi Multi:MultipleString">gigi</prop>
		// <prop type = "x-Titi Multi:MultipleString">mumu</prop>
		for (const auto& value : values->MultiTexts) {
			const auto& field = _fieldValues.GetCustomField(value.CustomFieldID);
			for (const auto & text : value.Texts)
				WriteProperty(file, "x-" + field.FieldName + ":" + FieldTypeToTmx(field.FieldType), text);
		}
		for (const auto& value : values->Combos) {
			const auto& field = _fieldValues.GetCustomField(value.CustomFieldID);
			WriteProperty(file, "x-" + field.FieldName + ":" + FieldTypeToTmx(field.FieldType), value.Value);
		}
		// <prop type = "x-ListyShmisty:MultiplePicklist">abc</prop>
		// <prop type = "x-ListyShmisty:MultiplePicklist">def</prop>
		for (const auto& value : values->Checklists) {
			const auto& field = _fieldValues.GetCustomField(value.CustomFieldID);
			for(const auto & text : value.Values)
				WriteProperty(file, "x-" + field.FieldName + ":" + FieldTypeToTmx(field.FieldType), text);
		}
	}
}

namespace  {
	void ParseSegmentXmlText(QString& tmx, const QDomElement& node) {
		tmx += node.firstChild().nodeValue();
	}
	void ParseSegmentXmlTag(QString& tmx, const QDomElement& node) {
		auto titi = node.nodeName();


		auto tagType = node.firstChildElement("Type").firstChild().nodeValue();
		auto anchor = node.firstChildElement("Anchor").firstChild().nodeValue();
		auto alignmentAnchor = node.firstChildElement("AlignmentAnchor").firstChild().nodeValue();
		auto tagId = node.firstChildElement("TagID").firstChild().nodeValue();

		if (tagType == "Start") {
			tmx += "<bpt i=\"" + anchor + "\"";
			if (!tagId.isNull())
				tmx += " type=\"" + tagId + "\"";
			if (!alignmentAnchor.isNull())
				tmx += " x=\"" + alignmentAnchor + "\"";
			tmx += " />";
		} else if (tagType == "End") {
			tmx += "<ept i=\"" + anchor + "\" />";
		} else if (tagType == "Standalone") {
			tmx += "<ph " ;
			if (!alignmentAnchor.isNull())
				tmx += " x=\"" + alignmentAnchor + "\"";
			if (!tagId.isNull())
				tmx += " type=\"" + tagId + "\"";
			tmx += " />";
		} else if (tagType == "TextPlaceholder" || tagType == "LockedContent") {
			tmx += "<bpt i=\"" + anchor + "\"";

			QString id = "x-" + tagType;
			if (!tagId.isNull())
				id += tagId;
			tmx += " type=\"" + id + "\"";

			if (!alignmentAnchor.isNull())
				tmx += " x=\"" + alignmentAnchor + "\"";

			tmx += " />";

			auto textEquivalent = node.firstChildElement("TextEquivalent").firstChild().nodeValue();
			tmx += textEquivalent;

			tmx += "<ept i=\"" + anchor + "\" />";
		} 
	 }

	void ParseSegmentXml(QString & tmx, QDomElement & node) {
		auto name = node.nodeName();
		if (name == "Text")
			ParseSegmentXmlText(tmx, node.firstChildElement("Value"));
		else if (name == "Tag")
			ParseSegmentXmlTag(tmx, node);
		else
			assert(false);
	}
}

void ExportSqlToTmx::WriteSegment(QFile& file, const QString& segment) {
	// note: the laguage is present in the xml
	QDomDocument xml("xml");
	xml.setContent(segment);
	auto root = xml.firstChild().firstChildElement("Elements");
	QString tmx;
	for (auto child = root.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
		ParseSegmentXml(tmx, child);
	}

	auto language = xml.firstChild().firstChildElement("CultureName").firstChild().nodeValue();

	file.write(("      <tuv xml:lang=\"" + language + "\">\r\n").toUtf8());
	file.write("        <seg>");
	file.write(tmx.toUtf8());
	file.write("</seg>\r\n       </tuv>\r\n");
}

void ExportSqlToTmx::WriteTuEnd(QFile& file) {
	file.write("    </tu>\r\n");
}
