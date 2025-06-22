#pragma once

#include <UI/UIBase.hpp>

class MenuBar : public UIComponent
{
public:

    UI_COMPONENT_CONSTRUCTOR(MenuBar);

    virtual void Render() final override;

};