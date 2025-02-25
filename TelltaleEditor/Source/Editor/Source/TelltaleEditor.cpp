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
}

TelltaleEditor::~TelltaleEditor()
{
	TTE_ASSERT(!_Running, "Update was not called properly in Telltale Editor: still running");
	
	RenderContext::Shutdown();
	
	DestroyToolContext();
	_ModdingContext = nullptr;
}

Bool TelltaleEditor::Update(Bool forceQuit)
{
	return true;
}
