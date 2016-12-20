// Usage - define this macro, include this file, undefine macro
#ifndef MESSAGE_FUNC
#error
#endif

MESSAGE_FUNC(MonsterState)
MESSAGE_FUNC(WallPosition)
MESSAGE_FUNC(PlayerSpawn)
MESSAGE_FUNC(PlayerPosition) // position of player, which recieve this message.
MESSAGE_FUNC(PlayerState)
MESSAGE_FUNC(PlayerWeapon)
MESSAGE_FUNC(ItemState)
MESSAGE_FUNC(StaticModelState)
MESSAGE_FUNC(SpriteEffectBirth)
MESSAGE_FUNC(ParticleEffectBirth)
MESSAGE_FUNC(MapEventSound)
MESSAGE_FUNC(MonsterLinkedSound)
MESSAGE_FUNC(MonsterSound)
MESSAGE_FUNC(RocketState)
MESSAGE_FUNC(RocketBirth)
MESSAGE_FUNC(RocketDeath)

// Reliable server to client
MESSAGE_FUNC(MapChange)
MESSAGE_FUNC(MonsterBirth)
MESSAGE_FUNC(MonsterDeath)
MESSAGE_FUNC(TextMessage)

// Unrealiable client to server
MESSAGE_FUNC(PlayerMove)

// Reliable client to server
