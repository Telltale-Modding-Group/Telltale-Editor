#pragma once

// High level render API

#include <Core/Config.hpp>
#include <Renderer/LinearHeap.hpp>
#include <Renderer/Camera.hpp>
#include <Scheduler/JobScheduler.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <stack>
#include <vector>
#include <mutex>

class RenderContext;

enum class RenderShaderType
{
	FRAGMENT,
	VERTEX,
};

enum class RenderSurfaceFormat
{
	UNKNOWN,
	RGBA8,
};

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

/// A texture.
struct RenderTexture
{
	
	RenderContext* _Context = nullptr;
	SDL_GPUTexture* _Handle = nullptr;
	
	U32 Width = 0, Height = 0;
	
	RenderSurfaceFormat Format = RenderSurfaceFormat::RGBA8;
	
	// If not created, creates the texture to a 2D texture all black.
	void Create2D(RenderContext*, U32 width, U32 height, RenderSurfaceFormat format, U32 nMips);
	
};

