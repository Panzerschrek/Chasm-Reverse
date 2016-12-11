#pragma once

enum class Particels
{
	Bullet= 0u,
	Sparcle= 1u,
	Point= 2u, // HZ - what is this
	Glass= 3u,
	Blood0= 5u,
	Blood1= 6u,
	Explosion= 8u,
	HeavySmoke= 9u,
	Dust= 10u,
	RocketSmoke= 11u,
	LightSmoke= 12u,
	Magic= 14u, // Wing-man rocket trail
	SmokeFaustRocket= 16u,
	SmokeGrenade= 18u, // // Grenade launcher trail
};


enum class ParticleEffect : unsigned char
{
	Glass,
	Blood,
	Bullet,
	Sparkles,
	Explosion,
};
