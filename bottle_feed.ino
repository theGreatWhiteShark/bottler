//Sample using LiquidCrystal library
#include <LiquidCrystal.h>

/*******************************************************
 * 
 * This program will test the LCD panel and the buttons
 * Mark Bramwell, July 2010
 * 
 ********************************************************/


// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// Values respresenting the different views.
// 0 - Current and last bottle
// 1 - Menu
// 2 - Enter a new bottle
// 3 - Total consumption within the last 24 hours
// 4 - Bottle history within the last 24 hours
// 5 - Set time
int current_view = 0;
#define VIEW_CURRENT_BOTTLE     0
#define VIEW_MENU               1
#define VIEW_NEW_BOTTLE         2
#define VIEW_TOTAL_CONSUMPTION  3
#define VIEW_HISTORY            4
#define VIEW_SET_TIME           5

/* Amount of microseconds the program will pause after detecting a button event. This is necessary since several button events per second would be detected otherwise.*/
#define DELAY_TIME 100

#define DEFAULT_VOLUME 140
#define DEFAULT_SPOONS 3.5

#define MAX_BOTTLES 2560

struct Bottle {
  byte spoons;
  byte volume;
  byte time;
};

Bottle bottles[MAX_BOTTLES];

// read the buttons
int readLCDButtons()
{
  adc_key_in = analogRead(0);      // read the value from the sensor

  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 790) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result

  delay(DELAY_TIME);

  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 195)  return btnUP;
  if (adc_key_in < 380)  return btnDOWN;
  if (adc_key_in < 555)  return btnLEFT;
  if (adc_key_in < 790)  return btnSELECT;

  return btnNONE;  // when all others fail, return this...
}

// Time will be handled in 


// Global constants used within the functions.

// Additional constant for browsing the different menu options.
// 2 - new bottle
// 3 - total consumption
// 4 - history
// 5 - set time
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

// Resets the global variables to their initial state (done before entering the view).
void restoreDefaults()
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
}

void setup()
{
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Bottle-Spass");
  Serial.begin(9600);
}

/* Shows the current (top) and the last (bottom) bottle. */
void currentBottleView()
{
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
    case VIEW_HISTORY:
      {
        lcd.print("> History");
        lcd.setCursor(2,1);
        lcd.print("Set time");
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
      // Abort and return to current bottle view.
      current_view = VIEW_CURRENT_BOTTLE;
      startView(0);
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
      }
      else 
      {
        Serial.println("New bottle ready:");
        Serial.println(bottle_spoons);
        Serial.println(bottle_volume);
        Serial.println(bottle_time);
        break;
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
  case VIEW_CURRENT_BOTTLE:
    {
      currentBottleView();
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
  case VIEW_HISTORY:
    {
      historyView();
      break;
    }
  case VIEW_SET_TIME:
    {
      setTimeView();
    }
  default:
    {
      currentBottleView();
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

void loop()
{
  // Update the LCD display.
  startView(current_view);
}









