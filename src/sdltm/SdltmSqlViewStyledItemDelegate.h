#pragma once
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "BatchEdit.h"


class SdltmUpdateCache;

// allows painting highlighted text - for Batch Edit (Find and Replace)
class SdltmSqlViewStyledItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	explicit SdltmSqlViewStyledItemDelegate(QObject* parent = nullptr);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	const FindAndReplaceTextInfo& HighlightText() const { return _highlightInfo; }
	void SetHightlightText(const FindAndReplaceTextInfo& highlight);

	void SetUpdateCache(const SdltmUpdateCache& updateCache);

private:
	mutable QTextDocument _cachehtmlDoc;

	FindAndReplaceTextInfo _highlightInfo;
	mutable int _translationUnitId = -1;
	const SdltmUpdateCache* _updateCache;
};

