#include "../map_loader.hpp"
#include "i_map_drawer.hpp"
#include "map_state.hpp"

#include "cutscene_player.hpp"

namespace PanzerChasm
{


CutscenePlayer::CutscenePlayer(
	const GameResourcesConstPtr& game_resoruces,
	MapLoader& map_loader,
	IMapDrawer& map_drawer,
	const unsigned int map_number )
	: map_drawer_(map_drawer)
{
	const Vfs& vfs= *game_resoruces->vfs;

	MapDataConstPtr map_data= map_loader.LoadMap( 99u );
	if( map_data == nullptr )
		return;

	script_= LoadCutsceneScript( vfs, map_number );
	if( script_ == nullptr )
		return;

	// Create old map data from new map data.
	// Add to map characters as map models.
	const MapDataPtr map_data_patched= std::make_shared<MapData>( *map_data );
	map_data= nullptr; // destroy old map data

	first_character_static_model_index_= map_data_patched->static_models.size();
	for( const CutsceneScript::Character& character : script_->characters )
	{
		const unsigned int model_id= map_data_patched->models.size();
		map_data_patched->models.emplace_back();
		map_data_patched->models_description.emplace_back();
		Model& model= map_data_patched->models.back();
		// MapData::ModelDescription& description= map_data_patched->models_description.back();

		{ // Load model and animations.
			std::vector<Vfs::FileContent> animations_content;
			// Animations.
			for( unsigned int a= 0u; a < CutsceneScript::c_max_character_animations; a++ )
			{
				if( character.animations_file_name[a][0] == '\0' )
					continue;
				animations_content.emplace_back();
				vfs.ReadFile( character.animations_file_name[a], animations_content.back() );
			}
			// Idle animation.
			animations_content.emplace_back();
			vfs.ReadFile( character.idle_animation_file_name, animations_content.back() );

			const Vfs::FileContent model_content= vfs.ReadFile( character.model_file_name );
			LoadModel_o3(
				model_content,
				animations_content.data(), animations_content.size(),
				model );
		}

		map_data_patched->static_models.emplace_back();
		MapData::StaticModel& static_model= map_data_patched->static_models.back();

		static_model.pos= m_Vec2( 0.0f, 0.0f );
		static_model.angle= character.angle;
		static_model.difficulty_flags= 0u;
		static_model.is_dynamic= false;
		static_model.model_id= model_id;
	}

	cutscene_map_data_= map_data_patched;
	map_state_.reset( new MapState( cutscene_map_data_, game_resoruces, Time::CurrentTime() ) );
}

CutscenePlayer::~CutscenePlayer()
{
}

void CutscenePlayer::Process()
{
	// TODO
}

void CutscenePlayer::Draw()
{
	// TODO
	map_drawer_.Draw( *map_state_, m_Mat4(), m_Vec3(0.0f, 0.0f, 0.0f), ViewClipPlanes(), 0u );
}

bool CutscenePlayer::IsFinished() const
{
	if( script_ == nullptr )
		return true;
	return true;
}

} // namespace PanzerChasm
