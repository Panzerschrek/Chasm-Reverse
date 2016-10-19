#include "menu.hpp"

namespace PanzerChasm
{

/*
CONSOLE.CEL
FONT256.CEL // Chars, keys
M_NETWORK.CEL
LFONT2.CEL // only numbers
LOADING.CEL
M_MAIN.CEL
M_NETWORK.CEL
M_NEW.CEL
M_PAUSE.CEL
M_TILE1.CEL

*/

using KeyCode= SystemEvent::KeyEvent::KeyCode;

// Menu base

class MenuBase
{
public:
	MenuBase( MenuBase* const parent_menu );
	virtual ~MenuBase();

	MenuBase* GetParent() const;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) = 0;

	// Returns next menu after event
	virtual MenuBase* ProcessEvent( const SystemEvent& event )= 0;

private:
	MenuBase* const parent_menu_;
};

MenuBase::MenuBase( MenuBase* const parent_menu )
	: parent_menu_(parent_menu)
{}

MenuBase::~MenuBase()
{}

MenuBase* MenuBase::GetParent() const
{
	return parent_menu_;
}

// Main menu

class MainMenu final : public MenuBase
{
public:
	MainMenu();
	~MainMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;
private:

	int current_row_= 0;
};

MainMenu::MainMenu()
	: MenuBase( nullptr )
{}

MainMenu::~MainMenu()
{}

void MainMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{
	const unsigned int w= menu_drawer.GetPictureWidth ( MenuDrawer::MenuPicture::Main );
	const unsigned int h= menu_drawer.GetPictureHeight( MenuDrawer::MenuPicture::Main );
	menu_drawer.DrawMenuBackground( w * 3u, h * 3u, 3u );

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

	menu_drawer.DrawMenuPicture( MenuDrawer::MenuPicture::Main, colors, 3u );
}

MenuBase* MainMenu::ProcessEvent( const SystemEvent& event )
{
	if( event.type == SystemEvent::Type::Key &&
		event.event.key.pressed )
	{
		if( event.event.key.key_code == KeyCode::Up )
			current_row_= ( current_row_ - 1 + 6 ) % 6;

		if( event.event.key.key_code == KeyCode::Down )
			current_row_= ( current_row_ + 1 + 6 ) % 6;
	}
	return this;
}

// Quit Menu

class QuitMenu final : public MenuBase
{
public:
	QuitMenu();
	~QuitMenu() override;

	virtual void Draw( MenuDrawer& menu_drawer, TextDraw& text_draw ) override;
	virtual MenuBase* ProcessEvent( const SystemEvent& event ) override;
private:
};

QuitMenu::QuitMenu()
	: MenuBase( nullptr )
{}

QuitMenu::~QuitMenu()
{}

void QuitMenu::Draw( MenuDrawer& menu_drawer, TextDraw& text_draw )
{}

MenuBase* QuitMenu::ProcessEvent( const SystemEvent& event )
{
	switch( event.type )
	{
	case SystemEvent::Type::Key:
		if(
			event.event.key.pressed &&
			event.event.key.key_code == KeyCode::Enter )
		{
			// TODO - quit here
		}
		break;

	default:
		break;
	}

	return this;
}

// Menu

Menu::Menu(
	unsigned int viewport_width ,
	unsigned int viewport_height,
	const GameResourcesPtr& game_resources )
	: text_draw_( viewport_width, viewport_height, *game_resources )
	, menu_drawer_( viewport_width, viewport_height, *game_resources )
	, root_menu_( new MainMenu )
{
	current_menu_= root_menu_.get();
}

Menu::~Menu()
{
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
					current_menu_= current_menu_->GetParent();
				else
					current_menu_= root_menu_.get();
			}
			break;

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
		current_menu_->Draw( menu_drawer_, text_draw_ );
}

} // namespace PanzerChasm
