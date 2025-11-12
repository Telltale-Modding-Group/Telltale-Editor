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

class ApplicationUI;

struct PropertyVisualAdapter;

using PropertyRenderFn = void(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance&);

namespace PropertyRenderFunctions
{

    Bool _DrawVectorInput(const char* label, float* valArray, U32 n, Bool bColor4, const PropertyVisualAdapter& adapter); // editor ui cpp

    inline void RenderFloat(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        ImGui::InputFloat("##Float", (float*)datum._GetInternal());
    }

    template<ImGuiDataType T>
    inline void RenderScalar(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        ImGui::InputScalar("##IntrinScalar", T, (void*)datum._GetInternal());
    }

    inline void RenderBool(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        ImGui::Checkbox("##Bool", (bool*)datum._GetInternal());
    }

    inline void RenderString(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        ImGui::InputText("##Str", (String*)datum._GetInternal());
    }

    inline void RenderColour(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        _DrawVectorInput(0, (float*)datum._GetInternal(), 4, true, adapter);
    }

    void RenderEnum(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum);

    void RenderPolar(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum);

    void RenderSymbol(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum);

    template<U32 N>
    inline void RenderFloatN(EditorUI& ui, const PropertyVisualAdapter& adapter, const Meta::ClassInstance& datum)
    {
        _DrawVectorInput(0, (float*)datum._GetInternal(), N, false, adapter); // assuming this is all packed tight. from impl, it should. if not we will see it break lol.
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
    { "int", "kPropRenderInt", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S32>},
    { "int32", "kPropRenderInt", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S32>},
    { "long", "kPropRenderInt", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S32>},
    { "uint32", "kPropRenderUnsignedInt", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U32>},
    { "uint", "kPropRenderUnsignedInt", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U32>},
    { "uint16", "kPropRenderUnsignedShort", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U16>},
    { "ushort", "kPropRenderUnsignedShort", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U16>},
    { "int16", "kPropRenderShort", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S16>},
    { "short", "kPropRenderShort", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S16>},
    { "int8", "kPropRenderByte", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S8>},
    { "char", "kPropRenderByte", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S8>},
    { "uint8", "kPropRenderByte", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U8>},
    { "uchar", "kPropRenderByte", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U8>},
    { "uint64", "kPropRenderUnsignedInt64", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U64>},
    { "unsigned __int64", "kPropRenderUnsignedInt64", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U64>},
    { "unsigned long long", "kPropRenderUnsignedInt64", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_U64>},
    { "int64", "kPropRenderInt64", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S64>},
    { "__int64", "kPropRenderInt64", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S64>},
    { "long long", "kPropRenderInt64", &PropertyRenderFunctions::RenderScalar<ImGuiDataType_S64>},
    { "Vector2", "kPropRenderVector2",&PropertyRenderFunctions::RenderFloatN<2> },
    { "Vector3", "kPropRenderVector3",& PropertyRenderFunctions::RenderFloatN<3> },
    { "Vector4", "kPropRenderVector4",& PropertyRenderFunctions::RenderFloatN<4> },
    { "bool", "kPropRenderBool",&PropertyRenderFunctions::RenderBool },
    { "Colour", "kPropRenderColour",&PropertyRenderFunctions::RenderColour },
    { "String", "kPropRenderString",&PropertyRenderFunctions::RenderString },
    { "Enum", "kPropRenderEnum",&PropertyRenderFunctions::RenderEnum },
    { "Polar", "kPropRenderPolar", &PropertyRenderFunctions::RenderPolar },
    { "Symbol", "kPropRenderSymbol", &PropertyRenderFunctions::RenderSymbol },
    { 0, 0, 0 }, // null term
};

// A specific property renderer instruction for agent modules, prop files, etc.
struct PropertyVisualAdapter
{

    enum FlagVals
    {
        NO_REPOSITION = 1,
    };

    String Name;
    U32 ClassID;
    U32 Flags;
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
