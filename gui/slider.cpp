/*
	Copyright 2017 TeamWin
	This file is part of TWRP/TeamWin Recovery Project.

	TWRP is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	TWRP is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with TWRP.  If not, see <http://www.gnu.org/licenses/>.
*/

// slider.cpp - GUISlider object
// Pulled & ported from https://raw.github.com/agrabren/RecoverWin/master/gui/slider.cpp

#include <stdbool.h>
#include <stddef.h>

#include "../data.hpp"
#include "../minuitwrp/minui.h"
#include "../twcommon.h"
#include "objects.hpp"
#include "pages.hpp"
#include "placement.h"
#include "rapidxml.hpp"
#include "resources.hpp"

GUISlider::GUISlider(xml_node<>* node) : GUIObject(node)
{
	xml_attribute<>* attr;
	xml_node<>* child;

	sAction = NULL;
	sSliderLabel = NULL;
	sSlider = NULL;
	sSliderUsed = NULL;
	sTouch = NULL;
	sTouchW = 20;

	if (!node)
	{
		LOGERR("GUISlider created without XML node\n");
		return;
	}

	// Load the resources
	child = FindNode(node, "resource");
	if (child)
	{
		sSlider = LoadAttrImage(child, "base");
		sSliderUsed = LoadAttrImage(child, "used");
		sTouch = LoadAttrImage(child, "touch");
	}

	// Load the text label
	sSliderLabel = new GUIText(node);
	if (sSliderLabel->Render() < 0)
	{
		delete sSliderLabel;
		sSliderLabel = NULL;
	}

	// Load the placement
	Placement TextPlacement = CENTER;
	LoadPlacement(FindNode(node, "placement"), &mRenderX, &mRenderY, &mRenderW, &mRenderH, &TextPlacement);

	mRenderW = sSlider->GetWidth();
	mRenderH = sSlider->GetHeight();
	if (TextPlacement == CENTER || TextPlacement == CENTER_X_ONLY) {
		mRenderX = mRenderX - (mRenderW / 2);
		if (TextPlacement == CENTER) {
			mRenderY = mRenderY - (mRenderH / 2);
		}
	}
	if (sSliderLabel) {
		int sTextX = mRenderX + (mRenderW / 2);
		int w, h;
		sSliderLabel->GetCurrentBounds(w, h);
		int sTextY = mRenderY + ((mRenderH - h) / 2);
		sSliderLabel->SetRenderPos(sTextX, sTextY);
		sSliderLabel->SetMaxWidth(mRenderW);
	}
	if (sTouch && sTouch->GetResource())
	{
		sTouchW = sTouch->GetWidth();  // Width of the "touch image" that follows the touch (arrow)
		sTouchH = sTouch->GetHeight(); // Height of the "touch image" that follows the touch (arrow)
	}

	//LOGINFO("mRenderW: %i mTouchW: %i\n", mRenderW, mTouchW);
	mActionX = mRenderX;
	mActionY = mRenderY;
	mActionW = mRenderW;
	mActionH = mRenderH;

	sAction = new GUIAction(node);

	sCurTouchX = mRenderX;
	sUpdate = 1;
}

GUISlider::~GUISlider()
{
	delete sAction;
	delete sSliderLabel;
}

int GUISlider::Render(void)
{
	if (!isConditionTrue())
		return 0;

	if (!sSlider || !sSlider->GetResource())
		return -1;

	// Draw the slider
	gr_blit(sSlider->GetResource(), 0, 0, mRenderW, mRenderH, mRenderX, mRenderY);

	// Draw the used
	if (sSliderUsed && sSliderUsed->GetResource() && sCurTouchX > mRenderX)
		gr_blit(sSliderUsed->GetResource(), 0, 0, sCurTouchX - mRenderX, mRenderH, mRenderX, mRenderY);

	// Draw the touch icon
	if (sTouch && sTouch->GetResource())
		gr_blit(sTouch->GetResource(), 0, 0, sTouchW, sTouchH, sCurTouchX, (mRenderY + ((mRenderH - sTouchH) / 2)));

	if (sSliderLabel) {
		int ret = sSliderLabel->Render();
		if (ret < 0)		return ret;
	}

	sUpdate = 0;
	return 0;
}

int GUISlider::Update(void)
{
	if (!isConditionTrue())
		return 0;

	if (sUpdate)
		return 2;
	return 0;
}

int GUISlider::NotifyTouch(TOUCH_STATE state, int x, int y)
{
	if (!isConditionTrue())
		return -1;

	static bool dragging = false;

	switch (state)
	{
	case TOUCH_START:
		if (x >= mRenderX && x <= mRenderX + sTouchW &&
			y >= mRenderY && y <= mRenderY + mRenderH)
		{
			sCurTouchX = x - (sTouchW / 2);
			if (sCurTouchX < mRenderX)
				sCurTouchX = mRenderX;
			dragging = true;
		}
		break;

	case TOUCH_DRAG:
		if (!dragging)
			return 0;
		if (y < mRenderY - sTouchH || y > mRenderY + (sTouchH * 2))
		{
			sCurTouchX = mRenderX;
			dragging = false;
			sUpdate = 1;
			break;
		}
		sCurTouchX = x - (sTouchW / 2);
		if (sCurTouchX < mRenderX)
			sCurTouchX = mRenderX;
		if (sCurTouchX > mRenderX + mRenderW - sTouchW)
			sCurTouchX = mRenderX + mRenderW - sTouchW;
		sUpdate = 1;
		break;

	case TOUCH_RELEASE:
		if (!dragging)
			return 0;

		if (sCurTouchX >= mRenderX + mRenderW - sTouchW) {
			DataManager::Vibrate("tw_button_vibrate");
			sAction->doActions();
		}

		sCurTouchX = mRenderX;
		dragging = false;
		sUpdate = 1;
	case TOUCH_REPEAT:
	case TOUCH_HOLD:
		break;
	}
	return 0;
}
