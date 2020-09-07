//Sample using LiquidCrystal library
#include <Wire.h>
#include <LiquidCrystal.h>
#include <RTClib.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
RTC_DS1307 rtc;

/* Amount of microseconds the program will pause after detecting a button event. This is necessary since several button events per second would be detected otherwise.*/
#define DELAY_TIME 100

#define DEFAULT_VOLUME 140
#define DEFAULT_SPOONS 3.5

// Just having 50 bottles will hopefully do the job. Having 256 would
// be too much for the Arduino Uno to handle (although it's no problem
// for the Arduino Mega)
#define MAX_BOTTLES 50

// Amount of seconds the display stays on until it powers off automatically.
#define DISPLAY_ACTIVE_DURATION 30

// Specifies whether the custom shield or - if not defined - the DF
// Robot LCD Keypad Shield was used.
#define CUSTOM_SHIELD

enum Button { buttonRight, buttonUp, buttonDown, buttonLeft,
	buttonSelect, buttonNone };
enum Views { viewHistory, viewMenu, viewNewBottle, viewConsumption, viewTime, viewNone };
enum BottleItem { bottleSpoons, bottleVolume, bottleRemaining, bottleTime, bottleNone };

struct Bottle {
	float spoons;
	int volume;
	int remaining;
	DateTime time;
};

class Crate {
public:
	Crate();
	~Crate();

	int totalVolume(int from, int till) const;
	int totalSpoons(int from, int till) const;

	int getCurrentBottleIdx() const;
	Bottle getBottle(int idx) const;
	void setBottle(int idx, Bottle b);

	bool isBottleUndefined(int idx) const;
	/* Adds a new bottle to the global bottles array and increments
	   current_bottle. This array will be filled in a cyclic fashion while
	   overwriting the oldest entry.

	   Upon adding a bottle is considered full.*/
	void addBottle(float spoons, int volume, DateTime time);

	void printContent() const;
	
private:
	Bottle m_bottles[MAX_BOTTLES];

	byte m_current_bottle_idx;
};

Crate::Crate() : m_current_bottle_idx( 0 )
			   , m_bottles {} {
}
Crate::~Crate(){
}

int Crate::getCurrentBottleIdx() const {
	return m_current_bottle_idx;
}
Bottle Crate::getBottle(int idx) const {
	return m_bottles[idx];
}
void Crate::setBottle(int idx, Bottle b) {
	m_bottles[idx] = b;
}
bool Crate::isBottleUndefined(int idx) const {
	if ( m_bottles[idx].spoons == 0 &&
		 m_bottles[idx].volume == 0 &&
		 m_bottles[idx].remaining == 0 ) {
		return true;
	}
	return false;
}
void Crate::printContent() const {
	Serial.println("\nBottles stored in crate:");
	for ( int ii = 0; ii < MAX_BOTTLES; ii++ ) {
		if ( !isBottleUndefined( ii ) ) {
			Serial.print(ii);
			Serial.print(":\t");
			Serial.print(m_bottles[ii].spoons);
			Serial.print("\t");
			Serial.print(m_bottles[ii].volume);
			Serial.print(" ml\t");
			Serial.print(m_bottles[ii].remaining);
			Serial.print(" ml\t");
			Serial.println(m_bottles[ii].time.toString("hh:mm"));
		}
	}
}

void Crate::addBottle(float spoons, int volume, DateTime time)
{
	Bottle new_bottle = {spoons, volume, volume, time};

	m_current_bottle_idx++;
	if ( m_current_bottle_idx >= MAX_BOTTLES ) {
		m_current_bottle_idx = 0;
	}

	m_bottles[m_current_bottle_idx] = new_bottle;

	Serial.println("adding a bottle");
	printContent();
}

class Interface {

public:
	Interface();
	~Interface();

	void view();

private:

	void bottleView(int index);
	void historyView();
	void menuView();
	void newBottleView();
	void consumptionView();
	void setTimeView();

	void toggleDisplay();

	int readButtons() const;
	// Resets the global variables to their initial state (done before entering the view).
	void restoreDefaults();

	Crate m_crate;

	// Specifies whether to turn on the LCD display or not. This is
	// used to reduce power consumption and the display will be set to
	// off after `DISPLAY_ACTIVE_DURATION` without no user input
	// automatically.
	bool m_display_active;
	// Timestamp of the last user interaction.
	DateTime m_last_user_interaction;

	int m_current_view;
	
	// Specifies which bottle in bottles is the most recent one.
	int m_displayed_bottle;
	
	// Global constants used within the functions.

	// Additional constant for browsing the different menu options.
	// 2 - new bottle
	// 3 - total consumption
	// 4 - set time
	int m_menu_item;
	int m_old_menu_item;

	// One after another the
	// 0 - amount of spoons
	// 1 - volume
	// 2 - time of creation will be inserted.
	// The following variable will keep track of which is the current one.
	int m_bottle_option;
	int m_old_bottle_option;
	float m_bottle_spoons;
	float m_old_bottle_spoons;
	int m_bottle_volume;
	int m_old_bottle_volume;
	DateTime m_bottle_time;
	DateTime m_old_bottle_time;
	int m_old_displayed_bottle;

};


Interface::Interface() : m_current_view( viewHistory )
					   , m_menu_item( viewNewBottle )
					   , m_old_menu_item( viewNone )
					   , m_bottle_option( bottleSpoons )
					   , m_old_bottle_option( bottleNone )
					   , m_bottle_spoons( DEFAULT_SPOONS )
					   , m_old_bottle_spoons( -1.0 )
					   , m_bottle_volume( DEFAULT_VOLUME )
					   , m_old_bottle_volume( 1 )
					   , m_bottle_time( rtc.now() )
					   , m_old_bottle_time( DateTime(0,0,0) )
					   , m_displayed_bottle( 0 )
					   , m_old_displayed_bottle( -1 )
					   , m_display_active( true )
					   , m_crate{}{
}

Interface::~Interface(){
}

int Interface::readButtons() const {
	int adc_key_in = analogRead(0);      // read the value from the sensor

	// my buttons when read are centered at these values: 0, 144, 329, 504, 741
	// we add approx 50 to those values and check to see if we are
	// close

#ifndef CUSTOM_SHIELD
	if ( adc_key_in > 790 ) {
		return buttonNone;
	}
#endif
	
	// We make this the 1st option for speed reasons since it will be the most likely result
	delay(DELAY_TIME);

#ifdef CUSTOM_SHIELD
	if ( adc_key_in < 500 ) {
		return buttonNone;
	} else if ( adc_key_in < 720 ) {
		return buttonSelect;
	} else if ( adc_key_in < 900 ) {
		return buttonLeft;
	} else if ( adc_key_in < 950 ) {
		return buttonDown;
	} else if ( adc_key_in < 1005 ) {
		return buttonUp;
	} else {
		return buttonRight;
	}
#else
	if ( adc_key_in < 50 ) {
		return buttonRight;
	} else if ( adc_key_in < 195 ) {
		return buttonUp;
	} else if ( adc_key_in < 380 ) {
		return buttonDown;
	} else if ( adc_key_in < 555 ) {
		return buttonLeft;
	} else if ( adc_key_in < 790 ) {
		return buttonSelect;
	}
#endif
	
	return buttonNone;
}

void Interface::toggleDisplay() {
	if ( m_display_active ) {
		// if ( now() - m_last_user_interaction < DISPLAY_ACTIVE_DURATION ) {
		// 	lcd.noDisplay();
		// 	m_display_active = false;
		// }
	} else {
		lcd.display();
		m_display_active = true;
	}
}

// Resets the global variables to their initial state (done before entering the view).
void Interface::restoreDefaults()
{
	m_menu_item = viewNewBottle;
	m_old_menu_item = viewNone;

	m_bottle_option = bottleSpoons;
	m_old_bottle_option = bottleNone;
	m_bottle_spoons = DEFAULT_SPOONS;
	m_old_bottle_spoons = -1.0;
	m_bottle_volume = DEFAULT_VOLUME;
	m_old_bottle_volume = -1;
	m_bottle_time = rtc.now();
	m_old_bottle_time = DateTime(0, 0, 0);

	m_displayed_bottle = m_crate.getCurrentBottleIdx();
	m_old_displayed_bottle = -1;
}


/* General view displaying a single bottle (determined by `index`
   within the array `bottles`. */
void Interface::bottleView(int index)
{
	bool update_display = false;
	if ( m_bottle_option != m_old_bottle_option ) {
		m_old_bottle_option = m_bottle_option;
		update_display = true;
	}

	if ( m_old_displayed_bottle != m_displayed_bottle ) {
		m_old_displayed_bottle = m_displayed_bottle;
		update_display = true;
	}

	if ( m_bottle_option == bottleRemaining &&
		 m_bottle_volume != m_old_bottle_volume ) {
		m_old_bottle_volume = m_bottle_volume;
		update_display = true;
	}

	Bottle current_bottle = m_crate.getBottle( index );
	
	if ( update_display ) {
		lcd.clear();
		lcd.setCursor(0,0);

		switch ( m_bottle_option ) {
		case bottleRemaining: {
			// Specify what's left of the bottle.
			lcd.print("Remaining:");
			if ( m_bottle_volume >= 100 ) {
				lcd.setCursor(0,1);
			}
			else {
				lcd.setCursor(1,1);
			}
	
			lcd.print(min(m_bottle_volume,
						  current_bottle.volume));
			lcd.setCursor(3,1);
			lcd.print("ml");
			break;
		}

		default: {
			// Default view showing the details of the particular
			// bottle.
      
			lcd.setCursor( 0, 0 );
			lcd.print( current_bottle.spoons );

			if ( current_bottle.remaining < 100 ) {
				lcd.setCursor( 8, 0 );
			} else {
				lcd.setCursor( 7, 0 );
			}
			lcd.print(current_bottle.remaining);
	
			lcd.setCursor(10, 0);
			lcd.print("/");

			if (current_bottle.volume < 100) {
				lcd.setCursor(12, 0);
			} else {
				lcd.setCursor(11, 0);
			}
			lcd.print(current_bottle.volume);
			lcd.setCursor(14, 0);
			lcd.print("ml");
	  

			lcd.setCursor(3, 1);
			lcd.print(current_bottle.time.toString("hh:mm"));
			// lcd.setCursor(5,1);
			// lcd.print(":");
			// lcd.setCursor(6,1);
			// lcd.print(current_bottle.time);
	  
	
			break;
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case buttonRight: {
		if ( m_bottle_option != bottleRemaining ) {
			m_bottle_option = bottleRemaining;
			break;
		} else {
			current_bottle.remaining = m_bottle_volume;
			m_crate.setBottle( index, current_bottle );
			m_bottle_option = bottleSpoons;
			break;
		}
	}
	case buttonLeft: {
		if ( m_bottle_option != bottleRemaining ) {
			// Abort and return to current bottle view.
			view();
			break;
		} else {
			m_bottle_option = bottleSpoons;
			break;
		}
	}
	case buttonUp: {
		if ( m_bottle_option != bottleRemaining ) {
			if ( m_displayed_bottle != MAX_BOTTLES - 1 ){
				m_displayed_bottle++;
			} else {
				m_displayed_bottle = 0;
			}
		} else {
			if ( m_bottle_volume < current_bottle.volume ) {
				m_bottle_volume = m_bottle_volume + 10;
			}
			break;
		}
		break;
	}
	case buttonDown: {
		if ( m_bottle_option != bottleRemaining ) {
			if ( m_displayed_bottle != 0 ){
				m_displayed_bottle--;
			} else {
				m_displayed_bottle = MAX_BOTTLES - 1;
			}
		} else {
			// There is a maximum value of a bottle of 150 ml.
			if ( m_bottle_volume > 0 ) {
				m_bottle_volume = m_bottle_volume - 10;
			}
		}
		break;
	}
	}
}

/* Main navigation utility allowing the user to reach all the different views and options. */
void Interface::menuView()
{
	if ( m_menu_item != m_old_menu_item ){
		m_old_menu_item = m_menu_item;

		lcd.clear();
		lcd.setCursor(0,0);
		switch ( m_menu_item ) {

		case viewNewBottle: {
			lcd.print("> New bottle");
			lcd.setCursor(2,1);
			lcd.print("Total consumption");
			break;
		}
		case viewConsumption: {
			lcd.print("> Total consumption");
			lcd.setCursor(2,1);
			lcd.print("Set time");
			break;
		}
		case viewTime: {
			lcd.print("> Set time");
			lcd.setCursor(2,1);
			lcd.print("New bottle");
			break;
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case buttonRight: {
		// Restore the default values for all options within the particular views.
		restoreDefaults();

		// Enter the selected view.
		m_current_view = m_menu_item;
		break;
	}
	case buttonLeft: {
		restoreDefaults();
		// Abort and return to current bottle view.
		m_current_view = viewHistory;
		m_displayed_bottle = m_crate.getCurrentBottleIdx();
		break;
	}
	case buttonUp: {
		if ( m_menu_item == viewNewBottle ) {
			m_menu_item = viewTime;
		} else if ( m_menu_item == viewConsumption ) {
			m_menu_item = viewNewBottle;
		} else if ( m_menu_item == viewTime ) {
			m_menu_item = viewConsumption;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
		break;
	}
	case buttonDown: {
		if ( m_menu_item == viewNewBottle ) {
			m_menu_item = viewConsumption;
		} else if ( m_menu_item == viewConsumption ) {
			m_menu_item = viewTime;
		} else if ( m_menu_item == viewTime ) {
			m_menu_item = viewNewBottle;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
		break;
	}
	}
}

/* Allows the user to add a new bottle.*/
void Interface::newBottleView()
{
	bool update_display = false;
	if ( m_bottle_option != m_old_bottle_option ) {
		m_old_bottle_option = m_bottle_option;
		update_display = true;
	}

	if ( m_bottle_option == bottleSpoons &&
		 m_bottle_spoons != m_old_bottle_spoons) {
		m_old_bottle_spoons = m_bottle_spoons;
		update_display = true;
	}
	else if ( m_bottle_option == bottleVolume &&
			  m_bottle_volume != m_old_bottle_volume ) {
		m_old_bottle_volume = m_bottle_volume;
		update_display = true;
	}
	else if ( m_bottle_option == bottleTime &&
			  m_bottle_time != m_old_bottle_time ) {
		m_old_bottle_time = m_bottle_time;
		update_display = true;
	}

	if ( update_display ) {
		lcd.clear();
		lcd.setCursor(0,0);
		switch ( m_bottle_option ) {
		case bottleSpoons: {
			lcd.print("Spoons:");
			lcd.setCursor(8,0);
			lcd.print( m_bottle_spoons );
			break;
		}
		case bottleVolume: {
			lcd.print("Volume:");
			if ( m_bottle_volume >= 100 ) {
				lcd.setCursor(8,0);
			} else {
				lcd.setCursor(9,0);
			}
			lcd.print( m_bottle_volume );
			lcd.setCursor(11,0);
			lcd.print("ml");
			break;
		}
		case bottleTime: {
			lcd.print("Time:");
			lcd.setCursor(6,0);
			lcd.print( m_bottle_time.toString("hh:mm" ));
			break;
		}
		default: {
			Serial.println("Unsupported bottle_option in switch in newBottleView");
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case buttonRight: {
		if ( m_bottle_option == 0 ) {
			m_bottle_option = bottleVolume;
			break;
		} else if ( m_bottle_option == 1 ) {
			m_bottle_option = bottleTime;
			break;
		} else {
			m_crate.addBottle( m_bottle_spoons,
							   m_bottle_volume,
							   m_bottle_time );
			m_displayed_bottle = m_crate.getCurrentBottleIdx();
			restoreDefaults();
			m_current_view = viewHistory;
			break;
		}
	}
	case buttonLeft: {
		if ( m_bottle_option == 0 ) {
			restoreDefaults();
			// Abort and return to current bottle view.
			m_current_view = viewHistory;
			break;
		} else if ( m_bottle_option == 1 ) {
			m_bottle_option = bottleSpoons;
			break;
		} else {
			m_bottle_option = bottleVolume;
			break;
		}
	}
	case buttonUp: {
		switch ( m_bottle_option ) {
		case bottleSpoons: {
			m_bottle_spoons = m_bottle_spoons + 0.25;
			break;
		}
		case bottleVolume: {
			// There is a maximum value of a bottle.
			if ( m_bottle_volume < 250 ) {
				m_bottle_volume = m_bottle_volume + 10;
			}
			break;
		}
		case bottleTime: {
			m_bottle_time = m_bottle_time + TimeSpan(0,0,1,0);
			break;
		}
		default: {
			Serial.println("Unsupported BottleItem for up in newBottleView");
			break;
		}
		}
		break;
	}
	case buttonDown: {
		switch ( m_bottle_option ) {
		case bottleSpoons: {
			if ( m_bottle_spoons > 0 ) {
				m_bottle_spoons = m_bottle_spoons - 0.25;
			}
			break;
		}
		case bottleVolume: {
			if ( m_bottle_volume > 0 ) {
				m_bottle_volume = m_bottle_volume - 10;
			}
			break;
		}
		case bottleTime: {
			m_bottle_time = m_bottle_time - TimeSpan(0,0,1,0);
			break;
		}
		default: {
			Serial.println("Unsupported BottleItem for up in newBottleView");
			break;
		}
		}
		break;
	}
	}

}

/* Displays the total consumption in volume and number of spoons within the last 24 hours.*/
void Interface::consumptionView()
{
}

/* Displays all bottles stored in memory with latest first. Uses the currentBottleView() for a particular pair of bottles.*/
void Interface::historyView()
{
	bottleView(m_displayed_bottle);
}

/* Allows the user to insert a reference time.*/
void Interface::setTimeView()
{
}

/* Displays a particular view. */
void Interface::view()
{
	if ( m_display_active ) {
		switch ( m_current_view ) {
		case viewHistory: {
			historyView();
			break;
		}
		case viewMenu: {
			menuView();
			break;
		}
		case viewNewBottle: {
			newBottleView();
			break;
		}
		case viewConsumption: {
			consumptionView();
			break;
		}
		case viewTime: {
			setTimeView();
			break;
		}
		default: {
			historyView();
			break;
		}
		}
	}

	int lcd_key = readButtons();

	if ( lcd_key != buttonNone ) {
		if ( m_display_active ) {
			if ( lcd_key == buttonSelect ) {
				m_current_view = viewMenu;
			}
		} else {
			toggleDisplay();
		}
	} else {
		// If no button was hit we will check if it is already time to
		// automatically turn off the display.
		toggleDisplay();
	}
}

Interface app;

void setup(){

	if ( !rtc.begin() ) {
		lcd.println("Couldn't find RTC");
		while (1);
	}

	if ( !rtc.isrunning() ) { 
		lcd.print("RTC is NOT running!");
	}

	// Use the time of the computer uploading this sketch to set the
	// RTC.
    rtc.adjust( DateTime(F(__DATE__), F(__TIME__)) );
  
	lcd.begin(16, 2);
	lcd.setCursor(0,0);
	lcd.print("Bottle-Spass");
	Serial.begin(9600);
}

void loop()
{
	app.view();
}









