#pragma once

#include <UI/UIBase.hpp>

class EditorUI;

class MenuBar : public UIComponent
{
public:
    
    EditorUI& _Editor;

    MenuBar(EditorUI& ui);

    virtual void Render() final override;

};
