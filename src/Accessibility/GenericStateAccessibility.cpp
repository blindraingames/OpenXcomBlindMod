#include "GenericStateAccessibility.h"
#include "AccessibilityText.h"
#include "../Engine/State.h"
#include "../Engine/Surface.h"
#include "../Engine/InteractiveSurface.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/TextEdit.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Slider.h"
#include "../Interface/ArrowButton.h"
#include "../Battlescape/ActionMenuItem.h"
#include "../Battlescape/Inventory.h"
#include "../Battlescape/MedikitState.h"
#include "../Battlescape/MedikitView.h"
#include "../Battlescape/PrimeGrenadeState.h"
#include <cmath>
#include <limits>
#include <sstream>

namespace OpenXcom
{

std::string GenericStateAccessibility::findLabel(State *state, Surface *control) const
{
	std::string result;
	double best = std::numeric_limits<double>::max();
	const std::vector<Surface*> &surfaces = state->getSurfaces();
	for (std::vector<Surface*>::const_iterator i = surfaces.begin(); i != surfaces.end(); ++i)
	{
		Text *text = dynamic_cast<Text*>(*i);
		if (!text || !text->getVisible() || text->getHidden()) continue;
		const std::string candidate = AccessibilityText::clean(text->getText());
		if (candidate.empty()) continue;
		const double dx = static_cast<double>((text->getX() + text->getWidth() / 2) - (control->getX() + control->getWidth() / 2));
		const double dy = static_cast<double>((text->getY() + text->getHeight() / 2) - (control->getY() + control->getHeight() / 2));
		double score = std::sqrt(dx * dx + dy * dy);
		if (text->getY() > control->getY() + control->getHeight()) score += 1000.0;
		if (score < best)
		{
			best = score;
			result = candidate;
		}
	}
	return result;
}

std::string GenericStateAccessibility::describeListRow(State *state, TextList *list) const
{
	if (!list || list->getTexts() == 0) return std::string();
	unsigned int row = list->getSelectedRow();
	if (row == static_cast<unsigned int>(-1) || row >= list->getTexts()) row = 0;
	std::string result;
	const size_t columns = list->getColumnCount(row);
	for (size_t col = 0; col < columns; ++col)
	{
		const std::string cell = AccessibilityText::clean(list->getCellText(row, col));
		if (cell.empty()) continue;
		if (!result.empty()) result += ", ";
		result += cell;
	}
	std::ostringstream position;
	position << state->tr("STR_ACCESSIBILITY_POSITION_OF").arg(row + 1).arg(list->getTexts());
	if (!result.empty()) result += ", ";
	result += position.str();
	return result;
}

std::string GenericStateAccessibility::getTitle(State *state) const
{
	if (!state) return std::string();
	if (!state->getAccessibilityTitle().empty()) return AccessibilityText::clean(state->getAccessibilityTitle());
	const std::vector<Surface*> &surfaces = state->getSurfaces();
	for (std::vector<Surface*>::const_iterator i = surfaces.begin(); i != surfaces.end(); ++i)
	{
		Text *text = dynamic_cast<Text*>(*i);
		if (text && text->getVisible() && !text->getHidden())
		{
			const std::string title = AccessibilityText::clean(text->getText());
			if (!title.empty()) return title;
		}
	}
	return std::string();
}

std::string GenericStateAccessibility::getSummary(State *state) const
{
	const std::string title = getTitle(state);
	const size_t count = getNodes(state).size();
	std::ostringstream result;
	if (!title.empty()) result << title << ". ";
	result << state->tr("STR_ACCESSIBILITY_CONTROL_COUNT").arg(count);
	return result.str();
}

std::vector<AccessibleNode> GenericStateAccessibility::getNodes(State *state) const
{
	std::vector<AccessibleNode> result;
	if (!state) return result;
	const std::vector<Surface*> &surfaces = state->getSurfaces();
	for (std::vector<Surface*>::const_iterator i = surfaces.begin(); i != surfaces.end(); ++i)
	{
		Surface *surface = *i;
		if (!surface || !surface->getVisible() || surface->getHidden()) continue;
		InteractiveSurface *interactive = dynamic_cast<InteractiveSurface*>(surface);
		if (!interactive) continue;

		AccessibleNode node;
		node.control = interactive;
		if (dynamic_cast<Inventory*>(surface))
		{
			continue;
		}
		else if (MedikitView *medikit = dynamic_cast<MedikitView*>(surface))
		{
			node.role = ACCESSIBLE_LIST;
			node.name = state->tr("STR_ACCESSIBILITY_BODY_PART");
			node.value = medikit->getAccessibilityDescription();
		}
		else if (ActionMenuItem *menuItem = dynamic_cast<ActionMenuItem*>(surface))
		{
			node.role = ACCESSIBLE_BUTTON;
			node.name = menuItem->getDescription();
			node.value = menuItem->getAccuracyText();
			if (!menuItem->getTUText().empty())
			{
				if (!node.value.empty()) node.value += ", ";
				node.value += menuItem->getTUText();
			}
		}
		else if (ToggleTextButton *toggle = dynamic_cast<ToggleTextButton*>(surface))
		{
			node.role = ACCESSIBLE_TOGGLE_BUTTON;
			node.name = toggle->getText();
			node.selected = toggle->getPressed();
			node.state = state->tr(node.selected ? "STR_ACCESSIBILITY_ON" : "STR_ACCESSIBILITY_OFF");
		}
		else if (TextButton *button = dynamic_cast<TextButton*>(surface))
		{
			node.role = ACCESSIBLE_BUTTON;
			node.name = button->getText();
			node.selected = button->isPressed();
			if (node.selected) node.state = state->tr("STR_ACCESSIBILITY_SELECTED");
		}
		else if (ComboBox *combo = dynamic_cast<ComboBox*>(surface))
		{
			node.role = ACCESSIBLE_COMBO_BOX;
			node.name = findLabel(state, surface);
			node.value = combo->getText();
		}
		else if (TextEdit *edit = dynamic_cast<TextEdit*>(surface))
		{
			node.role = ACCESSIBLE_EDIT;
			node.name = findLabel(state, surface);
			node.value = edit->getText();
		}
		else if (Slider *slider = dynamic_cast<Slider*>(surface))
		{
			node.role = ACCESSIBLE_SLIDER;
			node.name = findLabel(state, surface);
			std::ostringstream value;
			value << slider->getValue();
			node.value = value.str();
		}
		else if (TextList *list = dynamic_cast<TextList*>(surface))
		{
			node.role = ACCESSIBLE_LIST;
			node.name = findLabel(state, surface);
			if (node.name.empty()) node.name = state->tr("STR_ACCESSIBILITY_ITEMS_LIST");
			node.value = describeListRow(state, list);
		}
		else if (ArrowButton *arrow = dynamic_cast<ArrowButton*>(surface))
		{
			node.role = ACCESSIBLE_SPIN_BUTTON;
			node.name = findLabel(state, surface);
			switch (arrow->getShape())
			{
			case ARROW_BIG_UP:
			case ARROW_SMALL_UP:
			case ARROW_SMALL_RIGHT: node.value = state->tr("STR_ACCESSIBILITY_INCREASE"); break;
			case ARROW_BIG_DOWN:
			case ARROW_SMALL_DOWN:
			case ARROW_SMALL_LEFT: node.value = state->tr("STR_ACCESSIBILITY_DECREASE"); break;
			default: node.value = state->tr("STR_ACCESSIBILITY_CHANGE"); break;
			}
		}
		else
		{
			node.role = ACCESSIBLE_BUTTON;
			const bool tacticalPopup = dynamic_cast<PrimeGrenadeState*>(state) || dynamic_cast<MedikitState*>(state);
			if (surface->getTooltip().empty() && !tacticalPopup) continue;
			node.name = surface->getTooltip().empty() ? findLabel(state, surface) : state->tr(surface->getTooltip());
			if (node.name.empty()) continue;
		}
		node.name = AccessibilityText::clean(node.name);
		node.value = AccessibilityText::clean(node.value);
		if (node.name.empty() && node.value.empty()) continue;
		switch (node.role)
		{
		case ACCESSIBLE_BUTTON: node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_BUTTON"); break;
		case ACCESSIBLE_TOGGLE_BUTTON: node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_TOGGLE_BUTTON"); break;
		case ACCESSIBLE_LIST: node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_LIST"); break;
		case ACCESSIBLE_EDIT: node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_EDIT"); break;
		case ACCESSIBLE_COMBO_BOX: node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_COMBO_BOX"); break;
		case ACCESSIBLE_SLIDER: node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_SLIDER"); break;
		case ACCESSIBLE_SPIN_BUTTON: node.roleName = state->tr("STR_ACCESSIBILITY_ROLE_SPIN_BUTTON"); break;
		default: break;
		}
		result.push_back(node);
	}
	return result;
}

}
