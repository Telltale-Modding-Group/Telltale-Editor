#include <TelltaleEditor.hpp>

static TelltaleEditor* _MyContext = nullptr;

TelltaleEditor* CreateEditorContext(GameSnapshot s, Bool ui)
{
	TTE_ASSERT(_MyContext == nullptr, "A context already exists");
	_MyContext = TTE_NEW(TelltaleEditor, MEMORY_TAG_TOOL_CONTEXT, s, ui);
	return _MyContext;
}

void FreeEditorContext()
{
	if(_MyContext)
		TTE_DEL(_MyContext);
	_MyContext = nullptr;
}

TelltaleEditor::TelltaleEditor(GameSnapshot s, Bool ui)
{
	_UI = ui;
	_Running = true;
	_ModdingContext = CreateToolContext();
	_ModdingContext->Switch(s);
	RenderContext::Initialise();
	_RenderContext = TTE_NEW(RenderContext, MEMORY_TAG_RENDERER, "Telltale Editor " TTE_VERSION);
}

TelltaleEditor::~TelltaleEditor()
{
	TTE_ASSERT(!_Running, "Update was not called properly in Telltale Editor: still running");
	TTE_DEL(_RenderContext);
	
	_RenderContext = nullptr;
	RenderContext::Shutdown();
	
	DestroyToolContext();
	_ModdingContext = nullptr;
}

Bool TelltaleEditor::Update(Bool forceQuit)
{
	Bool state = _RenderContext->FrameUpdate(forceQuit);
	_Running = state;
	return state;
}
