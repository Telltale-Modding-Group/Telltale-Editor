#include <Common/Skeleton.hpp>

class SkeletonAPI
{
public:
    
    static void GetXYZW(Meta::ClassInstance& clazz, Float& x, Float& y, Float& z, Float* w)
    {
        x = Meta::GetMember<Float>(clazz, "x");
        y = Meta::GetMember<Float>(clazz, "y");
        z = Meta::GetMember<Float>(clazz, "z");
        if(w)
            *w = Meta::GetMember<Float>(clazz, "w");
    }
    
    static Skeleton* Task(LuaManager& man)
    {
        return (Skeleton*)man.ToPointer(1);
    }
    
    static U32 luaPushEntry(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage of push entry");
        Skeleton* skl = Task(man);
        
        SkeletonEntry entry{};
        
        ScriptManager::TableGet(man, "Joint Name");
        entry.JointName = ScriptManager::PopString(man);
        ScriptManager::TableGet(man, "Parent Name");
        entry.ParentJointName = ScriptManager::PopString(man);
        ScriptManager::TableGet(man, "Parent Index");
        entry.ParentIndex = ScriptManager::PopInteger(man);
        
        ScriptManager::TableGet(man, "Local Position");
        Meta::ClassInstance clazz = Meta::AcquireScriptInstance(man, -1);
        man.Pop(1);
        GetXYZW(clazz, entry.LocalPosition.x, entry.LocalPosition.y, entry.LocalPosition.z, nullptr);
        
        ScriptManager::TableGet(man, "Local Rotation");
        clazz = Meta::AcquireScriptInstance(man, -1);
        man.Pop(1);
        GetXYZW(clazz, entry.LocalRotation.x, entry.LocalRotation.y, entry.LocalRotation.z, &entry.LocalRotation.w);
        
        ScriptManager::TableGet(man, "Global Scale");
        clazz = Meta::AcquireScriptInstance(man, -1);
        man.Pop(1);
        GetXYZW(clazz, entry.GlobalTranslationScale.x,
                entry.GlobalTranslationScale.y, entry.GlobalTranslationScale.z, nullptr);
        
        ScriptManager::TableGet(man, "Local Scale");
        clazz = Meta::AcquireScriptInstance(man, -1);
        man.Pop(1);
        GetXYZW(clazz, entry.LocalTranslationScale.x,
                entry.LocalTranslationScale.y, entry.LocalTranslationScale.z, nullptr);
        
        ScriptManager::TableGet(man, "Anim Scale");
        clazz = Meta::AcquireScriptInstance(man, -1);
        man.Pop(1);
        GetXYZW(clazz, entry.AnimTranslationScale.x,
                entry.AnimTranslationScale.y, entry.AnimTranslationScale.z, nullptr);
        
        ScriptManager::TableGet(man, "Group Membership");
        clazz = Meta::AcquireScriptInstance(man, -1);
        Meta::ClassInstanceCollection& map = Meta::CastToCollection(clazz);
        U32 num = map.GetSize();
        for(U32 i = 0; i < num; i++)
        {
            clazz = map.GetValue(i);
            const Float* pVal = (const Float*)clazz._GetInternal();
            clazz = map.GetKey(i);
            const Meta::Class& keyclz = Meta::GetClass(clazz.GetClassID());
            String key{};
            if(Meta::IsSymbolClass(keyclz))
                key = SymbolTable::Find(*((const Symbol*)clazz._GetInternal()));
            else
            {
                key = *(const String*)clazz._GetInternal();
                GetRuntimeSymbols().Register(key); // ensure we know all bone names
            }
            entry.ResourceGroupMembership[std::move(key)] = *pVal;
        }
    
        skl->_Entries.push_back(std::move(entry));
        return 0;
    }
    
};

std::atomic<U32> Skeleton::_sSerial{1};

Skeleton::Skeleton(Skeleton&& rhs) : HandleableRegistered<Skeleton>(std::move(rhs))
{
    _Serial = rhs._Serial;
    rhs._Serial = 0;
}

Skeleton::Skeleton(const Skeleton& rhs) : HandleableRegistered<Skeleton>(rhs)
{
    _Serial = _sSerial++;
    if(_Serial == UINT32_MAX)
        _sSerial = 1;
    _Entries = rhs._Entries;
}

Skeleton::Skeleton(Ptr<ResourceRegistry> registry) : HandleableRegistered<Skeleton>(std::move(registry))
{
    _Serial = _sSerial++;
    if(_Serial == UINT32_MAX)
        _sSerial = 1;
}

U32 Skeleton::GetSerial() const
{
    return _Serial;
}

void Skeleton::FinaliseNormalisationAsync()
{

}

void Skeleton::RegisterScriptAPI(LuaFunctionCollection &Col)
{
    PUSH_FUNC(Col, "CommonSkeletonPushEntry", &SkeletonAPI::luaPushEntry, "nil CommonSkeletonPushEntry(state, entryInfoTable)",
              "Pushes skeleton entry information to the common skeleton");
}
