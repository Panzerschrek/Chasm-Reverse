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
	KeyChecker( Settings& settings, const InputState& input_state );
	KeyChecker( Settings& settings, const SystemEvent& event );
	bool operator()( const char* const key_setting_name, const KeyCode default_value ) const;
private:
	Settings& settings_;
	bool event_source_;
	const InputState& input_state_;
	const SystemEvent& event_;
};

} // namespace PanzerChasm
