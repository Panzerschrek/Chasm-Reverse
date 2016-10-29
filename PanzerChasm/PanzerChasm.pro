include(../Common/Common.pri)

SDL_INCLUDES_DIR = ../../SDL2-2.0.3/include
SDL_LIBS_DIR = ../../SDL2-2.0.3/lib/x86

LIBS+= $$SDL_LIBS_DIR/SDL2main.lib
LIBS+= $$SDL_LIBS_DIR/SDL2.lib
LIBS+= libopengl32

CONFIG( debug, debug|release ) {
	DEFINES+= DEBUG
}

INCLUDEPATH+= ../../panzer_ogl_lib
INCLUDEPATH+= $$SDL_INCLUDES_DIR

SOURCES+= \
	client/client.cpp \
	client/map_drawer.cpp \
	client/map_state.cpp \
	client/movement_controller.cpp \
	connection_info.cpp \
	game_resources.cpp \
	host.cpp \
	images.cpp \
	log.cpp \
	loopback_buffer.cpp \
	main.cpp \
	map_loader.cpp \
	menu.cpp \
	menu_drawer.cpp \
	messages_extractor.cpp \
	messages_sender.cpp \
	model.cpp \
	server/map.cpp \
	server/player.cpp \
	server/server.cpp \
	system_window.cpp \
	time.cpp \
	text_draw.cpp \
	vfs.cpp \

HEADERS+= \
	assert.hpp \
	client/client.hpp \
	client/map_drawer.hpp \
	client/map_state.hpp \
	client/movement_controller.hpp \
	connection_info.hpp \
	game_resources.hpp \
	host.hpp \
	host_commands.hpp \
	i_connection.hpp \
	images.hpp \
	log.hpp \
	loopback_buffer.hpp \
	map_loader.hpp \
	math_utils.hpp \
	menu.hpp \
	menu_drawer.hpp \
	messages.hpp \
	messages_extractor.hpp \
	messages_extractor.inl \
	messages_sender.hpp \
	model.hpp \
	rendering_context.hpp \
	server/map.hpp \
	server/player.hpp \
	server/server.hpp \
	size.hpp \
	system_event.hpp \
	system_window.hpp \
	text_draw.hpp \
	time.hpp \
	vfs.hpp \

SOURCES+= \
	../Common/files.cpp \
	../../panzer_ogl_lib/polygon_buffer.cpp \
	../../panzer_ogl_lib/shaders_loading.cpp \
	../../panzer_ogl_lib/texture.cpp \
	../../panzer_ogl_lib/framebuffer.cpp \
	../../panzer_ogl_lib/func_addresses.cpp \
	../../panzer_ogl_lib/glsl_program.cpp \
	../../panzer_ogl_lib/matrix.cpp \
	../../panzer_ogl_lib/ogl_state_manager.cpp \

HEADERS+= \
	../Common/files.hpp \
	../../panzer_ogl_lib/ogl_state_manager.hpp \
	../../panzer_ogl_lib/panzer_ogl_lib.hpp \
	../../panzer_ogl_lib/polygon_buffer.hpp \
	../../panzer_ogl_lib/shaders_loading.hpp \
	../../panzer_ogl_lib/texture.hpp \
	../../panzer_ogl_lib/vec.hpp \
	../../panzer_ogl_lib/framebuffer.hpp \
	../../panzer_ogl_lib/func_declarations.hpp \
	../../panzer_ogl_lib/glsl_program.hpp \
	../../panzer_ogl_lib/matrix.hpp \
	../../panzer_ogl_lib/bbox.hpp \
