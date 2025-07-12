class HungerThirstHUD : SCR_InfoDisplay
{
   
    private SliderWidget m_wWaterLevelImage;
	private SliderWidget m_wFoodLevelImage;
   	private SliderWidget m_wHPLevelImage;
	private SliderWidget m_wStaminaLevelImage;
	private ref Color foodColor ;
	private ref Color waterColor;
	private ref Color hpColor;
	
	private ImageWidget HPArrow1;
	private ImageWidget HPArrow2;
	private ImageWidget HPArrow3;
    override event void OnStartDraw(IEntity owner)
    {
        super.OnStartDraw(owner);

            if (!m_wFoodLevelImage){
			         m_wFoodLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("FoodLevel"));
			foodColor = m_wFoodLevelImage.GetColor();
		}
       

        if (!m_wWaterLevelImage)
		{
			    m_wWaterLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("WaterLevel"));
			waterColor=m_wWaterLevelImage.GetColor();
		}
        
		
		if(!m_wHPLevelImage)
		{
		   m_wHPLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("HPLevel"));
			hpColor=m_wHPLevelImage.GetColor();
		}
		if(!m_wStaminaLevelImage)
		{
		   m_wStaminaLevelImage = SliderWidget.Cast(m_wRoot.FindAnyWidget("StaminaLevel"));

		}
		
		
		HPArrow1 = ImageWidget.Cast(m_wRoot.FindAnyWidget("HPArrow1"));
		
		HPArrow2 = ImageWidget.Cast(m_wRoot.FindAnyWidget("HPArrow2"));
		
		HPArrow3 = ImageWidget.Cast(m_wRoot.FindAnyWidget("HPArrow3"));
    }
	override void OnInit (IEntity owner)
	{

	
	}
   void UpdateStats(float food, float water, float HP, float stamina, int healthUp)
{
	// ---- FOOD BAR ----
	if (m_wFoodLevelImage)
	{
		if (food < 30)
			m_wFoodLevelImage.SetColor(Color(0.9,0.1,0.1,1)); // Red
		else if (food < 50)
			m_wFoodLevelImage.SetColor(Color(0.9, 0.9, 0.1,1)); // Yellow
		else
			m_wFoodLevelImage.SetColor(foodColor); // Default

		m_wFoodLevelImage.SetCurrent(food / 100);
	}

	// ---- WATER BAR ----
	if (m_wWaterLevelImage)
	{
		if (water < 30)
			m_wWaterLevelImage.SetColor(Color(0.9,0.1,0.1,1)); // Red
		else if (water < 50)
			m_wWaterLevelImage.SetColor(Color(0.9, 0.9, 0.1,1)); // Yellow
		else
			m_wWaterLevelImage.SetColor(waterColor); // Default

		m_wWaterLevelImage.SetCurrent(water / 100);
	}

	// ---- HP TEXT ----
	if (m_wHPLevelImage)
	{
		if (HP < 20)
			m_wHPLevelImage.SetColor(Color(1, 0, 0,1)); // Red
		else if (HP < 50)
			m_wHPLevelImage.SetColor(Color(1, 1, 0,1)); // Yellow
		else
			m_wHPLevelImage.SetColor(hpColor); // Default

		m_wHPLevelImage.SetCurrent(HP / 100);
		
			HPArrow1.SetVisible(false);
			HPArrow2.SetVisible(false);
			HPArrow3.SetVisible(false);
			HPArrow1.SetRotation(0);
			HPArrow2.SetRotation(0);
			HPArrow3.SetRotation(0);
			if(healthUp<0){
				HPArrow1.SetVisible(true);
				HPArrow1.SetRotation(180);
			}
			if(healthUp<-1){
				HPArrow2.SetVisible(true);
				HPArrow2.SetRotation(180);
			}
			if(healthUp<-2){
				HPArrow3.SetVisible(true);
				HPArrow3.SetRotation(180);
			}
			if(healthUp>0)
			HPArrow1.SetVisible(true);
				
			if(healthUp>1)
			HPArrow2.SetVisible(true);
				
			if(healthUp>2)
			HPArrow3.SetVisible(true);
	}
		
		// ---- STAMINA BAR ----
	if (m_wStaminaLevelImage)
	{
		
		if (stamina < 100)
			m_wStaminaLevelImage.SetColor(Color(1, 1, 1,0.43)); // Red

		else
			m_wStaminaLevelImage.SetColor(Color(1, 1, 1,0)); // Default
			
			
		m_wStaminaLevelImage.SetCurrent(stamina/ 100);
	}
}

}
