#include "UIScrollPanel.h"

synui::UIScrollPanel::UIScrollPanel(MainWindow* a_window): UIComponent(a_window), m_minExtent{0,0}, m_maxExtent{0,0}, m_scrollOffset{0,0} {}

void synui::UIScrollPanel::scroll(const Vector2i& a_amt) {
	setScrollPos(m_scrollOffset + a_amt);
}

void synui::UIScrollPanel::setScrollPos(const Vector2i& a_pos) {
	m_scrollOffset = a_pos;
	m_scrollOffset = m_scrollOffset.cwiseMax(m_minExtent);
	m_scrollOffset = m_scrollOffset.cwiseMin(m_maxExtent);
	for (shared_ptr<UIComponent> child : m_children) {
		updateChildPosition_(child);
	}
}

Eigen::Vector2i synui::UIScrollPanel::getScrollOffset() const {
	return m_scrollOffset;
}

bool synui::UIScrollPanel::onMouseScroll(const UICoord& a_relCursor, const Vector2i& a_diffCursor, int a_scrollAmt) {
	if (UIComponent::onMouseScroll(a_relCursor, a_diffCursor, a_scrollAmt))
		return true;
	scroll({0,-10 * a_scrollAmt});
	return true;
}

void synui::UIScrollPanel::setChildrenStyles(NVGcontext* a_nvg) {
	nvgIntersectScissor(a_nvg, 0, 0, size().x(), size().y());
}

void synui::UIScrollPanel::updateExtents_() {
	m_maxExtent = { 0,0 };
	m_minExtent = { 0,0 };
	for (shared_ptr<UIComponent> child : m_children) {
		m_minExtent = m_minExtent.cwiseMin(m_originalPositions[child.get()]);
		m_maxExtent = m_maxExtent.cwiseMax(m_originalPositions[child.get()] + child->size() - size());
	}
	m_scrollOffset = m_scrollOffset.cwiseMax(m_minExtent);
	m_scrollOffset = m_scrollOffset.cwiseMin(m_maxExtent);
	setMinSize({ m_maxExtent.x() + m_scrollBarWidth, -1 });
}

void synui::UIScrollPanel::updateChildPosition_(shared_ptr<UIComponent> a_child) {
	// Suppress the call to _onChildMoved
	m_dirtyMap[a_child.get()] = false;
	// Move child to match scroll offset
	a_child->setRelPos(m_originalPositions[a_child.get()] - m_scrollOffset);
	// Stop suppressing calls to _onChildMoved
	m_dirtyMap[a_child.get()] = true;
}

Eigen::Vector2i synui::UIScrollPanel::scrollBarPos() const {
	int yBar = m_scrollOffset.y() * (size().y() - scrollBarSize().y()) * (1.0f / m_maxExtent.y());
	int xBar = size().x() - scrollBarSize().x();
	return Vector2i{ xBar,yBar };
}

Eigen::Vector2i synui::UIScrollPanel::scrollBarSize() const {
	return Vector2i{ m_scrollBarWidth, 10.0f };
}

void synui::UIScrollPanel::notifyChildResized(UIComponent* a_child) {
	updateExtents_();
}

void synui::UIScrollPanel::_onAddChild(shared_ptr<UIComponent> a_newchild) {
	m_dirtyMap[a_newchild.get()] = true;
	_onChildMoved(a_newchild.get());
}

void synui::UIScrollPanel::_onRemoveChild() {
	updateExtents_();
}

void synui::UIScrollPanel::_onChildMoved(UIComponent* a_child) {
	if (m_dirtyMap[a_child]) {
		m_originalPositions[a_child] = a_child->getRelPos();
		updateExtents_();
		updateChildPosition_(a_child->getSharedPtr());
	}
}

void synui::UIScrollPanel::_onResize() {
	updateExtents_();
}

void synui::UIScrollPanel::draw(NVGcontext* a_nvg) {
	// Draw vertical scroll bar
	if (m_maxExtent.y() > size().y()) {
		
		nvgBeginPath(a_nvg);
		nvgFillColor(a_nvg, Color(0.7f, 0.75f));
		nvgRect(a_nvg, size().x() - m_scrollBarWidth, 0.0f, m_scrollBarWidth, size().y());
		nvgFill(a_nvg);

		Vector2i barPos = scrollBarPos();
		Vector2i barSize = scrollBarSize();

		nvgBeginPath(a_nvg);
		nvgRect(a_nvg, barPos.x(), barPos.y(), barSize.x(), barSize.y());
		NVGpaint barPaint = nvgLinearGradient(a_nvg,
			0.0f, barPos.y(),
			0.0f, barPos.y() + barSize.y(),
			Color(0.5f, 1.0f), Color(0.4f, 1.0f));
		nvgFillPaint(a_nvg, barPaint);
		nvgFill(a_nvg);
	}
}