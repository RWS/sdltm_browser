#include "SdltmSqlViewStyledItemDelegate.h"

#include <qabstracttextdocumentlayout.h>
#include <QPainter>
#include <QTextDocument>

#include "SdltmUpdateCache.h"

namespace {
	// if we don't have at least these many chars to highlight on, don't do anything
	const int MIN_TEXT_SIZE = 2;

	const int EXTRA_HEIGHT = 5;

	const QString HTML_PREFIX = "<html><head/><body><p><span style=\"font-size:10pt;\">";
	const QString HTML_SUFFIX = "</span></p></body></html>";

	const QString HTML_HIGHLIGHT_PREFIX = "<span style=\" font-size:10pt; background-color:#ff00ff; color:#ffffff; \">";
	const QString HTML_HIGHLIGHT_SUFFIX = "</span>";

	struct TextInfo {
		QString text;
		bool isHighlighted;
	};

	std::vector<TextInfo> SplitText(const QString & text, const QString & find, bool caseSensitive) {
		std::vector<TextInfo> split;
		auto idx = 0;
		while(true) {
			auto nextIdx = text.indexOf(find, idx, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
			if (nextIdx < 0) {
				auto remaining = text.right(text.size() - idx);
				if (remaining.trimmed() != "")
					split.push_back({ remaining, false });
				break;
			}

			auto beforeHighlight = text.mid(idx, nextIdx - idx);
			split.push_back({ beforeHighlight, false });
			split.push_back({ find, true});
			idx = nextIdx + find.size();
		}
		return split;
	}
	std::vector<TextInfo> SplitTextRegex(const QString& text, const QString& regexFind, bool caseSensitive) {
		std::vector<TextInfo> split;

		QRegularExpression regex(regexFind);
		if (regexFind.trimmed() != "" && regex.isValid()) {
			auto matches = caseSensitive ? regex.globalMatch(text) : regex.globalMatch(text, QRegularExpression::CaseInsensitiveOption);
			auto idx = 0;
			while (matches.hasNext()) {
				auto match = matches.next();

				auto nextIdx = match.capturedStart();
				auto len = match.capturedLength();

				auto beforeHighlight = text.mid(idx, nextIdx - idx);
				auto found = text.mid(nextIdx, len);
				split.push_back({ beforeHighlight, false });
				split.push_back({ found, true });
				idx = nextIdx + len;
			}

			auto remaining = text.right(text.size() - idx);
			if (remaining.trimmed() != "")
				split.push_back({ remaining, false });
		}

		if (split.empty())
			// regex is invalid, or regex found nothing
			split.push_back({ text, false });

		
		return split;
	}

	QString ToHtml(const std::vector<TextInfo> & texts) {
		auto text = HTML_PREFIX;
		for(const auto & ti : texts) {
			if (ti.isHighlighted)
				text += HTML_HIGHLIGHT_PREFIX;
			text += ti.text;
			if (ti.isHighlighted)
				text += HTML_HIGHLIGHT_SUFFIX;
		}
		text += HTML_SUFFIX;
		return text;
	}

	QString ToRegex(const FindAndReplaceTextInfo & info) {
		if (info.UseRegex)
			return info.Find;
		assert(info.WholeWordOnly);
		return "\\b" + info.Find + "\\b";
	}
}

SdltmSqlViewStyledItemDelegate::SdltmSqlViewStyledItemDelegate(QObject* parent)
		: QStyledItemDelegate(parent) {
}

void SdltmSqlViewStyledItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const {
	auto itemInfo = option;
	initStyleOption(&itemInfo, index);

	painter->save();

	if (index.column() == 0) {
		// IMPORTANT: in order to access data from the cache, I need the translation unit ID.
		// however, when drawing source or target texts, I can't access this. So, I assume data is printed left-to-right, and top-to-bottom
		_translationUnitId = index.data().toInt();
	}

	bool matchesSourceOrTargetColumn = 
		(index.column() == 1 && (_highlightInfo.Type == FindAndReplaceTextInfo::SearchType::Both || _highlightInfo.Type == FindAndReplaceTextInfo::SearchType::Source))
		|| (index.column() == 2 && (_highlightInfo.Type == FindAndReplaceTextInfo::SearchType::Both || _highlightInfo.Type == FindAndReplaceTextInfo::SearchType::Target));
	if (!matchesSourceOrTargetColumn) {
		// default paint
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}

	auto isSource = index.column() == 1;
	if (_updateCache != nullptr && _updateCache->Has(_translationUnitId, isSource ? SdltmUpdateCache::ValueType::Source : SdltmUpdateCache::ValueType::Target))
		itemInfo.text = _updateCache->GetFriendlyText(_translationUnitId, isSource ? SdltmUpdateCache::ValueType::Source : SdltmUpdateCache::ValueType::Target);

	std::vector<TextInfo> split;
	auto isRegex = _highlightInfo.UseRegex || _highlightInfo.WholeWordOnly;
	if (_highlightInfo.Find.size() >= MIN_TEXT_SIZE)
		split = isRegex ? SplitTextRegex(itemInfo.text, ToRegex(_highlightInfo), _highlightInfo.MatchCase) : SplitText(itemInfo.text, _highlightInfo.Find, _highlightInfo.MatchCase);

	if (split.empty())
		// nothing to highlight
		split.push_back( { itemInfo.text, false} );

	auto text = ToHtml(split);
	_cachehtmlDoc.setHtml(text);

	itemInfo.text = "";
	itemInfo.widget->style()->drawControl(QStyle::CE_ItemViewItem, &itemInfo, painter);

	painter->translate(itemInfo.rect.left(), itemInfo.rect.top());
	QRect clip(0, 0, itemInfo.rect.width(), itemInfo.rect.height());
	painter->setClipRect(clip);
	_cachehtmlDoc.setTextWidth(itemInfo.rect.width());
	_cachehtmlDoc.drawContents(painter, clip);
	painter->restore();
}

QSize SdltmSqlViewStyledItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto size = QStyledItemDelegate::sizeHint(option, index);
	// seems when writing HTML, it needs a bit more space
	size.setHeight(size.height() + EXTRA_HEIGHT);
	return size;
}

void SdltmSqlViewStyledItemDelegate::SetHightlightText(const FindAndReplaceTextInfo& highlight) {
	_highlightInfo = highlight;
}

void SdltmSqlViewStyledItemDelegate::SetUpdateCache(const SdltmUpdateCache& updateCache) {
	_updateCache = &updateCache;
}
