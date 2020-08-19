//Sample using LiquidCrystal library
#include <LiquidCrystal.h>

/* Amount of microseconds the program will pause after detecting a button event. This is necessary since several button events per second would be detected otherwise.*/
#define DELAY_TIME 100
#define DEFAULT_VOLUME 140
#define DEFAULT_SPOONS 3.5
#define MAX_BOTTLES 256

enum Button { right, up, down, left, select, none };

enum Views { history, menu, newBottle, consumption, time };

class Interface {

public:

	void view();

private:

	void bottleView();
	void historyView();
	void menuView();
	void newBottleView();
	void consumptionView();
	void setTimeView();

	int readButtons() static;
	// Resets the global variables to their initial state (done before entering the view).
	void restoreDefaults();
	
	// Specify the pins used by the LCD
	LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

	Crate crate;

	int lcd_key      = 0;
	int adc_key_in   = 0;
	int current_view = 0;
	
	// Specifies which bottle in bottles is the most recent one.
	unsigned current_bottle = 0;
	unsigned displayed_bottle = 0;
	
	// Global constants used within the functions.

	// Additional constant for browsing the different menu options.
	// 2 - new bottle
	// 3 - total consumption
	// 4 - set time
	int menu_item=2;
	int old_menu_item=-1;

	// One after another the
	// 0 - amount of spoons
	// 1 - volume
	// 2 - time of creation will be inserted.
	// The following variable will keep track of which is the current one.
	int bottle_option = 0;
	int old_bottle_option = -1;
	float bottle_spoons = DEFAULT_SPOONS;
	float old_bottle_spoons = -1.0;
	int bottle_volume = DEFAULT_VOLUME;
	int old_bottle_volume = -1;
	int bottle_time = 12;
	int old_bottle_time = -1;
	int old_displayed_bottle = -1;

};


struct Bottle {
  float spoons;
  int volume;
  int remaining;
  int time;
};

class Crate {
public:
/* Adds a new bottle to the global bottles array and increments
   current_bottle. This array will be filled in a cyclic fashion while
   overwriting the oldest entry.

Upon adding a bottle is considered full.*/
	void addBottle(float spoons, int volume, int time);

	int totalVolume(int from, int till);
	int totalSpoons(int from, int till);

	byte get_current_bottle() static;
	Bottle get_bottle(int index) static;
	
private:
	Bottle bottles[MAX_BOTTLES];

	byte current_bottle;
};

inline byte Crate::get_current_bottle() static {
	return current_bottle;
}
inline Bottle Crate::get_bottle(int index) static {
	return bottles[index];
}



void Crate::addBottle(float spoons, int volume, int time)
{
  Bottle new_bottle = {spoons, volume, volume, time};

  current_bottle++;
  if ( current_bottle >= MAX_BOTTLES ) {
    current_bottle = 0;
  }

  bottles[current_bottle] = new_bottle;
}

int Interface::readButtons() static {
{
  adc_key_in = analogRead(0);      // read the value from the sensor

  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if ( adc_key_in > 790 ) {
	  return Button::none;
  }

  // We make this the 1st option for speed reasons since it will be the most likely result

  delay(DELAY_TIME);

  if ( adc_key_in < 50 ) {
	  return Button::right;
  }
  if ( adc_key_in < 195 ) {
	  return Button::up;
  }
  if ( adc_key_in < 380 ) {
	  return Button::down;
  }
  if ( adc_key_in < 555 ) {
	  return Button::left;
  }
  if ( adc_key_in < 790 ) {
	  return Button::select;
  }

  return Button::none;
}

// Resets the global variables to their initial state (done before entering the view).
 void Interface::restoreDefaults()
{
  menu_item = 2;
  old_menu_item = -1;

  bottle_option = 0;
  old_bottle_option = -1;
  bottle_spoons = DEFAULT_SPOONS;
  old_bottle_spoons = -1.0;
  bottle_volume = DEFAULT_VOLUME;
  old_bottle_volume = -1;
  bottle_time = 12;
  old_bottle_time = -1;

  displayed_bottle = current_bottle;
  old_displayed_bottle = -1;
}


/* General view displaying a single bottle (determined by `index`
   within the array `bottles`. */
void bottleView(int index)
{
  bool update_display=false;
  if (bottle_option != old_bottle_option)
  {
    old_bottle_option = bottle_option;
    update_display = true;
  }

  if (old_displayed_bottle != displayed_bottle) {
    old_displayed_bottle = displayed_bottle;
    update_display = true;
  }

  // Actually specifies 'remaining' and not volume.
  if (bottle_option == 1 && bottle_volume != old_bottle_volume)
  {
    old_bottle_volume = bottle_volume;
    update_display = true;
  }

  if (update_display)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    Serial.print("index: ");
    Serial.print(index);
    Serial.print(" bottle: ");
    Serial.print(bottles[index].spoons);
    Serial.print(", ");
    Serial.print(bottles[index].volume);
    Serial.print(", ");
    Serial.print(bottles[index].remaining);
    Serial.print(", ");
    Serial.println(bottles[index].time);
    switch (bottle_option)
    {

    case 0:
      // Default view showing the details of the particular bottle.
      
      {
        lcd.setCursor(0,0);
	lcd.print(bottles[index].spoons);

	if (bottles[index].remaining < 100) {
	  lcd.setCursor(8, 0);
	} else {
	  lcd.setCursor(7, 0);
	}
	lcd.print(bottles[index].remaining);
	
	lcd.setCursor(10, 0);
	lcd.print("/");

	if (bottles[index].volume < 100) {
	  lcd.setCursor(12, 0);
	} else {
	  lcd.setCursor(11, 0);
	}
	lcd.print(bottles[index].volume);
	lcd.setCursor(14, 0);
	lcd.print("ml");
	  

	lcd.setCursor(3, 1);
	lcd.print(bottles[index].time);
	lcd.setCursor(5,1);
	lcd.print(":");
	lcd.setCursor(6,1);
	lcd.print(bottles[index].time);
	  
	
        break;
      }
    case 1:
      // Specify what's left of the bottle.
      {
        lcd.print("Remaining:");
        if (bottle_volume >= 100)
        {
          lcd.setCursor(0,1);
        }
        else
        {
          lcd.setCursor(1,1);
        }
	
        lcd.print(min(bottle_volume,bottles[index].volume));
        lcd.setCursor(3,1);
        lcd.print("ml");
        break;
      }
    }
  }

  lcd_key = readLCDButtons();

  switch (lcd_key)
  {
  case btnRIGHT:
    {
      if (bottle_option < 1)
      {
        bottle_option++;
        break;
      }
      else 
      {
	bottles[index].remaining = bottle_volume;
	bottle_option = 0;
        break;
      }
    }
    case btnLEFT:
      {
        if (bottle_option == 0)
        {
          // Abort and return to current bottle view.
          startView(0);
          break;
        }
        else
        {
          bottle_option--;
          break;
        }
      }
    case btnUP:
      {
	if (bottle_option == 0) {
	  if (displayed_bottle != MAX_BOTTLES - 1){
	    displayed_bottle++;
	  } else {
	    displayed_bottle = 0;
	  }
	} else if (bottle_option == 1) {
            if (bottle_volume < bottles[index].volume)
            {
              bottle_volume = bottle_volume + 10;
            }
            break;
	}
	break;
      }
  case btnDOWN:
    {
      if (bottle_option == 0) {
	if (displayed_bottle != 0){
	  displayed_bottle--;
	} else {
	  displayed_bottle = MAX_BOTTLES - 1;
	}
      } else if (bottle_option == 1) {
          // There is a maximum value of a bottle of 150 ml.
          if (bottle_volume > 0)
          {
            bottle_volume = bottle_volume - 10;
          }
          break;
        }
    }
    break;
  }

}

/* Main navigation utility allowing the user to reach all the different views and options. */
void menuView()
{
  if (menu_item != old_menu_item)
  {
    old_menu_item = menu_item;

    lcd.clear();
    lcd.setCursor(0,0);
    switch (menu_item)
    {

    case VIEW_NEW_BOTTLE:
      {
        lcd.print("> New bottle");
        lcd.setCursor(2,1);
        lcd.print("Total consumption");
        break;
      }
    case VIEW_TOTAL_CONSUMPTION:
      {
        lcd.print("> Total consumption");
        lcd.setCursor(2,1);
        lcd.print("History");
        break;
      }
    case VIEW_SET_TIME:
      {
        lcd.print("> Set time");
        lcd.setCursor(2,1);
        lcd.print("New bottle");
        break;
      }
    }
  }

  lcd_key = readLCDButtons();

  switch (lcd_key)
  {
  case btnRIGHT:
    {
      // Restore the default values for all options within the particular views.
      restoreDefaults();

      // Enter the selected view.
      current_view = menu_item;
      break;
    }
  case btnLEFT:
    {
      restoreDefaults();
      // Abort and return to current bottle view.
      current_view = VIEW_HISTORY;
      displayed_bottle = current_bottle;
      break;
    }
  case btnUP:
    {
      if (menu_item > VIEW_NEW_BOTTLE) {
        menu_item = menu_item - 1;
      } 
      else {
        menu_item = VIEW_SET_TIME;
      }
      break;
    }
  case btnDOWN:
    {
      if (menu_item < VIEW_SET_TIME) {
        menu_item = menu_item + 1;
      } 
      else {
        menu_item = VIEW_NEW_BOTTLE;
      }
      break;
    }
  }
}

/* Allows the user to add a new bottle.*/
void newBottleView()
{
  bool update_display=false;
  if (bottle_option != old_bottle_option)
  {
    old_bottle_option = bottle_option;
    update_display = true;
  }

  if (bottle_option == 0 && bottle_spoons != old_bottle_spoons)
  {
    old_bottle_spoons = bottle_spoons;
    update_display = true;
  }
  else if (bottle_option == 1 && bottle_volume != old_bottle_volume)
  {
    old_bottle_volume = bottle_volume;
    update_display = true;
  }
  else if (bottle_option == 2 && bottle_time != old_bottle_time)
  {
    old_bottle_time = bottle_time;
    update_display = true;
  }

  if (update_display)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    switch (bottle_option)
    {

    case 0:
      {
        lcd.print("Spoons:");
        lcd.setCursor(8,0);
        lcd.print(bottle_spoons);
        break;
      }
    case 1:
      {
        lcd.print("Volume:");
        if (bottle_volume >= 100)
        {
          lcd.setCursor(8,0);
        }
        else
        {
          lcd.setCursor(9,0);
        }
        lcd.print(bottle_volume);
        lcd.setCursor(11,0);
        lcd.print("ml");
        break;
      }
    case 2:
      {
        lcd.print("Time:");
        lcd.setCursor(6,0);
        lcd.print(bottle_time);
        break;
      }
    }
  }

  lcd_key = readLCDButtons();

  switch (lcd_key)
  {
  case btnRIGHT:
    {
      if (bottle_option < 2)
      {
        bottle_option++;
        break;
      } else if (bottle_option == 2) {
	addBottle(
	  bottle_spoons,
	  bottle_volume,
	  bottle_time);
	displayed_bottle = current_bottle;
	restoreDefaults();
	current_view = VIEW_HISTORY;
        break;
      }
    case btnLEFT:
      {
        if (bottle_option == 0)
        {
	  restoreDefaults();
          // Abort and return to current bottle view.
	  current_view=VIEW_HISTORY;
          break;
        }
        else
        {
          bottle_option--;
          break;
        }
      }
    case btnUP:
      {
        switch (bottle_option)
        {
        case 0:
          {
            bottle_spoons = bottle_spoons + 0.25;
            break;
          }
        case 1:
          {
            // There is a maximum value of a bottle of 150 ml.
            if (bottle_volume < 150)
            {
              bottle_volume = bottle_volume + 10;
            }
            break;
          }
        case 2:
          {
            bottle_time++;
            break;
          }
        }
      }
      break;
    }
  case btnDOWN:
    {
      switch (bottle_option)
      {
      case 0:
        {
          if (bottle_spoons > 0) 
          {
            bottle_spoons = bottle_spoons - 0.25;
          }
          break;
        }
      case 1:
        {
          // There is a maximum value of a bottle of 150 ml.
          if (bottle_volume > 0)
          {
            bottle_volume = bottle_volume - 10;
          }
          break;
        }
      case 2:
        {
          bottle_time--;
          break;
        }
      }
    }
    break;
  }

}

/* Displays the total consumption in volume and number of spoons within the last 24 hours.*/
void consumptionView()
{
}

/* Displays all bottles stored in memory with latest first. Uses the currentBottleView() for a particular pair of bottles.*/
void historyView()
{
  bottleView(displayed_bottle);
}

/* Allows the user to insert a reference time.*/
void setTimeView()
{
}

/* Displays a particular view. */
void startView(byte selection)
{
  switch (selection)
  {
  case VIEW_HISTORY:
    {
      historyView();
      break;
    }
  case VIEW_MENU:
    {
      menuView();
      break;
    }
  case VIEW_NEW_BOTTLE:
    {
      newBottleView();
      break;
    }
  case VIEW_TOTAL_CONSUMPTION:
    {
      consumptionView();
      break;
    }
  case VIEW_SET_TIME:
    {
      setTimeView();
    }
  default:
    {
      historyView();
      break;
    }
  }

  lcd_key = readLCDButtons();

  switch (lcd_key)
  {
  case btnSELECT:
    {
      current_view=VIEW_MENU;
      break;
    }
  }
}

void setup()
{
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Bottle-Spass");
  Serial.begin(9600);
}

void loop()
{
  // Update the LCD display.
  view(current_view);
}









