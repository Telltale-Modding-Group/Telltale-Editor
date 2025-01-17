#pragma once

#include <Core/Config.hpp>
#include <Renderer/LinearHeap.hpp>
#include <Renderer/Camera.hpp>
#include <Scheduler/JobScheduler.hpp>

#include <SDL3/SDL.h>

#include <stack>
#include <vector>

/// A renderable object in the scene
struct RenderObjectInterface
{
    RenderObjectInterface* _Next; // next object in linked list
};

/// A view into the scene
struct RenderSceneView
{
    RenderSceneView* _Next; // next object in linked list
    Camera _Camera; // view camera
};

/// A renderable scene.
struct RenderScene
{
    
    RenderScene* _Next; // next scene in the linked list
    
    RenderObjectInterface* _Objects; // linked list of objects. (all memory safe in the heap for it)
    RenderSceneView* _ViewStack; // stack of scene views
    
    String _Name; // scene name
    
};

/// A render frame. Two are active at one time, one is filled up my the main thread and then executed and run on the GPU in the render job which is done async.
struct RenderFrame
{
    
    LinearHeap _Heap; // frame heap memory
    U64 _FrameNumber; // this frame number
    RenderScene* _SceneStack; // ordered scene stack
    
    // Reset and clear the frame to new frame number
    inline void Reset(U64 newFrameNumber)
    {
        _FrameNumber = newFrameNumber;
        _Heap.FreeAll(); // free all objects, but keep memory
        _SceneStack = nullptr;
    }
    
};

/// This render context represents a window in which a scene stack can be rendered to.
class RenderContext
{
public:
    
    RenderContext(String windowName); // creates window. start rendering by calling frame update each main thread frame
    ~RenderContext(); // closes window and exists the context, freeing memory
    
    // Call each frame on the main thread to wait for the renderer job (render last frame) and prepare next scene render
    // Pass in if you want to force this to be the last frame (destructor called after). Returns if user wants to quit.
    Bool FrameUpdate(Bool isLastFrame);
    
    // Current frame index.
    inline U64 GetCurrentFrameNumber()
    {
        return _Frame[_MainFrameIndex]._FrameNumber;
    }
    
    // Current frame index of what is being rendered.
    inline U64 GetRenderFrameNumber()
    {
        return _Frame[_MainFrameIndex ^ 1]._FrameNumber;
    }
    
    // Call these before and after any instance is created. Initialises SDL for multiple contexts
    static Bool Initialise();
    static void Shutdown();
    
private:
    
    // Perform main thread render
    void _RenderMain();
    
    // async render job to perform render of last frame
    static Bool _Render(const JobThread& jobThread, void* pRenderCtx, void* pFrame);
    
    RenderFrame _Frame[2]; // index swaps
    JobHandle _RenderJob; // actual gpu render job
    U32 _MainFrameIndex; // 0 or 1. NOT of this value is the render frame index, into the _Frame array.
    
    SDL_Window* _Window; // SDL3 window handle
    SDL_Renderer* _Renderer; // SDL3 renderer handle
    SDL_Surface* _BackBuffer; // SDL3 window surface
    
};
