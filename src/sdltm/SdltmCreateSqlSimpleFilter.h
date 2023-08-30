#pragma once
#include "CustomFieldService.h"
#include "SdltmFilter.h"


class SdltmCreateSqlSimpleFilter
{
public:
	SdltmCreateSqlSimpleFilter(const SdltmFilter& filter, const std::vector<CustomField> & customFields);

	enum class FilterType {
		UI, Export,
	};

	QString ToSqlFilter(FilterType filterType = FilterType::UI) const;

private:
	QString CustomExpression(const SdltmFilterItem& fi) const;

private:
	SdltmFilter _filter;
	std::vector<CustomField> _customFields;
};

