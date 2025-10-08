#include <TelltaleEditor.hpp>
#include <UI/ApplicationUI.hpp>

// Run full application
I32 CommandLine::Executor_Editor(const std::vector<TaskArgument>& args)
{
    ApplicationUI App{};
    return App.Run(args);
}

#include <Resource/PSPKG.hpp>

int main(int argc, char** argv)
{
    int exit = CommandLine::GuardedMain(argc, argv);
#ifdef DEBUG
    Memory::DumpTrackedMemory();
#endif
    return exit;
}
