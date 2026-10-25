// The frame window: menu, tab strip, sidebar and one view per open document.

#pragma once

#include <windows.h>

#include <vector>

class View;

class MainWindow {
public:
    static void RegisterWindowClass(HINSTANCE instance);
    static HWND Create(HINSTANCE instance, int showCommand);
    static MainWindow* FromHandle(HWND window);

    HWND Handle() const { return hwnd_; }
    View* ActiveView() const;

    HWND NewDocument();
    void OnIdle();

private:
    static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    explicit MainWindow(HWND window) : hwnd_(window) {}
    ~MainWindow();

    bool OnCreate();
    void Layout();
    void OnCommand(int id);
    void OnInitMenuPopup(HMENU menu);
    bool OnClose();

    void ActivateTab(int index);
    void CloseTab(int index);
    void StepTab(int delta);
    void RefreshChrome();

    void OpenDocument();

    HWND hwnd_ = nullptr;
    HWND tabBar_ = nullptr;
    HWND stylePanel_ = nullptr;
    HWND palette_ = nullptr;

    std::vector<HWND> views_;
    int activeIndex_ = -1;
    bool sidebarVisible_ = true;
};

// Both return false when the user cancels.
bool SaveDocument(View& view, bool forceSaveAs);
bool PromptToSave(View& view);
