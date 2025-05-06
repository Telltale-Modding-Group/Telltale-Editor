#pragma once

#include <Core/Config.hpp>
#include <Resource/ResourceRegistry.hpp>
#include <Scripting/ScriptManager.hpp>
#include <Core/BitSet.hpp>
#include <Core/Math.hpp>

#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_gamepad.h>

// IMAP codes. Thank god telltale didn't change these once from texas holem all up to sam and max remaster.
// Some codes are more specific than others but still work
enum class InputCode : U32 {
    
    // T3 COMMON INPUT CODES. (PLATFORM SPECIFIC BELOW, WHICH MAP TO THESE)
    
    COMMON_MAPPINGS_START = 0x0,
    
    NONE = 0x0,
    
    BACKSPACE = 0x8,
    TAB = 0x9,
    CLEAR = 0xC,
    RETURN = 0xD,
    SHIFT = 0x10,
    CONTROL = 0x11,
    ALT = 0x12,
    PAUSE = 0x13,
    CAPS_LOCK = 0x14,
    ESCAPE = 0x1B,
    SPACE_BAR = 0x20,
    PAGE_UP = 0x21,
    PAGE_DOWN = 0x22,
    END = 0x23,
    HOME = 0x24,
    
    LEFT_ARROW = 0x25,
    UP_ARROW = 0x26,
    RIGHT_ARROW = 0x27,
    DOWN_ARROW = 0x28,
    
    PRINT_SCREEN = 0x2A,
    SNAPSHOT = 0x2C,
    INSERT = 0x2D,
    DELETE = 0x2E,
    HELP = 0x2F,  // 'Delp' in exe?
    
    ZERO = 0x30, // ASCII for the numbers
    ONE = 0x31,
    TWO = 0x32,
    THREE = 0x33,
    FOUR = 0x34,
    FIVE = 0x35,
    SIX = 0x36,
    SEVEN = 0x37,
    EIGHT = 0x38,
    NINE = 0x39,
    
    // ASCII
    A = 0x41, B = 0x42, C = 0x43, D = 0x44, E = 0x45, F = 0x46, G = 0x47, H = 0x48, I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C, M = 0x4D,
    N = 0x4E, O = 0x4F, P = 0x50, Q = 0x51, R = 0x52, S = 0x53, T = 0x54, U = 0x55, V = 0x56, W = 0x57, X = 0x58, Y = 0x59, Z = 0x5A,
    
    WINDOWS_LEFT = 0x5B,
    WINDOWS_RIGHT = 0x5C,
    WINDOWS_APPLICATION = 0x5D, // 'VK_APPS'
    
    NUMPAD_0 = 0x60,
    NUMPAD_1 = 0x61,
    NUMPAD_2 = 0x62,
    NUMPAD_3 = 0x63,
    NUMPAD_4 = 0x64,
    NUMPAD_5 = 0x65,
    NUMPAD_6 = 0x66,
    NUMPAD_7 = 0x67,
    NUMPAD_8 = 0x68,
    NUMPAD_9 = 0x69,
    NUMPAD_STAR = 0x6A,
    NUMPAD_PLUS = 0x6B,
    NUMPAD_MINUS = 0x6D,
    NUMPAD_DOT = 0x6E,
    NUMPAD_SLASH = 0x6F,
    
    F1 = 0x70, F2 = 0x71, F3 = 0x72, F4 = 0x73, F5 = 0x74, F6 = 0x75, F7 = 0x76, F8 = 0x77, F9 = 0x78, F10 = 0x79, F11 = 0x7A, F12 = 0x7B,
    
    NUM_LOCK = 0x90,
    SCROLL_LOCK = 0x91,
    LEFT_SHIFT = 0xA0,
    RIGHT_SHIFT = 0xA1,
    LEFT_CONTROL = 0xA2,
    RIGHT_CONTROL = 0xA3,
    LEFT_MENU = 0xA4,
    RIGHT_MENU = 0xA5,
    
    COLON = 0xBA,
    EQUALS = 0xBB,
    COMMA = 0xBC,
    MINUS = 0xBD,
    FULL_STOP = 0xBE,
    SLASH = 0xBF,
    TILDE = 0xC0, // ~
    LEFT_BRACKET = 0xDB,
    RIGHT_BRACKET = 0xDD,
    BACKSLASH = 0xDC,
    QUOTE = 0xDE,

    ENTER = 0x100,
    
    GAMEPAD_0 = 0x200,
    GAMEPAD_1 = 0x201,
    GAMEPAD_2 = 0x202,
    GAMEPAD_3 = 0x203,
    GAMEPAD_4 = 0x204,
    GAMEPAD_5 = 0x205,
    GAMEPAD_6 = 0x206,
    GAMEPAD_7 = 0x207,
    GAMEPAD_8 = 0x208,
    GAMEPAD_9 = 0x209,
    GAMEPAD_10 = 0x20A,
    GAMEPAD_11 = 0x20B,
    GAMEPAD_DPAD_UP = 0x20C,
    GAMEPAD_DPAD_DOWN = 0x20D,
    GAMEPAD_DPAD_RIGHT = 0x20E,
    GAMEPAD_DPAD_LEFT = 0x20F,
    
    ABSTRACT_CONFIRM = 0x300, // any enter, yes, etc action (left mouse mapped to this)
    ABSTRACT_CANCEL = 0x301, // cancel buttons etc (right mouse mapped to this)
    MOUSE_MIDDLE = 0x302,
    MOUSE_X = 0x303,
    MOUSE_DOUBLE_LEFT = 0x304,
    MOUSE_MOVE = 0x310,
    MOUSE_ROLLOVER = 0x320,
    MOUSE_WHEEL_UP = 0x321,
    MOUSE_WHEEL_DOWN = 0x322,
    MOUSE_LEFT_BUTTON = 0x330,
    MOUSE_RIGHT_BUTTON = 0x331,
    
    LEFT_STICK_MOVE = 0x400, // when any stick moves
    RIGHT_STICK_MOVE = 0x401,
    TRIGGER_MOVE = 0x402,
    LEFT_TRIGGER_MOVE = 0x403,
    RIGHT_TRIGGER_MOVE = 0x404,
    
    GESTURE_2TOUCH_SINGLE_TAP = 0x500,
    GESTURE_2TOUCH_DOUBLE_TAP = 0x501,
    GESTURE_1TOUCH_SWIPE_LEFT = 0x510,
    GESTURE_1TOUCH_SWIPE_UP = 0x511,
    GESTURE_1TOUCH_SWIPE_RIGHT = 0x512,
    GESTURE_1TOUCH_SWIPE_DOWN = 0x513,
    GESTURE_2TOUCHES_SWITCH_LEFT = 0x514,
    GESTURE_2TOUCHES_SWITCH_UP = 0x515,
    GESTURE_2TOUCHES_SWITCH_RIGHT = 0x516,
    GESTURE_2TOUCHES_SWITCH_DOWN = 0x517,
    GESTURE_2TOUCHES_DOWN = 0x518,
    GESTURE_3TOUCHES_DOWN = 0x519,
    
    COMMON_MAPPINGS_END = 0x1000,
    PLATFORM_MAPPINGS_START = 0x1000,
    
    // PLATFORM SPECIFICS. byte 2: 0x11 => XB360, 0x12 => WIIU, 0x13 => PS3, 0x14 => iPhone, 0x15 => Vita, 0x16 => Apple: Gamepads, tvOS
    // 0x17 => PS4, 0x18 => XBOne, 0x19 => Switch
    
    PC_MOUSE_LEFT = 0x1080, // left click. PC / mac / linux
    PC_MOUSE_RIGHT = 0x1081,
    
    XB360_A = 0x1100,
    XB360_B = 0x1101,
    XB360_X = 0x1102,
    XB360_Y = 0x1103,
    XB360_L = 0x1104,
    XB360_R = 0x1105,
    XB360_LEFT_TRIGGER = 0x1106,
    XB360_RIGHT_TRIGGER = 0x1107,
    XB360_START = 0x1108,
    XB360_BACK = 0x1109,
    XB360_LEFT_STICK = 0x110A,
    XB360_RIGHT_STICK = 0x110B,
    XB360_LEFT = 0x110C,
    XB360_RIGHT = 0x110D,
    XB360_UP = 0x110E,
    XB360_DOWN = 0x110F,
    
    WIIU_A = 0x1200,
    WIIU_B = 0x1201,
    WIIU_X = 0x1202,
    WIIU_Y = 0x1203,
    WIIU_L = 0x1204,
    WIIU_R = 0x1205,
    WIIU_ZL = 0x1206,
    WIIU_ZR = 0x1207,
    WIIU_PLUS = 0x1208,
    WIIU_MINUS = 0x1209,
    WIIU_LEFT_STICK = 0x120A,
    WIIU_RIGHT_STICK = 0x120B,
    WIIU_LEFT = 0x120C,
    WIIU_RIGHT = 0x120D,
    WIIU_UP = 0x120E,
    WIIU_DOWN = 0x120F,
    
    PS3_SQUARE = 0x1300,
    PS3_TRIANGLE = 0x1301,
    PS3_CIRCLE = 0x1302,
    PS3_CROSS = 0x1303,
    PS3_L1 = 0x1304,
    PS3_L2 = 0x1305,
    PS3_L3 = 0x1306,
    PS3_R1 = 0x1307,
    PS3_R2 = 0x1308,
    PS3_R3 = 0x1309,
    PS3_START = 0x130A,
    PS3_SELECT = 0x130B,
    PS3_LEFT = 0x130C,
    PS3_RIGHT = 0x130D,
    PS3_UP = 0x130E,
    PS3_DOWN = 0x130F,
    
    IOS_DOUBLE_TAP = 0x1400,
    
    VITA_SQUARE = 0x1500,
    VITA_TRIANGLE = 0x1501,
    VITA_CIRCLE = 0x1502,
    VITA_CROSS = 0x1503,
    VITA_L1 = 0x1504,
    VITA_L2 = 0x1505,
    VITA_L3 = 0x1506,
    VITA_R1 = 0x1507,
    VITA_R2 = 0x1508,
    VITA_R3 = 0x1509,
    VITA_START = 0x150A,
    VITA_SELECT = 0x150B,
    VITA_LEFT = 0x150C,
    VITA_RIGHT = 0x150D,
    VITA_UP = 0x150E,
    VITA_DOWN = 0x150F,
    
    APPLE_GAMEPAD_A = 0x1600,
    APPLE_GAMEPAD_B = 0x1601,
    APPLE_GAMEPAD_X = 0x1602,
    APPLE_GAMEPAD_Y = 0x1603,
    APPLE_GAMEPAD_L = 0x1604,
    APPLE_GAMEPAD_R = 0x1605,
    APPLE_GAMEPAD_LEFT = 0x1606,
    APPLE_GAMEPAD_RIGHT = 0x1607,
    APPLE_GAMEPAD_UP = 0x1608,
    APPLE_GAMEPAD_DOWN = 0x1609,
    
    PS4_SQUARE = 0x1700,
    PS4_TRIANGLE = 0x1701,
    PS4_CIRCLE = 0x1702,
    PS4_CROSS = 0x1703,
    PS4_L1 = 0x1704,
    PS4_L2 = 0x1705,
    PS4_L3 = 0x1706,
    PS4_R1 = 0x1707,
    PS4_R2 = 0x1708,
    PS4_R3 = 0x1709,
    PS4_OPTIONS = 0x170A,
    PS4_TOUCHPAD = 0x170B,
    PS4_LEFT = 0x170C,
    PS4_RIGHT = 0x170D,
    PS4_UP = 0x170E,
    PS4_DOWN = 0x170F,
    
    XBONE_A = 0x1800,
    XBONE_B = 0x1801,
    XBONE_X = 0x1802,
    XBONE_Y = 0x1803,
    XBONE_L = 0x1804,
    XBONE_R = 0x1805,
    XBONE_LEFT_TRIGGER = 0x1806,
    XBONE_RIGHT_TRIGGER = 0x1807,
    XBONE_START = 0x1808,
    XBONE_BACK = 0x1809,
    XBONE_LEFT_STICK = 0x180A,
    XBONE_RIGHT_STICK = 0x180B,
    XBONE_LEFT = 0x180C,
    XBONE_RIGHT = 0x180D,
    XBONE_UP = 0x180E,
    XBONE_DOWN = 0x180F,
    
    NX_A = 0x1900,
    NX_B = 0x1901,
    NX_X = 0x1902,
    NX_Y = 0x1903,
    NX_L = 0x1904, // Bug in engine, they forgot to add get name for this
    NX_R = 0x1905,
    NX_ZL = 0x1906,
    NX_ZR = 0x1907,
    NX_PLUS = 0x1908,
    NX_MINUS = 0x1909,
    NX_LEFT_STICK = 0x190A,
    NX_RIGHT_STICK = 0x190B,
    NX_LEFT = 0x190C,
    NX_RIGHT = 0x190D,
    NX_UP = 0x190E,
    NX_DOWN = 0x190F,
    NX_LEFT_SL = 0x1910,
    NX_LEFT_SR = 0x1911,
    NX_RIGHT_SL = 0x1912,
    NX_RIGHT_SR = 0x1913,
    
    PLATFORM_MAPPINGS_END = 0x2000,
    
};

struct InputCodeDesc
{
    
    InputCode T3InputCode;
    U32 SDLCode;
    CString Name;
    SDL_GamepadButton Controller = SDL_GAMEPAD_BUTTON_INVALID;
    SDL_GamepadAxis ControllerAxis = SDL_GAMEPAD_AXIS_INVALID;
    
};

extern const InputCodeDesc InputCodeDescs[];

class RuntimeInputEvent;

class InputMapper : public Handleable
{
public:
    
    enum class EventType
    {
        BEGIN,
        END,
        MOUSE_MOVE,
        BEGIN_OR_END,
        FORCE,
    };
    
    struct Mapping
    {
        InputCode Code;
        EventType Type;
        String ScriptFunction;
        I32 ControllerIndexOverride;
    };
    
    // Registers scene normalisers and specialisers
    static void RegisterScriptAPI(LuaFunctionCollection& Col);
    
    // Parses the given SDL event. If its a runtime input event, puts into output event and returns true. May create multiple for multiple mappings
    static void ConvertRuntimeEvents(const SDL_Event& sdlEvent, std::vector<RuntimeInputEvent>& events, Bool bInPrimaryWindow, Vector2 windowSize);
    
    virtual void FinaliseNormalisationAsync() override;
    
    static constexpr Bool IsCommonInputCode(InputCode code) { return ((U32)code & 0x1000) == 0 && (((U32)code & 0xFFF) != 0); }
    
    static constexpr Bool IsPlatformInputCode(InputCode code) { return ((U32)code & 0x1000) != 0; }
    
    static String GetInputCodeName(InputCode key);
    
    static InputCode GetInputCode(String name);
    
    inline InputMapper(Ptr<ResourceRegistry> reg) : Handleable(reg) {}
    
private:
    
    friend class InputMapperAPI;
    
    std::vector<Mapping> _EventMappings;
    
    String _Name;
    
};

// Setup by the Lua for game switches, this is the platform input mapper which maps platform codes to common input codes. These then get processed by IMAPS.
class PlatformInputMapper
{
    
    String _Platform;
    
    BitSetRanged<InputCode, InputCode::COMMON_MAPPINGS_START, InputCode::COMMON_MAPPINGS_END> _CommonKeyFlags; // for fast map. if key is 1, then has a mapping
    BitSetRanged<InputCode, InputCode::PLATFORM_MAPPINGS_START, InputCode::PLATFORM_MAPPINGS_END> _PlatformKeyFlags; // for fast map. if key is 1, then has a mapping
    
    struct EventMapping
    {
        U32 PlatformInputCode;
        U32 CommonInputCode;
    };
    
    std::vector<EventMapping> _MappedEvents;
    
    std::mutex _Lock;
    
    void _AddEvent(U32 platformKey, U32 key); // not thread safe done in init
    
public:
    
    // queues a platform event. if its a platform key with no mapping, its ignored. all common ones go through always.
    // result is always a common key or is none if it should be ignored
    static void QueuePlatformEvent(RuntimeInputEvent event, std::vector<RuntimeInputEvent>& outEvents);
    
    static void GetMappingForInput(InputCode key, std::vector<InputCode>& outPlatformKeys); // safely gets mappings a common key
    
    static void Initialise(String platformName);
    static void Shutdown();
    
};

// ============================================= RUNTIME EVENT INPUT MAPPINGS =============================================

// Runtime event for input. In terms of the layer stack, these get passed *DOWN*
class RuntimeInputEvent
{
public:
    
    enum Flag
    {
        EVENT_HANDLED = 1, // already handled by a higher level. You can still process it.
        EVENT_PRIMARY_WINDOW = 2, // primary window is currently focused. may be ignored for multiple windows.
    };
    
    mutable Flags EventFlags;
    InputCode Code; // Mouse L, etc
    InputMapper::EventType Type; // Begin, End, etc
    Float X, Y; // if a pressure value etc, then stored in X. secondary values in Y.
    
    // THINK about using this event if its been handled already. In editor, if it has, tend to ignore. Unless trying to quit scene runtime for example.
    inline Bool BeenHandled() const { return EventFlags.Test(EVENT_HANDLED); }
    
    // Note this can be done even in const!
    inline void SetHandled() const { EventFlags.Add(EVENT_HANDLED); }
    
};
