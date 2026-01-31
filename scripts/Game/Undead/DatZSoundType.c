// DatZ Sound Types - Constants for zombie hearing system

class DatZSoundType
{
	// Gunshots - unsuppressed
	static const int GUNSHOT_PISTOL = 1;
	static const int GUNSHOT_RIFLE = 2;
	static const int GUNSHOT_SHOTGUN = 3;
	static const int GUNSHOT_MG = 4;
	static const int GUNSHOT_SNIPER = 5;

	// Gunshots - suppressed
	static const int GUNSHOT_PISTOL_SUPPRESSED = 11;
	static const int GUNSHOT_RIFLE_SUPPRESSED = 12;
	static const int GUNSHOT_SHOTGUN_SUPPRESSED = 13;
	static const int GUNSHOT_MG_SUPPRESSED = 14;
	static const int GUNSHOT_SNIPER_SUPPRESSED = 15;

	// Explosions
	static const int EXPLOSION = 20;
	static const int EXPLOSION_GRENADE = 21;
	static const int EXPLOSION_ROCKET = 22;
	static const int EXPLOSION_VEHICLE = 23;

	// Vehicles
	static const int VEHICLE_ENGINE = 30;
	static const int VEHICLE_HORN = 31;
	static const int VEHICLE_CRASH = 32;
	static const int VEHICLE_DOOR = 33;

	// Footsteps
	static const int FOOTSTEP_PRONE = 40;
	static const int FOOTSTEP_CROUCH = 41;
	static const int FOOTSTEP_WALK = 42;
	static const int FOOTSTEP_RUN = 43;
	static const int FOOTSTEP_SPRINT = 44;

	// Actions
	static const int MELEE_SWING = 50;
	static const int MELEE_HIT = 51;
	static const int DOOR_OPEN = 52;
	static const int DOOR_BREAK = 53;
	static const int ITEM_DROP = 54;
	static const int ITEM_PICKUP = 55;
	static const int RELOAD = 56;
	static const int VOICE = 57;
}
