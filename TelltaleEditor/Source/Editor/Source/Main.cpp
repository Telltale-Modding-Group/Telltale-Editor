#include <Core/Context.hpp>
#include <Resource/TTArchive2.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

void RunMod(ToolContext& Context)
{
    
    {
        
        ToolContext& Context = *CreateToolContext();
        // for now assume user has called 'editorui.exe "../../Dev/mod.lua"
        String src = Context.LoadLibraryStringResource("mod.lua");
        Context.GetLibraryLVM().RunText(src.c_str(), (U32)src.length(), false, "mod.lua"); // dont lock the context, allow any modding.
        Context.GetLibraryLVM().GC(); // gc after
        
        DestroyToolContext();
        
    }
    
    DumpTrackedMemory(); // dump all memort leaks. after destroy context, nothing should exist
}

// ==== TESTING SDL3

/* Constants */
//Screen dimension constants
constexpr int kScreenWidth{ 640 };
constexpr int kScreenHeight{ 480 };

/* Function Prototypes */
//Starts up SDL and creates window
bool init();

//Frees media and shuts down SDL
void close();

/* Global Variables */
//The window we'll be rendering to
SDL_Window* gWindow{ nullptr };

//The surface contained by the window
SDL_Surface* gScreenSurface{ nullptr };

/* Function Implementations */
bool init()
{
    //Initialization flag
    bool success{ true };
    
    //Initialize SDL
    if( !SDL_Init( SDL_INIT_VIDEO ) )
    {
        SDL_Log( "SDL could not initialize! SDL error: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        //Create window
        gWindow = SDL_CreateWindow( "SDL3 Tutorial: Hello SDL3", kScreenWidth, kScreenHeight, 0 );
        if(gWindow == nullptr )
        {
            SDL_Log( "Window could not be created! SDL error: %s\n", SDL_GetError() );
            success = false;
        }
        else
        {
            //Get window surface
            gScreenSurface = SDL_GetWindowSurface( gWindow );
        }
    }
    
    return success;
}

void close()
{
    
    //Destroy window
    SDL_DestroyWindow( gWindow );
    gWindow = nullptr;
    gScreenSurface = nullptr;
    
    //Quit SDL subsystems
    SDL_Quit();
}

int main( int argc, char* args[] )
{
    //Final exit code
    int exitCode{ 0 };
    
    //Initialize
    if( !init() )
    {
        SDL_Log( "Unable to initialize program!\n" );
        exitCode = 1;
    }
    else
    {
        //The quit flag
        bool quit{ false };
        
        //The event data
        SDL_Event e;
        SDL_zero( e );
        
        //The main loop
        while( quit == false )
        {
            //Get event data
            while( SDL_PollEvent( &e ) )
            {
                //If event is quit type
                if( e.type == SDL_EVENT_QUIT )
                {
                    //End the main loop
                    quit = true;
                }
            }
            
            //Fill the surface white
            SDL_FillSurfaceRect( gScreenSurface, nullptr, SDL_MapSurfaceRGB( gScreenSurface, 0xFF, 0x00, 0xFF ) );
            
            //Update the surface
            SDL_UpdateWindowSurface( gWindow );
        }
    }
    
    //Clean up
    close();
    
    return exitCode;
}
