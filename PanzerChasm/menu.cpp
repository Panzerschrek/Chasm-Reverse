#include <cstring>

#include "assert.hpp"
#include "drawers.hpp"
#include "net/net.hpp"
#include "sound/sound_engine.hpp"
#include "sound/sound_id.hpp"

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

// Menu base

class MenuBase
{
public:
	MenuBase( MenuBase* const parent_menu, const Sound::SoundEnginePtr& sound_engine );
	virtual ~MenuBase();

	MenuBase* GetParent() const;
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

MenuBase* MenuBase::GetParent() const
{
	return parent_menu_;
}

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
	const unsigned int size[2]= { 260u, text_draw.GetLineHeight() * 5u };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		int( viewport_size.xy[0] >> 1u ),
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Connect",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	static const char* const texts[4u]
	{
		"served ip address", "server tcp port", "client tcp port", "client udp port"
	};
	for( unsigned int i= 0u; i < 4u; i++ )
	{
		const bool active= int(i) == current_row_ ;
		const int row_y= y + scale * int( i * text_draw.GetLineHeight() );

		text_draw.Print(
			x, row_y,
			texts[i],
			scale,
			active ? TextDraw::FontColor::Golden : TextDraw::FontColor::White,
			TextDraw::Alignment::Left );

		char value_with_cursor[ c_value_max_size + 2u ];
		std::snprintf( value_with_cursor, sizeof(value_with_cursor), "%s%s", values_[i], active ? "_" : "" );

		text_draw.Print(
			x + 140 * scale, row_y,
			value_with_cursor,
			scale,
			active ? TextDraw::FontColor::Golden : TextDraw::FontColor::White,
			TextDraw::Alignment::Left );
	}

	text_draw.Print(
		int(viewport_size.xy[1] >> 1u), y + scale * int( 4u * text_draw.GetLineHeight() ),
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
	std::unique_ptr<MenuBase> submenus_[2];
	int current_row_= 0;
};

NetworkMenu::NetworkMenu( MenuBase* parent, const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( parent, sound_engine )
	, host_commands_(host_commands)
{
	submenus_[0].reset( new NetworkConnectMenu( this, sound_engine, host_commands ) );
}

NetworkMenu::~NetworkMenu()
{}

void NetworkMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const Size2 viewport_size= menu_drawer.GetViewportSize();

	const int scale= int( menu_drawer.GetMenuScale() );
	const unsigned int size[2]= { 108u, text_draw.GetLineHeight() * 2u };

	const int x= int(viewport_size.xy[0] >> 1u) - int( ( scale * size[0] ) >> 1 );
	const int y= int(viewport_size.xy[1] >> 1u) - int( ( scale * size[1] ) >> 1 );
	const int y1= y + scale * int(text_draw.GetLineHeight());
	const int x_center= int( viewport_size.xy[0] >> 1u );

	menu_drawer.DrawMenuBackground( x, y, size[0] * scale, size[1] * scale );

	text_draw.Print(
		x_center,
		y - int( ( g_menu_caption_offset + text_draw.GetLineHeight() ) * scale ),
		"Network",
		scale,
		TextDraw::FontColor::White, TextDraw::Alignment::Center );

	text_draw.Print(
		x_center, y,
		"Connect",
		scale,
		current_row_ == 0 ? TextDraw::FontColor::Golden : TextDraw::FontColor::White,
		TextDraw::Alignment::Center );

	text_draw.Print(
		x_center, y1,
		"Create server",
		scale,
		current_row_ == 1 ? TextDraw::FontColor::Golden : TextDraw::FontColor::White,
		TextDraw::Alignment::Center );
}

MenuBase* NetworkMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key &&
		event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ - 1 + 2 ) % 2;
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
			current_row_= ( current_row_ + 1 + 2 ) % 2;
		}

		if( event.event.key.key_code == KeyCode::Enter )
		{
			PlayMenuSound( Sound::SoundId::MenuSelect );
			return submenus_[ current_row_ ].get();
		}
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
	std::unique_ptr<MenuBase> submenus_[6];
	int current_row_= 0;

};

MainMenu::MainMenu( const Sound::SoundEnginePtr& sound_engine, HostCommands& host_commands )
	: MenuBase( nullptr, sound_engine )
{
	submenus_[0].reset( new NewGameMenu( this, sound_engine, host_commands ) );
	submenus_[1].reset( new NetworkMenu( this, sound_engine, host_commands ) );
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
		}

		if( event.event.key.key_code == KeyCode::Down )
		{
			PlayMenuSound( Sound::SoundId::MenuChange );
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
					current_menu_= current_menu_->GetParent();
				}
				else
				{
					current_menu_= root_menu_.get();
					current_menu_->PlayMenuSound( Sound::SoundId::MenuOn );
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
			current_menu_= current_menu_->ProcessEvent( event );
	}
}

void Menu::Draw()
{
	if( current_menu_ )
		current_menu_->Draw( drawers_->menu, drawers_->text );
}

} // namespace PanzerChasm
