#pragma once

#include <UI/UIBase.hpp>

class EditorUI;

class MenuBar : public UIComponent
{

    EditorUI& _Editor;

public:

    MenuBar(EditorUI& ui);

    virtual void Render() final override;

};