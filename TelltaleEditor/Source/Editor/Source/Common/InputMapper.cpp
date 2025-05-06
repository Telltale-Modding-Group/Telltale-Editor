#include <Common/InputMapper.hpp>

#include <sstream>

// =========================== INPUT MAPPER (IMAP)

class InputMapperAPI
{
public:
    
    static U32 luaIMAPSetName(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 2, "Incorrect usage");
        InputMapper* imap = (InputMapper*)man.ToPointer(1);
        imap->_Name = man.ToString(2);
        return 0;
    }
    
    static U32 luaIMAPPushMapping(LuaManager& man)
    {
        TTE_ASSERT(man.GetTop() == 5, "Incorrect usage");
        InputMapper* imap = (InputMapper*)man.ToPointer(1);
        InputMapper::Mapping mapping{};
        mapping.Code = (InputCode)man.ToInteger(2);
        mapping.Type = (InputMapper::EventType)man.ToInteger(3);
        mapping.ScriptFunction = man.ToString(4);
        mapping.ControllerIndexOverride = man.ToInteger(5);
        
        imap->_EventMappings.push_back(mapping);
        return 0;
    }
    
};

void InputMapper::FinaliseNormalisationAsync()
{
    ;
}

void InputMapper::RegisterScriptAPI(LuaFunctionCollection& Col)
{
    PUSH_FUNC(Col, "CommonInputMapperSetName", &InputMapperAPI::luaIMAPSetName, "nil CommonInputMapperSetName(state, name)", "Set name of input mapper");
    PUSH_FUNC(Col, "CommonInputMapperPushMapping", &InputMapperAPI::luaIMAPPushMapping,
              "nil CommonInputMapperPushMapping(state, code, type, scriptFunction, controllerIndexOverride)", "Push a mapping to the input mapper common class");
    PUSH_GLOBAL_I(Col, "kCommonInputMapperTypeBegin", (U32)InputMapper::EventType::BEGIN, "Trigger when an event begins");
    PUSH_GLOBAL_I(Col, "kCommonInputMapperTypeEnd", (U32)InputMapper::EventType::END, "Trigger when an event ends");
    PUSH_GLOBAL_I(Col, "kCommonInputMapperTypeMouseMove", (U32)InputMapper::EventType::MOUSE_MOVE, "Trigger on mouse move event");
    PUSH_GLOBAL_I(Col, "kCommonInputMapperTypeForce", (U32)InputMapper::EventType::FORCE, "Trigger forced(?)");
    PUSH_GLOBAL_I(Col, "kCommonInputMapperTypeBeginOrEnd", (U32)InputMapper::EventType::BEGIN_OR_END,"Trigger on event begin or end");
}

// =========================== PLATFORM INPUT MAPPER

static PlatformInputMapper* _ActiveMapper = nullptr;

void PlatformInputMapper::_AddEvent(U32 platformKey, U32 key)
{
    TTE_ASSERT(InputMapper::IsCommonInputCode((InputCode)key) && InputMapper::IsPlatformInputCode((InputCode)platformKey), "Invalid code");
    EventMapping mapping{};
    mapping.PlatformInputCode = platformKey;
    mapping.CommonInputCode = key;
    for(auto& m: _MappedEvents)
    {
        if(m.CommonInputCode == key && m.PlatformInputCode == platformKey)
            return; // quietly ignore duplicate
    }
    _MappedEvents.push_back(std::move(mapping));
    _CommonKeyFlags.Set((InputCode)key, true);
    _PlatformKeyFlags.Set((InputCode)platformKey, true);
}

void PlatformInputMapper::Initialise(String platform)
{
    if(!_ActiveMapper)
    {
        _ActiveMapper = TTE_NEW(PlatformInputMapper, MEMORY_TAG_COMMON_INSTANCE);
        _ActiveMapper->_Platform = std::move(platform);
        
        LuaManager& L = GetThreadLVM();
        // Register platform mappings from lua
        ScriptManager::GetGlobal(L, "RegisterPlatformMappings", true);
        if(L.Type(-1) == LuaType::FUNCTION)
        {
            L.PushLString(_ActiveMapper->_Platform);
            L.CallFunction(1, 1, true);
            TTE_ASSERT(L.Type(-1) == LuaType::TABLE, "Incorrect return value");
            L.PushNil();
            while (L.TableNext(-2) != 0) // table of key to platform key
            {
                String commonKeys = ScriptManager::PopString(L); // pop value
                String platformKey = L.ToString(-1); // get table key
                InputCode pkey = InputMapper::GetInputCode(platformKey);
                if (pkey == InputCode::NONE)
                {
                    TTE_LOG("WARNING: Unknown platform input mappings: platform '%s' is invalid", platformKey.c_str());
                    continue;
                }
                else if (!InputMapper::IsPlatformInputCode(pkey))
                {
                    TTE_LOG("WARNING: The platform input code '%s' is incorrect.", platformKey.c_str());
                    continue;
                }
                
                std::stringstream ss(commonKeys);
                String segment;
                while (std::getline(ss, segment, ';'))
                {
                    segment = StringTrim(segment);
                    
                    InputCode ikey = InputMapper::GetInputCode(segment);
                    
                    if (ikey == InputCode::NONE)
                    {
                        TTE_LOG("WARNING: Unknown platform input mappings: common key '%s' is invalid", segment.c_str());
                        continue;
                    }
                    else if (!InputMapper::IsCommonInputCode(ikey))
                    {
                        TTE_LOG("WARNING: The common input code '%s' is incorrect.", platformKey.c_str());
                        continue;
                    }
                    
                    _ActiveMapper->_AddEvent((U32)pkey, (U32)ikey);
                }
            }

        }
        else
        {
            L.Pop(1);
            TTE_ASSERT(false, "The platform input mapper script registration function was not found!");
        }
        
    }
}

void PlatformInputMapper::Shutdown()
{
    if(_ActiveMapper)
        TTE_DEL(_ActiveMapper);
    _ActiveMapper = nullptr;
}

void PlatformInputMapper::GetMappingForInput(InputCode key, std::vector<InputCode> &outPlatformKeys)
{
    TTE_ASSERT(_ActiveMapper, "No mapper");
    std::lock_guard<std::mutex> G(_ActiveMapper->_Lock);
    if(InputMapper::IsCommonInputCode(key) && _ActiveMapper->_MappedEvents.size() && _ActiveMapper->_CommonKeyFlags[key])
    {
        for(auto& mapping: _ActiveMapper->_MappedEvents)
        {
            if(mapping.CommonInputCode == (U32)key)
                outPlatformKeys.push_back((InputCode)mapping.PlatformInputCode);
        }
    }
}

void PlatformInputMapper::QueuePlatformEvent(RuntimeInputEvent event, std::vector<RuntimeInputEvent>& outEvents)
{
    if(InputMapper::IsCommonInputCode(event.Code))
    {
        // Good already
        outEvents.push_back(std::move(event));
    }
    else
    {
        std::lock_guard<std::mutex> G(_ActiveMapper->_Lock);
        if(_ActiveMapper->_PlatformKeyFlags[event.Code])
        {
            for(auto& evnt: _ActiveMapper->_MappedEvents)
            {
                if(evnt.PlatformInputCode == (U32)event.Code)
                {
                    RuntimeInputEvent copy = event;
                    copy.Code = (InputCode)evnt.CommonInputCode;
                    outEvents.push_back(std::move(copy));
                }
            }
        } // else ignore the event
    }
}

// =========================== SDL EVENT CONVERSION

static Bool MapSDLEvent(RuntimeInputEvent& out, const SDL_Event& in, Vector2 windowSize)
{
    // This function can be widely extended to handle loads of other input types like touch screen gestures and controllers
    if(in.type == SDL_EVENT_KEY_UP || in.type == SDL_EVENT_KEY_DOWN)
    {
        out.Code = InputCode::NONE;
        const InputCodeDesc* pDesc = InputCodeDescs;
        while(pDesc->T3InputCode != InputCode::NONE)
        {
            if(pDesc->SDLCode == in.key.key)
            {
                out.Code = pDesc->T3InputCode;
                break;
            }
            pDesc++;
        }
        if(out.Code != InputCode::NONE)
        {
            out.Type = in.type == SDL_EVENT_KEY_UP ? InputMapper::EventType::END : InputMapper::EventType::BEGIN;
        }
        else return false; // other character we are not interested in
    }
    else if(in.type == SDL_EVENT_MOUSE_MOTION)
    {
        out.Code = InputCode::MOUSE_MOVE;
        out.Type = InputMapper::EventType::BEGIN;
        out.X = in.motion.x / windowSize.x; // position of mouse
        out.Y = in.motion.y / windowSize.y;
    }
    else if(in.type == SDL_EVENT_MOUSE_BUTTON_UP || in.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        out.Type = in.type == SDL_EVENT_MOUSE_BUTTON_UP ? InputMapper::EventType::END : InputMapper::EventType::BEGIN;
        if(in.button.button == SDL_BUTTON_LEFT)
            out.Code = in.button.clicks == 2 ? InputCode::MOUSE_DOUBLE_LEFT : InputCode::PC_MOUSE_LEFT;
        else if(in.button.button == SDL_BUTTON_RIGHT)
            out.Code = InputCode::PC_MOUSE_RIGHT;
        else if(in.button.button == SDL_BUTTON_MIDDLE)
            out.Code = InputCode::MOUSE_MIDDLE;
        else if(in.button.button == SDL_BUTTON_X1 || in.button.button == SDL_BUTTON_X2)
            out.Code = InputCode::MOUSE_X;
        else return false; // unknown one
        out.X = in.motion.x / windowSize.x; // position of mouse
        out.Y = in.motion.y / windowSize.y;
    }
    else if(in.type == SDL_EVENT_MOUSE_WHEEL)
    {
        out.Type = InputMapper::EventType::BEGIN;
        out.Code = in.wheel.y >= 0.0f ? InputCode::MOUSE_WHEEL_UP : InputCode::MOUSE_WHEEL_DOWN;
        out.Y = 0.0f; // use X as primary argument
        out.X = fabsf(in.wheel.y);
    }
    else if(in.type == SDL_EVENT_GAMEPAD_BUTTON_UP || in.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
    {
        const InputCodeDesc* pDesc = InputCodeDescs;
        while(pDesc->T3InputCode != InputCode::NONE)
        {
            if(pDesc->Controller == in.gbutton.button)
            {
                out.Code = pDesc->T3InputCode;
                break;
            }
            pDesc++;
        }
        if(out.Code != InputCode::NONE)
        {
            out.Type = in.type == SDL_EVENT_GAMEPAD_BUTTON_UP ? InputMapper::EventType::END : InputMapper::EventType::BEGIN;
        }
        else return false; // other button
    }
    else if(in.type == SDL_EVENT_GAMEPAD_AXIS_MOTION)
    {
        const InputCodeDesc* pDesc = InputCodeDescs;
        while(pDesc->T3InputCode != InputCode::NONE)
        {
            if(pDesc->ControllerAxis == in.gaxis.axis)
            {
                out.Code = pDesc->T3InputCode;
                break;
            }
            pDesc++;
        }
        out.Type = InputMapper::EventType::BEGIN;
        out.X = out.Y = 0.0f;
        if(out.Code == InputCode::NONE)
        {
            if(in.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX)
            {
                out.Code = InputCode::LEFT_STICK_MOVE;
                out.X = fminf(in.gaxis.value / 32767.f, 1.0f);
            }
            else if(in.gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY)
            {
                out.Code = InputCode::LEFT_STICK_MOVE;
                out.Y = fminf(in.gaxis.value / 32767.f, 1.0f);
            }
            else if(in.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTX)
            {
                out.Code = InputCode::RIGHT_STICK_MOVE;
                out.X = fminf(in.gaxis.value / 32767.f, 1.0f);
            }
            else if(in.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTY)
            {
                out.Code = InputCode::RIGHT_STICK_MOVE;
                out.Y = fminf(in.gaxis.value / 32767.f, 1.0f);
            } else return false;
        }
        else out.X = fminf(in.gaxis.value / 32767.f, 1.0f);
    } else return false;
    return true;
}

void InputMapper::ConvertRuntimeEvents(const SDL_Event &sdlEvent, std::vector<RuntimeInputEvent> &events, Bool bPrimaryWindowFocused, Vector2 windowSize)
{
    RuntimeInputEvent event{};
    if(bPrimaryWindowFocused)
        event.EventFlags.Add(RuntimeInputEvent::EVENT_PRIMARY_WINDOW);
    if(MapSDLEvent(event, sdlEvent, windowSize))
    {
        PlatformInputMapper::QueuePlatformEvent(event, events);
    }
}

// =========================== ALL ENUM DESCRIPTORS

String InputMapper::GetInputCodeName(InputCode key)
{
    const InputCodeDesc* pDesc = InputCodeDescs;
    while(pDesc->T3InputCode != InputCode::NONE)
    {
        if(pDesc->T3InputCode == key)
            return pDesc->Name;
        pDesc++;
    }
    return "- UNKNOWN -";
}

InputCode InputMapper::GetInputCode(String name)
{
    const InputCodeDesc* pDesc = InputCodeDescs;
    while(pDesc->T3InputCode != InputCode::NONE)
    {
        if(CompareCaseInsensitive(pDesc->Name, name))
            return pDesc->T3InputCode;
        pDesc++;
    }
    return InputCode::NONE;
}

// This took me way too long
const InputCodeDesc InputCodeDescs[] =
{
    {InputCode::BACKSPACE, SDLK_BACKSPACE, "Backspace"},
    {InputCode::TAB, SDLK_TAB, "Tab"},
    {InputCode::CLEAR, SDLK_CLEAR, "Clear"},
    {InputCode::RETURN, SDLK_RETURN, "Return"},
    {InputCode::SHIFT, SDLK_LSHIFT, "Shift"},
    {InputCode::CONTROL, SDLK_LCTRL, "Control"},
    {InputCode::ALT, SDLK_LALT, "Alt"},
    {InputCode::PAUSE, SDLK_PAUSE, "Pause"},
    {InputCode::CAPS_LOCK, SDLK_CAPSLOCK, "Caps Lock"},
    {InputCode::ESCAPE, SDLK_ESCAPE, "Escape"},
    {InputCode::SPACE_BAR, SDLK_SPACE, "Space Bar"},
    {InputCode::PAGE_UP, SDLK_PAGEUP, "Page Up"},
    {InputCode::PAGE_DOWN, SDLK_PAGEDOWN, "Page Down"},
    {InputCode::END, SDLK_END, "End"},
    {InputCode::HOME, SDLK_HOME, "Home"},
    {InputCode::LEFT_ARROW, SDLK_LEFT, "Left Arrow"},
    {InputCode::UP_ARROW, SDLK_UP, "Up Arrow"},
    {InputCode::RIGHT_ARROW, SDLK_RIGHT, "Right Arrow"},
    {InputCode::DOWN_ARROW, SDLK_DOWN, "Down Arrow"},
    {InputCode::PRINT_SCREEN, SDLK_PRINTSCREEN, "Print Screen"},
    {InputCode::SNAPSHOT, 0, "Snapshot"},
    {InputCode::INSERT, SDLK_INSERT, "Insert"},
    {InputCode::DELETE, SDLK_DELETE, "Delete"},
    {InputCode::HELP, SDLK_HELP, "Help"},
    {InputCode::ZERO, SDLK_0, "Zero"},
    {InputCode::ONE, SDLK_1, "One"},
    {InputCode::TWO, SDLK_2, "Two"},
    {InputCode::THREE, SDLK_3, "Three"},
    {InputCode::FOUR, SDLK_4, "Four"},
    {InputCode::FIVE, SDLK_5, "Five"},
    {InputCode::SIX, SDLK_6, "Six"},
    {InputCode::SEVEN, SDLK_7, "Seven"},
    {InputCode::EIGHT, SDLK_8, "Eight"},
    {InputCode::NINE, SDLK_9, "Nine"},
    {InputCode::A, SDLK_A, "A"},
    {InputCode::B, SDLK_B, "B"},
    {InputCode::C, SDLK_C, "C"},
    {InputCode::D, SDLK_D, "D"},
    {InputCode::E, SDLK_E, "E"},
    {InputCode::F, SDLK_F, "F"},
    {InputCode::G, SDLK_G, "G"},
    {InputCode::H, SDLK_H, "H"},
    {InputCode::I, SDLK_I, "I"},
    {InputCode::J, SDLK_J, "J"},
    {InputCode::K, SDLK_K, "K"},
    {InputCode::L, SDLK_L, "L"},
    {InputCode::M, SDLK_M, "M"},
    {InputCode::N, SDLK_N, "N"},
    {InputCode::O, SDLK_O, "O"},
    {InputCode::P, SDLK_P, "P"},
    {InputCode::Q, SDLK_Q, "Q"},
    {InputCode::R, SDLK_R, "R"},
    {InputCode::S, SDLK_S, "S"},
    {InputCode::T, SDLK_T, "T"},
    {InputCode::U, SDLK_U, "U"},
    {InputCode::V, SDLK_V, "V"},
    {InputCode::W, SDLK_W, "W"},
    {InputCode::X, SDLK_X, "X"},
    {InputCode::Y, SDLK_Y, "Y"},
    {InputCode::Z, SDLK_Z, "Z"},
    {InputCode::WINDOWS_LEFT, SDLK_LEFT, "Windows Left"},
    {InputCode::WINDOWS_RIGHT, SDLK_RIGHT, "Windows Right"},
    {InputCode::WINDOWS_APPLICATION, SDLK_APPLICATION, "Application"},
    {InputCode::NUMPAD_0, SDLK_KP_0, "Numpad 0"},
    {InputCode::NUMPAD_1, SDLK_KP_1, "Numpad 1"},
    {InputCode::NUMPAD_2, SDLK_KP_2, "Numpad 2"},
    {InputCode::NUMPAD_3, SDLK_KP_3, "Numpad 3"},
    {InputCode::NUMPAD_4, SDLK_KP_4, "Numpad 4"},
    {InputCode::NUMPAD_5, SDLK_KP_5, "Numpad 5"},
    {InputCode::NUMPAD_6, SDLK_KP_6, "Numpad 6"},
    {InputCode::NUMPAD_7, SDLK_KP_7, "Numpad 7"},
    {InputCode::NUMPAD_8, SDLK_KP_8, "Numpad 8"},
    {InputCode::NUMPAD_9, SDLK_KP_9, "Numpad 9"},
    {InputCode::NUMPAD_STAR, SDLK_KP_MULTIPLY, "Numpad *"},
    {InputCode::NUMPAD_PLUS, SDLK_KP_PLUS, "Numpad +"},
    {InputCode::NUMPAD_MINUS, SDLK_KP_MINUS, "Numpad -"},
    {InputCode::NUMPAD_DOT, SDLK_KP_PERIOD, "Numpad ."},
    {InputCode::NUMPAD_SLASH, SDLK_KP_DIVIDE, "Numpad /"},
    {InputCode::F1, SDLK_F1, "F1"},
    {InputCode::F2, SDLK_F2, "F2"},
    {InputCode::F3, SDLK_F3, "F3"},
    {InputCode::F4, SDLK_F4, "F4"},
    {InputCode::F5, SDLK_F5, "F5"},
    {InputCode::F6, SDLK_F6, "F6"},
    {InputCode::F7, SDLK_F7, "F7"},
    {InputCode::F8, SDLK_F8, "F8"},
    {InputCode::F9, SDLK_F9, "F9"},
    {InputCode::F10, SDLK_F10, "F10"},
    {InputCode::F11, SDLK_F11, "F11"},
    {InputCode::F12, SDLK_F12, "F12"},
    {InputCode::NUM_LOCK, SDLK_NUMLOCKCLEAR, "Num Lock"},
    {InputCode::SCROLL_LOCK, SDLK_SCROLLLOCK, "Scroll Lock"},
    {InputCode::LEFT_SHIFT, SDLK_LSHIFT, "Left Shift"},
    {InputCode::RIGHT_SHIFT, SDLK_RSHIFT, "Right Shift"},
    {InputCode::LEFT_CONTROL, SDLK_LCTRL, "Left Control"},
    {InputCode::RIGHT_CONTROL, SDLK_RCTRL, "Right Control"},
    {InputCode::LEFT_MENU, SDLK_MENU, "Left Menu"},
    {InputCode::RIGHT_MENU, SDLK_MENU, "Right Menu"},
    {InputCode::COLON, SDLK_COLON, "Colon"},
    {InputCode::EQUALS, SDLK_EQUALS, "Equals"},
    {InputCode::COMMA, SDLK_COMMA, "Comma"},
    {InputCode::MINUS, SDLK_MINUS, "Minus"},
    {InputCode::FULL_STOP, SDLK_PERIOD, "Period"},
    {InputCode::SLASH, SDLK_SLASH, "Slash"},
    {InputCode::TILDE, SDLK_TILDE, "Tilde"},
    {InputCode::LEFT_BRACKET, SDLK_LEFTPAREN, "Left Bracket"},
    {InputCode::RIGHT_BRACKET, SDLK_RIGHTPAREN, "Right Bracket"},
    {InputCode::BACKSLASH, SDLK_BACKSLASH, "Backslash"},
    {InputCode::QUOTE, 0, "Quote"},
    {InputCode::ENTER, SDLK_RETURN, "Enter"},
    {InputCode::GAMEPAD_0, 0, "Gamepad 0", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::GAMEPAD_1, 0, "Gamepad 1", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::GAMEPAD_2, 0, "Gamepad 2", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::GAMEPAD_3, 0, "Gamepad 3", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::GAMEPAD_4, 0, "Gamepad 4", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::GAMEPAD_5, 0, "Gamepad 5", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::GAMEPAD_6, 0, "Gamepad 6", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::GAMEPAD_7, 0, "Gamepad 7", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::GAMEPAD_8, 0, "Gamepad 8", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::GAMEPAD_9, 0, "Gamepad 9", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::GAMEPAD_10, 0, "Gamepad 10", SDL_GAMEPAD_BUTTON_START},
    {InputCode::GAMEPAD_11, 0, "Gamepad 11", SDL_GAMEPAD_BUTTON_GUIDE},
    {InputCode::GAMEPAD_DPAD_UP, 0, "Gamepad DPad Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::GAMEPAD_DPAD_DOWN, 0, "Gamepad DPad Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::GAMEPAD_DPAD_RIGHT, 0, "Gamepad DPad Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::GAMEPAD_DPAD_LEFT, 0, "Gamepad DPad Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::ABSTRACT_CONFIRM, 0, "Abstract Confirm"},
    {InputCode::ABSTRACT_CANCEL, 0, "Abstract Cancel"},
    {InputCode::MOUSE_MIDDLE, 0, "Mouse Middle Button"}, // Mouse stuff handled separately
    {InputCode::MOUSE_X, 0, "Mouse X Button"},
    {InputCode::MOUSE_DOUBLE_LEFT, 0, "Mouse Double Left Button"},
    {InputCode::MOUSE_MOVE, 0, "Mouse Move"},
    {InputCode::MOUSE_ROLLOVER, 0, "Mouse Rollover"},
    {InputCode::MOUSE_WHEEL_UP, 0, "Mouse Wheel Up"},
    {InputCode::MOUSE_WHEEL_DOWN, 0, "Mouse Wheel Down"},
    {InputCode::MOUSE_LEFT_BUTTON, 0, "Mouse Left Button"},
    {InputCode::MOUSE_RIGHT_BUTTON, 0, "Mouse Right Button"},
    {InputCode::LEFT_STICK_MOVE, 0, "Left stick move"}, // On move handled separately
    {InputCode::RIGHT_STICK_MOVE, 0, "Right stick move"},
    {InputCode::TRIGGER_MOVE, 0, "Trigger move"}, // not sure how this one is used
    {InputCode::LEFT_TRIGGER_MOVE, 0, "Left trigger move", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::RIGHT_TRIGGER_MOVE, 0, "Right trigger move", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::GESTURE_2TOUCH_SINGLE_TAP, 0, "Gesture 2-Touch Single Tap"}, // Specific gestures handled separately but not needed as only PC app (TTE)
    {InputCode::GESTURE_2TOUCH_DOUBLE_TAP, 0, "Gesture 2-Touch Double Tap"},
    {InputCode::GESTURE_1TOUCH_SWIPE_LEFT, 0, "Gesture 1-Touch Swipe Left"},
    {InputCode::GESTURE_1TOUCH_SWIPE_UP, 0, "Gesture 1-Touch Swipe Up"},
    {InputCode::GESTURE_1TOUCH_SWIPE_RIGHT, 0, "Gesture 1-Touch Swipe Right"},
    {InputCode::GESTURE_1TOUCH_SWIPE_DOWN, 0, "Gesture 1-Touch Swipe Down"},
    {InputCode::GESTURE_2TOUCHES_SWITCH_LEFT, 0, "Gesture 2-Touches Swipe Left"},
    {InputCode::GESTURE_2TOUCHES_SWITCH_UP, 0, "Gesture 2-Touches Swipe Up"},
    {InputCode::GESTURE_2TOUCHES_SWITCH_RIGHT, 0, "Gesture 2-Touches Swipe Right"},
    {InputCode::GESTURE_2TOUCHES_SWITCH_DOWN, 0, "Gesture 2-Touches Swipe Down"},
    {InputCode::GESTURE_2TOUCHES_DOWN, 0, "2-Touches Down"},
    {InputCode::GESTURE_3TOUCHES_DOWN, 0, "3-Touches Down"},
    {InputCode::PC_MOUSE_LEFT, 0, "Left Click"},
    {InputCode::PC_MOUSE_RIGHT, 0, "Right Click"},
    {InputCode::XB360_A, 0, "Xbox A", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::XB360_B, 0, "Xbox B", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::XB360_X, 0, "Xbox X", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::XB360_Y, 0, "Xbox Y", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::XB360_L, 0, "Xbox L", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::XB360_R, 0, "Xbox R", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::XB360_LEFT_TRIGGER, 0, "Xbox Left Trigger", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::XB360_RIGHT_TRIGGER, 0, "Xbox Right Trigger", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::XB360_START, 0, "Xbox Start", SDL_GAMEPAD_BUTTON_START},
    {InputCode::XB360_BACK, 0, "Xbox Back", SDL_GAMEPAD_BUTTON_BACK},
    {InputCode::XB360_LEFT_STICK, 0, "Xbox Left Stick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::XB360_RIGHT_STICK, 0, "Xbox Right Stick", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::XB360_LEFT, 0, "Xbox Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::XB360_RIGHT, 0, "Xbox Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::XB360_UP, 0, "Xbox Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::XB360_DOWN, 0, "Xbox Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::WIIU_A, 0, "WiiU A", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::WIIU_B, 0, "WiiU B", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::WIIU_X, 0, "WiiU X", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::WIIU_Y, 0, "WiiU Y", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::WIIU_L, 0, "WiiU L", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::WIIU_R, 0, "WiiU R", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::WIIU_ZL, 0, "WiiU ZL"}, // no mappings for these (i mean, who is using their WiiU with TTE...)
    {InputCode::WIIU_ZR, 0, "WiiU ZR"},
    {InputCode::WIIU_PLUS, 0, "WiiU +"},
    {InputCode::WIIU_MINUS, 0, "WiiU -"},
    {InputCode::WIIU_LEFT_STICK, 0, "WiiU Left Stick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::WIIU_RIGHT_STICK, 0, "WiiU Right Stick", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::WIIU_LEFT, 0, "WiiU Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::WIIU_RIGHT, 0, "WiiU Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::WIIU_UP, 0, "WiiU Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::WIIU_DOWN, 0, "WiiU Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::PS3_SQUARE, 0, "PS3 Square", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::PS3_TRIANGLE, 0, "PS3 Triangle", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::PS3_CIRCLE, 0, "PS3 Circle", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::PS3_CROSS, 0, "PS3 Cross", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::PS3_L1, 0, "PS3 L1", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::PS3_L2, 0, "PS3 L2", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::PS3_L3, 0, "PS3 L3", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::PS3_R1, 0, "PS3 R1", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::PS3_R2, 0, "PS3 R2", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::PS3_R3, 0, "PS3 R3", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::PS3_START, 0, "PS3 Start", SDL_GAMEPAD_BUTTON_START},
    {InputCode::PS3_SELECT, 0, "PS3 Select", SDL_GAMEPAD_BUTTON_BACK},
    {InputCode::PS3_LEFT, 0, "PS3 Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::PS3_RIGHT, 0, "PS3 Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::PS3_UP, 0, "PS3 Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::PS3_DOWN, 0, "PS3 Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::IOS_DOUBLE_TAP, 0, "iPhone Double Tap"}, // No use
    {InputCode::VITA_SQUARE, 0, "Vita Square", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::VITA_TRIANGLE, 0, "Vita Triangle", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::VITA_CIRCLE, 0, "Vita Circle", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::VITA_CROSS, 0, "Vita Cross", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::VITA_L1, 0, "Vita L1", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::VITA_L2, 0, "Vita L2", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::VITA_L3, 0, "Vita L3", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::VITA_R1, 0, "Vita R1", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::VITA_R2, 0, "Vita R2", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::VITA_R3, 0, "Vita R3", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::VITA_START, 0, "Vita Start", SDL_GAMEPAD_BUTTON_START},
    {InputCode::VITA_SELECT, 0, "Vita Select", SDL_GAMEPAD_BUTTON_BACK},
    {InputCode::VITA_LEFT, 0, "Vita Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::VITA_RIGHT, 0, "Vita Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::VITA_UP, 0, "Vita Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::VITA_DOWN, 0, "Vita Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::APPLE_GAMEPAD_A, 0, "Apple Gamepad A", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::APPLE_GAMEPAD_B, 0, "Apple Gamepad B", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::APPLE_GAMEPAD_X, 0, "Apple Gamepad X", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::APPLE_GAMEPAD_Y, 0, "Apple Gamepad Y", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::APPLE_GAMEPAD_L, 0, "Apple Gamepad L", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::APPLE_GAMEPAD_R, 0, "Apple Gamepad R", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::APPLE_GAMEPAD_LEFT, 0, "Apple Gamepad Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::APPLE_GAMEPAD_RIGHT, 0, "Apple Gamepad Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::APPLE_GAMEPAD_UP, 0, "Apple Gamepad Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::APPLE_GAMEPAD_DOWN, 0, "Apple Gamepad Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::PS4_SQUARE, 0, "PS4 Square", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::PS4_TRIANGLE, 0, "PS4 Triangle", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::PS4_CIRCLE, 0, "PS4 Circle", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::PS4_CROSS, 0, "PS4 Cross", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::PS4_L1, 0, "PS4 L1", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::PS4_L2, 0, "PS4 L2", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::PS4_L3, 0, "PS4 L3", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::PS4_R1, 0, "PS4 R1", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::PS4_R2, 0, "PS4 R2", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::PS4_R3, 0, "PS4 R3", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::PS4_OPTIONS, 0, "PS4 Options", SDL_GAMEPAD_BUTTON_START},
    {InputCode::PS4_TOUCHPAD, 0, "PS4 Touchpad", SDL_GAMEPAD_BUTTON_MISC1},
    {InputCode::PS4_LEFT, 0, "PS4 Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::PS4_RIGHT, 0, "PS4 Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::PS4_UP, 0, "PS4 Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::PS4_DOWN, 0, "PS4 Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::XBONE_A, 0, "Xbox One A", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::XBONE_B, 0, "Xbox One B", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::XBONE_X, 0, "Xbox One X", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::XBONE_Y, 0, "Xbox One Y", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::XBONE_L, 0, "Xbox One L", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::XBONE_R, 0, "Xbox One R", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::XBONE_LEFT_TRIGGER, 0, "Xbox One Left Trigger", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::XBONE_RIGHT_TRIGGER, 0, "Xbox One Right Trigger", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::XBONE_START, 0, "Xbox One Start", SDL_GAMEPAD_BUTTON_START},
    {InputCode::XBONE_BACK, 0, "Xbox One Back", SDL_GAMEPAD_BUTTON_BACK},
    {InputCode::XBONE_LEFT_STICK, 0, "Xbox One Left Stick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::XBONE_RIGHT_STICK, 0, "Xbox One Right Stick", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::XBONE_LEFT, 0, "Xbox One Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::XBONE_RIGHT, 0, "Xbox One Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::XBONE_UP, 0, "Xbox One Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::XBONE_DOWN, 0, "Xbox One Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::NX_A, 0, "NX A", SDL_GAMEPAD_BUTTON_SOUTH},
    {InputCode::NX_B, 0, "NX B", SDL_GAMEPAD_BUTTON_EAST},
    {InputCode::NX_X, 0, "NX X", SDL_GAMEPAD_BUTTON_WEST},
    {InputCode::NX_Y, 0, "NX Y", SDL_GAMEPAD_BUTTON_NORTH},
    {InputCode::NX_L, 0, "NX L", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {InputCode::NX_R, 0, "NX R", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {InputCode::NX_ZL, 0, "NX ZL", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
    {InputCode::NX_ZR, 0, "NX ZR", SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
    {InputCode::NX_PLUS, 0, "NX Plus", SDL_GAMEPAD_BUTTON_START},
    {InputCode::NX_MINUS, 0, "NX Minus", SDL_GAMEPAD_BUTTON_BACK},
    {InputCode::NX_LEFT_STICK, 0, "NX Left Stick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {InputCode::NX_RIGHT_STICK, 0, "NX Right Stick", SDL_GAMEPAD_BUTTON_RIGHT_STICK},
    {InputCode::NX_LEFT, 0, "NX Left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {InputCode::NX_RIGHT, 0, "NX Right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {InputCode::NX_UP, 0, "NX Up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {InputCode::NX_DOWN, 0, "NX Down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {InputCode::NX_LEFT_SL, 0, "NX Left SL"},
    {InputCode::NX_LEFT_SR, 0, "NX Left SR"},
    {InputCode::NX_RIGHT_SL, 0, "NX Right SL"},
    {InputCode::NX_RIGHT_SR, 0, "NX Right SR"},
    {InputCode::NONE, 0, "None"}, // dont add below, add above
};
