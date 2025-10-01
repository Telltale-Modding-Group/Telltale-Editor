#pragma once

// This inline is only used for UI wide module rendering registration in the telltale editor class as its switch dependent

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <Meta/Meta.hpp>

// =============================== RENDER CALLBACKS

struct PropertyRenderFunctionCache
{
    String SymbolCache; // for handle etc
};

struct PropertyVisualAdapter;

using PropertyRenderFn = void(const PropertyVisualAdapter& adapter, const Meta::ClassInstance&);

namespace PropertyRenderFunctions
{

    Bool _DrawVectorInput(const char* label, float* valArray, U32 n, Bool bColor4); // editor ui cpp

    inline void RenderFloat(const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        ImGui::InputFloat("##Float", (float*)datum._GetInternal());
    }

    inline void RenderBool(const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        ImGui::Checkbox("##Bool", (bool*)datum._GetInternal());
    }

    inline void RenderString(const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        ImGui::InputText("##Str", (String*)datum._GetInternal());
    }

    inline void RenderColour(const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        _DrawVectorInput(0, (float*)datum._GetInternal(), 4, true);
    }

    void RenderEnum(const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum);

    template<U32 N>
    inline void RenderFloatN(const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        _DrawVectorInput(0, (float*)datum._GetInternal(), N, false); // assuming this is all packed tight. from impl, it should. if not we will see it break lol.
    }

}

// =============================== STRUCTS

static constexpr struct PropertyRenderInstruction
{
    CString Name;
    CString ConstantName;
    PropertyRenderFn* Render;
}
PropertyRenderInstructions[] =
{
    { "float", "kPropRenderFloat", &PropertyRenderFunctions::RenderFloat},
    { "Vector2", "kPropRenderVector2",&PropertyRenderFunctions::RenderFloatN<2> },
    { "Vector3", "kPropRenderVector3",& PropertyRenderFunctions::RenderFloatN<3> },
    { "Vector4", "kPropRenderVector4",& PropertyRenderFunctions::RenderFloatN<4> },
    { "bool", "kPropRenderBool",&PropertyRenderFunctions::RenderBool },
    { "Colour", "kPropRenderColour",&PropertyRenderFunctions::RenderColour },
    { "String", "kPropRenderString",&PropertyRenderFunctions::RenderString },
    { "Enum", "kPropRenderEnum",&PropertyRenderFunctions::RenderEnum },
    { 0, 0, 0 }, // null term
};

// A specific property renderer instruction for agent modules.
struct PropertyVisualAdapter
{

    String Name;
    U32 ClassID;
    Symbol Key;

    std::vector<String> SubPaths;
    const PropertyRenderInstruction* RenderInstruction = nullptr;
    U32 HandleClassID; // if render instruction is null, this should be non 0.
    mutable PropertyRenderFunctionCache RuntimeCache;

};

// Group of rendering instructions for rendering in the inspector view for a specific UI module eg module_skeleton.prop
struct ModuleUI
{

    std::vector<PropertyVisualAdapter> VisualProperties;
    String ImagePath;

};