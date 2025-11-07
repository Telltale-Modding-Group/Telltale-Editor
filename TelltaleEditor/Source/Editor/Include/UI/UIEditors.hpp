#include <UI/EditorUI.hpp>
#include <Core/Callbacks.hpp>
#include <imgui.h>
#include <set>

class AbstractListSelectionPopup : public EditorPopup
{

    Float _RefreshRate;
    std::vector<String> _ListItems;
    String _SelectedItem;
    U64 _CacheTs;
    String UserMask;

protected:

    inline virtual void _RefreshItems(std::vector<String>& values) {}

    inline virtual String _GetToolTipText(const String& hoveredItem) { return ""; }

    virtual void _OnSelect(const String& selectedItem) = 0;

    inline AbstractListSelectionPopup(String title, Float refreshListRate) : EditorPopup(title), _RefreshRate(refreshListRate), _CacheTs(UINT64_MAX) {}

public:

    inline virtual ImVec2 GetPopupSize() override final
    {
        return ImVec2{ 500.0f, 400.0f };
    }

    virtual Bool Render() override final;

};

class TextFieldPopup : public EditorPopup
{
    
    String _Prompt;
    String _InputText;
    Ptr<FunctionBase> _Callback;

public:

    inline TextFieldPopup(String title, Ptr<FunctionBase> cb, String prompt) : EditorPopup(title), _Prompt(prompt), _InputText(""), _Callback(cb) {}

    inline virtual ImVec2 GetPopupSize() override final
    {
        return ImVec2{ 400.0f, 130.0f };
    }

    virtual Bool Render() override final;

};

// IMPL IN RESOURCE MANAGERS CPP
class ResourcePickerPopup : public AbstractListSelectionPopup
{

    StringMask Mask;
    Ptr<FunctionBase> CompletionCallback;

protected:

    virtual void _OnSelect(const String& selectedItem) override;

    virtual void _RefreshItems(std::vector<String>& values) override;

    virtual String _GetToolTipText(const String& hoveredItem) override;

public:

    inline ResourcePickerPopup(String title, StringMask mask, Ptr<FunctionBase> cb) : AbstractListSelectionPopup(title, 1.0f), Mask(mask), CompletionCallback(cb) {}


};

class MetaClassPickerPopup : public AbstractListSelectionPopup
{

    Ptr<FunctionBase> CompletionCallback;

protected:

    virtual void _OnSelect(const String& selectedItem) override;

    virtual String _GetToolTipText(const String& hoveredItem) override;

    virtual void _RefreshItems(std::vector<String>& values) override;

public:

    inline MetaClassPickerPopup(String title, Ptr<FunctionBase> cb) : AbstractListSelectionPopup(title, 0.0f), CompletionCallback(cb) {}


};

class UIResourceEditorBase : public EditorUIComponent
{
protected:

    // Returns new state, pass in current state
    Bool SelectionBox(Float xPos, Float yPos, Float xSizePix, Float ySizePix, Bool bSelectedOn);

    // returns if clicked
    Bool ImageButton(CString iconTex, Float xPos, Float yPos, Float xSizePix, Float ySizePix, U32 colScale = 0xFFFFFFFF);

public:
    
    inline UIResourceEditorBase(EditorUI& ui) : EditorUIComponent(ui) {}
    
    virtual ~UIResourceEditorBase() = default;
    
    virtual Bool IsAlive() const = 0;
    
};

template<typename CommonT = I32>
class UIResourceEditor : public UIResourceEditorBase
{
    
    enum class Type
    {
        STRONG_META,
        WEAK_META,
        COMMON,
        ISOLATED, // no dependence
    };
    
    Type AggregateType;
    String Title;
    Meta::ClassInstance MetaObj;
    Meta::ParentWeakReference WeakParent;
    Ptr<CommonT> Object;
    Bool Alive;
    
protected:
    
    inline virtual void OnExit() {}
    
    virtual Bool RenderEditor() = 0; // true => exit
    
    inline Meta::ClassInstance GetMetaObject()
    {
        return MetaObj;
    }
    
    inline Ptr<CommonT>& GetCommonObject()
    {
        return Object;
    }
    
    inline const String& GetTitle()
    {
        return Title;
    }
    
public:
    
    // FOR WEAK REF'ED
    inline UIResourceEditor(EditorUI& ui, Meta::ClassInstance weakRef, Meta::ParentWeakReference p, String t)
        : UIResourceEditorBase(ui), MetaObj(weakRef), Object{}, Title(t),
                AggregateType(Type::WEAK_META), WeakParent{p}, Alive(true) {}
    
    // FOR COMMON TYPE
    inline UIResourceEditor(EditorUI& ui, Ptr<CommonT> pObj, String t)
        : UIResourceEditorBase(ui), MetaObj{}, Object(pObj), Title(t),
                AggregateType(Type::COMMON), WeakParent{}, Alive(true)  {}

    // FOR NO DATA, ISOLATED
    inline UIResourceEditor(EditorUI& ui, String t)
        : UIResourceEditorBase(ui), MetaObj{}, Object{}, Title(t),
        AggregateType(Type::ISOLATED), WeakParent{}, Alive(true) {
    }

    // FOR STRONG META INSTANCE
    inline UIResourceEditor(EditorUI& ui, String t, Meta::ClassInstance strongRef)
        : UIResourceEditorBase(ui), MetaObj{strongRef}, Object{}, Title(t),
        AggregateType(Type::STRONG_META), WeakParent{}, Alive(true) {
    }

    inline virtual void Render() override final
    {
        if(Alive && ((AggregateType == Type::WEAK_META && WeakParent.expired()) || ((AggregateType == Type::STRONG_META || AggregateType == Type::WEAK_META) && MetaObj.Expired()) || RenderEditor()))
        {
            OnExit();
            Alive = false;
            MetaObj = {};
            WeakParent = {};
        }
    }
    
    virtual inline Bool IsAlive() const override
    {
        return Alive;
    }
    
};

enum class PropViewMode
{
    AGENT,
    TRANSIENT,
    RUNTIME,
    SCENE,
    CLASS,
    POSITION,
    NUM,
};

enum class PropAction
{
    NONE,
    REMOVE,
    MAKE_LOCAL,
};

enum class PropGlobalAction
{
    NONE,
    REMOVE,
    REMOVE_AND_LOCALS,
};

static struct PropViewModeDesc
{
    PropViewMode ModeVal;
    CString Icon;
    CString Name;
    U8 R, G, B;
}
ModeDescs[6]
{
    {PropViewMode::AGENT, "PropertySet/Agent.png", "misc.agent_props", 137, 154, 156},
    {PropViewMode::TRANSIENT, "PropertySet/Transient.png", "misc.transient_props", 230, 197, 32},
    {PropViewMode::RUNTIME, "PropertySet/Runtime.png", "misc.runtime_props",229, 55, 17},
    {PropViewMode::SCENE, "PropertySet/Scene.png", "misc.scene_props", 209, 113, 17},
    {PropViewMode::CLASS, "PropertySet/Class.png", "misc.class_props", 101, 222, 82},
    {PropViewMode::POSITION, "PropertySet/Position.png", "misc.pos_props", 59, 111, 217},
};

class UIPropertySet : public UIResourceEditor<>
{
public:
    
    // FOR WEAK REF PROP
    UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop, Meta::ParentWeakReference p);
    // FOR STRONG PROP
    UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop);
    // FOR AGENT
    UIPropertySet(EditorUI& ui, String agent, WeakPtr<ReferenceObjectInterface> scene);

protected:

    PropViewMode AgentMode = PropViewMode::AGENT;
    
    I32 _RowNum = 0;
    std::set<Symbol> _OpenDrops; // open props
    SymbolTable _HandleTable;
    std::set<Symbol> _UnknownSymbols;
    Meta::ClassInstance _AgentProperties; // actual agent properties name, cached here only while rendering

    String _NewLocalType;

    String Agent;
    WeakPtr<ReferenceObjectInterface> AgentScene; // if inspecting an agent
    
    PropGlobalAction _RenderProp(Float& currentY, Float indentX, CString name, CString propName, Meta::ClassInstance prop, String stackPath, Bool bClassProp);
    
    PropAction _RenderPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, Bool red, Bool bClassProp, String stackPath, Bool bIsClassMember = false);

    PropAction _RenderPropActionContext(const Meta::Class& typeName, const String& keyName, Bool bClassProp);

    Bool _RenderSinglePropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, String stackPath, Bool bDoRender);

    Bool _RenderDropdownPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, String stackPath, Bool bDoRender, Bool bClassProp, PropAction& actionOut);

    void _RenderClassMembers(Float& currentY, Float indentX, String stackPath, const Meta::Class& cls, Meta::ClassInstance& value, CString memberName, Bool bClassProp, PropAction& actionOut);

    Bool _RenderDropdown(Float& currentY, Float indentX, CString name, String stackPath, Bool bClassProp);
    
    virtual Bool RenderEditor();

    virtual Bool IsAlive() const override;

    void _AddGlobalCallback(String globalProp);

    void _SelectLocalTypeCallback(String localType);

    void _AddLocalCallback(String localName);

    Scene* _GetRawScene();
    
};
