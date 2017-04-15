#include "../map_loader.hpp"
#include "i_map_drawer.hpp"
#include "map_state.hpp"
#include "movement_controller.hpp"

#include "cutscene_player.hpp"

namespace PanzerChasm
{


CutscenePlayer::CutscenePlayer(
	const GameResourcesConstPtr& game_resoruces,
	MapLoader& map_loader,
	IMapDrawer& map_drawer,
	MovementController& movement_controller,
	const unsigned int map_number )
	: map_drawer_(map_drawer)
	, camera_controller_(movement_controller)
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

	// Search room.
	for( const MapData::Monster& monster : map_data_patched->monsters )
	{
		if( monster.monster_id == 0u )
		{
			if( monster.difficulty_flags == script_->room_number )
			{
				room_pos_= m_Vec3( monster.pos, 0.0f );
				room_angle_= monster.angle - Constants::half_pi;
				break;
			}
		}
	}
	m_Mat4 rotation_mat, translate_mat, room_mat;
	rotation_mat.RotateZ( room_angle_ );
	translate_mat.Translate( room_pos_ );
	room_mat= rotation_mat * translate_mat;

	// Load characers.
	first_character_static_model_index_= map_data_patched->static_models.size();
	for( const CutsceneScript::Character& character : script_->characters )
	{
		const unsigned int model_id= map_data_patched->models.size();
		map_data_patched->models.emplace_back();
		map_data_patched->models_description.emplace_back();
		Model& model= map_data_patched->models.back();
		// MapData::ModelDescription& description= map_data_patched->models_description.back();

		{ // Load model and animations.

			const Vfs::FileContent model_content= vfs.ReadFile( character.model_file_name );
			if( !model_content.empty() )
			{
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

				LoadModel_o3(
					model_content,
					animations_content.data(), animations_content.size(),
					model );

				const int c_inv_models_scale= 4;
				for( Model::AnimationVertex& v : model.animations_vertices )
				{
					v.pos[0] /= c_inv_models_scale;
					v.pos[1] /= c_inv_models_scale;
					v.pos[2] /= c_inv_models_scale;
				}
				for( m_BBox3& bbox : model.animations_bboxes )
				{
					bbox.min/= float(c_inv_models_scale);
					bbox.max/= float(c_inv_models_scale);
				}
			}
		}

		map_data_patched->static_models.emplace_back();
		MapData::StaticModel& static_model= map_data_patched->static_models.back();

		static_model.pos= ( character.pos * room_mat ).xy();
		static_model.angle= character.angle + room_angle_;
		static_model.difficulty_flags= 0u;
		static_model.is_dynamic= false;
		static_model.model_id= model_id;
	}

	cutscene_map_data_= map_data_patched;
	map_state_.reset( new MapState( cutscene_map_data_, game_resoruces, Time::CurrentTime() ) );
	map_drawer.SetMap( cutscene_map_data_ );

	// Set characters z.
	for( const CutsceneScript::Character& character : script_->characters )
	{
		const unsigned int index= first_character_static_model_index_ + ( &character - script_->characters.data() );
		const MapData::StaticModel& static_model= cutscene_map_data_->static_models[ index ];
		Messages::StaticModelState message;

		PositionToMessagePosition( static_model.pos, message.xyz );
		message.xyz[2]= CoordToMessageCoord( character.pos.z );
		message.angle= AngleToMessageAngle( static_model.angle );
		message.model_id= static_model.model_id;
		message.static_model_index= index;
		message.animation_playing= true;
		message.visible= true;

		const Model& model= cutscene_map_data_->models[ static_model.model_id ];
		if( !model.animations.empty() )
			message.animation_frame= model.animations.back().first_frame;
		else
			message.animation_frame= 0u;

		map_state_->ProcessMessage( message );
	}

	// Seti initial camera position.
	camera_pos_= script_->camera_params.pos * rotation_mat;
	camara_angles_.z= script_->camera_params.angle - Constants::half_pi - 0.5f;
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
	const m_Vec3 cam_pos=
		room_pos_ +
		camera_pos_
		+ 0.3f * m_Vec3( -std::sin( camera_controller_.GetViewAngleZ()), std::cos(camera_controller_.GetViewAngleZ()), 0.0f );

//	camera_controller_.SetAngles( camara_angles_.z, 0.0f );

	m_Mat4 view_rotation_and_projection_matrix;
	camera_controller_.GetViewRotationAndProjectionMatrix( view_rotation_and_projection_matrix );

	ViewClipPlanes view_clip_planes;
	camera_controller_.GetViewClipPlanes( cam_pos, view_clip_planes );

	map_drawer_.Draw(
		*map_state_,
		view_rotation_and_projection_matrix,
		cam_pos,
		view_clip_planes,
		0u );
}

bool CutscenePlayer::IsFinished() const
{
	if( script_ == nullptr )
		return true;
	return true;
}

} // namespace PanzerChasm
