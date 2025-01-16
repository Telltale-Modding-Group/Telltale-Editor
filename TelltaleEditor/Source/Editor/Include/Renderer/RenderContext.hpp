#pragma once

#include <Core/Config.hpp>
#include <Renderer/LinearHeap.hpp>

#include <stack>
#include <vector>

/// A renderable object in the scene
struct RenderObjectBase
{
    RenderObjectBase* _Next; // next object in linked list
};

/// A renderable scene.
struct RenderScene
{
    RenderObjectBase* _Objects; // linked list of objects. (all memory safe in the heap for it)
};

/// This render context represents a window in which a scene stack can be rendered to.
class RenderContext
{
public:
    
    RenderContext();
    
    ~RenderContext() = default; // default destructor
    
private:
    
    LinearHeap _Heap;
    std::stack<RenderScene> _SceneStack;
    
};
