#pragma once

#include "panzer_ogl_lib.hpp"

class r_OGLState final
{
	friend class r_OGLStateManager;

public:
	// Default state variables.
	// Not equal to OpenGL initial state.
	// Use this, if has no difference, what to use.
	static const float default_clear_color[4];
	static constexpr const float default_clear_depth= 1.0f;
	static constexpr const GLenum default_cull_face_mode= GL_BACK;
	static constexpr const bool default_depth_mask= true;

	// State on OpenGL startup.
	r_OGLState();

	r_OGLState(
		bool blend, bool cull_face, bool depth_test, bool program_point_size,
		const GLenum* blend_func,
		const float* clear_color= default_clear_color,
		float clear_depth= default_clear_depth,
		GLenum cull_face_mode= default_cull_face_mode,
		bool depth_mask= default_depth_mask );

protected:
	bool blend_;
	bool cull_face_;
	bool depth_test_;
	bool program_point_size_;

	GLenum blend_func_[2];
	float clear_color_[4];
	float clear_depth_;
	GLenum cull_face_mode_;
	bool depth_mask_;
	GLenum depth_func_;
};

class r_OGLStateManager final
{
public:
	static void UpdateState( const r_OGLState& state );
	static void ResetState(); // Reset state to state at OpenGL startup.

	static const r_OGLState& GetCurrentState();

private:
	static r_OGLState state_;
};
