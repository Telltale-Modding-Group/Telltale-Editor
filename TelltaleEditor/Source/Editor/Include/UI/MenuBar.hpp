#pragma once

#include <UI/UIBase.hpp>

class EditorUI;

class MenuBar : public UIComponent
{
public:
    
    Float _ImGuiMenuHeight = 0.0f;
    EditorUI& _Editor;

    MenuBar(EditorUI& ui);

    virtual void Render() final override;

};
