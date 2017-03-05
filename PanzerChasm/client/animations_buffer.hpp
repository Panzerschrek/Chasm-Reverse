#pragma once

#include <buffer_texture.hpp>
#include <texture.hpp>

#include "../model.hpp"

namespace PanzerChasm
{

class AnimationsBuffer final
{
public:
	static constexpr unsigned int c_2d_texture_width= 1024u;

	static AnimationsBuffer AsTextureBuffer( const Model::AnimationVertex* vertices, unsigned int animation_vertex_count );
	static AnimationsBuffer As2dTexture( const Model::AnimationVertex* vertices, unsigned int animation_vertex_count );

	AnimationsBuffer();
	AnimationsBuffer( AnimationsBuffer&& other );
	AnimationsBuffer( const AnimationsBuffer& other )= delete;

	~AnimationsBuffer();

	AnimationsBuffer& operator=( AnimationsBuffer&& other );
	AnimationsBuffer& operator=( const AnimationsBuffer& other )= delete;

	void Bind( unsigned int unit= 0 );

private:
	r_Texture texture_2d_;
	r_BufferTexture buffer_texture_;
};

} // PanzerChasm
