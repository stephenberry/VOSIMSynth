#include "vosimsynth/widgets/EnhancedWindow.h"
#include <nanogui/entypo.h>
#include <nanogui/button.h>
#include <nanogui/screen.h>

synui::EnhancedWindow::EnhancedWindow(Widget* a_parent, const std::string& a_title)
    : Window(a_parent, a_title) {
    if (!a_title.empty()) {
        // Close button
        auto closeButton = new nanogui::Button(buttonPanel(), "", ENTYPO_ICON_CROSS);
        closeButton->setFixedWidth(18);
        closeButton->setCallback([this]() { setVisible(false); });
    }
}

bool synui::EnhancedWindow::mouseButtonEvent(const Eigen::Vector2i& p, int button, bool down, int modifiers) {
    if (!title().empty()) {
        return Window::mouseButtonEvent(p, button, down, modifiers);
    } else {
        Widget::mouseButtonEvent(p, button, down, modifiers);
        return true;
    }
}

void synui::EnhancedWindow::draw(NVGcontext* ctx) {
    if (m_drawCallback) {
        m_drawCallback(this, ctx);
    } else {
        Window::draw(ctx);
    }
}

nanogui::Button* synui::EnhancedWindow::createOpenButton(Widget* a_parent, const std::string& text, int icon,
                                                         std::function<void()> a_callback) {
    auto openButton = new nanogui::Button(a_parent, text, icon);
    openButton->setFixedSize({20,20});
    openButton->setCallback([this, a_callback]()
    {
        setVisible(!visible());
        screen()->moveWindowToFront(this);
        if (a_callback)
            a_callback();
    });
    return openButton;
}

void synui::EnhancedWindow::setDrawCallback(DrawFunc f) { m_drawCallback = f; }
synui::EnhancedWindow::DrawFunc synui::EnhancedWindow::getDrawCallback() const { return m_drawCallback; }
