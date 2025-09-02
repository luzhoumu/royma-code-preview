#include <ui/royma_text_view.h>

namespace royma
{
	REGISTER_CLASS(TextView,

		BorderWeight = 4.0f;
		SliderSize = 20.0f;
		m_textSize = float2(0, 0);
		setSelectionColor(rgba(0x007f7f7f));

		Background = ResourceManager::create<ImageView>();
		Background->disable();
		Background->setTag(L"Background");
		addSubView(Background);

		CandList = ResourceManager::create<Label>();
		CandList->setTextAlignment(TEXT_ALIGNMENT::LEADING_TOP);
		CandList->setForcePrint(true);
		CandList->disable();
		CandList->setTag(L"CandList");
		addSubView(CandList);

		Composition = ResourceManager::create<Label>();
		Composition->setTextAlignment(TEXT_ALIGNMENT::LEADING_TOP);
		Composition->disable();
		Composition->setTag(L"Composition");
		getContent()->addSubView(Composition);

		m_cursor = ResourceManager::create<Cursor>();
		m_cursor->setComposition(Composition);
		m_cursor->setTag(L"Cursor");
		getContent()->addSubView(m_cursor);

		getContent()->raiseToTop();
		getContent()->disable();
	);

	TextView::~TextView(){}

	slong TextView::calcCharIndexWithCursor(float x, float y)
	{
		slong nCharIndex = -1;
		float fPadding;
		SRect myFrame = getFrame();
		SRect contentFrame = getContent()->getFrame();

		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			fPadding = SliderSize + BorderWeight;
		}
		else
		{
			fPadding = getBorderWeight();
			myFrame.top += (myFrame.height() - getFont()->lineHeight()) / 2.0f;
		}

		if (x > myFrame.left && y > myFrame.top && x < myFrame.right - fPadding && y < myFrame.bottom - fPadding)
		{
			slong nLineStart = 0;
			float fCursorX = 0.0f;
			float fontWidth = (float)Composition->getFont()->getFontSize();
			float lineHeight = getFont()->lineHeight();
			float2 localLineSize = { 0 };
			float2 lastLineSize = { 0 };
			nCharIndex = getText().length();
			const wchar* szLine;
			float fCurrentCharWidth = 0.0f;

			for (const STextLineInfo& lineInfo : m_cursor->lines())
			{
				localLineSize.x = lineInfo.lineWidth;
				localLineSize.y = lastLineSize.y;
				szLine = *lineInfo.lineString;

				if ((y - contentFrame.top) - localLineSize.y < lineHeight)
				{
					nCharIndex = nLineStart + lineInfo.lineString.length();

					for (slong i = 0; i <= lineInfo.lineString.length(); i++)
					{
						fCurrentCharWidth = getFont()->getCharWidth(szLine[i]);

						if ((x - contentFrame.left) - fCursorX < fCurrentCharWidth / 2.0f)
						{
							nCharIndex = nLineStart + i;
							break;
						}

						fCursorX += fCurrentCharWidth;
					}

					break;
				}
				else
				{
					nLineStart += lineInfo.lineString.length() + 1;
					lastLineSize.y += lineHeight;
				}
			}
		}

		return nCharIndex;
	}

	void TextView::onActive(float x, float y)
	{
		slong nCharIndex = calcCharIndexWithCursor(x, y);

		if (nCharIndex > -1)
		{
			m_cursor->beginDrag(nCharIndex);
			m_cursor->endDrag(nCharIndex);
			m_cursor->show();
		}
	}

	void TextView::onDrag(float x, float y)
	{
		slong nCharIndex = calcCharIndexWithCursor(x, y);

		if (nCharIndex > -1)
		{
			m_cursor->endDrag(nCharIndex);
			followCursor();
		}
	}

	void TextView::onLayoutSubViews()
	{
		if (focus() == this || (isBackgroundResponder() && isAncestryOf(focus())))
		{
			const SRect myFrame = getFrame();
			const SRect contentFrame = getContent()->getFrame();

			string strComposition = Ime::global()->composition();
			string strCandList = Ime::global()->candList();
			string strImeName = Ime::global()->name();

			if (strImeName != L"")
			{
				CandList->setText(strCandList << L"\n" << strImeName << L": " << strComposition);
			}
			else
			{
				CandList->setText(L"");
			}

			if (m_cursor->charIndex() != m_cursor->startIndex())
			{
				slong nStartIndex = min(m_cursor->charIndex(), m_cursor->startIndex());
				slong nEndIndex = max(m_cursor->charIndex(), m_cursor->startIndex());

				slong nLineStart = 0;
				float2 frameOffset = { contentFrame.left, contentFrame.top };
				bool bFirstLine = true;
				strong<Screen> screen = Screen::global();
				float2 startSize = { 0 };
				float2 endSize = { 0 };

				for (const STextLineInfo& lineInfo : m_cursor->lines())
				{
					if (nLineStart + lineInfo.lineString.length() < nStartIndex)
					{
						nLineStart += lineInfo.lineString.length() + 1;
						frameOffset.y += getFont()->lineHeight();
					}
					else if (nLineStart + lineInfo.lineString.length() >= nEndIndex)
					{
						if (bFirstLine)
						{
							startSize = getCursor()->calcTextSize(lineInfo.lineString, nStartIndex - nLineStart);
							endSize = getCursor()->calcTextSize(lineInfo.lineString, nEndIndex - nLineStart);

							screen->drawSolidRect(SRect(frameOffset.x + startSize.x,
								frameOffset.y,
								endSize.x - startSize.x,
								endSize.y),
								getContent()->getDirtyRect(),
								getSelectionColor());
						}
						else
						{
							endSize = getCursor()->calcTextSize(lineInfo.lineString, nEndIndex - nLineStart);

							screen->drawSolidRect(SRect(frameOffset.x,
								frameOffset.y,
								endSize.x,
								getFont()->lineHeight()),
								getContent()->getDirtyRect(),
								getSelectionColor());
						}

						break;
					}
					else
					{
						if (bFirstLine)
						{
							startSize = getCursor()->calcTextSize(lineInfo.lineString, nStartIndex - nLineStart);
							//endSize = getCursor()->calcTextSize(lineInfo.lineString, lineInfo.lineString.length());
							endSize = float2(lineInfo.lineWidth, getFont()->lineHeight());

							screen->drawSolidRect(SRect(frameOffset.x + startSize.x,
								frameOffset.y,
								endSize.x - startSize.x,
								endSize.y),
								getContent()->getDirtyRect(),
								getSelectionColor());

							nLineStart += lineInfo.lineString.length() + 1;
							frameOffset.y += getFont()->lineHeight();
							bFirstLine = false;
						}
						else
						{
							//endSize = getCursor()->calcTextSize(lineInfo.lineString, lineInfo.lineString.length());
							endSize = float2(lineInfo.lineWidth, getFont()->lineHeight());

							screen->drawSolidRect(SRect(frameOffset.x,
								frameOffset.y,
								endSize.x,
								getFont()->lineHeight()),
								getContent()->getDirtyRect(),
								getSelectionColor());

							nLineStart += lineInfo.lineString.length() + 1;
							frameOffset.y += getFont()->lineHeight();
						}
					}
				}
			}
		}
		else
		{
			m_cursor->forceHide();
		}
	}

	void TextView::onStringInput(const string& str)
	{
		insert(str);
	}

	void TextView::deleteChar(bool bBack)
	{
		if (bBack)
		{
			if (m_cursor->charIndex() - m_cursor->startIndex())
			{
				insert(L"");
			}
			else
			{
				m_cursor->beginDrag(m_cursor->startIndex() - 1);
				insert(L"");
			}
		}
		else
		{
			if (m_cursor->charIndex() - m_cursor->startIndex())
			{
				insert(L"");
			}
			else
			{
				m_cursor->endDrag(m_cursor->charIndex() + 1);
				insert(L"");
			}
		}
	}

	void TextView::onKeyDown(KEY key)
	{
		int nLineStart = 0;
		int nIndexInLine = -1;
		bool bShift = key & KEY_SHIFT;

		switch (key & 0xff)
		{
		case KEY_DEL:
			deleteChar(false);
			followCursor();
			break;

		case KEY_BACK:
			deleteChar(true);
			followCursor();
			break;

		case KEY_ENTER:
			if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
			{
				insert(L"\n");
			}
			else
			{
				if (enterAction() != nullptr)
				{
					enterAction()(this);
				}
			}
			break;

		case KEY_CHECK_ALL:
			m_cursor->beginDrag(0);
			m_cursor->endDrag(Composition->getText().length());
			break;

		case KEY_COPY:
			SystemBuffer::global()->copy(m_cursor->selectedString());
			break;

		case KEY_CUT:
			SystemBuffer::global()->copy(m_cursor->selectedString());
			insert(L"");
			break;

		case KEY_PASTE:
			insert(SystemBuffer::global()->paste());
			break;

		case KEY_UP:
			m_cursor->verticalMove(false, bShift);
			followCursor();
			break;

		case KEY_DOWN:
			m_cursor->verticalMove(true, bShift);
			followCursor();
			break;

		case KEY_LEFT:
			m_cursor->horizontalMove(false, bShift);
			followCursor();
			break;

		case KEY_RIGHT:
			m_cursor->horizontalMove(true, bShift);
			followCursor();
			break;

		case KEY_PAGE_UP:
			m_cursor->page(false, bShift, (int)ceilf((getFrame().height() - 2.0f*SliderSize) / getFont()->lineHeight()));
			followCursor();
			break;

		case KEY_PAGE_DOWN:
			m_cursor->page(true, bShift, (int)ceilf((getFrame().height() - 2.0f*SliderSize) / getFont()->lineHeight()));
			followCursor();
			break;

		case KEY_HOME:
			m_cursor->moveEnds(false, bShift);
			followCursor();
			break;

		case KEY_END:
			m_cursor->moveEnds(true, bShift);
			followCursor();
			break;

		default:
			break;
		}
	}

	void TextView::onScroll(float2 offset)
	{
		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			VerticalBar->onScroll(offset);
		}
		else
		{
			ScrollView::onScroll(offset);
		}
	}

	const std::function<void(View*)> TextView::enterAction()
	{
		return m_enterAction;
	}

	void TextView::setEnterAction(std::function<void(View*)> action)
	{
		m_enterAction = action;
	}

	void TextView::followCursor()
	{
		resetFrame();
		SRect myFrame = getFrame();
		SRect cursorFrame = m_cursor->getFrame();

		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			if (cursorFrame.right > myFrame.right - 2.0f*SliderSize)
			{
				HorizontalBar->onScroll(float2{ (myFrame.right - 2.0f*SliderSize) - cursorFrame.right, 0.0f });
			}

			if (cursorFrame.left < myFrame.left + BorderWeight)
			{
				HorizontalBar->onScroll(float2{ (myFrame.left + BorderWeight) - cursorFrame.left, 0.0f });
			}

			if (cursorFrame.bottom > myFrame.bottom - 2.0f*SliderSize)
			{
				VerticalBar->onScroll(float2{ 0.0f, (myFrame.bottom - 2.0f*SliderSize) - cursorFrame.bottom });
			}

			if (cursorFrame.top < myFrame.top + BorderWeight)
			{
				VerticalBar->onScroll(float2{ 0.0f, (myFrame.top + BorderWeight) - cursorFrame.top });
			}
		}
		else
		{
			if (cursorFrame.right > myFrame.right - 2.0f*BorderWeight)
			{
				onScroll(float2{ (myFrame.right - 2.0f*BorderWeight) - cursorFrame.right, 0.0f });
			}

			if (cursorFrame.left < myFrame.left + BorderWeight)
			{
				onScroll(float2{ (myFrame.left + BorderWeight) - cursorFrame.left, 0.0f });
			}
		}
	}

	void TextView::insert(const string& str)
	{
		slong nStartIndex = min(m_cursor->startIndex(), m_cursor->charIndex());
		slong nEndIndex = max(m_cursor->startIndex(), m_cursor->charIndex());
		slong nReplacedLength = nEndIndex - nStartIndex;

		string strBuffer = Text;
		strBuffer.replace(nStartIndex,
			nReplacedLength,
			str);

		setText(strBuffer);
		m_textSize = getFont()->calcTextSize(Composition->getText());
		m_cursor->updateComposition();
		m_cursor->beginDrag(nEndIndex + str.length() - nReplacedLength);
		m_cursor->endDrag(nEndIndex + str.length() - nReplacedLength);

		followCursor();
	}

	void TextView::setFont(strong<Font> font)
	{
		Composition->setFont(font);
	}

	strong<Font> TextView::getFont()
	{
		return Composition->getFont();
	}

	string TextView::getText()
	{
		return Text;
	}

	void TextView::setText(const string& strText)
	{
		Text = strText;
		Composition->setText(getStyle() == TEXT_VIEW_STYLE::PASSWORD ? string::repeat(L'*', Text.length()) : Text);
		m_textSize = getFont()->calcTextSize(Composition->getText());
		m_cursor->updateComposition();
		m_cursor->beginDrag(0);
		m_cursor->endDrag(0);

		resetFrame();
	}

	/*void TextView::setComposition(strong<Label> composition)
	{
	Composition->removeFromSuperView();
	Composition = composition;
	Composition->disable();
	getContent()->addSubView(Composition, false);

	m_textSize = getFont()->calcTextSize(Composition->getText());

	m_cursor->setComposition(Composition);
	m_cursor->beginDrag(0);
	m_cursor->endDrag(0);

	resetFrame();
	}*/

	strong<Label> TextView::getComposition()
	{
		return Composition;
	}

	LinearColor TextView::getTextColor()
	{
		return Composition->getTextColor();
	}

	void TextView::setTextColor(const LinearColor& nColor)
	{
		Composition->setTextColor(nColor);
	}

	TEXT_ALIGNMENT TextView::getTextAlignment()
	{
		return Composition->getTextAlignment();
	}

	void TextView::setTextAlignment(TEXT_ALIGNMENT textAlignment)
	{
		Composition->setTextAlignment(textAlignment);
	}

	void TextView::setBackgroundImage(strong<Rectangle> image)
	{
		Background->setImage(image);
	}

	void TextView::setVerticalBar(strong<ScrollBar> scrollBar)
	{
		if (scrollBar == nullptr)
		{
			scrollBar = ResourceManager::create<ScrollBar>();
		}
		replaceSubView(VerticalBar, scrollBar);

		VerticalBar = scrollBar;
		VerticalBar->setVertical(true);
		VerticalBar->setScrollAction([this](View* sender, float2 newOffset, float2 mouseScroll)
		{
			float2 offset = getContentOffset();
			offset.y = newOffset.y;
			setContentOffset(offset);
			VerticalBar->setContentOffset(offset);
		});

		VerticalBar->setTag(L"VerticalBar");

		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			VerticalBar->show();
			VerticalBar->enable();
		}
		else
		{
			VerticalBar->hide();
			VerticalBar->disable();
		}
	}

	void TextView::setHorizontalBar(strong<ScrollBar> scrollBar)
	{
		if (scrollBar == nullptr)
		{
			scrollBar = ResourceManager::create<ScrollBar>();
		}
		replaceSubView(HorizontalBar, scrollBar);

		HorizontalBar = scrollBar;
		HorizontalBar->setVertical(false);
		HorizontalBar->setScrollAction([this](View* sender, float2 newOffset, float2 mouseScroll)
		{
			float2 offset = getContentOffset();
			offset.x = newOffset.x;
			setContentOffset(offset);
			HorizontalBar->setContentOffset(offset);
		});

		HorizontalBar->setTag(L"HorizontalBar");

		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			HorizontalBar->show();
			HorizontalBar->enable();
		}
		else
		{
			HorizontalBar->hide();
			HorizontalBar->disable();
		}
	}

	void TextView::onResetFrame()
	{
		Super::onResetFrame();
		SRect rect = getFrame();

		Composition = cast<Label>(getSubView(L"Composition"));
		m_textSize = getFont()->calcTextSize(Composition->getText());
		m_cursor = cast<Cursor>(getSubView(L"Cursor"));
		m_cursor->setComposition(Composition);
		CandList = cast<Label>(getSubView(L"CandList"));

		Background = cast<ImageView>(getSubView(L"Background"));
		setHorizontalBar(cast<ScrollBar>(getSubView(L"HorizontalBar")));
		setVerticalBar(cast<ScrollBar>(getSubView(L"VerticalBar")));

		//计算字符最小包装矩形
		const float2 textSize = m_textSize;
		float fTrailingScape;

		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			fTrailingScape = 2.0f * SliderSize;
		}
		else
		{
			fTrailingScape = 2.0f * BorderWeight;
		}

		float fWidthInSelf = (textSize.x + fTrailingScape) / rect.width();
		float fHeightInSelf = (textSize.y + fTrailingScape) / rect.height();

		if (fWidthInSelf < 1.0f)
		{
			fWidthInSelf = 1.0f;
		}

		if (fHeightInSelf < 1.0f)
		{
			fHeightInSelf = 1.0f;
		}

		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			HorizontalBar->setContentSize(float2{ fWidthInSelf, 0.0f });
			VerticalBar->setContentSize(float2{ 0.0f, fHeightInSelf });
			modifyContentSize(float2{ fWidthInSelf, fHeightInSelf });
		}
		else
		{
			modifyContentSize(float2{ fWidthInSelf, 1.0f });
		}

		//////////////////////////////////////

		SRect candListRect = SRect(rect.left, rect.top - CandList->getFont()->getFontSize() * 2, rect.width(), CandList->getFont()->getFontSize() * 2.0f);
		CandList->setFrame(candListRect);

		if (getStyle() == TEXT_VIEW_STYLE::MULTILINE)
		{
			VerticalBar->setFrame(SRect(rect.right - getSliderSize() - getBorderWeight(), rect.top + getBorderWeight(), getSliderSize(), rect.height() - getSliderSize() - getBorderWeight()));
			HorizontalBar->setFrame(SRect(rect.left + getBorderWeight(), rect.bottom - getSliderSize() - getBorderWeight(), rect.width() - getSliderSize() - getBorderWeight(), getSliderSize()));

			rect.left += getBorderWeight();
			rect.top += getBorderWeight();
			rect.right -= BorderWeight + SliderSize;
			rect.bottom -= BorderWeight + SliderSize;

			auto dirtRect = getFrame();
			setDirtyRect(&dirtRect);
			getContent()->setDirtyRect(&rect);

			SRect contentFrameScale = getContent()->getFrameScale();
			float2 borderSize = localSizeOfPixels(float2{ getBorderWeight(), getBorderWeight() });

			getContent()->setFrameScale(SRect(borderSize.x + getContentOffset().x,
				borderSize.y + getContentOffset().y,
				contentFrameScale.width(),
				contentFrameScale.height()));
		}
		else
		{
			float fTopPadding = ((rect.height() - textSize.y) / 2.0f) / rect.height();

			rect.left += getBorderWeight();
			rect.top += getBorderWeight();
			rect.right -= getBorderWeight();
			rect.bottom -= getBorderWeight();

			auto dirtRect = getFrame();
			setDirtyRect(&dirtRect);
			getContent()->setDirtyRect(&rect);

			SRect contentFrameScale = getContent()->getFrameScale();
			float2 borderSize = localSizeOfPixels(float2{ getBorderWeight(), getBorderWeight() });

			getContent()->setFrameScale(SRect(borderSize.x + getContentOffset().x,
				fTopPadding + getContentOffset().y,
				contentFrameScale.width(),
				contentFrameScale.height()));
		}
	}

	strong<Cursor> TextView::getCursor()
	{
		return m_cursor;
	}

	void TextView::onDraw()
	{
		View::onDraw();
	}

	TextView::TEXT_VIEW_STYLE TextView::getStyle()
	{
		return Style;
	}

	void TextView::setStyle(Enum<TEXT_VIEW_STYLE> style)
	{
		Style = style;

		if (VerticalBar != nullptr)
		{
			if (Style == TEXT_VIEW_STYLE::MULTILINE)
			{
				VerticalBar->show();
				VerticalBar->enable();
			}
			else
			{
				VerticalBar->hide();
				VerticalBar->disable();
			}
		}		

		if (HorizontalBar != nullptr)
		{
			if (Style == TEXT_VIEW_STYLE::MULTILINE)
			{
				HorizontalBar->show();
				HorizontalBar->enable();
			}
			else
			{
				HorizontalBar->hide();
				HorizontalBar->disable();
			}
		}

		resetFrame();
	}

	float TextView::getBorderWeight()
	{
		return BorderWeight;
	}

	void TextView::setBorderWeight(float fBorderWeight)
	{
		BorderWeight = fBorderWeight;
	}

	float TextView::getSliderSize()
	{
		return SliderSize;
	}

	void TextView::setSliderSize(float fSliderSize)
	{
		SliderSize = fSliderSize;
	}

}