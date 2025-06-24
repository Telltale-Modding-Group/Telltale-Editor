#pragma once

#include <UI/UIBase.hpp>
#include <UI/MenuBar.hpp>

#define DEFAULT_WINDOW_SIZE 1280, 720

class EditorUI : public UIStackable
{

    MenuBar _MenuBar;

public:

    EditorUI(ApplicationUI& app);

    virtual void Render() override;

};