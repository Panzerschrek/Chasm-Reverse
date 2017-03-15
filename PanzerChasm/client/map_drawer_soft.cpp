#include "../assert.hpp"
#include "../map_loader.hpp"

#include "map_drawer_soft.hpp"

namespace PanzerChasm
{

MapDrawerSoft::MapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
	: game_resources_( game_resources )
	, rendering_context_( rendering_context )
	, rasterizer_(
		rendering_context.viewport_size.Width(), rendering_context.viewport_size.Height(),
		rendering_context.row_pixels, rendering_context.window_surface_data )
{
	PC_ASSERT( game_resources_ != nullptr );

	// TODO
	PC_UNUSED( settings );
}

MapDrawerSoft::~MapDrawerSoft()
{}

void MapDrawerSoft::SetMap( const MapDataConstPtr& map_data )
{
	current_map_data_= map_data;
	if( map_data == nullptr )
		return;

	const PaletteTransformed& palette= *rendering_context_.palette_transformed;

	unsigned int texel_count= 0u;
	for( const Model& model : map_data->models )
	{
		texel_count+= model.texture_data.size();
	}

	map_models_.textures_data.resize( texel_count );
	map_models_.models.resize( map_data->models.size() );

	unsigned int texture_data_offset= 0u;
	for( unsigned int m= 0u; m < map_models_.models.size(); m++ )
	{
		const Model& in_model= map_data->models[m];
		ModelsGroup::ModelEntry& model_entry= map_models_.models[m];

		model_entry.texture_size[0]= in_model.texture_size[0];
		model_entry.texture_size[1]= in_model.texture_size[1];
		model_entry.texture_data_offset= texture_data_offset;

		for( unsigned int t= 0u; t < in_model.texture_data.size(); t++ )
			map_models_.textures_data[ texture_data_offset + t ]= palette[ in_model.texture_data[t] ];

		texture_data_offset+= in_model.texture_data.size();
	}
}

void MapDrawerSoft::Draw(
	const MapState& map_state,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
	const EntityId player_monster_id )
{
	PC_UNUSED( player_monster_id );

	if( current_map_data_ == nullptr )
		return;

	const float viewport_size_x= float(rendering_context_.viewport_size.Width ());
	const float viewport_size_y= float(rendering_context_.viewport_size.Height());
	const float screen_transform_x= viewport_size_x * 0.5f;
	const float screen_transform_y= viewport_size_y * 0.5f;

	m_Mat4 cam_shift_mat, cam_mat, screen_flip_mat;
	cam_shift_mat.Translate( -camera_position );
	screen_flip_mat.Scale( m_Vec3( 1.0f, -1.0f, 1.0f ) );
	cam_mat= cam_shift_mat * view_rotation_and_projection_matrix * screen_flip_mat;

	for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
	{
		if( static_model.model_id >= current_map_data_->models_description.size() )
			continue;

		m_Mat4 rotate_mat, translate_mat, view_mat;
		rotate_mat.RotateZ( static_model.angle );
		translate_mat.Translate( static_model.pos );

		view_mat= rotate_mat * translate_mat * cam_mat;

		const Model& model= current_map_data_->models[ static_model.model_id ];
		const unsigned int first_animation_vertex= model.animations_vertices.size() / model.frame_count * static_model.animation_frame;

		for( unsigned int t= 0u; t < model.regular_triangles_indeces.size(); t+= 3u )
		{
			RasterizerVertexTextured vertices_fixed[3];
			bool clipped= false;
			for( unsigned int tv= 0u; tv < 3u; tv++ )
			{
				const Model::Vertex& vertex= model.vertices[ model.regular_triangles_indeces[t + tv] ];
				const Model::AnimationVertex& animation_vertex= model.animations_vertices[ first_animation_vertex + vertex.vertex_id ];


				const m_Vec3 vertex_pos= m_Vec3( float(animation_vertex.pos[0]), float(animation_vertex.pos[1]), float(animation_vertex.pos[2]) ) / 2048.0f;
				m_Vec3 vertex_projected= vertex_pos * view_mat;
				const float w= vertex_pos.x * view_mat.value[3] + vertex_pos.y * view_mat.value[7] + vertex_pos.z * view_mat.value[11] + view_mat.value[15];
				if( w <= 0.25f )
				{
					clipped= true;
					break;
				}

				vertex_projected/= w;
				vertex_projected.z= w;

				vertex_projected.x= ( vertex_projected.x + 1.0f ) * screen_transform_x;
				vertex_projected.y= ( vertex_projected.y + 1.0f ) * screen_transform_y;

				if( vertex_projected.x < 0.0f || vertex_projected.x > viewport_size_x ||
					vertex_projected.y < 0.0f || vertex_projected.y > viewport_size_y )
				{
					clipped= true;
					break;
				}

				vertices_fixed[tv].x= fixed16_t( vertex_projected.x * 65536.0f );
				vertices_fixed[tv].y= fixed16_t( vertex_projected.y * 65536.0f );
				vertices_fixed[tv].u= fixed16_t( vertex.tex_coord[0] * float(model.texture_size[0]) * 65536.0f );
				vertices_fixed[tv].v= fixed16_t( vertex.tex_coord[1] * float(model.texture_size[1]) * 65536.0f );
			}
			if( clipped ) continue;

			// Back side culling.
			// TODO - make it befor frustrum culling.
			fixed16_t vecs[2][2];
			vecs[0][0]= vertices_fixed[1].x - vertices_fixed[0].x;
			vecs[0][1]= vertices_fixed[1].y - vertices_fixed[0].y;
			vecs[1][0]= vertices_fixed[2].x - vertices_fixed[1].x;
			vecs[1][1]= vertices_fixed[2].y - vertices_fixed[1].y;
			if( Fixed16Mul( vecs[0][0], vecs[1][1] ) > Fixed16Mul( vecs[1][0], vecs[0][1] ) )
				continue;

			const ModelsGroup::ModelEntry& model_entry= map_models_.models[ static_model.model_id ];
			rasterizer_.SetTexture(
				model_entry.texture_size[0], model_entry.texture_size[1],
				map_models_.textures_data.data() + model_entry.texture_data_offset );

			rasterizer_.DrawAffineTexturedTriangle( vertices_fixed );
		}
	}
}

void MapDrawerSoft::DrawWeapon(
	const WeaponState& weapon_state,
	const m_Mat4& projection_matrix,
	const m_Vec3& camera_position,
	const float x_angle, const float z_angle )
{
	// TODO
	PC_UNUSED( weapon_state );
	PC_UNUSED( projection_matrix );
	PC_UNUSED( camera_position );
	PC_UNUSED( x_angle );
	PC_UNUSED( z_angle );
}

} // PanzerChasm
