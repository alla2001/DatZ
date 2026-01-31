# DatZ Undead System - Setup Guide

## Scripts Created (Ready to Use)

| File | Purpose |
|------|---------|
| `DatZUndeadBase.c` | Core zombie entity - handles AI, targeting, attacks |
| `DatZUndeadBrain.c` | State machine AI (IDLE/WANDER/CHASE/ATTACK/etc) |
| `DatZUndeadCoordinator.c` | Global manager - target sharing, spatial tracking |
| `DatZUndeadSpawner.c` | Zone-based spawner with player proximity activation |
| `DatZUndeadDamageHandler.c` | Custom damage processing (headshots, resistance) |
| `DatZUndeadMotion.c` | Movement wobble/stumble effects |
| `DatZUndeadHeavy.c` | Tank variant (slow, tough, high damage) |
| `DatZUndeadRunner.c` | Fast variant (quick, fragile, swarm attacks) |
| `DatZUndeadFaction.c` | Faction class (non-playable, hostile) |

## Workbench Setup Required

### 1. Create Faction (FactionManager)

In your GameMode prefab's FactionManager, add a new faction:
- **Type**: `DatZUndeadFaction`
- **Faction Key**: `UNDEAD`
- **Name**: `Undead`
- **Is Playable**: `false`
- **Is Military**: `false`
- **Friendly To Self**: `true`
- **Friendly Factions**: (none - hostile to all)

### 2. Create Undead Character Prefab

Create a new prefab inheriting from ChimeraCharacter:
1. Set entity class to `DatZUndeadBase`
2. Add components:
   - `DatZUndeadDamageHandler` (replaces SCR_CharacterDamageManagerComponent)
   - `DatZUndeadMotion` (optional - for shambling movement)
   - `FactionAffiliationComponent` → Default Faction: `UNDEAD`
   - `SCR_CharacterControllerComponent`
   - `CharacterAnimationComponent`
   - `RplComponent` (for multiplayer sync)
3. Configure attributes:
   - Walk Speed: 3.5
   - Sprint Speed: 6.0
   - Attack Damage: 15
   - Attack Range: 1.5
   - Detection Radius: 50

### 3. Create Variant Prefabs

**Heavy Zombie:**
- Inherit from base zombie prefab
- Set entity class to `DatZUndeadHeavy`
- Use larger/bulkier character model
- Increase health in damage handler

**Runner Zombie:**
- Inherit from base zombie prefab
- Set entity class to `DatZUndeadRunner`
- Use thinner character model
- Decrease health in damage handler

### 4. Create Spawner Prefab

Create a new prefab:
1. Set entity class to `DatZUndeadSpawner`
2. Configure attributes:
   - Undead Prefabs: Add your zombie prefabs
   - Zone Radius: 100-200m
   - Min Population: 5
   - Max Population: 15
   - Activation Range: 200m
   - Spawn Interval: 30s

### 5. Add Coordinator to GameMode

In your GameMode prefab, add component:
- `DatZUndeadCoordinator`

### 6. Place Spawners in World

Place `DatZUndeadSpawner` entities in cities/towns where zombies should appear.

## Architecture Comparison

| BaconZ (Original) | DatZ (New) |
|-------------------|------------|
| InfectedCharacter | DatZUndeadBase |
| Behavior Trees (.bt) | DatZUndeadBrain (state machine) |
| SeekerEntity + InfectedGroup | DatZUndeadCoordinator |
| InfectedSpawnerEntity | DatZUndeadSpawner |
| ExplodeOnDeathComponent | DatZUndeadDamageHandler |
| ClumsinessComponent | DatZUndeadMotion |

## Testing

1. Place a spawner in the world
2. Enter play mode
3. Approach the spawner zone (within activation range)
4. Zombies should spawn and begin wandering
5. Get close enough for detection
6. Zombies should chase and attack
7. Kill zombies to test damage/death handling
