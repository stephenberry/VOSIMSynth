#include "vosimsynth/ChildWindow.h"
#include "vosimsynth/MainGUI.h"
#include "vosimsynth/common.h"
#include <vosimlib/Command.h>
#include <vosimlib/Logging.h>
#include <GLFW/glfw3.h>

#if !defined(GL_VERSION_MAJOR)
#define GL_VERSION_MAJOR 3
#endif
#if !defined(GL_VERSION_MINOR)
#define GL_VERSION_MINOR 3
#endif

#if defined(_WIN32)
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>

static int g_nGlfwClassReg = 0;

VOID CALLBACK synui::ChildWindow::_timerProc(HWND a_hwnd, UINT /*message*/, UINT /*idTimer*/, DWORD /*dwTime*/) {
    GLFWwindow* window = static_cast<GLFWwindow*>(GetPropW(a_hwnd, L"GLFW"));
    ChildWindow* _this = static_cast<ChildWindow*>(glfwGetWindowUserPointer(window));
    if (_this->m_isOpen) {
        _this->_flushMessageQueues();
        _this->_runLoop();
    }
}

void synui::ChildWindow::_openWindowImplem(HWND a_system_window) {
    SYN_TIMING_TRACE

    HWND hwnd = glfwGetWin32Window(m_window);
    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) | WS_POPUP | WS_CHILD);
    SetParent(hwnd, a_system_window);
    // Try to force the window to update
    SetWindowPos(hwnd, a_system_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    UINT_PTR success = SetTimer(hwnd, m_timerId, 1000 / 120, reinterpret_cast<TIMERPROC>(_timerProc)); // timer callback
    if (!success) {
        SYN_MSG_TRACE("Unable to create timer!");
        throw std::runtime_error("Unable to create timer!");
    }
}

void synui::ChildWindow::_closeWindowImplem() {
    SYN_TIMING_TRACE
    HWND hwnd = glfwGetWin32Window(m_window);
    SetParent(hwnd, NULL);
    SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) & ~WS_CHILD);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    KillTimer(hwnd, m_timerId);
}
#endif // _WIN32

synui::ChildWindow::ChildWindow(int a_width, int a_height)
    : m_window(nullptr),
      m_isOpen(false),
      m_guiInternalMsgQueue(MAX_GUI_MSG_QUEUE_SIZE),
      m_guiExternalMsgQueue(MAX_GUI_MSG_QUEUE_SIZE) {
    SYN_TIMING_TRACE
    auto errorCallback = [](int a_error, const char* a_description)
    {
        SYN_MSG_TRACE(a_description);
        puts(a_description);
    };
    glfwSetErrorCallback(errorCallback);
    // Create GLFW window
    if (!(g_nGlfwClassReg++) && !glfwInit()) {
        SYN_MSG_TRACE("Failed to init GLFW.");
        throw std::runtime_error("Failed to init GLFW.");
    } else {
        SYN_MSG_TRACE("Successfully initialized GLFW.");
    }
    _createGlfwWindow(a_width, a_height);
}

synui::ChildWindow::~ChildWindow() {
    SYN_TIMING_TRACE
    closeWindow();
    if (!--g_nGlfwClassReg)
        glfwTerminate();
}

void synui::ChildWindow::_createGlfwWindow(int a_width, int a_height) {
    SYN_TIMING_TRACE

#if defined(GL_VERSION_MAJOR)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
#endif
#if defined(GL_VERSION_MINOR)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
#endif
#if !defined(NDEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    m_window = glfwCreateWindow(a_width, a_height, "VOSIMSynth", nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        SYN_MSG_TRACE("Failed to create GLFW window.");
        throw std::runtime_error("Failed to create GLFW window.");
    } else {
        SYN_MSG_TRACE("Successfully created GLFW window.");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);
    glfwSwapBuffers(m_window);

#if defined(TRACER_BUILD)
        int glMajorVer = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MAJOR);
        int glMinorVer = glfwGetWindowAttrib(m_window, GLFW_CONTEXT_VERSION_MINOR);
        SYN_MSG_TRACE("Using OpenGL version: %d.%d", glMajorVer, glMinorVer)
#endif
}

#if defined(_WIN32)
bool synui::ChildWindow::openWindow(HWND a_systemWindow) {
    SYN_TIMING_TRACE
    if (!m_isOpen) {
        m_isOpen = true;
        _openWindowImplem(a_systemWindow);
        glfwShowWindow(m_window);
        glfwFocusWindow(m_window);
        _onOpen();
    }
    return true;
}
#endif

void synui::ChildWindow::closeWindow() {
    SYN_TIMING_TRACE
    if (m_isOpen) {
        m_isOpen = false;
        _closeWindowImplem();
        glfwHideWindow(m_window);
        _onClose();
    }
}

bool synui::ChildWindow::queueExternalMessage(syn::Command* a_msg) {
    SYN_TIMING_TRACE
    if (!m_guiExternalMsgQueue.write_available()) {
        return false;
    }
    m_guiExternalMsgQueue.push(a_msg);
    return true;
}

bool synui::ChildWindow::queueInternalMessage(syn::Command* a_msg) {
    SYN_TIMING_TRACE
    if (!m_guiInternalMsgQueue.write_available()) {
        return false;
    }
    m_guiInternalMsgQueue.push(a_msg);
    return true;
}

int synui::ChildWindow::getWidth() const {
    int width;
    glfwGetWindowSize(m_window, &width, nullptr);
    return width;
}

int synui::ChildWindow::getHeight() const {
    int height;
    glfwGetWindowSize(m_window, nullptr, &height);
    return height;
}

void synui::ChildWindow::_flushMessageQueues() {
    syn::Command* msg;
    while (m_guiInternalMsgQueue.pop(msg)) {
        (*msg)();
        delete msg;

    }
    while (m_guiExternalMsgQueue.pop(msg)) {
        (*msg)();
        delete msg;
    }
}