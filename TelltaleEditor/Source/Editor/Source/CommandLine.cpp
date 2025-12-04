#include <TelltaleEditor.hpp>

#include <sstream>
#include <filesystem>

namespace CommandLine
{
    
    // =============================== EXECUTORS =============================
    
    static GameSnapshot GetSnapshot(const std::vector<TaskArgument> &args)
    {
        GameSnapshot snapshot = {GetStringArgumentOrDefault(args, "-game", ""),
            GetStringArgumentOrDefault(args, "-platform", "PC"), GetStringArgumentOrDefault(args, "-vendor", "")};
        if(snapshot.ID.length() == 0)
            printf("** No game ID specified!\n");
        return snapshot;
    }
    
    // ===================== DOCS DUMP =====================
    
    static void _DumpCollection(LuaFunctionCollection Col, std::ostringstream& out)
    {
        for(const auto& fn: Col.Functions)
        {
            out << "--- ";
            out << StringWrapText(fn.Description, "\n--- ") << "\n";
            String ret{};
            std::vector<String> args{};
            StringParseFunctionSignature(fn.Declaration, ret, args);
            for(const auto& arg: args)
                out << "--- @param " << arg << " nil\n";
            out << "--- @return " << ret << "\n";
            out << "function " << fn.Name << "(";
            Bool bDo = false;
            for(const auto& arg: args)
            {
                if(bDo)
                    out << ", ";
                out << arg;
                bDo = true;
            }
            out << ")\nend\n\n";
        }
        for(const auto& str: Col.GlobalDescriptions)
        {
            out << "--- " << StringWrapText(str.second, "\n--- ");
            if(Col.StringGlobals.find(str.first) != Col.StringGlobals.end())
                out << "\n--- @type string\n" << str.first << " = \"\"\n\n";
            else
                out << "\n--- @type number\n" << str.first << " = 0\n\n";
        }
    }
    
    static I32 Executor_GenerateDocs(const std::vector<TaskArgument>& args)
    {
        std::ostringstream docs{};
        String ofile = GetStringArgumentOrDefault(args, "-out", "");
        if(ofile.find('\\') == String::npos || ofile.find('/') == String::npos)
            ofile = "./" + ofile;
        if(ofile.length() == 0 || !std::filesystem::exists(std::filesystem::path(ofile).parent_path()))
        {
            printf("** The output file directory '%s' does not exist. Please create the parent directory first\n", ofile.c_str());
            return 1;
        }
        DataStreamFile fileStream(ofile);
        Bool excludeLib = HasArgument(args, "-exclude-tte-lib");
        docs << "-- Telltale Editor generated Lua documentation. Using v" TTE_VERSION "\n\n\n";
        _DumpCollection(luaGameEngine(false), docs);
        if(!excludeLib)
        {
            LuaFunctionCollection PropKonst{};
            for (const auto& prop : GetPropKeyConstants())
            {
                PUSH_GLOBAL_S(PropKonst, prop.first, prop.second, "Telltale Property Keys");
            }
            _DumpCollection(PropKonst, docs);
            _DumpCollection(luaLibraryAPI(false), docs);
            _DumpCollection(CreateScriptAPI(), docs);
        }
        String s = docs.str();
        fileStream.Write((const U8*)s.c_str(), s.length());
        printf("** Dumped Lua documentation to %s!", ofile.c_str());
        return 0;
    }
    
    // ===================== USAGE =====================
    
    static void _PrintArguments(const std::vector<TaskArgument>& args, Bool bReq)
    {
        for(const auto& arg: args)
        {
            printf(bReq ? " -<%s" : " -[%s" ,arg.Name.c_str() + 1);
            for(const auto& alias: arg.Aliases)
                printf("/%s",alias.c_str() + 1);
            printf(bReq ? ">" : "]");
            if(arg.Type == ArgumentType::BOOL)
                printf(" (yes/no)");
            else if(arg.Type == ArgumentType::INT)
                printf(" (integer)");
            else if(arg.Type == ArgumentType::STRING)
                printf(" (string)");
        }
    }
    
    static I32 Executor_Usage(const std::vector<TaskArgument>& args)
    {
        std::vector<TaskInfo> tasks = CreateTasks();
        printf("\n******** Telltale Editor v" TTE_VERSION "\n**\n**\n** Run with no command "
               "line arguments to run the normal editor in your current directory.\n"
               "**\n**\n** Otherwise, choose from any of the options below\n** You can execute "
               "as many as you want in one go, they will all be executed."
               "\n**\n**\n");
        for(const auto& task: tasks)
        {
            printf("** Command: tte %s", task.Name.c_str());
            _PrintArguments(task.RequiredArguments, true);
            _PrintArguments(task.OptionalArguments, false);
            String wrapped = StringWrapText(task.Desc.c_str(), "\n** ", 10, 100).c_str();
            printf("\n** Description: %s\n**\n**\n", wrapped.c_str());
        }
        printf("** Thank you for using the Telltale Editor!\n**\n** Made with love by Lucas Saragosa and Ivan!\n********\n");
        return 0;
    }
    
    // ===================== SYMMAP GEN =====================
    
    static I32 Executor_GenSM(const std::vector<TaskArgument> &args)
    {
        String mount = GetStringArgumentOrDefault(args, "-mount", "./");
        GameSnapshot game = GetSnapshot(args);
        if(!game.ID.length())
            return 1;
        {
            TelltaleEditor* editor = CreateEditorContext(game);
            
            Ptr<ResourceRegistry> registry = editor->CreateResourceRegistry(false);
            registry->MountSystem("<Data>/", mount, true);
            
            std::set<String> all{};
            registry->GetResourceNames(all, nullptr);
            
            DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(GetStringArgumentOrDefault(args, "-out", "./default.symmap"));
            SymbolTable t(true);
            for(auto& f: all)
                t.Register(f);
            t.SerialiseOut(out);
            TTE_LOG("** Dumped symbol map!");
            FreeEditorContext();
        }
        return 0;
    }
    
    static I32 Executor_GenRTSM(const std::vector<TaskArgument> &args)
    {
        DataStreamRef out = DataStreamManager::GetInstance()->CreateFileStream(GetStringArgumentOrDefault(args, "-out", "./default.symmap"));
        GetRuntimeSymbols().SerialiseOut(out);
        TTE_LOG("** Dumped symbol map!");
        return 0;
    }
    
    // ===================== ARCHIVEX =====================
    
    static void Executor_Extract_CallbackAsync(const String& file)
    {
        TTE_LOG("** %s", file.c_str());
    }
    
    static I32 Executor_Extract(const std::vector<TaskArgument> &args)
    {
        String mount = GetStringArgumentOrDefault(args, "-in", "");
        String out = GetStringArgumentOrDefault(args, "-out", "./Out/");
        if(!StringEndsWith(out, "/"))
            out += "/";
        try
        {
            std::filesystem::create_directories(out);
        }
        catch(...)
        {
            TTE_LOG("** The output directory '%s' does not exist on your local machine!", out.c_str());
            return 1;
        }
        Bool bFlat = HasArgument(args, "-flat");
        String filter = GetStringArgumentOrDefault(args, "-filter", "*");
        GameSnapshot game = GetSnapshot(args);
        if(!game.ID.length())
            return 1;
        {
            TelltaleEditor* editor = CreateEditorContext(game);
            Ptr<ResourceRegistry> registry = editor->CreateResourceRegistry(false);
            if(std::filesystem::is_regular_file(mount))
                registry->MountArchive("<Data>/", mount);
            else if(std::filesystem::is_directory(mount))
                registry->MountSystem("<Data>/", mount, true);
            else
            {
                FreeEditorContext();
                TTE_LOG("** The mount point '%s' does not exist on your local machine!", mount.c_str());
                return 1;
            }
            editor->EnqueueResourceLocationExtractTask(registry, "<Data>/", out, filter, !bFlat, &Executor_Extract_CallbackAsync);
            editor->Wait();
            TTE_LOG("\n** Successfully extracted files from resource location");
            registry = {};
            FreeEditorContext();
        }
        return 0;
    }
    
    // ===================== JSON =====================
    
    static I32 Executor_JSON(const std::vector<TaskArgument>& args)
    {
        String in = GetStringArgumentOrDefault(args, "-in", "");
        String out = GetStringArgumentOrDefault(args, "-out", "./out.json");
        I32 bufSize = GetIntArgumentOrDefault(args, "-max-buffer-size", 128);
        GameSnapshot game = GetSnapshot(args);
        if(!game.ID.length())
            return 1;
        {
            TelltaleEditor* editor = CreateEditorContext(game);
            DataStreamRef infile = DataStreamManager::GetInstance()->CreateFileStream(in);
            DataStreamRef outfile = DataStreamManager::GetInstance()->CreateFileStream(out);
            if(!infile->GetSize())
            {
                FreeEditorContext();
                TTE_LOG("\n** Input file not found or empty");
                return 1;
            }
            if(Meta::ReadMetaStream(FileGetFilename(in), infile, outfile, bufSize))
                TTE_LOG("\n** Successfully converted to JSON");
            else
                TTE_LOG("\n** Failed to fully serialise the file input. Please contact us. The JSON is written until where the error occured.");
            FreeEditorContext();
        }
        return 0;
    }
    
    // ===================== LUA =====================
    
    static I32 Executor_Lua(const std::vector<TaskArgument>& args)
    {
        String inf = GetStringArgumentOrDefault(args, "-in", "");
        String outfo = GetStringArgumentOrDefault(args, "-out", "./");
        std::vector<String> infiles = GetInputFiles(inf);
        if(!infiles.size())
            return 1;
        if(std::filesystem::is_regular_file(outfo))
            outfo = std::filesystem::absolute(outfo).parent_path().string();
        else if(!StringEndsWith(outfo, "/") && !StringEndsWith(outfo, "\\"))
            outfo += "/";
        {
            GameSnapshot game = GetSnapshot(args);
            TelltaleEditor* editor = CreateEditorContext(game);
            for(String in: infiles)
            {
                String out = infiles.size() == 1 && HasArgument(args, "-out") ? GetStringArgumentOrDefault(args, "-out", "") : (outfo + FileGetFilename(in));
                if(!game.ID.length())
                {
                    FreeEditorContext();
                    return 1;
                }
                Bool bEncryptDisable = Meta::GetInternalState().GetActiveGame().Caps[GameCapability::SCRIPT_ENCRYPTION_DISABLED];
                DataStreamRef infile = DataStreamManager::GetInstance()->CreateFileStream(in);
                if(HasArgument(args, "-encrypt"))
                {
                    if(Meta::GetInternalState().GetActiveGame().Caps[GameCapability::USES_LENC] && !StringEndsWith(out, "lenc"))
                        out = FileSetExtension(out, "lenc");
                    DataStreamRef outfile = DataStreamManager::GetInstance()->CreateFileStream(out);
                    if(bEncryptDisable)
                    {
                        TTE_LOG("** Error: the game snapshot provided does not support script encryption");
                        FreeEditorContext();
                        return 1;
                    }
                    DataStreamRef encrypted = ScriptManager::EncryptScript(infile, StringEndsWith(out, ".lenc"));
                    DataStreamManager::GetInstance()->Transfer(encrypted, outfile, encrypted->GetSize());
                }
                else if(HasArgument(args, "-decrypt"))
                {
                    out = FileSetExtension(out, "lua");
                    DataStreamRef outfile = DataStreamManager::GetInstance()->CreateFileStream(out);
                    if(bEncryptDisable)
                    {
                        TTE_LOG("** Warning: the game snapshot provided does not support script encryption. Are you sure it's correct?");
                    }
                    DataStreamRef encrypted = ScriptManager::DecryptScript(infile);
                    DataStreamManager::GetInstance()->Transfer(encrypted, outfile, encrypted->GetSize());
                }
                else
                {
                    TTE_LOG("** No lua sub-task specified. Please specify -encrypt,-decrypt etc...");
                    FreeEditorContext();
                    return 1;
                }
                TTE_LOG("** Done %s", in.c_str());
            }
            FreeEditorContext();
        }
        return 0;
    }
    
    // ===================== EXEC =====================
    
    // Simply run mod.lua
    static I32 Executor_Exec(const std::vector<TaskArgument>& args)
    {
        I32 ret = 0;
        String exec = GetStringArgumentOrDefault(args, "-script", "mod.lua");
        GameSnapshot game = GetSnapshot(args);
        if(!game.ID.length())
            return 1;
        TelltaleEditor* editor = CreateEditorContext(game);
        {
            Ptr<ResourceRegistry> userReg = editor->CreateResourceRegistry(true);
            DataStreamRef file = DataStreamManager::GetInstance()->CreateFileStream(exec);
            if(!file->GetSize())
            {
                TTE_LOG("** Input script %s not found or empty", exec.c_str());
                ret = 1;
            }
            else
            {
                Memory::FastBufferAllocator alloc{};
                U8* s = alloc.Alloc(file->GetSize(), 4);
                file->Read(s, file->GetSize());
                GetThreadLVM().RunText((CString)s, (U32)file->GetSize(), false, exec.c_str());
            }
        }
        FreeEditorContext();
        return ret;
    }
    
    // ===================== DECRYPT MS =====================
    
    static I32 Executor_DecryptMS(const std::vector<TaskArgument>& args)
    {
        String inf = GetStringArgumentOrDefault(args, "-in", "");
        String outfo = GetStringArgumentOrDefault(args, "-out", "./");
        std::vector<String> infiles = GetInputFiles(inf);
        if(!infiles.size())
            return 1;
        if(std::filesystem::is_regular_file(outfo))
            outfo = std::filesystem::absolute(outfo).parent_path().string();
        else if(!StringEndsWith(outfo, "/") && !StringEndsWith(outfo, "\\"))
            outfo += "/";
        {
            GameSnapshot game = GetSnapshot(args);
            TelltaleEditor* editor = CreateEditorContext(game);
            for(String in: infiles)
            {
                String out = infiles.size() == 1 && HasArgument(args, "-out") ? GetStringArgumentOrDefault(args, "-out", "") : (outfo + FileGetFilename(in));
                if(!game.ID.length())
                {
                    FreeEditorContext();
                    return 1;
                }
                Bool bEncryptDisable = Meta::GetInternalState().GetActiveGame().Caps[GameCapability::SCRIPT_ENCRYPTION_DISABLED];
                DataStreamRef infile = DataStreamManager::GetInstance()->CreateFileStream(in);
                DataStreamRef outfile = DataStreamManager::GetInstance()->CreateFileStream(out);
                infile = Meta::MapDecryptingStream(infile);
                DataStreamManager::GetInstance()->Transfer(infile, outfile, infile->GetSize());
                TTE_LOG("** Done %s", in.c_str());
            }
            FreeEditorContext();
        }
        return 0;
    }
    
    // ===================== LOAD ALL DBG =====================
    
    static I32 Executor_LoadAll(const std::vector<TaskArgument>& args)
    {
        String inf = GetStringArgumentOrDefault(args, "-in", "");
        String outerr = GetStringArgumentOrDefault(args, "-errorfile", "./ProbeLog.txt");
        Bool bNormalise = GetBoolArgumentOrDefault(args, "-normalise", false);
        std::set<String> nonMeta{}, failedOpen{}, failed{}, failedNormalise{}, failedNoCommonClass{};
        U32 numOk = 0;
        std::vector<String> infiles = GetInputFiles(inf);
        if(!infiles.size())
            return 1;
        GameSnapshot game = GetSnapshot(args);
        TelltaleEditor* editor = CreateEditorContext(game);
        Ptr<ResourceRegistry> reg = editor->CreateResourceRegistry(false);
        {
            U32 i = 0;
            Float scale = 100.0f / (Float)infiles.size();
            for(const auto& fileName: infiles)
            {
                Float percent = (Float)i * scale;
                String ext = FileGetExtension(fileName);
                if(Meta::FindClassByExtension(ext, 0) == 0)
                {
                    nonMeta.insert(fileName);
                    TTE_LOG("** %.04f%% Not a meta stream: %s", percent, fileName.c_str());
                }
                else
                {
                    DataStreamRef stream = DataStreamManager::GetInstance()->CreateFileStream(fileName);
                    if(!stream || stream->GetSize() == 0)
                    {
                        TTE_LOG("** %.04f%% Failed to open: %s", percent, fileName.c_str());
                        failedOpen.insert(fileName);
                    }
                    else
                    {
                        Meta::ClassInstance instance = Meta::ReadMetaStream(fileName, stream);
                        Bool ok = false;
                        if(instance)
                        {
                            ok = true;
                            if(bNormalise)
                            {
                                ok = false;
                                Ptr<Handleable> pResource = CommonClassInfo::GetCommonClassInfoByExtension(ext).ClassAllocator(reg);
                                if(pResource)
                                {
                                    if(InstanceTransformation::PerformNormaliseAsync(pResource, instance, GetThreadLVM()))
                                    {
                                        ok = true;
                                    }
                                    else
                                    {
                                        failedNormalise.insert(fileName);
                                    }
                                }
                                else
                                {
                                    failedNoCommonClass.insert(fileName);
                                }
                            }
                        }
                        if(ok)
                        {
                            numOk++;
                            TTE_LOG("** %.04f%% Passed: %s", percent, fileName.c_str());
                        }
                        else
                        {
                            failed.insert(fileName);
                            TTE_LOG("** %.04f%% Failed: %s", percent, fileName.c_str());
                        }
                    }
                }
                i++;
            }
            {
                std::stringstream log{};
                log << "** Probed files output generated by the Telltale Editor v" TTE_VERSION "\n\n";
                log << "** Out of the " << infiles.size() << " input files, " << numOk << " successfully were read fully.\n\n";
                log << "\n** The following files failed to be opened by Telltale Editor (check permissions):\n\n";
                for(const auto& f: failedOpen)
                    log << "** " << f << "\n";
                log << "\n** The following files are not meta stream files:\n\n";
                for(const auto& f: nonMeta)
                    log << "** " << f << "\n";
                log << "\n** The following files failed to read successfully:\n\n";
                for(const auto& f: failed)
                    log << "** " << f << "\n";
                log << "\n** The following files failed to normalise:\n\n";
                for(const auto& f: failedNormalise)
                    log << "** " << f << "\n";
                log << "\n** The following files failed has no common class to normalise to:\n\n";
                for(const auto& f: failedNoCommonClass)
                    log << "** " << f << "\n";
                DataStreamRef logf = DataStreamManager::GetInstance()->CreateFileStream(outerr);
                String s = log.str();
                logf->Write((const U8*)s.c_str(), s.length());
            }
        }
        reg = {};
        FreeEditorContext();
        return 0;
    }
    
    // ===================== DEFAULT =====================
    
    std::vector<TaskInfo> CreateTasks()
    {
        std::vector<TaskInfo> tasks{};
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"mkdocs", "Generate Lua documentation for all telltale scripts", &Executor_GenerateDocs});
            task.RequiredArguments.push_back({"-out",ArgumentType::STRING, {"-o"}});
            task.OptionalArguments.push_back({"-exclude-tte-lib", ArgumentType::NONE});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"usage", "Display usage help menu", &Executor_Usage});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"mksymmap", "Generate string symbol map of all files from game directories", &Executor_GenSM});
            task.OptionalArguments.push_back({"-out",ArgumentType::STRING, {"-o"}});
            task.RequiredArguments.push_back({"-mount",ArgumentType::STRING});
            task.RequiredArguments.push_back({"-game",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-platform",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-vendor",ArgumentType::STRING});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"mkrtsymmap", "Generate string symbol map for all runtime telltale symbols", &Executor_GenRTSM});
            task.OptionalArguments.push_back({"-out",ArgumentType::STRING, {"-o"}});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"extract", "Extract files from archives or any compound telltale file or whole game directory."
                " Specify an optional filename filter, eg '*.scene;!*.dlg'. Flat doesn't export folders.", &Executor_Extract});
            task.OptionalArguments.push_back({"-out",ArgumentType::STRING, {"-o"}});
            task.RequiredArguments.push_back({"-in",ArgumentType::STRING, {"-i", "-f", "-file"}});
            task.RequiredArguments.push_back({"-game",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-filter",ArgumentType::STRING, {"-mask"}});
            task.OptionalArguments.push_back({"-flat",ArgumentType::NONE});
            task.OptionalArguments.push_back({"-platform",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-vendor",ArgumentType::STRING});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"json", "Convert a telltale file into human-readable JSON", &Executor_JSON});
            task.OptionalArguments.push_back({"-out",ArgumentType::STRING, {"-o"}});
            task.RequiredArguments.push_back({"-in",ArgumentType::STRING, {"-i", "-f", "-file"}});
            task.RequiredArguments.push_back({"-game",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-platform",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-vendor",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-max-buffer-size",ArgumentType::INT});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"lua", "Various lua script helpers which are applied to input file or "
                "all files in the input folder", &Executor_Lua});
            task.OptionalArguments.push_back({"-out",ArgumentType::STRING, {"-o"}});
            task.RequiredArguments.push_back({"-in",ArgumentType::STRING, {"-i"}});
            task.RequiredArguments.push_back({"-game",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-platform",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-vendor",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-decrypt",ArgumentType::NONE});
            task.OptionalArguments.push_back({"-encrypt",ArgumentType::NONE});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"decrypt", "Decrypt encrypted meta streams (MBES/others). Does not decrypt lua scripts or any"
                " other script! Prefer to use the lua command for that. Pass in a file or whole folder.", &Executor_DecryptMS});
            task.OptionalArguments.push_back({"-out",ArgumentType::STRING, {"-o"}});
            task.RequiredArguments.push_back({"-in",ArgumentType::STRING, {"-i"}});
            task.RequiredArguments.push_back({"-game",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-platform",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-vendor",ArgumentType::STRING});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"probe", "This probe command loads every single file (recursively) in the input"
                " mount directory. Any erroring files go to the output error file.", &Executor_LoadAll});
            task.OptionalArguments.push_back({"-in",ArgumentType::STRING, {"-i"}});
            task.RequiredArguments.push_back({"-game",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-normalise",ArgumentType::BOOL, {"-n"}});
            task.OptionalArguments.push_back({"-platform",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-vendor",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-errorfile",ArgumentType::STRING, {"-ef","-errf"}});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"execute", "Execute a mod script. Runs 'mod.lua' by default.", &Executor_Exec});
            task.OptionalArguments.push_back({"-script",ArgumentType::STRING, {"-s", "-f", "-file"}});
            task.RequiredArguments.push_back({"-game",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-platform",ArgumentType::STRING});
            task.OptionalArguments.push_back({"-vendor",ArgumentType::STRING});
        }
        
        {
            auto& task = tasks.emplace_back(TaskInfo{"run", "Runs the Telltale Editor application. Optionally pass in the user directory for your workspace, or project file path to load."
                " You can also pass in the file resource location (or just file name) to open on startup after project selection.", &Executor_Editor});
            task.OptionalArguments.push_back({ "-userdir",ArgumentType::STRING, {"-cwd"} });
            task.OptionalArguments.push_back({ "-project",ArgumentType::STRING, {"-p","-proj"} });
            task.OptionalArguments.push_back({ "-file",ArgumentType::STRING, {"-f","-open"} });
            task.DefaultTask = true;
        }
        
        return tasks;
    }
    
    // ============================= INTERNAL IMPL ===========================
    
    Bool HasArgument(const std::vector<TaskArgument>& args, String arg)
    {
        for(const auto& test: args)
        {
            if(_Impl::MatchArgument(arg, test))
                return true;
        }
        return false;
    }
    
    String GetStringArgumentOrDefault(const std::vector<TaskArgument>& args, String arg, String def)
    {
        for(const auto& test: args)
        {
            if(_Impl::MatchArgument(arg, test))
                return test.StringValue;
        }
        return def;
    }
    
    I32 GetIntArgumentOrDefault(const std::vector<TaskArgument>& args, String arg, I32 def)
    {
        for(const auto& test: args)
        {
            if(_Impl::MatchArgument(arg, test))
                return test.IntValue;
        }
        return def;
    }
    
    Bool GetBoolArgumentOrDefault(const std::vector<TaskArgument>& args, String arg, Bool def)
    {
        for(const auto& test: args)
        {
            if(_Impl::MatchArgument(arg, test))
                return test.BoolValue;
        }
        return def;
    }
    
    std::vector<String> GetInputFiles(const String& _path)
    {
        namespace fs = std::filesystem;
        fs::path path = _path;
        
        if (fs::is_regular_file(path))
            return {fs::absolute(path).string()};
        
        if (!fs::is_directory(path))
        {
            TTE_LOG("** At '%s': not a directory or file", _path.c_str());
            return {};
        }
        
        std::vector<String> files;
        fs::path base = fs::absolute(path);
        
        for (const auto& file : fs::recursive_directory_iterator(base))
        {
            if (fs::is_regular_file(file))
            {
                fs::path rel = fs::relative(file.path(), base);
                files.push_back((path / rel).string());
            }
        }
        
        if (!files.empty())
            return files;
        
        TTE_LOG("** At '%s': no files found", _path.c_str());
        return {};
    }
    
    namespace _Impl
    {
        
        I32 ExecuteTask(std::vector<String>& argsStack, const std::vector<TaskInfo>& availableTasks)
        {
            if(argsStack.empty())
                return 0;
            String task = argsStack.back();
            argsStack.pop_back();
            TaskInfo curTask = ResolveTask(availableTasks, task);
            if(curTask.Name.length() == 0)
            {
                printf("Error executing command line: task '%s' not recognised or arguments were incorrect. For help, try 'tte usage'!\n", task.c_str());
                return 1;
            }
            std::vector<TaskArgument> resolvedOptional{}, resolvedRequired{};
            for(;;)
            {
                if(argsStack.empty())
                    break;
                String arg = argsStack.back();
                argsStack.pop_back();
                Bool bResolvedThisArg = false;
                for(const auto& requiredArg: curTask.RequiredArguments)
                {
                    if(MatchArgument(arg, requiredArg))
                    {
                        if(!requiredArg.Multiple && MatchesAnyArgument(arg, resolvedRequired))
                        {
                            printf("Error parsing command line: at '%s': argument %s duplicate\n", task.c_str(), requiredArg.Name.c_str());
                            return 1;
                        }
                        TaskArgument resolvedArg = requiredArg;
                        if(!ParseUserArgument(resolvedArg, argsStack))
                            return 1;
                        resolvedRequired.push_back(resolvedArg);
                        bResolvedThisArg = true;
                        break;
                    }
                }
                if(bResolvedThisArg)
                    continue;
                for(const auto& optionalArg: curTask.OptionalArguments)
                {
                    if(MatchArgument(arg, optionalArg))
                    {
                        if(!optionalArg.Multiple && MatchesAnyArgument(arg, resolvedOptional))
                        {
                            printf("Error parsing command line: at '%s': argument %s duplicate\n", task.c_str(), optionalArg.Name.c_str());
                            return 1;
                        }
                        TaskArgument resolvedArg = optionalArg;
                        if(!ParseUserArgument(resolvedArg, argsStack))
                            return 1;
                        resolvedOptional.push_back(resolvedArg);
                        bResolvedThisArg = true;
                        break;
                    }
                }
                if(bResolvedThisArg)
                    continue;
                if(ResolveTask(availableTasks, arg).Name.length())
                {
                    argsStack.push_back(std::move(arg));
                    break;
                }
                printf("Error parsing command line: at '%s': extraneous unknown argument '%s'\n", task.c_str(), arg.c_str());
                return 1;
            }
            for(const auto& require: curTask.RequiredArguments)
            {
                if(MatchesAnyArgument(require.Name, resolvedRequired))
                    continue;
                printf("Error parsing command line: at '%s': required argument '%s' was not supplied\n", task.c_str(), require.Name.c_str());
                return 1;
            }
            resolvedRequired.insert(resolvedRequired.end(), std::make_move_iterator(resolvedOptional.begin()), std::make_move_iterator(resolvedOptional.end()));
            return curTask.Executor(resolvedRequired);
        }
        
        TaskInfo ResolveTask(const std::vector<TaskInfo>& tasks, String stackArg)
        {
            for(const auto& task: tasks)
                if(CompareCaseInsensitive(stackArg, task.Name))
                    return task;
            return {};
        }
        
        Bool ParseUserArgument(TaskArgument& arg, std::vector<String>& argsStack)
        {
            if(arg.Type == ArgumentType::NONE)
                return true;
            if(argsStack.size() == 0)
            {
                printf("Error parsing command line: at argument %s: expected a value\n", arg.Name.c_str());
                return false;
            }
            if(arg.Type == ArgumentType::BOOL)
            {
                String boolVal = argsStack.back();
                argsStack.pop_back();
                if(boolVal[0] == 't' || boolVal[0] == 'T' ||  boolVal[0] == 'y' || boolVal[0] == 'Y')
                    arg.BoolValue = true;
                else if(boolVal[0] == 't' || boolVal[0] == 'T' ||  boolVal[0] == 'y' || boolVal[0] == 'Y')
                    arg.BoolValue = false;
                else
                {
                    printf("Error parsing command line: at argument %s: invalid boolean '%s'\n", arg.Name.c_str(), boolVal.c_str());
                    return false;
                }
            }
            else if(arg.Type == ArgumentType::INT)
            {
                String intVal = argsStack.back();
                argsStack.pop_back();
                I32 val{};
                if(sscanf(intVal.c_str(), "%d", &val) != 1)
                {
                    printf("Error parsing command line: at argument %s: invalid integer '%s'\n", arg.Name.c_str(), intVal.c_str());
                    return false;
                }
                arg.IntValue = val;
            }
            else if(arg.Type == ArgumentType::STRING)
            {
                String strVal = argsStack.back();
                argsStack.pop_back();
                arg.StringValue = strVal;
            }
            else
            {
                TTE_ASSERT(false, "Internal error");
                return false;
            }
            return true;
        }
        
        Bool MatchesAnyArgument(String userArg, const std::vector<TaskArgument>& test)
        {
            for(const auto& task: test)
                if(MatchArgument(userArg, task))
                    return true;
            return false;
        }
        
        Bool MatchArgument(String userArg, const TaskArgument& test)
        {
            if(CompareCaseInsensitive(test.Name, userArg))
                return true;
            for(const auto& arg: test.Aliases)
            {
                if(CompareCaseInsensitive(userArg, arg))
                    return true;
            }
            return false;
        }
        
    }
    
    std::vector<String> ParseArgsStack(int argc, char** argv)
    {
        std::vector<String> args{};
        for (int i = argc-1; i >= 1; i--)
            args.emplace_back(argv[i]);
        return args;
    }
    
    I32 GuardedMain(int argc, char** argv)
    {
        struct _PadOnDestruct {~_PadOnDestruct() {printf("\n");}} _{};
        printf("\n"); // Some consoles print some garbage without terminator. Skip that.
        std::vector<TaskInfo> tasks = CreateTasks();
        std::vector<String> argsStack = ParseArgsStack(argc, argv);
        if(argsStack.size() == 0)
        {
            for(auto& task: tasks)
            {
                if(task.DefaultTask)
                    return task.Executor({});
            }
            printf("No default command line executor found: exiting quietly...\n");
        }
        else
        {
            while(argsStack.size())
            {
                I32 result = _Impl::ExecuteTask(argsStack, tasks);
                if(result != 0)
                    return result;
            }
        }
        return 0;
    }
    
}
