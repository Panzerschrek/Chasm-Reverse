#include "messages_extractor.hpp"

namespace PanzerChasm
{

size_t MessagesExtractor::c_messages_size[ size_t(MessageId::NumMessages) ]=
{
	[size_t(MessageId::Unknown)]= sizeof(Messages::MessageBase),

	[size_t(MessageId::EntityState)]= sizeof(Messages::EntityState),
	[size_t(MessageId::WallPosition)]= sizeof(Messages::WallPosition),
	[size_t(MessageId::PlayerPosition)]= sizeof(Messages::PlayerPosition),
	[size_t(MessageId::StaticModelState)]= sizeof(Messages::StaticModelState),
	[size_t(MessageId::SpriteEffectBirth)]= sizeof(Messages::SpriteEffectBirth),

	[size_t(MessageId::MapChange)]= sizeof(Messages::MapChange),
	[size_t(MessageId::EntityBirth)]= sizeof(Messages::EntityBirth),
	[size_t(MessageId::EntityDeath)]= sizeof(Messages::EntityDeath),
	[size_t(MessageId::TextMessage)]= sizeof(Messages::TextMessage),

	[size_t(MessageId::PlayerMove)]= sizeof(Messages::PlayerMove),

	[size_t(MessageId::PlayerShot)]= sizeof(Messages::PlayerShot),
};

MessagesExtractor::MessagesExtractor( IConnectionPtr connection )
	: connection_(std::move(connection))
{}

MessagesExtractor::~MessagesExtractor()
{}

} // namespace PanzerChasm
