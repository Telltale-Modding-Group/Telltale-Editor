#include <Common/Procedural.hpp>
#include <Common/Chore.hpp>
#include <AnimationManager.hpp>

class ProceduralAPI
{
public:
    
    
    
};

// ======= LOOK AT

void Procedural_LookAt::RegisterScriptAPI(LuaFunctionCollection& Col)
{
    
}

void Procedural_LookAt::FinaliseNormalisationAsync()
{
    
}

void Procedural_LookAt::GetRenderParameters(Vector3& bgColourOut, CString& iconName) const
{
    bgColourOut = VECTOR3_COL(255, 247, 25);
    iconName = "Chore/Animation.png";
}

void Procedural_LookAt::AddToChore(const Ptr<Chore>& pChore, String myName)
{
    Chore::Resource* pResource = const_cast<Chore::Resource*>(pChore->GetResource(myName));
    // no time in proc look ats, its always the normal time linear.
    if(!pResource->ControlAnimation->HasValue("contribution"))
    {
        pResource->ControlAnimation->GetAnimatedValues().push_back(TTE_NEW_PTR(KeyframedValue<Float>, MEMORY_TAG_ANIMATION_DATA, "contribution"));
    }
    pResource->ResFlags.Add(Chore::Resource::EMBEDDED);
}

Float Procedural_LookAt::GetLength() const
{
    return 1.0f; // WHY?
}
