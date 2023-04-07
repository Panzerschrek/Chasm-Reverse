// Usage - define this macro, include this file, undefine macro
#ifndef MESSAGE_FUNC
#error
#endif

MESSAGE_FUNC(DummyNetMessage)

MESSAGE_FUNC(ServerState)
MESSAGE_FUNC(MonsterState)
MESSAGE_FUNC(WallPosition)
MESSAGE_FUNC(PlayerSpawn)
MESSAGE_FUNC(PlayerPosition) // position of player, which recieve this message.
MESSAGE_FUNC(PlayerState)
MESSAGE_FUNC(PlayerWeapon)
MESSAGE_FUNC(PlayerItemPickup)
MESSAGE_FUNC(ItemState)
MESSAGE_FUNC(StaticModelState)
MESSAGE_FUNC(SpriteEffectBirth)
MESSAGE_FUNC(ParticleEffectBirth)
MESSAGE_FUNC(FullscreenBlendEffect)
MESSAGE_FUNC(MonsterPartBirth)
MESSAGE_FUNC(MapEventSound)
MESSAGE_FUNC(MonsterLinkedSound)
MESSAGE_FUNC(MonsterSound)
MESSAGE_FUNC(RocketState)
MESSAGE_FUNC(RocketBirth)
MESSAGE_FUNC(RocketDeath)
MESSAGE_FUNC(DynamicItemBirth)
MESSAGE_FUNC(DynamicItemUpdate)
MESSAGE_FUNC(DynamicItemDeath)
MESSAGE_FUNC(LightSourceBirth)
MESSAGE_FUNC(LightSourceDeath)
MESSAGE_FUNC(RotatingLightSourceBirth)
MESSAGE_FUNC(RotatingLightSourceDeath)

// Reliable server to client
MESSAGE_FUNC(MapChange)
MESSAGE_FUNC(MonsterBirth)
MESSAGE_FUNC(MonsterDeath)
MESSAGE_FUNC(TextMessage)
MESSAGE_FUNC(DynamicTextMessage)

// Unrealiable client to server
MESSAGE_FUNC(PlayerMove)

// Reliable client to server
MESSAGE_FUNC(PlayerName)
