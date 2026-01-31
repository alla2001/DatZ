// Force 6x day speed, 12x night speed, automatic weather/wind, biased toward cloudy
modded class SCR_TimeAndWeatherHandlerComponent : SCR_BaseGameModeComponent
{
	// Weather bias check interval in ms (5 real minutes)
	static const int WEATHER_BIAS_INTERVAL_MS = 300000;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer())
			return;

		// 6x day, 12x night (night passes faster so players spend less time in darkness)
		m_fDayTimeAcceleration = 6.0;
		m_fNightTimeAcceleration = 12.0;

		// Auto weather: random start + allow changes
		m_bRandomStartingWeather = true;
		m_bRandomWeatherChanges = true;

		// Disable wind override so weather system controls it
		m_bWindOverride = false;
	}

	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!Replication.IsServer())
			return;

		// Force start with cloudy/overcast weather (less rain)
		ChimeraWorld chWorld = ChimeraWorld.CastFrom(world);
		if (chWorld)
		{
			TimeAndWeatherManagerEntity manager = chWorld.GetTimeAndWeatherManager();
			if (manager)
			{
				// 50% Cloudy, 35% Overcast, 15% Rainy start
				int roll = Math.RandomInt(0, 100);
				string startWeather;
				if (roll < 50)
					startWeather = "Cloudy";
				else if (roll < 85)
					startWeather = "Overcast";
				else
					startWeather = "Rainy";

				manager.ForceWeatherTo(true, startWeather, 0, 0.001);
			}
		}

		// Periodically bias weather toward cloudy/overcast
		GetGame().GetCallqueue().CallLater(BiasWeather, WEATHER_BIAS_INTERVAL_MS, true);
	}

	// Every 5 real minutes, chance to push weather toward cloudy/overcast (but rarely rain)
	protected void BiasWeather()
	{
		if (!Replication.IsServer())
			return;

		ChimeraWorld chWorld = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!chWorld)
			return;

		TimeAndWeatherManagerEntity manager = chWorld.GetTimeAndWeatherManager();
		if (!manager)
			return;

		WeatherState currentState = manager.GetCurrentWeatherState();
		if (!currentState)
			return;

		string currentName = currentState.GetStateName();

		// If rainy, 40% chance to improve to overcast
		if (currentName == "Rainy")
		{
			if (Math.RandomFloat(0, 1) < 0.4)
				manager.ForceWeatherTo(true, "Overcast", 0.5, 0.001);
			return;
		}

		// If overcast, small chance (20%) to go rainy, otherwise stay
		if (currentName == "Overcast")
		{
			if (Math.RandomFloat(0, 1) < 0.2)
				manager.ForceWeatherTo(true, "Rainy", 0.5, 0.001);
			return;
		}

		// If cloudy, 30% chance to go overcast
		if (currentName == "Cloudy")
		{
			if (Math.RandomFloat(0, 1) < 0.3)
				manager.ForceWeatherTo(true, "Overcast", 0.5, 0.001);
			return;
		}

		// If clear, 50% chance to go cloudy
		if (currentName == "Clear")
		{
			if (Math.RandomFloat(0, 1) < 0.5)
				manager.ForceWeatherTo(true, "Cloudy", 0.5, 0.001);
			return;
		}
	}
}
