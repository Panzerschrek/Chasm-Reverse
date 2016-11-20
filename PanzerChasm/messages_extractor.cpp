#include "messages_extractor.hpp"

namespace PanzerChasm
{

size_t MessagesExtractor::c_messages_size[ size_t(MessageId::NumMessages) ]=
{
	[size_t(MessageId::Unknown)]= sizeof(Messages::MessageBase),

	[size_t(MessageId::MonsterState)]= sizeof(Messages::MonsterState),
	[size_t(MessageId::WallPosition)]= sizeof(Messages::WallPosition),
	[size_t(MessageId::PlayerPosition)]= sizeof(Messages::PlayerPosition),
	[size_t(MessageId::PlayerState)]= sizeof(Messages::PlayerState),
	[size_t(MessageId::StaticModelState)]= sizeof(Messages::StaticModelState),
	[size_t(MessageId::SpriteEffectBirth)]= sizeof(Messages::SpriteEffectBirth),
	[size_t(MessageId::RocketState)]= sizeof(Messages::RocketState),
	[size_t(MessageId::RocketBirth)]= sizeof(Messages::RocketBirth),
	[size_t(MessageId::RocketDeath)]= sizeof(Messages::RocketDeath),

	[size_t(MessageId::MapChange)]= sizeof(Messages::MapChange),
	[size_t(MessageId::MonsterBirth)]= sizeof(Messages::MonsterBirth),
	[size_t(MessageId::MonsterDeath)]= sizeof(Messages::MonsterDeath),
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
