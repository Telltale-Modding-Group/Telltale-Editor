#pragma once

#include <Core/Config.hpp>

// This is the low level LUA API. This deals with calls to the lua library for compilation and running of scripts. Decompilation is also managed here
// in terms of luadec.

// IMPORTANT: *NO* lua C headers are to be included anywhere APART from the adapter translation units (Scripting/Adapter/Lua5**.cpp)!

// Supported LUA versions which telltale use in release games.
enum class LuaVersion
{
    LUA_NONE,  // No version set
    LUA_5_2_3, // Lua 5.2.3
    LUA_5_1_4, // Lua 5.1.4
    LUA_5_0_2  // Lua 5.0.2
};

enum class LuaOp
{
    EQ, //==
    LT, //<
    LE, //<=
};

enum class LuaType
{
    NONE = -1,
    NIL = 0,
    BOOL = 1,
    LIGHT_OPAQUE = 2,
    NUMBER = 3,
    STRING = 4,
    TABLE = 5,
    FUNCTION = 6,
    // full opaque is not exposed (buffers. see lua doc)
    // THREAD = 8,
};

// For use below
class LuaAdapterBase;
class LuaManager;

// Used interally for lua states for defined in each Lua version.
struct lua_State;

// User defined C function proto. Return number of return values.
using LuaCFunction = U32 (*)(LuaManager &L);

// Lua Manager class for managing lua calling and versioning. Not thread safe.
class LuaManager
{
  public:
    // Initialises, must be called after construction, the lua manager with the given version. Only one instance can run per version!
    void Initialise(LuaVersion Vers);

    // Runs a chunk of uncompiled lua source. Pass in the C string and its length. Pass in the Name of the lua file as the optional last argument.
    Bool RunText(CString Code, U32 Len, CString Name = "defaultrun.lua");

    // Calls the function along with its arguments on the stack. Push the function FIRST, then Nargs arguments. Then call this.
    // Use LUA_MULTRET for Nresults if you want all returned values, else use a lower number to cap the number of return values pushed.
    void CallFunction(U32 Nargs, U32 Nresults);

    // Loads a lua chunk, isCompiled being if its complied else source, into the Lua VM.
    Bool LoadChunk(const String &nm, const U8 *chunk, U32 chunkSizeBytes, Bool isCompiled);

    // Checks if we can add <extra> elements onto the stack without causing a stack overflow.
    Bool CheckStack(U32 extra);

    // Pushes a bool to the stack.
    void PushBool(Bool value);

    // Pushes _G/_ENV table onto the stack
    void PushEnv();

    // Pushes a signed integer onto the stack
    void PushInteger(I32 value);

    // Pushes a userdata pointer onto the stack
    void PushOpaque(void *value);

    // Pushes a string onto the stack
    void PushLString(String value);

    // Pushes nil onto the stack
    void PushNil();

    // Pushes a float onto the stack
    void PushFloat(Float value);

    // Pushes a new empty table onto the stack
    void PushTable();

    // Pops N elements from the stack
    void Pop(U32 N);

    // Pushes a C function defined in this codespace onto the stack.
    void PushFn(LuaCFunction);

    // Returns the positive index of the top of the stack.
    I32 GetTop();

    // Pushes and gets a table element. Push the key on the top of the stack, which will be popped. Index is the table index on the stack.
    // Set raw to true such that no metamethods are executed. Pushes nil if not found (tables cannot contain nil).
    void GetTable(I32 index, Bool bRaw = false);

    // Sets a table element. Push the key THEN the value such that those are on the top of the stack. Index is the table index on the stack.
    // Set raw to true such that no metamethods are executed. Key and value are popped.
    void SetTable(I32 index, Bool bRaw = false);

    // Sets a table element without any metamethods. Table is on the stack at index. arrayIndex is the index into the table, 1 being first,
    // to set. Value is popped from top of stack to be set, so index cannot be -1.
    void SetTableRaw(I32 index, I32 arrayIndex);

    // Pushes a table element without any metamethods. Table is on the stack at index. arrayIndex is the index into the table, 1 being first,
    // to set.
    void GetTableRaw(I32 index, I32 arrayIndex);

    // Iterate through table elements. index points to the table on the stack to be traversed. The function pops a key from the stack,
    // and pushes a key,value pair from the table (the "next" pair after the given key). When there are no more elements, it returns 0
    // (and pushes nothing). Use a nil key to signal the start of a traversal.
    I32 TableNext(I32 index);

    // May return NONE if its an internal invalid type (eg thread/full opaque). Returns type of element on the stack at index.
    LuaType Type(I32 index);

    // Performs the LuaOp on elements on the stack at lhs (left) and rhs (right hand side). Only performs less,less equal, equal.
    // Switch the lhs and rhs indices to perform greater than checks.
    Bool Compare(I32 lhs, I32 rhs, LuaOp op);

    // Returns the typename of a given lua type.
    CString Typename(LuaType);

    // Pushes onto the stack the metatable of the value at the given index.
    // If the value does not have a metatable, the function returns false and pushes nothing on the stack.
    Bool GetMetatable(I32 index);

    // Pops a table from the stack and sets it as the new metatable for the value at the given index. Returns false when it cannot set the
    // metatable of the given object (that is, when the object is neither a userdata nor a table); even then it pops the table from the stack.
    Bool SetMetatable(I32 index);

    // Accepts any acceptable index, or 0, and sets the stack top to that index. If the new top is larger than the old one,
    // then the new elements are filled with nil. If index is 0, then all stack elements are removed.
    void SetTop(I32 index);

    // Pushes on the stack a copy of the element at stack index index.
    void PushCopy(I32 index);

    // Removes from the stack the element at index.
    void Remove(I32 index);

    // Moves the top element into the given position, shifting up the elements above that position to open space.
    void Insert(I32 index);

    // Moves the top element into the given position, without shifting any element (therefore replacing the value at the given position).
    void Replace(I32 index);

    // Gets the boolean value at index on the stack
    Bool ToBool(I32 index);

    // Gets the float value at index on the stack
    Float ToFloat(I32 index);

    // Gets the string value at index on the stack
    String ToString(I32 index);

    // Gets the userdata pointer value at index on the stack
    void *ToPointer(I32 index);

    // Gets the integer at index on the stack
    I32 ToInteger(I32 index);

    // Push an error string first. Generates a lua error.
    void Error();

    // Gets the stack index for the given upvalue index for the current function. Index must be 1 or higher.
    I32 UpvalueIndex(I32 index);

    // Default constructor.
    LuaManager() = default;

    // Releases lua.
    ~LuaManager();

    // Gets the version of this lua manager.
    inline LuaVersion GetVersion() { return _Version; }

  private:
    LuaVersion _Version = LuaVersion::LUA_NONE; // This lua version

    LuaAdapterBase *_Adapter = nullptr; // Adapter for the version, see below
};

// Base class for lua version adapters (see below for each). Each includes the lua headers from the given version in its own translation unit for
// defining these functions.
class LuaAdapterBase
{
  protected:
    LuaManager &_Manager;
    lua_State *_State = nullptr; // lua_State specific to the verison.

  public:
    virtual void Initialise() = 0;

    virtual void Shutdown() = 0;

    virtual Bool RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name) = 0;

    virtual void CallFunction(U32 Nargs, U32 Nresults) = 0;

    virtual Bool CheckStack(U32 extra) = 0;

    // type being 999 means env, 888 means integer
    virtual void Push(LuaType type, void *pValue) = 0;

    virtual void Pop(U32 N) = 0;

    virtual I32 GetTop() = 0;

    virtual Bool LoadChunk(const String &nm, const U8 *, U32, Bool) = 0;

    virtual void GetTable(I32 index, Bool bRaw) = 0;

    virtual void SetTable(I32 index, Bool bRaw) = 0;

    virtual void SetTableRaw(I32 index, I32 arrayIndex) = 0;

    virtual void GetTableRaw(I32 index, I32 arrayIndex) = 0;

    virtual I32 TableNext(I32 index) = 0;

    virtual LuaType Type(I32 index) = 0;

    virtual Bool Compare(I32 lhs, I32 rhs, LuaOp op) = 0;

    virtual CString Typename(LuaType) = 0;

    virtual Bool GetMetatable(I32 index) = 0;

    virtual Bool SetMetatable(I32 index) = 0;

    virtual void SetTop(I32 index) = 0;

    virtual void PushCopy(I32 index) = 0;

    virtual void Remove(I32 index) = 0;

    virtual void Insert(I32 index) = 0;

    virtual void Replace(I32 index) = 0;

    virtual Bool ToBool(I32 index) = 0;

    virtual Float ToFloat(I32 index) = 0;

    virtual String ToString(I32 index) = 0;

    virtual void *ToPointer(I32 index) = 0;

    virtual I32 ToInteger(I32 index) = 0;

    virtual void Error() = 0;

    virtual I32 UpvalueIndex(I32 index) = 0;

    inline LuaAdapterBase(LuaManager &manager) : _Manager(manager) {}

    inline virtual ~LuaAdapterBase() {}
};

// For Lua 5.2.3
class LuaAdapter_523 : public LuaAdapterBase
{
  public:
    inline LuaAdapter_523(LuaManager &_Man) : LuaAdapterBase(_Man) {}

    void Initialise() override;

    void Shutdown() override;

    virtual Bool RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name) override;

    virtual Bool LoadChunk(const String &nm, const U8 *, U32, Bool) override;

    virtual void CallFunction(U32 Nargs, U32 Nresults) override;

    virtual Bool CheckStack(U32 extra) override;

    virtual void Push(LuaType type, void *pValue) override;

    virtual void Pop(U32 N) override;

    virtual I32 GetTop() override;

    virtual void GetTable(I32 index, Bool bRaw) override;

    virtual void SetTable(I32 index, Bool bRaw) override;

    virtual void SetTableRaw(I32 index, I32 arrayIndex) override;

    virtual void GetTableRaw(I32 index, I32 arrayIndex) override;

    virtual I32 TableNext(I32 index) override;

    virtual LuaType Type(I32 index) override;

    virtual Bool Compare(I32 lhs, I32 rhs, LuaOp op) override;

    virtual CString Typename(LuaType) override;

    virtual Bool GetMetatable(I32 index) override;

    virtual Bool SetMetatable(I32 index) override;

    virtual void SetTop(I32 index) override;

    virtual void PushCopy(I32 index) override;

    virtual void Remove(I32 index) override;

    virtual void Insert(I32 index) override;

    virtual void Replace(I32 index) override;

    virtual Bool ToBool(I32 index) override;

    virtual Float ToFloat(I32 index) override;

    virtual String ToString(I32 index) override;

    virtual void *ToPointer(I32 index) override;

    virtual I32 ToInteger(I32 index) override;

    virtual void Error() override;

    virtual I32 UpvalueIndex(I32 index) override;
};

// For Lua 5.1.4
class LuaAdapter_514 : public LuaAdapterBase
{
  public:
    inline LuaAdapter_514(LuaManager &_Man) : LuaAdapterBase(_Man) {}

    void Initialise() override;

    void Shutdown() override;

    virtual Bool RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name) override;

    virtual Bool LoadChunk(const String &nm, const U8 *, U32, Bool) override;

    virtual void CallFunction(U32 Nargs, U32 Nresults) override;

    virtual Bool CheckStack(U32 extra) override;

    virtual void Push(LuaType type, void *pValue) override;

    virtual void Pop(U32 N) override;

    virtual I32 GetTop() override;

    virtual void GetTable(I32 index, Bool bRaw) override;

    virtual void SetTable(I32 index, Bool bRaw) override;

    virtual void SetTableRaw(I32 index, I32 arrayIndex) override;

    virtual void GetTableRaw(I32 index, I32 arrayIndex) override;

    virtual I32 TableNext(I32 index) override;

    virtual LuaType Type(I32 index) override;

    virtual Bool Compare(I32 lhs, I32 rhs, LuaOp op) override;

    virtual CString Typename(LuaType) override;

    virtual Bool GetMetatable(I32 index) override;

    virtual Bool SetMetatable(I32 index) override;

    virtual void SetTop(I32 index) override;

    virtual void PushCopy(I32 index) override;

    virtual void Remove(I32 index) override;

    virtual void Insert(I32 index) override;

    virtual void Replace(I32 index) override;

    virtual Bool ToBool(I32 index) override;

    virtual Float ToFloat(I32 index) override;

    virtual String ToString(I32 index) override;

    virtual I32 ToInteger(I32 index) override;

    virtual void *ToPointer(I32 index) override;

    virtual void Error() override;

    virtual I32 UpvalueIndex(I32 index) override;
};

// For Lua 5.0.2
class LuaAdapter_502 : public LuaAdapterBase
{
  public:
    inline LuaAdapter_502(LuaManager &_Man) : LuaAdapterBase(_Man) {}

    void Initialise() override;

    void Shutdown() override;

    virtual Bool RunChunk(U8 *Chunk, U32 Len, Bool IsCompiled, CString Name) override;

    virtual Bool LoadChunk(const String &nm, const U8 *, U32, Bool) override;

    virtual void CallFunction(U32 Nargs, U32 Nresults) override;

    virtual Bool CheckStack(U32 extra) override;

    virtual void Push(LuaType type, void *pValue) override;

    virtual void Pop(U32 N) override;

    virtual I32 GetTop() override;

    virtual void GetTable(I32 index, Bool bRaw) override;

    virtual void SetTable(I32 index, Bool bRaw) override;

    virtual void SetTableRaw(I32 index, I32 arrayIndex) override;

    virtual void GetTableRaw(I32 index, I32 arrayIndex) override;

    virtual I32 TableNext(I32 index) override;

    virtual LuaType Type(I32 index) override;

    virtual Bool Compare(I32 lhs, I32 rhs, LuaOp op) override;

    virtual CString Typename(LuaType) override;

    virtual Bool GetMetatable(I32 index) override;

    virtual Bool SetMetatable(I32 index) override;

    virtual void SetTop(I32 index) override;

    virtual void PushCopy(I32 index) override;

    virtual void Remove(I32 index) override;

    virtual void Insert(I32 index) override;

    virtual void Replace(I32 index) override;

    virtual Bool ToBool(I32 index) override;

    virtual Float ToFloat(I32 index) override;

    virtual String ToString(I32 index) override;

    virtual void *ToPointer(I32 index) override;

    virtual void Error() override;

    virtual I32 UpvalueIndex(I32 index) override;

    virtual I32 ToInteger(I32 index) override;
};
