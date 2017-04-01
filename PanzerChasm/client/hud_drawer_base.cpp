#include <cstring>

#include "../assert.hpp"
#include "../i_menu_drawer.hpp"
#include "../i_text_drawer.hpp"
#include "../shared_drawers.hpp"

#include "hud_drawer_base.hpp"

namespace PanzerChasm
{

const char HudDrawerBase::c_weapon_icons_image_file_name[]= "WICONS.CEL";
const char HudDrawerBase::c_hud_numbers_image_file_name[]= "BFONT2.CEL";
const char HudDrawerBase::c_hud_background_image_file_name[]= "STATUS2.CEL";

HudDrawerBase::HudDrawerBase(
	const GameResourcesConstPtr& game_resources,
	const SharedDrawersPtr& shared_drawers )
	: game_resources_(game_resources)
	, shared_drawers_(shared_drawers)
	, scale_( CalculateHudScale( shared_drawers->menu->GetViewportSize() ) )
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( shared_drawers_ != nullptr );

	std::memset( &player_state_, 0u, sizeof(player_state_) );
}

HudDrawerBase::~HudDrawerBase()
{}

void HudDrawerBase::AddMessage( const MapData::Message& message, const Time current_time )
{
	current_message_= message;
	current_message_time_= current_time;
}

void HudDrawerBase::ResetMessage()
{
	current_message_time_= Time::FromSeconds(0);
}

void HudDrawerBase::SetPlayerState( const Messages::PlayerState& player_state, const unsigned int current_weapon_number )
{
	player_state_= player_state;
	current_weapon_number_= current_weapon_number;
}

void HudDrawerBase::DrawCurrentMessage( const Time current_time )
{
	if( current_message_time_.ToSeconds() == 0.0f )
		return;

	const float time_left= ( current_time - current_message_time_ ).ToSeconds();
	if( time_left > current_message_.delay_s )
		return;

	for( const  MapData::Message::Text& text : current_message_.texts )
	{
		int x;
		ITextDrawer::Alignment alignemnt;
		if( text.x == -1 )
		{
			x= shared_drawers_->menu->GetViewportSize().Width() / 2u;
			alignemnt= ITextDrawer::Alignment::Center;
		}
		else
		{
			x= text.x * scale_;
			alignemnt= ITextDrawer::Alignment::Left;
		}

		shared_drawers_->text->Print(
			x, text.y * scale_,
			text.data.c_str(),
			scale_,
			ITextDrawer::FontColor::YellowGreen, alignemnt );
	}
}

void HudDrawerBase::CreateCrosshairTexture(
	unsigned int* out_size,
	std::vector<unsigned char>& out_data )
{
	constexpr unsigned int c_size[2]= { 17u, 17u };
	constexpr unsigned int c_center[2]= { c_size[0] / 2u, c_size[1] / 2u };
	constexpr unsigned char c_start_color_index= 159u;

	out_size[0]= c_size[0];
	out_size[1]= c_size[1];
	out_data.resize( c_size[0] * c_size[1], 0u );

	const auto set_pixel=
	[&]( const unsigned int x, const unsigned int y, const unsigned char color_index )
	{
		out_data[ x + y * c_size[0] ]= color_index;
	};

	for( unsigned int i= 0u; i < 4u; i++ )
	{
		const unsigned char color_index= c_start_color_index - i;
		set_pixel( c_center[0], c_center[1] + c_cross_offset + i, color_index );
		set_pixel( c_center[0], c_center[1] - c_cross_offset - i, color_index );
		set_pixel( c_center[0] + c_cross_offset + i, c_center[1], color_index );
		set_pixel( c_center[0] - c_cross_offset - i, c_center[1], color_index );
	}
}

void HudDrawerBase::DrawKeysAndStat( const unsigned int x_offset, const char* const map_name )
{
	const unsigned int c_left_x= 113u;
	const unsigned int c_right_x= 133u;
	const unsigned int c_top_y= 28u;
	const unsigned int c_bottom_y= 14u;

	const unsigned int c_text_x= 150u;
	const unsigned int c_text_y= 27u;

	const unsigned int viewport_height= shared_drawers_->menu->GetViewportSize().Height();

	if( ( player_state_.keys_mask & 1u ) != 0u )
		shared_drawers_->text->Print( x_offset + scale_ * c_left_x , viewport_height - c_top_y    * scale_, "\4", scale_ );
	if( ( player_state_.keys_mask & 2u ) != 0u )
		shared_drawers_->text->Print( x_offset + scale_ * c_right_x, viewport_height - c_top_y    * scale_, "\5", scale_ );
	if( ( player_state_.keys_mask & 4u ) != 0u )
		shared_drawers_->text->Print( x_offset + scale_ * c_left_x , viewport_height - c_bottom_y * scale_, "\6", scale_ );
	if( ( player_state_.keys_mask & 8u ) != 0u )
		shared_drawers_->text->Print( x_offset + scale_ * c_right_x, viewport_height - c_bottom_y * scale_, "\7", scale_ );

	shared_drawers_->text->Print(
		x_offset + scale_ * c_text_x,
		viewport_height - c_text_y * scale_,
		"Time: ", scale_ ); // TODO - print time here
	shared_drawers_->text->Print(
		x_offset + scale_ * c_text_x,
		viewport_height - c_text_y * scale_ + scale_ * ( 3u + shared_drawers_->text->GetLineHeight() ),
		map_name, scale_ );
}

unsigned int HudDrawerBase::CalculateHudScale( const Size2& viewport_size )
{
	// Status bu must have size 10% of screen height or smaller.
	const float c_y_relative_statusbar_size= 1.0f / 10.0f;

	float scale_f=
		std::min(
			float( viewport_size.Width () ) / float( GameConstants::min_screen_width  ),
			float( viewport_size.Height() ) / ( float(c_hud_line_height) / c_y_relative_statusbar_size ) );

	return std::max( 1u, static_cast<unsigned int>( scale_f ) );
}

} // namespace PanzerChasm
