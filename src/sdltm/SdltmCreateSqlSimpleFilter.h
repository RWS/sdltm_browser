#pragma once
#include "CustomFieldService.h"
#include "SdltmFilter.h"


class SdltmCreateSqlSimpleFilter
{
public:
	SdltmCreateSqlSimpleFilter(const SdltmFilter& filter, const std::vector<CustomField> & customFields);

	QString ToSqlFilter() const;

private:
	SdltmFilter _filter;
	std::vector<CustomField> _customFields;
};

