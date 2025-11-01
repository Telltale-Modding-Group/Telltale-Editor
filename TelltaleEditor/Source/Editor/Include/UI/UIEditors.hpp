#include <UI/EditorUI.hpp>

class UIResourceEditorBase : public EditorUIComponent
{
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
    
    inline UIResourceEditor(EditorUI& ui, Meta::ClassInstance weakRef, Meta::ParentWeakReference p, Ptr<CommonT> pObj, String t)
        : UIResourceEditorBase(ui), MetaObj(weakRef), Object(pObj), Title(t),
                AggregateType(Type::WEAK_META), WeakParent{p}, Alive(true) {}
    
    inline UIResourceEditor(EditorUI& ui, Meta::ClassInstance strongRef, Ptr<CommonT> pObj, String t)
        : UIResourceEditorBase(ui), MetaObj(strongRef), Object(pObj), Title(t),
                AggregateType(Type::STRONG_META), WeakParent{}, Alive(true)  {}
    
    inline virtual void Render() override final
    {
        if(Alive && ((AggregateType == Type::WEAK_META && WeakParent.expired()) || MetaObj.Expired() || RenderEditor()))
        {
            OnExit();
            Alive = false;
            MetaObj = {};
            WeakParent = {};
        }
    }
    
    virtual inline Bool IsAlive() const override final
    {
        return Alive;
    }
    
};

class UIPropertySet : public UIResourceEditor<>
{
public:
    
    UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop, Meta::ParentWeakReference p);
    UIPropertySet(EditorUI& ui, String title, Meta::ClassInstance prop);
    
protected:
    
    I32 _RowNum = 0;
    
    void _RenderProp(Float& currentY, Float indentX, CString name, Meta::ClassInstance prop);
    
    void _RenderPropItem(Float& currentY, Float indentX, CString name, Meta::ClassInstance value, Bool red);
    
    virtual Bool RenderEditor();
    
};
