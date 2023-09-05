#include "SimpleXml.h"

std::function<QDateTime(const QString&)> SimpleXmlNode:: ReadDate;

bool SimpleXmlNode::HasAttribute(const QString& name) const { return Attributes.find(name) != Attributes.end(); }

void SimpleXmlNode::ReadAttribute(const QString& name, QDateTime& value) {
	if (HasAttribute(name))
		value = ReadDate(Attributes[name]);
}

void SimpleXmlNode::ReadAttribute(const QString& name, QString& value) {
	if (HasAttribute(name))
		value = Attributes[name];
}

void SimpleXmlNode::ReadAttribute(const QString& name, int& value) {
	if (HasAttribute(name))
		value = Attributes[name].toInt();
}

// I assume I have at most one XML value and thus, at most, one set of attributes
SimpleXmlNode SimpleXmlNode::Parse(const QString& xml) {
	SimpleXmlNode node;

	auto idxName = xml.indexOf('<');
	if (idxName < 0)
		return node;
	++idxName;
	auto idxSpace = xml.indexOf(' ', idxName);
	auto idxEndOfAttributes = xml.indexOf('>', idxName);
	auto idxNextAttribute = idxName;
	if (idxSpace > 0 || idxEndOfAttributes > 0) {
		auto idxEndOfName = (idxSpace >= 0 && idxSpace < idxEndOfAttributes) || idxEndOfAttributes < 0 ? idxSpace : idxEndOfAttributes;
		node.Name = xml.mid(idxName, idxEndOfName - idxName);
		if (idxEndOfName == idxSpace)
			idxNextAttribute = idxEndOfName + 1;
		else
			idxNextAttribute = -1;
	}

	auto foundEnd = false;
	while (idxNextAttribute >= 0 && !foundEnd) {
		auto idxEqual = xml.indexOf('=', idxNextAttribute);
		if (idxEqual < 0)
			break;

		auto attributeName = xml.mid(idxNextAttribute, idxEqual - idxNextAttribute).trimmed();
		idxEqual++;

		// name="value"
		auto valueIsString = xml[idxEqual] == '\"';
		if (valueIsString)
			++idxEqual;
		auto endOfValue = valueIsString ? '\"' : ' ';
		auto idxEndOfAttributeValue = xml.indexOf(endOfValue, idxEqual);
		if (idxEndOfAttributeValue < 0)
			idxEndOfAttributeValue = xml.indexOf('>');
		if (idxEndOfAttributeValue < 0)
			break;

		auto attributeValue = xml.mid(idxEqual, idxEndOfAttributeValue - idxEqual);
		if (!valueIsString && attributeValue.endsWith('/')) {
			// in this case, we actually found end-of-xml-value (/>)
			attributeValue = attributeValue.left(attributeValue.size() - 1);
			foundEnd = true;
		}

		node.Attributes[attributeName] = attributeValue;
		if (xml[idxEndOfAttributeValue] == '>')
			break;
		idxNextAttribute = idxEndOfAttributeValue + 1;
	}

	if (foundEnd)
		return node;

	if (idxEndOfAttributes < 0)
		// we can't find end-of-attributes ('>' or '/>')
		return node;

	idxEndOfAttributes++;
	auto idxEndOfValue = xml.indexOf('<', idxEndOfAttributes);
	if (idxEndOfValue >= 0)
		node.Value = xml.mid(idxEndOfAttributes, idxEndOfValue - idxEndOfAttributes);
	else
		node.Value = xml.mid(idxEndOfAttributes);

	return node;
}
