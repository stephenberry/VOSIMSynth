﻿#include "UIComponent.h"

//#define DRAW_COMPONENT_BOUNDS

namespace syn
{
	UIComponent::UIComponent(VOSIMWindow* a_window):
		m_parent(nullptr), m_window(a_window), m_visible(true), m_focused(false), m_hovered(false),
		m_pos(0, 0), m_size(0, 0), m_minSize(-1, -1), m_maxSize(-1, -1) 
	{
		setVisible(true);
	}

	void UIComponent::recursiveDraw(NVGcontext* a_nvg) {
#ifdef DRAW_COMPONENT_BOUNDS
		if (m_hovered) {
			nvgStrokeWidth(a_nvg, 1.0f);
			nvgBeginPath(a_nvg);
			nvgRect(a_nvg, m_pos[0] - 2.0f, m_pos[1] - 2.0f, size()[0] + 2.0f, size()[1] + 2.0f);
			nvgStrokeColor(a_nvg, nvgRGBA(255, 255, 0, 225));
			nvgStroke(a_nvg);

			nvgStrokeWidth(a_nvg, 1.0f);
			nvgBeginPath(a_nvg);
			nvgRect(a_nvg, m_pos[0] - 0.5f, m_pos[1] - 0.5f, minSize()[0] + 0.5f, minSize()[1] + 0.5f);
			nvgStrokeColor(a_nvg, nvgRGBA(255, 0, 0, 200));
			nvgStroke(a_nvg);
		}
#endif


		nvgTranslate(a_nvg, m_pos[0], m_pos[1]);
		nvgSave(a_nvg);
		draw(a_nvg);
		nvgRestore(a_nvg);

		nvgSave(a_nvg);
		setChildrenStyles(a_nvg);
		for (auto zplane_iter = m_ZPlaneMap.rbegin(); zplane_iter != m_ZPlaneMap.rend(); ++zplane_iter) {
			const list<UIComponent*>& zplane = zplane_iter->second;
			for (auto zorder_iter = zplane.rbegin(); zorder_iter != zplane.rend(); ++zorder_iter) {
				UIComponent* child = *zorder_iter;
				if (child->visible())
					child->recursiveDraw(a_nvg);
			}
		}
		nvgRestore(a_nvg);

		nvgTranslate(a_nvg, -m_pos[0], -m_pos[1]);
	}

	void UIComponent::addChild(UIComponent* a_newChild, const string& a_group) {
		if (a_newChild->parent())
			return;
		shared_ptr<UIComponent> newChildPtr(a_newChild);
		addChild(newChildPtr, a_group);
	}

	void UIComponent::addChild(shared_ptr<UIComponent> a_newChild, const string& a_group) {
		if (a_newChild->parent())
			return;
		if (find(m_children.begin(), m_children.end(), a_newChild) == m_children.end()) {
			a_newChild->m_parent = this;
			m_children.push_back(a_newChild);
			// register with group maps
			m_groupMap[a_group].push_back(a_newChild.get());
			m_reverseGroupMap[a_newChild.get()] = a_group;

			// Push to front of z-order stack
			m_ZPlaneMap[0].push_front(a_newChild.get());
			m_reverseZPlaneMap[a_newChild.get()] = 0;

			// Execute callback
			_onAddChild(a_newChild);
		}
	}

	const string& UIComponent::getChildGroup(UIComponent* a_child) const {
		return m_reverseGroupMap.at(a_child);
	}

	const vector<UIComponent*>& UIComponent::getGroup(const string& a_group) const {
		return m_groupMap.at(a_group);
	}

	bool UIComponent::removeChild(UIComponent* a_child) {
		for (int i = 0; i < m_children.size(); i++) {
			if (m_children[i].get() == a_child) {
				return removeChild(i);
			}
		}
		return false;
	}

	bool UIComponent::removeChild(int a_index) {
		if (a_index < 0 || a_index >= m_children.size())
			return false;
		if (m_children[a_index]->focused())
			m_window->forfeitFocus(m_children[a_index].get());

		shared_ptr<UIComponent> child = m_children[a_index];

		// Remove from group maps
		const string& group = m_reverseGroupMap[child.get()];
		m_groupMap[group].erase(find(m_groupMap[group].begin(), m_groupMap[group].end(), child.get()));
		m_reverseGroupMap.erase(child.get());

		// Remove from z-order stack
		int zorder = m_reverseZPlaneMap[child.get()];
		m_reverseZPlaneMap.erase(child.get());
		m_ZPlaneMap[zorder].remove(child.get());

		m_children.erase(m_children.cbegin() + a_index);

		_onRemoveChild();
		return true;
	}

	shared_ptr<UIComponent> UIComponent::getChild(int i) const {
		return m_children[i];
	}

	shared_ptr<UIComponent> UIComponent::getChild(const UIComponent* a_comp) const {
		for (int i = 0; i < m_children.size(); i++) {
			if (m_children[i].get() == a_comp)
				return m_children[i];
		}
		return nullptr;
	}

	int UIComponent::getChildIndex(const UIComponent* a_comp) const {
		for (int i = 0; i < m_children.size(); i++) {
			if (m_children[i].get() == a_comp)
				return i;
		}
		return -1;
	}

	int UIComponent::numChildren() const {
		return m_children.size();
	}

	const vector<shared_ptr<UIComponent>>& UIComponent::children() const {
		return m_children;
	}

	UIComponent* UIComponent::parent() const {
		return m_parent;
	}

	shared_ptr<UIComponent> UIComponent::getSharedPtr() const
	{
		if(parent())
			return parent()->getChild(this);
		return nullptr;
	}

	Vector2i UIComponent::getRelPos() const {
		return m_pos;
	}

	Vector2i UIComponent::getRelCenter() const {
		return getRelPos() + size() / 2.0;
	}

	void UIComponent::setRelPos(const Vector2i& a_pos) {
		m_pos = a_pos;
	}

	Vector2i UIComponent::getAbsPos() const {
		return m_parent ? m_parent->getAbsPos() + getRelPos() : getRelPos();
	}

	Vector2i UIComponent::getAbsCenter() const {
		return getAbsPos() + size() / 2.0;
	}

	void UIComponent::move(const Vector2i& a_dist) {
		m_pos += a_dist;
	}

	Vector2i UIComponent::size() const {
		return visible() ? m_size : Vector2i{0,0};
	}

	Vector2i UIComponent::minSize() const {
		return m_minSize;
	}

	Vector2i UIComponent::maxSize() const {
		return m_maxSize;
	}

	void UIComponent::grow(const Vector2i& a_amt) {
		setSize(size() + a_amt);
	}

	NVGcontext* UIComponent::nvgContext() const {
		return m_window->getContext();
	}

	sf::RenderWindow* UIComponent::sfmlWindow() const {
		return m_window->GetWindow();
	}

	shared_ptr<Theme> UIComponent::theme() const {
		return m_window->theme();
	}

	bool UIComponent::visible() const {
		return m_visible;
	}

	bool UIComponent::focused() const {
		return m_focused;
	}

	bool UIComponent::hovered() const {
		return m_hovered;
	}

	bool UIComponent::onTextEntered(sf::Uint32 a_unicode) {
		return false;
	}

	bool UIComponent::onKeyDown(const sf::Event::KeyEvent& a_key) {
		return false;
	}

	bool UIComponent::onKeyUp(const sf::Event::KeyEvent& a_key) {
		return false;
	}

	void UIComponent::setVisible(bool a_visible) {
		if (a_visible == m_visible)
			return;
		if (!a_visible) {
			if (focused())
				m_window->clearFocus();
		}
		m_visible = a_visible;
	}

	bool UIComponent::contains(const Vector2i& a_parentPt) const {
		Eigen::Array<int,2,1> pt = (a_parentPt - getRelPos()).array();
		return (pt >= 0).all() && (pt <= size().array()).all();
	}

	UIComponent* UIComponent::findChild(const Vector2i& a_parentPt, const string& a_filterGroup) const {
		Vector2i relPt = a_parentPt - getRelPos();
		for (auto const& kv : m_ZPlaneMap) {
			for (UIComponent* child : kv.second) {
				if (a_filterGroup.empty() || getChildGroup(child) == a_filterGroup) {
					if (child->visible() && child->contains(relPt))
						return child;
				}
			}
		}
		return nullptr;
	}

	UIComponent* UIComponent::findChildRecursive(const Vector2i& a_parentPt) {
		Vector2i relPt = a_parentPt - getRelPos();
		for (auto const& kv : m_ZPlaneMap) {
			for (UIComponent* child : kv.second) {
				if (child->visible() && child->contains(relPt))
					return child->findChildRecursive(relPt);
			}
		}
		return nullptr;
	}

	void UIComponent::setSize(const Vector2i& a_size) {
		Vector2i newSize = a_size;
		if (newSize[0] < 0)
			newSize[0] = m_size[0];
		if (newSize[1] < 0)
			newSize[1] = m_size[1];

		if (m_minSize[0] >= 0)
			newSize[0] = MAX(m_minSize[0], newSize[0]);
		if (m_minSize[1] >= 0)
			newSize[1] = MAX(m_minSize[1], newSize[1]);
		if (m_maxSize[0] >= 0)
			newSize[0] = MIN(m_maxSize[0], newSize[0]);
		if (m_maxSize[1] >= 0)
			newSize[1] = MIN(m_maxSize[1], newSize[1]);
		if (m_size != newSize) {
			m_size = newSize;
			_onResize();
			if (parent())
				parent()->notifyChildResized(this);
		}
	}

	void UIComponent::setMinSize(const Vector2i& a_minSize) {
		m_minSize = a_minSize;
		if (m_maxSize[0] >= 0)
			m_maxSize[0] = MAX(m_maxSize[0], a_minSize[0]);
		if (m_maxSize[1] >= 0)
			m_maxSize[1] = MAX(m_maxSize[1], a_minSize[1]);
		setSize(m_size);
	}

	void UIComponent::setMaxSize(const Vector2i& a_maxSize) {
		m_maxSize = a_maxSize;
		if (m_minSize[0] >= 0)
			m_minSize[0] = MIN(m_minSize[0], a_maxSize[0]);
		if (m_minSize[1] >= 0)
			m_minSize[1] = MIN(m_minSize[1], a_maxSize[1]);
		setSize(m_size);
	}

	bool UIComponent::onMouseDrag(const Vector2i& a_relCursor, const Vector2i& a_diffCursor) {
		return false;
	}

	bool UIComponent::onMouseMove(const Vector2i& a_relCursor, const Vector2i& a_diffCursor) {
		for (auto const& kv : m_ZPlaneMap) {
			for (UIComponent* child : kv.second) {
				if (!child->visible())
					continue;
				bool hasMouse = child->contains(a_relCursor);
				bool hadMouse = child->contains(a_relCursor - a_diffCursor);
				if (hasMouse != hadMouse)
					child->onMouseEnter(a_relCursor - child->getRelPos(), a_diffCursor, hasMouse);
				if (hasMouse || hadMouse)
					child->onMouseMove(a_relCursor - child->getRelPos(), a_diffCursor);
			}
		}
		return false;
	}

	void UIComponent::onMouseEnter(const Vector2i& a_relCursor, const Vector2i& a_diffCursor, bool a_isEntering) {
		m_hovered = a_isEntering;
	}

	UIComponent* UIComponent::onMouseDown(const Vector2i& a_relCursor, const Vector2i& a_diffCursor, bool a_isDblClick) {
		for (auto const& kv : m_ZPlaneMap) {
			for (UIComponent* child : kv.second) {
				if (!child->visible())
					continue;
				if (child->contains(a_relCursor)) {
					UIComponent* ret = child->onMouseDown(a_relCursor - child->getRelPos(), a_diffCursor, a_isDblClick);
					if (ret)
						return ret;
				}
			}
		}
		return nullptr;
	}

	bool UIComponent::onMouseUp(const Vector2i& a_relCursor, const Vector2i& a_diffCursor) {
		for (auto const& kv : m_ZPlaneMap) {
			for (UIComponent* child : kv.second) {
				if (!child->visible())
					continue;
				if (child->contains(a_relCursor) && child->onMouseUp(a_relCursor - child->getRelPos(), a_diffCursor))
					return true;
			}
		}
		return false;
	}

	bool UIComponent::onMouseScroll(const Vector2i& a_relCursor, const Vector2i& a_diffCursor, int a_scrollAmt) {
		for (auto const& kv : m_ZPlaneMap) {
			for (UIComponent* child : kv.second) {
				if (!child->visible())
					continue;
				if (child->contains(a_relCursor) && child->onMouseScroll(a_relCursor - child->getRelPos(), a_diffCursor, a_scrollAmt))
					return true;
			}
		}
		return false;
	}

	void UIComponent::onFocusEvent(bool a_isFocused) {
		m_focused = a_isFocused;
		if (parent())
			parent()->bringToFront(this);
	}

	void UIComponent::bringToFront(UIComponent* a_child) {
		int frontZOrder = 0;
		setZOrder(a_child, frontZOrder);
	}

	void UIComponent::pushToBack(UIComponent* a_child) {
		int backZOrder = m_ZPlaneMap.rbegin()->first;
		setZOrder(a_child, backZOrder, false);
	}

	int UIComponent::getZOrder(UIComponent* a_child) {
		return m_reverseZPlaneMap[a_child];
	}

	void UIComponent::setZOrder(UIComponent* a_child, int a_newZOrder, bool toFront) {
		shared_ptr<UIComponent> child = getChild(a_child);
		if (child) {
			int oldZOrder = getZOrder(child.get());
			m_ZPlaneMap[oldZOrder].remove(child.get());
			if (toFront)
				m_ZPlaneMap[a_newZOrder].push_front(child.get());
			else
				m_ZPlaneMap[a_newZOrder].push_back(child.get());
			m_reverseZPlaneMap[child.get()] = a_newZOrder;
		}
	}
}
