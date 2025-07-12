//------------------------------------------------------------------------------------------------
modded class SCR_DestructibleBuildingEntity : Building
{
	[Attribute()]
	bool ignoreLoot;
	[Attribute()]
	bool ignoreDestruction;
	protected const vector TRACE_DIRECTIONS[3] = { vector.Up,vector.Right, vector.Forward};
	override void EOnInit(IEntity owner)
	{

		if (SCR_Global.IsEditMode())
			return;

		if(!Replication.IsServer())return;
		if(!ignoreLoot)
		GetGame().GetCallqueue().CallLater(SpawnServerLoot,15000,false,owner);
		if(!ignoreDestruction)
		GetGame().GetCallqueue().CallLater(RandomDesctruct,4000,false);
	}
	
	void SpawnServerLoot(IEntity owner)
	{
		World world = owner.GetWorld();
		DZLootSystem loot = DZLootSystem.GetInstance();
		if (!loot || !loot.defaultLootSpawner) return;

		vector minBound, maxBound;
		GetBounds(minBound, maxBound);

		// Add vertical margin (e.g., 0.5 meters above the floor and below the ceiling)
		float verticalMargin =2;
minBound[1] =0+2;
maxBound[1] = maxBound[1] - verticalMargin;
minBound[0] = minBound[0] + verticalMargin;
maxBound[0] = maxBound[0] - verticalMargin;
minBound[2] = minBound[2] + verticalMargin;
maxBound[2] = maxBound[2] - verticalMargin;
// Calculate max allowed spawns based on building bounds
		
vector size = maxBound - minBound;
float volume = size[0]  *size[2] + size[1];
int maxSpawns = Math.Clamp(Math.Ceil(volume / 15.0), 1, 25); // Between 1 and 12

int successfulSpawns = 0;

for (int i = 0; i <50; i++)
	{
			
	if (successfulSpawns >= maxSpawns)
		break;

	vector randomLocalPos = RandomVector(minBound, maxBound);
	vector randomWorldPos = CoordToParent(randomLocalPos);

	if (IsInside(randomWorldPos))
	{
				
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = randomWorldPos;

		IEntity ent = GetGame().SpawnEntityPrefab(Resource.Load(loot.defaultLootSpawner), world, params);

		//vector origin = ent.GetOrigin();
		//origin[1] = origin[1] - findGround(origin);
		//ent.SetOrigin(origin);
		SCR_EntityHelper.SnapToGround(ent,startOffset: "0 0.5 0",maxLength:30);
				ent.SetOrigin(ent.GetOrigin()+"0 1 0");
		successfulSpawns++;
				ent.Update();
	}
}

}
	void RandomDesctruct(){
	
	SCR_DestructibleBuildingComponent desc = SCR_DestructibleBuildingComponent.Cast(FindComponent(SCR_DestructibleBuildingComponent));
	if (!desc)
	return;

		int seed = Math.Floor( GetOrigin()[0]); // Replace with a unique seed value per building or session
		Math.Randomize(seed); // Set the random seed globally (affects subsequent Math.RandomFloat calls)

		if (Math.RandomFloat01() >= 0.85)
		{
		desc.SetHealthScaled(Math.RandomFloat(0.0, 1.0)); // Use full range now that it’s seeded
		}
	}
	protected vector RandomVector(vector min, vector max)
	{
		vector result;
		result[0] = Math.RandomFloat(min[0], max[0]);
		result[1] = Math.RandomFloat(min[1], max[1]);
		result[2] = Math.RandomFloat(min[2], max[2]);


		return result;
	}
	//------------------------------------------------------------------------------------------------
	protected bool PerformTrace(notnull TraceParam param, vector start, vector direction, notnull BaseWorld world, float lengthMultiplier = 1)
	{
		param.Start = start - direction * lengthMultiplier;
		param.End = start + direction * lengthMultiplier;
		world.TraceMove(param, TraceFilter);

		return param.TraceEnt != null;
	}
//------------------------------------------------------------------------------------------------
	protected bool PerformTraceUp(notnull TraceParam param, vector start, vector direction, notnull BaseWorld world, float lengthMultiplier = 1)
	{
		param.Start = start + direction;
		param.End = start + direction * lengthMultiplier;
		world.TraceMove(param, TraceFilter);

		return param.TraceEnt != null;
	}
	//------------------------------------------------------------------------------------------------
//! Checks whether or not an entity is inside of the building, and if there is enough free space.
protected bool IsInside(vector start)
{
    IEntity owner = this;
    BaseWorld world = owner.GetWorld();

    TraceParam param = new TraceParam();
    param.Flags = TraceFlags.ENTS;
    param.LayerMask = EPhysicsLayerDefs.Projectile;
    //param.Include = owner; // Include only the building for performance reasons

    bool traceResult;
    for (int i = 0; i < 3; i++)
    {
        float lengthMultiplier =1; // Vertical needs to be long
        if (i == 0){
				lengthMultiplier =  30;
			          traceResult = PerformTraceUp(param, start, TRACE_DIRECTIONS[i], world, lengthMultiplier);
			}
  
        else
            traceResult = PerformTrace(param, start, TRACE_DIRECTIONS[i], world, lengthMultiplier);

        if (!traceResult && i == 0&& Math.RandomFloat(0,10)<9)
            return false; // Must have a roof
			
		  if (!traceResult && i == 1&& Math.RandomFloat(0,10)<6)
            return false; // Must have a roof
			
			if (!traceResult && i == 2&& Math.RandomFloat(0,10)<5)
           return false; // Must have a roof
    }

    // --- ADDITION: Now perform box overlap to check free space ---
    if (!HasEnoughFreeSpace(start))
        return false;

    return true;
}

//------------------------------------------------------------------------------------------------
//! Checks if a box around the given position would collide with something (e.g., walls, clutter)
protected bool HasEnoughFreeSpace(vector center)
{
    IEntity owner = this;
    BaseWorld world = owner.GetWorld();

    // Define the size of the box you want to test
    vector halfExtents = "0.7 0.7 0.7"; // Width, Height, Depth (adjust as needed)

    TraceBox trace = new TraceBox();
    trace.Start = center;
    trace.End = center; // Stationary box
    trace.Mins = -halfExtents;
    trace.Maxs = halfExtents;
    trace.LayerMask = EPhysicsLayerDefs.Projectile;
   
    

    float fraction = world.TraceMove(trace, TraceFilter);

    if (fraction!=1)
        return false; // Something blocking inside the box

    return true; // Box is free
}

	
	//------------------------------------------------------------------------------------------------
	//! Checks whether or not an entity is inside of the building, using a trace in each world axis
protected float findGround(vector start)
{

		IEntity owner =this;
		BaseWorld world = owner.GetWorld();
	

		TraceParam param = new TraceParam();
		param.Flags = TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;
		param.Include = owner; // Include only the building for performance reasons


    param.Start = start;
    param.End = start - vector.Up * 50;
	float traceResult = GetGame().GetWorld().TraceMove(param, TraceFilter);
    if (traceResult != 1.0)
    {
		vector hitPos = param.Start + traceResult * (param.End - param.Start);
   
        return start[1] - hitPos[1]; // Vertical distance from start to hit point
    }

    return start[0]; // No hit detected
}
		//------------------------------------------------------------------------------------------------
	//! Filters out unwanted entities
	protected bool TraceFilter(notnull IEntity e, vector start = "0 0 0", vector dir = "0 0 0")
	{
		return e == this;
	}

  
};
