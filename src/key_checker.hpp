#pragma once
#include <algorithm>
#include "settings.hpp"
#include "math_utils.hpp"
#include "system_event.hpp"

namespace PanzerChasm
{

using KeyCode= SystemEvent::KeyEvent::KeyCode;

class KeyChecker
{
public:
	KeyChecker( Settings& settings, const InputState& input_state )
		: settings_(settings), input_state_(input_state), event_(SystemEvent()), event_source_(false)
	{}
	KeyChecker( Settings& settings, const SystemEvent& event )
		: settings_(settings), input_state_(InputState()), event_(event), event_source_(true)
	{}

	bool operator()( const char* const key_setting_name, const KeyCode default_value ) const
	{
		const char32_t key = std::clamp((const char32_t)settings_.GetOrSetInt( key_setting_name, static_cast<int>(default_value)), (const char32_t)KeyCode::Unknown, (const char32_t)KeyCode::KeyCount);

		if(event_source_)
		{
			char32_t key_event = KeyCode::Unknown;
			switch(event_.type)
			{
				case SystemEvent::Type::Wheel:
					if( event_.event.wheel.delta > 0 ) key_event = KeyCode::MouseWheelUp;
					if( event_.event.wheel.delta < 0 ) key_event = KeyCode::MouseWheelDown;
					break;
				case SystemEvent::Type::MouseKey:
					if( event_.event.mouse_key.pressed )
						key_event = KeyCode::MouseUnknown + (const char32_t)event_.event.mouse_key.mouse_button;
					break;
				case SystemEvent::Type::Key:
					if( event_.event.key.pressed ) key_event = event_.event.key.key_code;
					break;
				default:
					break;
			}
			if( key == std::clamp( key_event, (char32_t)KeyCode::Unknown, (char32_t)KeyCode::KeyCount ) ) return true;
		}
		else 
		{
			if( key < KeyCode::MouseUnknown)
				return input_state_.keyboard[ key ];
			else
				return input_state_.mouse[ key - KeyCode::MouseUnknown ];
		}
		return false;
	}

private:
	Settings& settings_;
	bool event_source_;
	const InputState& input_state_;
	const SystemEvent& event_;
};

} // namespace PanzerChasm
