#include <cstring>

#include "assert.hpp"
#include "drawers.hpp"
#include "net/net.hpp"
#include "settings.hpp"
#include "shared_settings_keys.hpp"
#include "sound/sound_engine.hpp"
#include "sound/sound_id.hpp"
#include "system_window.hpp"

#include "menu.hpp"

namespace PanzerChasm
{

/*
CONSOLE.CEL
FONT256.CEL // Chars, keys
M_NETWORK.CEL
LFONT2.CEL // only numbers
COMMON/LOADING.CEL
M_MAIN.CEL
M_NETWORK.CEL
M_NEW.CEL
M_PAUSE.CEL
M_TILE1.CEL

*/

using KeyCode= SystemEvent::KeyEvent::KeyCode;

static const unsigned int g_menu_caption_offset= 2u;
static const char g_yes[]= "yes";
static const char g_no[]= "no";
static const char g_on[]= "On";
static const char g_off[]= "Off";

// Menu base

class MenuBase
{
public:
	MenuBase( MenuBase* const parent_menu, const Sound::SoundEnginePtr& sound_engine );
	virtual ~MenuBase();

	virtual MenuBase* GetParent();
	virtual void OnActivated();
	void PlayMenuSound( unsigned int sound_id );

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) = 0;

	// Returns next menu after event
	virtual MenuBase* ProcessEvent( const SystemEvent& event )= 0;

private:
	MenuBase* const parent_menu_;
	const Sound::SoundEnginePtr& sound_engine_;
};

MenuBase::MenuBase( MenuBase* const parent_menu, const Sound::SoundEnginePtr& sound_engine )
	: parent_menu_(parent_menu)
	, sound_engine_(sound_engine)
{}

MenuBase::~MenuBase()
{}

MenuBase* MenuBase::GetParent()
{
	return parent_menu_;
}

void MenuBase::OnActivated()
{}

void MenuBase::PlayMenuSound( const unsigned int sound_id )
{
	if( sound_engine_ != nullptr )
		sound_engine_->PlayHeadSound( sound_id );
}

// NewGame Menu

class NewGameMenu final : public MenuBase
{
public:
	NewGameMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~NewGameMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	HostCommands& host_commands_;
	int current_row_= 0;
};

NewGameMenu::NewGameMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, host_commands_(host_commands)
{}

NewGameMenu::~NewGameMenu()
{}

void NewGameMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 pic_size= menu_drawer.GetPictureSize( MenuDrawer::MenuPicture::New );
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * pic_size.xy[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * pic_size.xy[1] ) >> 1 );

	menu_drawer.DrawMenuBackground(
		x, y,
		pic_size.xy[0] * scale, pic_size.xy[1] * scale );

	MenuDrawer::PictureColor colors[3]=
	{
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
	};
	colors[ current_row_ ]= MenuDrawer::PictureColor::Active;

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Difficulty",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	menu_drawer.DrawMenuPicture(
		x, y,
		MenuDrawer::MenuPicture::New, colors );
}

MenuBase* NewGameMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key &&
		event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + 3 ) % 3;
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 + 3 ) % 3;
		}

		if( event.event.key.key_code == KeyCode::Enter )
		{
			PlayMenuSound( Sound::SoundId::MenuSelect );

			DifficultyType difficulty= Difficulty::Easy;
			if( current_row_ == 1u )
				difficulty= Difficulty::Normal;
			if( current_row_ == 2u )
				difficulty= Difficulty::Hard;

			host_commands_.NewGame( difficulty );

			return nullptr;
		}
	}
	return this;
}

// NetworkConnectMenu

class NetworkConnectMenu final : public MenuBase
{
public:
	NetworkConnectMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~NetworkConnectMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	HostCommands& host_commands_;
	int current_row_= 0;

	static constexpr unsigned int c_value_max_size= 20u;
	char values_[4][c_value_max_size];
};

NetworkConnectMenu::NetworkConnectMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, host_commands_(host_commands)
{
	std::snprintf( values_[0], c_value_max_size, "%s", "127.0.0.1" );
	std::snprintf( values_[1], c_value_max_size, "%d", Net::c_default_server_tcp_port );
	std::snprintf( values_[2], c_value_max_size, "%d", Net::c_default_client_tcp_port );
	std::snprintf( values_[3], c_value_max_size, "%d", Net::c_default_client_udp_port );
}

NetworkConnectMenu::~NetworkConnectMenu()
{}

void NetworkConnectMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 280u, text_draw.GetLineHeight() * 5u };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Join setup",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	static const char* const texts[4u]
	{
		"served ip address:", "server tcp port:", "client tcp port:", "client udp port:"
	};
	for( unsigned int i= 0u; i < 4u; i++ )
	{
		const bool active= int(i) == current_row_ ;
		const int row_y= y + scale * int( i * text_draw.GetLineHeight() );

		text_draw.Print(
			x + 135 * scale, row_y,
			texts[i],
			scale,
			TextDraw::FontColor::White,
			TextDraw::Alignment::Right );

		char value_with_cursor[ c_value_max_size + 2u ];
		std::snprintf( value_with_cursor, sizeof(value_with_cursor), "%s%s", values_[i], active ? "_" : "" );

		text_draw.Print(
			x + 145 * scale, row_y,
			value_with_cursor,
			scale,
			active ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
			TextDraw::Alignment::Left );
	}

	text_draw.Print(
		int(viewport_size.xy[0] >> 1u), y + scale * int( 4u * text_draw.GetLineHeight() ),
		"connect",
		scale,
		current_row_ == 4 ? TextDraw::FontColor::Golden : TextDraw::FontColor::White,
		TextDraw::Alignment::Center );
}

MenuBase* NetworkConnectMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key &&
		event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + 5 ) % 5;
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 + 5 ) % 5;
		}

		if( event.event.key.key_code == KeyCode::Backspace &&
			current_row_ < 4 )
		{
			const unsigned int l= std::strlen( values_[ current_row_ ] );
			if( l > 0u )
				values_[ current_row_ ][ l - 1u ]= 0u;
		}

		if( event.event.key.key_code == KeyCode::Enter && current_row_ == 4 )
		{
			PlayMenuSound( Sound::SoundId::MenuSelect );

			const std::string full_address_str= std::string( values_[0] ) + ":" + values_[1];
			host_commands_.ConnectToServer(
				full_address_str.c_str(),
				std::atoi( values_[2] ), std::atoi( values_[3] ) );

			return nullptr;
		}
	}

	if( event.type == SystemEvent::Type::CharInput &&
		current_row_ < 4 )
	{
		const unsigned int l= std::strlen( values_[ current_row_ ] );
		if( l < c_value_max_size - 1u )
		{
			values_[ current_row_ ][l]= event.event.char_input.ch;
			values_[ current_row_ ][l + 1u]= 0u;
		}
	}
	return this;
}

// NetworkCreateServerMenu

class NetworkCreateServerMenu final : public MenuBase
{
public:
	NetworkCreateServerMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~NetworkCreateServerMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	const char* GetDifficultyStr();

private:
	HostCommands& host_commands_;

	struct Row
	{
		enum : int
		{
			Map= 0, Difficulty, Dedicated, TCPPort, UDPBasePort, Start, NumRows
		};
	};
	int current_row_= 0;

	unsigned int map_number_= 1u;
	unsigned int difficulty_= 1u;
	bool dedicated_= false;

	static constexpr unsigned int c_port_str_max_size= 8u;
	char tcp_port_[ c_port_str_max_size ];
	char base_udp_port_[ c_port_str_max_size ];
};

NetworkCreateServerMenu::NetworkCreateServerMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, host_commands_(host_commands)
{
	std::snprintf( tcp_port_, sizeof(tcp_port_), "%d", Net::c_default_server_tcp_port );
	std::snprintf( base_udp_port_, sizeof(base_udp_port_), "%d", Net::c_default_server_udp_base_port );
}

NetworkCreateServerMenu::~NetworkCreateServerMenu()
{}

void NetworkCreateServerMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 210u, text_draw.GetLineHeight() * Row::NumRows };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );
	const int param_descr_x= x + scale * 110;
	const int param_x= x + scale * 120;
	const int y_step= int(text_draw.GetLineHeight()) * scale;

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Start server",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	text_draw.Print(
		param_descr_x, y + Row::Map * y_step,
		"map:", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	char map_name[ 64 ];
	std::snprintf( map_name, sizeof(map_name), "map%02d", map_number_ ); // TODO - show real man name here
	text_draw.Print(
		param_x, y + Row::Map * y_step,
		map_name, scale,
		current_row_ == Row::Map ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::Difficulty * y_step,
		"difficulty:", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::Difficulty * y_step,
		GetDifficultyStr(), scale,
		current_row_ == Row::Difficulty ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::Dedicated * y_step,
		"dedicated:", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::Dedicated * y_step,
		dedicated_ ? g_yes : g_no, scale,
		current_row_ == Row::Dedicated ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	char port_str_with_cursor[ c_port_str_max_size + 2u ];

	text_draw.Print(
		param_descr_x, y + Row::TCPPort * y_step,
		"tcp port:", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	std::snprintf( port_str_with_cursor, sizeof(port_str_with_cursor), "%s%s", tcp_port_, current_row_ == Row::TCPPort ? "_" : "" );
	text_draw.Print(
		param_x, y + Row::TCPPort * y_step,
		port_str_with_cursor, scale,
		current_row_ == Row::TCPPort ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::UDPBasePort * y_step,
		"base udp port:", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	std::snprintf( port_str_with_cursor, sizeof(port_str_with_cursor), "%s%s", base_udp_port_, current_row_ == Row::UDPBasePort ? "_" : "" );
	text_draw.Print(
		param_x, y + Row::UDPBasePort * y_step,
		port_str_with_cursor, scale,
		current_row_ == Row::UDPBasePort ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ), y + Row::Start * y_step,
		"start", scale,
		current_row_ == Row::Start ? TextDraw::FontColor::Golden : TextDraw::FontColor::White,
		TextDraw::Alignment::Center );
}

MenuBase* NetworkCreateServerMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key && event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + Row::NumRows ) % Row::NumRows;
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 + Row::NumRows ) % Row::NumRows;
		}

		if( event.event.key.key_code == KeyCode::Left || event.event.key.key_code == KeyCode::Right )
		{
			if( current_row_ == Row::Map )
			{
				if( map_number_ > 1u && event.event.key.key_code == KeyCode::Left )
					map_number_--;
				else if( map_number_ < 99 && event.event.key.key_code == KeyCode::Right )
					map_number_++;

			}
			if( current_row_ == Row::Difficulty )
			{
				if( event.event.key.key_code == KeyCode::Left )
					difficulty_++;
				else
					difficulty_--;
				difficulty_= ( difficulty_ + 4u ) % 4u ;
			}
			if( current_row_ == Row::Dedicated )
				dedicated_= !dedicated_;
		}

		if( event.event.key.key_code == KeyCode::Enter && current_row_ == Row::Map && map_number_ < 99 )
			map_number_++;

		if( event.event.key.key_code == KeyCode::Enter && current_row_ == Row::Dedicated )
			dedicated_= !dedicated_;

		if( event.event.key.key_code == KeyCode::Enter && current_row_ == Row::Difficulty )
			difficulty_= ( difficulty_ + 1u ) % 4u ;

		if( event.event.key.key_code == KeyCode::Backspace && current_row_ == Row::TCPPort )
		{
			const unsigned int len= std::strlen( tcp_port_ );
			if( len > 0 ) tcp_port_[ len - 1u ]= '\0';
		}
		if( event.event.key.key_code == KeyCode::Backspace && current_row_ == Row::UDPBasePort )
		{
			const unsigned int len= std::strlen( base_udp_port_ );
			if( len > 0 ) base_udp_port_[ len - 1u ]= '\0';
		}
		if( event.event.key.key_code == KeyCode::Enter && current_row_ == Row::Start )
		{
			PlayMenuSound( Sound::SoundId::MenuSelect );

			host_commands_.StartServer(
				map_number_,
				static_cast<DifficultyType>( 1u << difficulty_ ),
				dedicated_,
				std::atoi( tcp_port_ ),
				std::atoi( base_udp_port_ ) );

			return nullptr;
		}
	}

	if( event.type == SystemEvent::Type::CharInput && current_row_ == Row::TCPPort )
	{
		const unsigned int l= std::strlen( tcp_port_ );
		if( l < c_port_str_max_size - 1u )
		{
			tcp_port_[l]= event.event.char_input.ch;
			tcp_port_[l + 1u]= 0u;
		}
	}
	if( event.type == SystemEvent::Type::CharInput && current_row_ == Row::UDPBasePort )
	{
		const unsigned int l= std::strlen( base_udp_port_ );
		if( l < c_port_str_max_size - 1u )
		{
			base_udp_port_[l]= event.event.char_input.ch;
			base_udp_port_[l + 1u]= 0u;
		}
	}

	return this;
}

const char* NetworkCreateServerMenu::GetDifficultyStr()
{
	switch( static_cast<DifficultyType>( 1u << difficulty_ ) )
	{
	case Difficulty::Easy: return "easy";
	case Difficulty::Normal: return "normal";
	case Difficulty::Hard: return "hard";
	case Difficulty::Deathmatch: return "deathmatch";
	};
	return "";
}

// NetworkMenu

class NetworkMenu final : public MenuBase
{
public:
	NetworkMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~NetworkMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	HostCommands& host_commands_;
	std::unique_ptr<MenuBase> submenus_[3];
	int current_row_= 0;
};

NetworkMenu::NetworkMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, host_commands_(host_commands)
{
	submenus_[0].reset( new NetworkConnectMenu( this, sound_engine, host_commands ) );
	submenus_[1].reset( new NetworkCreateServerMenu( this, sound_engine, host_commands ) );
}

NetworkMenu::~NetworkMenu()
{}

void NetworkMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 pic_size= menu_drawer.GetPictureSize( MenuDrawer::MenuPicture::Network );
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * pic_size.xy[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * pic_size.xy[1] ) >> 1 );

	menu_drawer.DrawMenuBackground(
		x, y,
		pic_size.xy[0] * scale, pic_size.xy[1] * scale );

	MenuDrawer::PictureColor colors[4]=
	{
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Disabled,
	};
	colors[ current_row_ ]= MenuDrawer::PictureColor::Active;

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Network",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	menu_drawer.DrawMenuPicture(
		x, y,
		MenuDrawer::MenuPicture::Network, colors );
}

MenuBase* NetworkMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key &&
		event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + 3 ) % 3;
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 + 3 ) % 3;
		}

		if( event.event.key.key_code == KeyCode::Enter )
		{
			PlayMenuSound( Sound::SoundId::MenuSelect );
			return submenus_[ current_row_ ].get();
		}
	}
	return this;
}


class SaveLoadMenu final : public MenuBase
{
public:
	enum class What
	{
		Save, Load
	};

	SaveLoadMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands, What what );
	~SaveLoadMenu() override;

	virtual void OnActivated() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	HostCommands& host_commands_;
	const What what_;
	int current_row_= 0;

	HostCommands::SavesNames saves_names_;

	static constexpr int c_rows= HostCommands::c_save_slots;
};

SaveLoadMenu::SaveLoadMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands, What what )
	: MenuBase( parent, sound_engine )
	, host_commands_(host_commands)
	, what_(what)
{
	for( SaveComment& save_comment : saves_names_ )
		save_comment[0]= '\0';
}

SaveLoadMenu::~SaveLoadMenu()
{}

void SaveLoadMenu::OnActivated()
{
	host_commands_.GetSavesNames( saves_names_ );
}

void SaveLoadMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 180u, text_draw.GetLineHeight() * c_rows };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );
	const int param_x= x + scale * 20;

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		what_ == What::Load ? "Load" : "Save",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	for( int i= 0u; i < c_rows; i++ )
	{
		const bool active= current_row_ == i;

		text_draw.Print(
			param_x, y + i * ( scale * int(text_draw.GetLineHeight()) ),
			saves_names_[i][0] == '\0' ? "..." : saves_names_[i].data(),
			scale,
			active ? TextDraw::FontColor::DarkYellow: TextDraw::FontColor::White,
			TextDraw::Alignment::Left );
	}
}

MenuBase* SaveLoadMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key &&
		event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + c_rows ) % c_rows;
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 ) % c_rows;
		}

		if( event.event.key.key_code == KeyCode::Enter )
		{
		}
	}

	return this;
}

// Controls Menu

class ControlsMenu final : public MenuBase
{
public:
	ControlsMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~ControlsMenu() override;

	// Hack for escape pressing in key set mode.
	virtual MenuBase* GetParent() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	struct KeySettings
	{
		const char* name;
		const char* setting_name;
		const KeyCode default_key_code;
	};

	static const KeySettings c_key_settings[];
	static const unsigned int c_key_setting_count;

private:
	Settings& settings_;
	int current_row_= 0;
	bool in_set_mode_= false;
};

const ControlsMenu::KeySettings ControlsMenu::c_key_settings[]=
{
	{ "Move Forward"	, SettingsKeys::key_forward		, KeyCode::W },
	{ "Move Backward"	, SettingsKeys::key_backward	, KeyCode::S },
	{ "Strafe Left"		, SettingsKeys::key_step_left	, KeyCode::A },
	{ "Strafe Right"	, SettingsKeys::key_step_right	, KeyCode::D },
	{ "Turn Left"		, SettingsKeys::key_turn_left	, KeyCode::Left },
	{ "Turn Right"		, SettingsKeys::key_turn_right	, KeyCode::Right },
	{ "Look Up"			, SettingsKeys::key_look_up		, KeyCode::Up },
	{ "Look Down"		, SettingsKeys::key_look_down	, KeyCode::Down },
	{ "Jump"			, SettingsKeys::key_jump		, KeyCode::Space },
};

const unsigned int ControlsMenu::c_key_setting_count= sizeof(ControlsMenu::c_key_settings) / sizeof(ControlsMenu::c_key_settings[0]);

MenuBase* ControlsMenu::GetParent()
{
	return in_set_mode_ ? this : MenuBase::GetParent();
}

ControlsMenu::ControlsMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, settings_(host_commands.GetSettings())
{}

ControlsMenu::~ControlsMenu()
{}

void ControlsMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 200u, text_draw.GetLineHeight() * c_key_setting_count };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );
	const int param_descr_x= x + scale * 115;
	const int param_x= x + scale * 130;
	const int y_step= int(text_draw.GetLineHeight()) * scale;

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Controls",
		scale,
		TextDraw::FontColor::White,TextDraw::Alignment::Center );

	for( unsigned int i= 0u; i < c_key_setting_count; i++ )
	{
		const bool active= current_row_ == int(i);

		const KeySettings& setting= c_key_settings[i];
		const KeyCode key_code= static_cast<KeyCode>( settings_.GetOrSetInt( setting.setting_name, static_cast<int>( setting.default_key_code ) ) );

		text_draw.Print(
			param_descr_x, y + int(i) * y_step,
			setting.name, scale,
			active ? ( in_set_mode_ ?  TextDraw::FontColor::Golden : TextDraw::FontColor::YellowGreen ) : TextDraw::FontColor::White,
			TextDraw::Alignment::Right );
		text_draw.Print(
			param_x, y + int(i) * y_step,
			( active && in_set_mode_ ) ? "?" : GetKeyName( key_code ),
			scale,
			( active && in_set_mode_ ) ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );
	}
}

MenuBase* ControlsMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key && event.event.key.pressed )
	{
		const auto key= event.event.key.key_code;

		if( in_set_mode_ )
		{
			if( key == KeyCode::Escape )
				in_set_mode_= false;
			else
			{
				settings_.SetSetting( c_key_settings[ current_row_ ].setting_name, static_cast<int>(key) );
				PlayMenuSound( Sound::SoundId::MenuSelect );
				in_set_mode_= false;
			}
		}
		else
		{
			if( key == KeyCode::Up )
			{
				PlayMenuSound( Sound::SoundId::MenuChange );
				current_row_= ( current_row_ - 1 + int(c_key_setting_count) ) % int(c_key_setting_count);
			}
			if( key == KeyCode::Down )
			{
				PlayMenuSound( Sound::SoundId::MenuChange );
				current_row_= ( current_row_ + 1 ) % int(c_key_setting_count);
			}

			if( key == KeyCode::Enter )
				in_set_mode_= true;
		}
	}

	return this;
}

// Video Menu

class VideoMenu final : public MenuBase
{
public:
	VideoMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~VideoMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	struct Row
	{
		enum : int
		{
			Fullscreen= 0, Display, FullscreenResolution, Frequency, WindowWidth, WindowHeight, NumRows
		};
	};

private:
	void UpdateSettings();

private:
	Settings& settings_;
	SystemWindow::DispaysVideoModes video_modes_;
	int current_row_= 0;

	unsigned int display_= 0u;
	unsigned int resolution_= 0u;
	unsigned int frequency_= 0u;

	static constexpr unsigned int c_max_window_size= 6u;
	char window_width_ [ c_max_window_size ];
	char window_height_[ c_max_window_size ];
};

VideoMenu::VideoMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, settings_(host_commands.GetSettings())
{
	const SystemWindow* system_window= host_commands.GetSystemWindow();
	if( system_window != nullptr )
		video_modes_= system_window->GetSupportedVideoModes();

	settings_.GetOrSetBool( SettingsKeys::fullscreen, false );

	display_= std::max( 0, std::min( settings_.GetOrSetInt( SettingsKeys::fullscreen_display, 0 ), int(video_modes_.size()) ) );

	const int width = settings_.GetInt( SettingsKeys::fullscreen_width  );
	const int height= settings_.GetInt( SettingsKeys::fullscreen_height );
	const int frequency= settings_.GetInt( SettingsKeys::fullscreen_frequency );
	if( !video_modes_.empty() )
	{
		const SystemWindow::VideoModes& display_modes= video_modes_[display_];
		for( unsigned int i= 0u; i < display_modes.size(); i++ )
		{
			if( width == int(display_modes[i].size.Width()) && height == int(display_modes[i].size.Height()) )
			{
				resolution_= i;
				for( unsigned int j= 0u; j < display_modes[i].supported_frequencies.size(); j++ )
				{
					if( int(display_modes[i].supported_frequencies[j]) == frequency )
					{
						frequency_= j;
						break;
					}
				}
				break;
			}
		}
	}

	std::snprintf( window_width_ , sizeof(window_width_ ), "%d", settings_.GetInt( SettingsKeys::window_width  ) );
	std::snprintf( window_height_, sizeof(window_height_), "%d", settings_.GetInt( SettingsKeys::window_height ) );
}

VideoMenu::~VideoMenu()
{}

void VideoMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 260u, text_draw.GetLineHeight() * Row::NumRows };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );
	const int param_descr_x= x + scale * 180;
	const int param_x= x + scale * 190;
	const int y_step= int(text_draw.GetLineHeight()) * scale;

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Video",
		scale,
		TextDraw::FontColor::White,TextDraw::Alignment::Center );

	text_draw.Print(
		param_descr_x, y + Row::Fullscreen * y_step,
		"Fullscreen", scale,
		current_row_ == Row::Fullscreen ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::Fullscreen * y_step,
		settings_.GetOrSetBool( SettingsKeys::fullscreen, false ) ? g_on : g_off,
		scale,
		current_row_ == Row::Fullscreen ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::Display * y_step,
		"Display", scale,
		current_row_ == Row::Display ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	char display[32];
	std::snprintf( display, sizeof(display), "%d", display_ );
	text_draw.Print(
		param_x, y + Row::Display * y_step,
		display, scale,
		current_row_ == Row::Display ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::FullscreenResolution * y_step,
		"Fullscreen Resolution", scale,
		current_row_ == Row::FullscreenResolution ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );

	if( !video_modes_.empty() && ! video_modes_[display_].empty() )
	{
		const SystemWindow::VideoMode& video_mode= video_modes_[display_][resolution_];

		char str[32];
		std::snprintf( str, sizeof(str), "%dx%d", video_mode.size.Width(), video_mode.size.Height() );

		text_draw.Print(
			param_x, y + Row::FullscreenResolution * y_step,
			str, scale,
			current_row_ == Row::FullscreenResolution ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
			TextDraw::Alignment::Left );
	}

	text_draw.Print(
		param_descr_x, y + Row::Frequency * y_step,
		"Fullscreen Refresh Rate", scale,
		current_row_ == Row::Frequency ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );

	if( !video_modes_.empty() && !video_modes_[display_].empty() && !video_modes_[display_][resolution_].supported_frequencies.empty() )
	{
		const SystemWindow::VideoMode& video_mode= video_modes_[display_][resolution_];

		char str[32];
		std::snprintf( str, sizeof(str), "%d", video_mode.supported_frequencies[ frequency_ ] );

		text_draw.Print(
			param_x, y + Row::Frequency * y_step,
			str, scale,
			current_row_ == Row::Frequency ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
			TextDraw::Alignment::Left );
	}

	char size_str[32];

	text_draw.Print(
		param_descr_x, y + Row::WindowWidth * y_step,
		"Windowed width", scale,
		current_row_ == Row::WindowWidth ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	std::snprintf( size_str, sizeof(size_str), current_row_ == Row::WindowWidth ? "%s_" : "%s", window_width_ );
	text_draw.Print(
		param_x, y + Row::WindowWidth * y_step,
		size_str, scale,
		current_row_ == Row::WindowWidth ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::WindowHeight * y_step,
		"Windowed height", scale,
		current_row_ == Row::WindowHeight ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	std::snprintf( size_str, sizeof(size_str), current_row_ == Row::WindowHeight ? "%s_" : "%s", window_height_ );
	text_draw.Print(
		param_x, y + Row::WindowHeight * y_step,
		size_str, scale,
		current_row_ == Row::WindowHeight ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );
}

MenuBase* VideoMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key && event.event.key.pressed )
	{
		const auto key= event.event.key.key_code;

		if( key == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + Row::NumRows ) % Row::NumRows;
		}
		if( key == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 ) % Row::NumRows;
		}

		// Boolean parameters.
		if( key == KeyCode::Enter || key == KeyCode::Left || key == KeyCode::Right )
		{
			switch(current_row_)
			{
			case Row::Fullscreen:
				settings_.SetSetting( SettingsKeys::fullscreen, !settings_.GetBool( SettingsKeys::fullscreen, false ) );
				PlayMenuSound( Sound::SoundId::MenuSelect );
				break;
			default:
				break;
			};
		} // boolean params.

		if( key == KeyCode::Left || key == KeyCode::Right )
		{
			switch(current_row_)
			{
			case Row::Display:
				if( video_modes_.size() >= 2u )
				{
					const unsigned int prev_display= display_;
					display_+= ( key == KeyCode::Right ) ? ( video_modes_.size() - 1u ) : ( 1u );
					display_%= video_modes_.size();

					// Change resolution index - search same resolution and frequency in other display.
					bool resolution_found= false;
					for( unsigned int i= 0u; i < video_modes_[display_].size(); i++ )
					{
						if( video_modes_[prev_display][resolution_].size == video_modes_[display_][i].size )
						{
							bool frequency_found= false;
							for( unsigned int j= 0u; j < video_modes_[display_][resolution_].supported_frequencies.size(); j++ )
							{
								if( video_modes_[prev_display][resolution_].supported_frequencies[frequency_] ==
									video_modes_[display_][i].supported_frequencies[j] )
								{
									frequency_= j;
									frequency_found= true;
									break;
								}
							}
							if( !frequency_found )
								frequency_= 0u;

							resolution_= i;
							resolution_found= true;
							break;
						}
					}
					if( !resolution_found )
						resolution_= 0u;

					PlayMenuSound( Sound::SoundId::MenuSelect );
					UpdateSettings();
				}
				break;

			case Row::FullscreenResolution:
				if( !video_modes_.empty() && video_modes_[display_].size() >= 2u )
				{
					const unsigned int prev_resolution= resolution_;
					resolution_+= ( key == KeyCode::Right ) ? ( video_modes_[display_].size() - 1u ) : ( 1u );
					resolution_%= video_modes_[display_].size();

					// Change resolution - search same frequency for different resolution.
					bool frequency_found= false;
					for( unsigned int i= 0u; i < video_modes_[display_][resolution_].supported_frequencies.size(); i++ )
					{
						if( video_modes_[display_][prev_resolution].supported_frequencies[frequency_] ==
							video_modes_[display_][resolution_].supported_frequencies[i] )
						{
							frequency_= i;
							frequency_found= true;
							break;
						}
					}
					if( !frequency_found )
						frequency_= 0u;

					PlayMenuSound( Sound::SoundId::MenuSelect );
					UpdateSettings();
				}
				break;

			case Row::Frequency:
				if( !video_modes_.empty() && !video_modes_[display_].empty() && video_modes_[display_][resolution_].supported_frequencies.size() >= 2u )
				{
					frequency_+= ( key == KeyCode::Right ) ? ( video_modes_[display_][resolution_].supported_frequencies.size() - 1u ) : ( 1u );
					frequency_%= video_modes_[display_][resolution_].supported_frequencies.size();

					PlayMenuSound( Sound::SoundId::MenuSelect );
					UpdateSettings();
				}
				break;

			default:
				break;
			};
		}

		if( key == KeyCode::Backspace )
		{
			if( current_row_ == Row::WindowWidth  )
			{
				const unsigned int len= std::strlen( window_width_  );
				if( len > 0u )
					window_width_ [len - 1u]= '\0';
				UpdateSettings();
			}
			if( current_row_ == Row::WindowHeight )
			{
				const unsigned int len= std::strlen( window_height_ );
				if( len > 0u )
					window_height_[len - 1u]= '\0';
				UpdateSettings();
			}
		}
	}

	if( event.type == SystemEvent::Type::CharInput &&
		event.event.char_input.ch >= '0' && event.event.char_input.ch <= '9' )
	{
		const char num= event.event.char_input.ch;
		if( current_row_ == Row::WindowWidth  )
		{
			const unsigned int len= std::strlen( window_width_  );
			if( len + 1u < c_max_window_size )
			{
				window_width_ [len]= num;
				window_width_ [len + 1u ]= '\0';
				UpdateSettings();
			}
		}
		if( current_row_ == Row::WindowHeight )
		{
			const unsigned int len= std::strlen( window_height_ );
			if( len + 1u < c_max_window_size )
			{
				window_height_[len]= num;
				window_height_[len + 1u ]= '\0';
				UpdateSettings();
			}
		}
	}

	return this;
}

void VideoMenu::UpdateSettings()
{
	settings_.SetSetting( SettingsKeys::fullscreen_display, int(display_ ) );
	if( !video_modes_.empty() && !video_modes_[display_].empty() )
	{
		settings_.SetSetting( SettingsKeys::fullscreen_width , int(video_modes_[display_][resolution_].size.Width ()) );
		settings_.SetSetting( SettingsKeys::fullscreen_height, int(video_modes_[display_][resolution_].size.Height()) );
		if( !video_modes_[display_][resolution_].supported_frequencies.empty() )
			settings_.SetSetting( SettingsKeys::fullscreen_frequency, int(video_modes_[display_][resolution_].supported_frequencies[frequency_]) );
	}

	if( std::strlen( window_width_  ) > 0 )
		settings_.SetSetting( SettingsKeys::window_width , window_width_  );
	if( std::strlen( window_height_ ) > 0 )
		settings_.SetSetting( SettingsKeys::window_height, window_height_ );
}

// Options Menu

class OptionsMenu final : public MenuBase
{
public:
	OptionsMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~OptionsMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	Settings& settings_;

	struct Row
	{
		enum : int
		{
			Controls, Video, AlwaysRun, Crosshair, RevertMouse, WeaponReset, FXVolume, CDVolume, MouseSEnsitivity, NumRows
		};
	};
	int current_row_= 0;

	bool always_run_;
	bool crosshair_;
	bool reverse_mouse_;
	bool weapon_reset_;

	const int c_max_slider_value= 16;
	int fx_volume_;
	int cd_volume_;
	int mouse_sensetivity_;

	ControlsMenu controls_menu_;
	VideoMenu video_menu_;
};

OptionsMenu::OptionsMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, settings_( host_commands.GetSettings() )
	, controls_menu_( this, sound_engine, host_commands )
	, video_menu_( this, sound_engine, host_commands )
{
	always_run_= settings_.GetOrSetBool( SettingsKeys::always_run, true );
	crosshair_= settings_.GetOrSetBool( SettingsKeys::crosshair, true );
	reverse_mouse_= settings_.GetOrSetBool( SettingsKeys::reverse_mouse, false );
	weapon_reset_= settings_.GetOrSetBool( SettingsKeys::weapon_reset, false );

	fx_volume_= static_cast<int>( std::round( float(c_max_slider_value) * settings_.GetOrSetFloat( SettingsKeys::fx_volume, 0.5f ) ) );
	cd_volume_= static_cast<int>( std::round( float(c_max_slider_value) * settings_.GetOrSetFloat( SettingsKeys::cd_volume, 0.5f ) ) );
	mouse_sensetivity_= static_cast<int>( std::round( float(c_max_slider_value) * settings_.GetOrSetFloat( SettingsKeys::mouse_sensetivity, 0.5f ) ) );
}

OptionsMenu::~OptionsMenu()
{}

void OptionsMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 210u, text_draw.GetLineHeight() * Row::NumRows };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );
	const int param_descr_x= x + scale * 130;
	const int param_x= x + scale * 140;
	const int y_step= int(text_draw.GetLineHeight()) * scale;

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Options",
		scale,
		TextDraw::FontColor::White,TextDraw::Alignment::Center );

	text_draw.Print(
		param_descr_x, y + Row::Controls * y_step,
		"Controls...", scale,
		current_row_ == Row::Controls ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	text_draw.Print(
		param_descr_x, y + Row::Video * y_step,
		"Video...", scale,
		current_row_ == Row::Video ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );

	text_draw.Print(
		param_descr_x, y + Row::AlwaysRun * y_step,
		"Alaways Run", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::AlwaysRun * y_step,
		always_run_ ? g_on : g_off, scale,
		current_row_ == Row::AlwaysRun ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::Crosshair * y_step,
		"Crosshair", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::Crosshair * y_step,
		crosshair_ ? g_on : g_off, scale,
		current_row_ == Row::Crosshair ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::RevertMouse * y_step,
		"Reverse Mouse", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::RevertMouse * y_step,
		reverse_mouse_ ? g_on : g_off, scale,
		current_row_ == Row::RevertMouse ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::WeaponReset * y_step,
		"Weapon Reset", scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::WeaponReset * y_step,
		weapon_reset_ ? g_on : g_off, scale,
		current_row_ == Row::WeaponReset ? TextDraw::FontColor::Golden : TextDraw::FontColor::DarkYellow,
		TextDraw::Alignment::Left );

	char slider_back_text[ 1u + 7u + 1u + 1u ];
	slider_back_text[0]= TextDraw::c_slider_left_letter_code;
	std::memset( slider_back_text + 1u, TextDraw::c_slider_back_letter_code, 7u );
	slider_back_text[8]= TextDraw::c_slider_right_letter_code;
	slider_back_text[9]= '\0';
	static const char slder_text[]= { TextDraw::c_slider_letter_code, '\0' };

	const int c_slider_pos_scale= 7;

	text_draw.Print(
		param_descr_x, y + Row::FXVolume * y_step,
		"FX Volume", scale,
		current_row_ == Row::FXVolume ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::FXVolume * y_step,
		slider_back_text, scale,
		TextDraw::FontColor::White,
		TextDraw::Alignment::Left );
	text_draw.Print(
		param_x + fx_volume_ * c_slider_pos_scale, y + Row::FXVolume * y_step,
		slder_text, scale,
		TextDraw::FontColor::Golden,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::CDVolume * y_step,
		"CD Volume", scale,
		current_row_ == Row::CDVolume ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::CDVolume * y_step,
		slider_back_text, scale,
		TextDraw::FontColor::White,
		TextDraw::Alignment::Left );
	text_draw.Print(
		param_x + cd_volume_ * c_slider_pos_scale, y + Row::CDVolume * y_step,
		slder_text, scale,
		TextDraw::FontColor::Golden,
		TextDraw::Alignment::Left );

	text_draw.Print(
		param_descr_x, y + Row::MouseSEnsitivity * y_step,
		"Mouse Sensetivity", scale,
		current_row_ == Row::MouseSEnsitivity ? TextDraw::FontColor::YellowGreen : TextDraw::FontColor::White,
		TextDraw::Alignment::Right );
	text_draw.Print(
		param_x, y + Row::MouseSEnsitivity * y_step,
		slider_back_text, scale,
		TextDraw::FontColor::White,
		TextDraw::Alignment::Left );
	text_draw.Print(
		param_x + mouse_sensetivity_ * c_slider_pos_scale, y + Row::MouseSEnsitivity * y_step,
		slder_text, scale,
		TextDraw::FontColor::Golden,
		TextDraw::Alignment::Left );
}

MenuBase* OptionsMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key && event.event.key.pressed )
	{
		const auto key= event.event.key.key_code;

		if( key == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + Row::NumRows ) % Row::NumRows;
		}
		if( key == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 ) % Row::NumRows;
		}

		if( key == KeyCode::Enter )
		{
			switch(current_row_)
			{
			case Row::Controls:
				PlayMenuSound( Sound::SoundId::MenuSelect );
				return &controls_menu_;
			case Row::Video:
				PlayMenuSound( Sound::SoundId::MenuSelect );
				return &video_menu_;
			default:
				break;
			};
		}

		// Boolean parameters.
		if( key == KeyCode::Enter || key == KeyCode::Left || key == KeyCode::Right )
		{
			switch(current_row_)
			{
			case Row::AlwaysRun:
				always_run_= !always_run_;
				settings_.SetSetting( SettingsKeys::always_run, always_run_ );
				PlayMenuSound( Sound::SoundId::MenuSelect );
				break;
			case Row::Crosshair:
				crosshair_= !crosshair_;
				settings_.SetSetting( SettingsKeys::crosshair, crosshair_ );
				PlayMenuSound( Sound::SoundId::MenuSelect );
				break;
			case Row::RevertMouse:
				reverse_mouse_= !reverse_mouse_;
				settings_.SetSetting( SettingsKeys::reverse_mouse, reverse_mouse_ );
				PlayMenuSound( Sound::SoundId::MenuSelect );
				break;
			case Row::WeaponReset:
				weapon_reset_= !weapon_reset_;
				settings_.SetSetting( SettingsKeys::weapon_reset, weapon_reset_ );
				PlayMenuSound( Sound::SoundId::MenuSelect );
				break;
			default:
				break;
			};
		} // boolean params.

		// Sliders
		if( key == KeyCode::Left || key == KeyCode::Right )
		{
			const int shift= key == KeyCode::Left ? -1 : 1;
			int new_value;

			switch(current_row_)
			{
			case Row::FXVolume:
				new_value= std::max( 0, std::min( fx_volume_ + shift, c_max_slider_value ) );
				if( new_value != fx_volume_ )
				{
					fx_volume_= new_value;
					settings_.SetSetting( SettingsKeys::fx_volume, float(fx_volume_) / float(c_max_slider_value) );
					PlayMenuSound( Sound::SoundId::MenuScroll );
				}
				break;
			case Row::CDVolume:
				new_value= std::max( 0, std::min( cd_volume_ + shift, c_max_slider_value ) );
				if( new_value != cd_volume_ )
				{
					cd_volume_= new_value;
					settings_.SetSetting( SettingsKeys::cd_volume, float(cd_volume_) / float(c_max_slider_value) );
					PlayMenuSound( Sound::SoundId::MenuScroll );
				}
				break;
			case Row::MouseSEnsitivity:
				new_value= std::max( 0, std::min( mouse_sensetivity_ + shift, c_max_slider_value ) );
				if( new_value != mouse_sensetivity_ )
				{
					mouse_sensetivity_= new_value;
					settings_.SetSetting( SettingsKeys::mouse_sensetivity, float(mouse_sensetivity_) / float(c_max_slider_value) );
					PlayMenuSound( Sound::SoundId::MenuScroll );
				}
				break;
			default:
				break;
			};
		} // sliders.
	}

	return this;
}

// Quit Menu

class QuitMenu final : public MenuBase
{
public:
	QuitMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~QuitMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	HostCommands& host_commands_;
};

QuitMenu::QuitMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, host_commands_(host_commands)
{}

QuitMenu::~QuitMenu()
{}

void QuitMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 140u, text_draw.GetLineHeight() * 3u };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Quit",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	text_draw.Print(
		viewport_size.xy[0] >> 1u, y,
		"Do you really want\nto quit this game?\n(enter/esc)",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );
}

MenuBase* QuitMenu::ProcessEvent( const SystemEvent& event )
{
	switch( event.type )
	{
	case SystemEvent::Type::Key:
		if(
			event.event.key.pressed &&
			event.event.key.key_code == KeyCode::Enter )
		{
			host_commands_.Quit();
		}
		break;

	default:
		break;
	}

	return this;
}

// Main menu

class MainMenu final : public MenuBase
{
public:
	 MainMenu( const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands );
	~MainMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;

private:
	 HostCommands& host_commands_;
	std::unique_ptr<MenuBase> submenus_[6];
	int current_row_= 0;

};

MainMenu::MainMenu( const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( nullptr, sound_engine )
	, host_commands_(host_commands)
{
	submenus_[0].reset( new NewGameMenu( this, sound_engine, host_commands ) );
	submenus_[1].reset( new NetworkMenu( this, sound_engine, host_commands ) );
	submenus_[2].reset( new SaveLoadMenu( this, sound_engine, host_commands, SaveLoadMenu::What::Save ) );
	submenus_[3].reset( new SaveLoadMenu( this, sound_engine, host_commands, SaveLoadMenu::What::Load ) );
	submenus_[4].reset( new OptionsMenu( this, sound_engine, host_commands ) );
	submenus_[5].reset( new QuitMenu( this, sound_engine, host_commands ) );
}

MainMenu::~MainMenu()
{}

void MainMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 pic_size= menu_drawer.GetPictureSize( MenuDrawer::MenuPicture::Main );
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * pic_size.xy[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * pic_size.xy[1] ) >> 1 );

	menu_drawer.DrawMenuBackground(
		x, y,
		pic_size.xy[0] * scale, pic_size.xy[1] * scale );

	MenuDrawer::PictureColor colors[6]=
	{
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
		MenuDrawer::PictureColor::Unactive,
	};
	colors[ current_row_ ]= MenuDrawer::PictureColor::Active;

	if( !host_commands_.SaveAvailable() ) colors[2]= MenuDrawer::PictureColor::Disabled;

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Main",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );


	menu_drawer.DrawMenuPicture(
		x, y,
		MenuDrawer::MenuPicture::Main, colors );
}

MenuBase* MainMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key &&
		event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + 6 ) % 6;

			if( current_row_ == 2 && !host_commands_.SaveAvailable() )
				current_row_= ( current_row_ - 1 + 6 ) % 6;
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 + 6 ) % 6;

			if( current_row_ == 2 && !host_commands_.SaveAvailable() )
				current_row_= ( current_row_ + 1 + 6 ) % 6;
		}

		if( event.event.key.key_code == KeyCode::Enter )
		{
			PlayMenuSound( Sound::SoundId::MenuSelect );
			return submenus_[ current_row_ ].get();
		}
	}
	return this;
}

// Menu

Menu::Menu(
	HostCommands& host_commands,
	const DrawersPtr& drawers,
	const Sound::SoundEnginePtr& sound_engine )
	: drawers_(drawers)
	, root_menu_( new MainMenu( sound_engine, host_commands ) )
{
	PC_ASSERT( drawers_ != nullptr );

	current_menu_= root_menu_.get();
}

Menu::~Menu()
{
}

bool Menu::IsActive() const
{
	return current_menu_ != nullptr;
}

void Menu::ProcessEvents( const SystemEvents& events )
{
	for( const SystemEvent& event : events )
	{
		switch( event.type )
		{
		case SystemEvent::Type::Key:
			if(
				event.event.key.pressed &&
				event.event.key.key_code == KeyCode::Escape )
			{
				if( current_menu_ != nullptr )
				{
					current_menu_->PlayMenuSound( Sound::SoundId::MenuOn );
					MenuBase* const new_menu= current_menu_->GetParent();
					if( new_menu != current_menu_ )
					{
						if( new_menu != nullptr )
							new_menu->OnActivated();
						current_menu_= new_menu;
					}
				}
				else
				{
					current_menu_= root_menu_.get();
					current_menu_->PlayMenuSound( Sound::SoundId::MenuOn );
					current_menu_->OnActivated();
				}
			}
			break;

		case SystemEvent::Type::CharInput: break;
		case SystemEvent::Type::MouseKey: break;
		case SystemEvent::Type::MouseMove: break;
		case SystemEvent::Type::Wheel: break;
		case SystemEvent::Type::Quit: break;
		};

		if( current_menu_ != nullptr )
		{
			MenuBase* const new_menu= current_menu_->ProcessEvent( event );
			if( new_menu != current_menu_ )
			{
				if( new_menu != nullptr )
					new_menu->OnActivated();
				current_menu_= new_menu;
			}
		}
	}
}

void Menu::Draw()
{
	if( current_menu_ )
		current_menu_->Draw( drawers_->menu, drawers_->text );
}

} // namespace PanzerChasm
