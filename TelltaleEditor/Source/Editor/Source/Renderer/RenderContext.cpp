#include <Renderer/RenderContext.hpp>

static Bool SDL3_Initialised = false;

Bool RenderContext::Initialise()
{
    if(!SDL3_Initialised)
    {
        SDL3_Initialised = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK);
    }
    return SDL3_Initialised;
}

void RenderContext::Shutdown()
{
    if(SDL3_Initialised)
    {
        SDL_Quit();
        SDL3_Initialised = false;
    }
}

RenderContext::RenderContext(String wName)
{
    TTE_ASSERT(JobScheduler::Instance, "Job scheduler has not been initialised. Ensure a ToolContext exists.");
    TTE_ASSERT(SDL3_Initialised, "SDL3 has not been initialised, or failed to.");
    
    _MainFrameIndex = 0;
    _RenderJob = JobHandle();
    _Frame[0].Reset(0);
    _Frame[1].Reset(0);
    
    // Create window and renderer
    TTE_ASSERT(SDL_CreateWindowAndRenderer(wName.c_str(), 780, 326, 0, &_Window, &_Renderer), "Failed to create SDL3 resources");
    _BackBuffer = SDL_GetWindowSurface(_Window);
}

RenderContext::~RenderContext()
{
    _Frame[0]._Heap.ReleaseAll();
    _Frame[1]._Heap.ReleaseAll();
}

// render the main thread frame
Bool RenderContext::_Render(const JobThread& jobThread, void* pRenderCtx, void* pRenderFrame)
{
    RenderContext* pContext = static_cast<RenderContext*>(pRenderCtx);
    RenderFrame* pFrame = static_cast<RenderFrame*>(pRenderFrame);
    
    // just fill in purple for now
    SDL_SetRenderDrawColor(pContext->_Renderer, 0xFF, 0x00, 0xFF, 0xFF);
    SDL_RenderClear(pContext->_Renderer);
    SDL_RenderPresent(pContext->_Renderer);
    
    return true;
}

// generate render info about what will be rendererd
void RenderContext::_RenderMain()
{
    // ;
}

Bool RenderContext::FrameUpdate(Bool isLastFrame)
{
    Bool bUserWantsQuit = false;
    
    // 1. increase frame number
    _Frame[_MainFrameIndex].Reset(_Frame[_MainFrameIndex]._FrameNumber + 1);
    
    // 2. poll SDL events
    SDL_Event e {0};
    while( SDL_PollEvent( &e ) )
    {
        if( e.type == SDL_EVENT_QUIT )
        {
            bUserWantsQuit = true;
        }
    }
    
    // 3. Do rendering
    _RenderMain();

    // 4. Wait for previous render to complete if needed
    if(_RenderJob)
    {
        JobScheduler::Instance->Wait(_RenderJob);
        _RenderJob.Reset(); // reset handle to empty
    }
    
    // 5. give render frame currently processed, ie switch them.
    _MainFrameIndex ^= 1;
    
    // 6. Kick off render job to render the last thread data (if our thread number is larger than 1)
    if(!isLastFrame && !bUserWantsQuit)
    {
        JobDescriptor job{};
        job.AsyncFunction = &_Render;
        job.Priority = JOB_PRIORITY_HIGHEST;
        job.UserArgA = this;
        job.UserArgB = &_Frame[_MainFrameIndex ^ 1]; // render thread frame
        _RenderJob = JobScheduler::Instance->Post(job);
        // in the future, maybe we can queue the next render job if we want more than just 2 snapshots of render state.
    }
    
    return bUserWantsQuit;
}
