//Sample using LiquidCrystal library
#include <LiquidCrystal.h>
#include <RTClib.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
RTC_DS1307 rtc;

/* Amount of microseconds the program will pause after detecting a button event. This is necessary since several button events per second would be detected otherwise.*/
#define DELAY_TIME 90

// Last X hours to take into account when calculating the total
// consumption.
#define CONSUMPTION_TIME_SPAN 24

#define DEFAULT_VOLUME 150
#define DEFAULT_REMAINING 40
#define DEFAULT_SPOONS 5

// Just having 50 bottles will hopefully do the job. Having 256 would
// be too much for the Arduino Uno to handle (although it's no problem
// for the Arduino Mega)
#define MAX_BOTTLES 50

// Amount of seconds the display stays on until it powers off
// automatically. Between 1 and 60.
#define DISPLAY_ACTIVE_DURATION 60

// Specifies whether the custom shield or - if not defined - the DF
// Robot LCD Keypad Shield was used.
#define CUSTOM_SHIELD

#ifndef CUSTOM_SHIELD
// Intermediate variable for adjusting the time if no RTC chip is
// present.
DateTime custom_time = DateTime(2020, 9, 9, 0, 0, 0);
#endif


// In case the DF Robot Shield (or just a LCD display with buttons but
// no RTC is used, we can not rely on it's now() function and have to
// use our own.
DateTime nowCustom() {
#ifdef CUSTOM_SHIELD
	return rtc.now();
#else
	return custom_time + TimeSpan(int32_t(millis()/1000));
#endif
}
	

enum Button { buttonRight, buttonUp, buttonDown, buttonLeft,
	buttonSelect, buttonNone };

enum Views { viewHistory, viewMenu, viewNewBottle, viewConsumption, viewTime, viewNone };
enum BottleItem { bottleSpoons, bottleVolume, bottleRemaining, bottleTime, bottleNone };


struct Bottle {
	float spoons;
	int volume;
	int remaining;
	DateTime time;

	void print() const;
	// Since the spoons of milk powder themselves contribute to the
	// total volume, they will be taken into account using this
	// function.
	//
	// According to the instruction on the Aptamil package roughly
	// three spoons amount to 10ml.
	float totalVolume() const;
	float consumedVolume() const;

	// Consumed volume time `spoons` to only account the milk which
	// actually got drunken.
	//
	// According to the instruction on the Aptamil package a spoons
	// corresponds to 4.6g.
	float consumedPowder() const;

	// Helper function determining whether the time stamp of a bottle
	// does lie within the last `hours` regardless of the shield used.
	bool consumedDuringLastXHours(byte hours) const;
};

void Bottle::print() const {
	Serial.println("bottle:");
	Serial.print("\tspoons: ");
	Serial.println(spoons);
	Serial.print("\tvolume: ");
	Serial.println(volume);
	Serial.print("\tremaining: ");
	Serial.println(remaining);
	Serial.print("\ttime: ");
	Serial.println(time.timestamp());
}

float Bottle::totalVolume() const {
	return static_cast<float>(volume) + spoons * 10 / 3;
}

float Bottle::consumedVolume() const {
	return totalVolume() - remaining;
}
float Bottle::consumedPowder() const {
	return consumedVolume() / totalVolume() * spoons * 4.6;
}

bool Bottle::consumedDuringLastXHours(byte hours) const {
	DateTime now = nowCustom();
	return time >= now - TimeSpan(0, hours, 0, 0);
}

class Crate {
public:
	Crate();
	~Crate();

	// Calculates the amount of consumed volume and milk powder -
	// based and the spoons used on percentage of the consumed volume
	// - within the past `since` hours. The results are provided using
	// the pointers `volume` and `powder`.
	void totalConsumption(byte since, int *volume, float *powder) const;

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
			Serial.println(m_bottles[ii].time.timestamp());
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

void Crate::totalConsumption(byte since, int *volume, float *powder) const {

	float consumed_volume = 0;
	float consumed_powder = 0;
	
	// Get the indices of all bottles of the last `since` hours.
	for ( int ii = 0; ii < MAX_BOTTLES; ii++ ) {
		if ( m_bottles[ii].consumedDuringLastXHours(since) ) {
			consumed_volume += m_bottles[ii].consumedVolume();
			consumed_powder += m_bottles[ii].consumedPowder();
			m_bottles[ii].print();
		}
	}

	*volume = consumed_volume;
	*powder = consumed_powder;
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
#ifndef CUSTOM_SHIELD
	void setTimeView();
#endif

	void toggleDisplay( bool on );

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

	// Whether to redraw the current view.
	bool m_update_display;
	
	// Global constants used within the functions.

	// Additional constant for browsing the different menu options.
	// 2 - new bottle
	// 3 - total consumption
	// 4 - set time
	int m_menu_item;

	// One after another the
	// 0 - amount of spoons
	// 1 - volume
	// 2 - time of creation will be inserted.
	// The following variable will keep track of which is the current one.
	int m_bottle_option;
	float m_bottle_spoons;
	int m_bottle_volume;
	int m_bottle_remaining;
	DateTime m_bottle_time;

	// 0 - setting the hours and 1 - setting the minutes for both the
	// custom time and a new bottle.
	byte m_set_time_state;
};


Interface::Interface() : m_current_view( viewHistory )
					   , m_menu_item( viewNewBottle )
					   , m_bottle_option( bottleSpoons )
					   , m_bottle_spoons( DEFAULT_SPOONS )
					   , m_bottle_volume( DEFAULT_VOLUME )
					   , m_bottle_time( nowCustom() )
					   , m_displayed_bottle( 0 )
					   , m_display_active( true )
					   , m_update_display( true )
					   , m_last_user_interaction( nowCustom() )
					   , m_set_time_state( 0 )
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
	}
	m_last_user_interaction = nowCustom();
	if ( adc_key_in < 720 ) {
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
	m_last_user_interaction = nowCustom();
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

void Interface::toggleDisplay( bool on ) {
	if ( !on && m_display_active ) {
		if ( nowCustom() - TimeSpan(0, 0, 0, DISPLAY_ACTIVE_DURATION) >
			 m_last_user_interaction ) {
			// lcd.noDisplay();
			m_display_active = false;
		}
	} else if ( on ) {
		lcd.display();
		m_display_active = true;
		m_update_display = true;
	}
}

// Resets the global variables to their initial state (done before entering the view).
void Interface::restoreDefaults()
{
	m_bottle_option = bottleSpoons;
	m_bottle_spoons = DEFAULT_SPOONS;
	m_bottle_volume = DEFAULT_VOLUME;
	m_bottle_remaining = DEFAULT_REMAINING;
	m_bottle_time = nowCustom();

	m_displayed_bottle = m_crate.getCurrentBottleIdx();

	m_set_time_state = 0;
}


/* General view displaying a single bottle (determined by `index`
   within the array `bottles`. */
void Interface::bottleView(int index)
{
	Bottle current_bottle = m_crate.getBottle( index );
	
	if ( m_update_display ) {
		m_update_display = false;
		lcd.clear();
		lcd.setCursor(0,0);

		// In case of an empty bottle, make 0 the default instead.
		if ( current_bottle.volume == 0 ) {
			m_bottle_remaining = 0;
		}

		switch ( m_bottle_option ) {
		case bottleRemaining: {
			// Specify what's left of the bottle.
			lcd.print("Remaining:");
			if ( m_bottle_remaining >= 100 ) {
				lcd.setCursor(0,1);
			}
			else if ( m_bottle_remaining >= 10 ) {
				lcd.setCursor(1,1);
			} else {
				lcd.setCursor(2,1);
			}
	
			lcd.print(m_bottle_remaining);
			lcd.setCursor(3,1);
			lcd.print("ml");
			break;
		}

		default: {
			// Default view showing the details of the particular
			// bottle.
      
			lcd.setCursor( 0, 0 );
			lcd.print( current_bottle.spoons );

			if ( current_bottle.remaining > 100 ) {
				lcd.setCursor( 7, 0 );
			} else if ( current_bottle.remaining >= 10 ) {
				lcd.setCursor( 8, 0 );
			} else {
				lcd.setCursor(9,0);
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
	  

			if ( current_bottle.time.hour() < 10 ) {
				lcd.setCursor(3,1);
				lcd.print("0");
				lcd.setCursor(4,1);
			} else {
				lcd.setCursor(3,1);
			}
			lcd.print(current_bottle.time.hour());
			lcd.setCursor(5,1);
			lcd.print(":");
			if ( current_bottle.time.minute() < 10 ) {
				lcd.setCursor(6,1);
				lcd.print("0");
				lcd.setCursor(7,1);
			} else {
				lcd.setCursor(6,1);
			}
			lcd.print(current_bottle.time.minute());
			break;
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case buttonRight: {
		if ( m_bottle_option != bottleRemaining ) {
			m_bottle_option = bottleRemaining;
		} else {
			current_bottle.remaining = m_bottle_remaining;
			m_crate.setBottle( index, current_bottle );
			m_bottle_option = bottleSpoons;
		}
		m_update_display = true;
		break;
	}
	case buttonLeft: {
		if ( m_bottle_option != bottleRemaining ) {
			// Abort and return to current bottle view.
			m_update_display = true;
			view();
			break;
		} else {
			m_bottle_option = bottleSpoons;
			m_update_display = true;
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
			if ( m_bottle_remaining < current_bottle.volume ) {
				m_bottle_remaining += 10;
			}
		}
		m_update_display = true;
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
			if ( m_bottle_remaining > 0 ) {
				m_bottle_remaining -= 10;
			}
		}
		m_update_display = true;
		break;
	}
	}
}

/* Main navigation utility allowing the user to reach all the different views and options. */
void Interface::menuView()
{
	if ( m_update_display ){
		m_update_display = false;
		
		lcd.clear();
		lcd.setCursor(0,0);
		switch ( m_menu_item ) {

#ifndef CUSTOM_SHIELD
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
		default: {
			Serial.println("Unknown m_menu_item in menuView");
		}
		}
#else
		case viewNewBottle: {
			lcd.print("> New bottle");
			lcd.setCursor(2,1);
			lcd.print("Total consumption");
			break;
		}
		case viewConsumption: {
			lcd.setCursor(2,0);
			lcd.print("New bottle");
			lcd.setCursor(0,1);
			lcd.print("> Total consumption");
			break;
		}
		default: {
			Serial.println("Unknown m_menu_item in menuView");
		}
		}
#endif
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case buttonRight: {
		// Restore the default values for all options within the particular views.
		restoreDefaults();

		// Enter the selected view.
		m_current_view = m_menu_item;
		m_update_display = true;
		break;
	}
	case buttonLeft: {
		restoreDefaults();
		// Abort and return to current bottle view.
		m_current_view = viewHistory;
		m_displayed_bottle = m_crate.getCurrentBottleIdx();
		m_update_display = true;
		break;
	}
	case buttonUp: {
#ifndef CUSTOM_SHIELD
		if ( m_menu_item == viewNewBottle ) {
			m_menu_item = viewTime;
		} else if ( m_menu_item == viewConsumption ) {
			m_menu_item = viewNewBottle;
		} else if ( m_menu_item == viewTime ) {
			m_menu_item = viewConsumption;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
#else
		if ( m_menu_item == viewNewBottle ) {
			m_menu_item = viewConsumption;
		} else if ( m_menu_item == viewConsumption ) {
			m_menu_item = viewNewBottle;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
#endif
		m_update_display = true;
		break;
	}
	case buttonDown: {
#ifndef CUSTOM_SHIELD
		if ( m_menu_item == viewNewBottle ) {
			m_menu_item = viewConsumption;
		} else if ( m_menu_item == viewConsumption ) {
			m_menu_item = viewTime;
		} else if ( m_menu_item == viewTime ) {
			m_menu_item = viewNewBottle;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
#else
		if ( m_menu_item == viewNewBottle ) {
			m_menu_item = viewConsumption;
		} else if ( m_menu_item == viewConsumption ) {
			m_menu_item = viewNewBottle;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
#endif
		m_update_display = true;
		break;
	}
	}
}

/* Allows the user to add a new bottle.*/
void Interface::newBottleView()
{
	if ( m_update_display ) {
		m_update_display = false;
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
			if ( m_bottle_time.hour() < 10 ) {
				lcd.setCursor(6,0);
				lcd.print("0");
				lcd.setCursor(7,0);
			} else {
				lcd.setCursor(6,0);
			}
			lcd.print(m_bottle_time.hour());
			lcd.setCursor(8,0);
			lcd.print(":");
			if ( m_bottle_time.minute() < 10 ) {
				lcd.setCursor(9,0);
				lcd.print("0");
				lcd.setCursor(10,0);
			} else {
				lcd.setCursor(9,0);
			}
			lcd.print(m_bottle_time.minute());

		if ( m_set_time_state == 0 ) {
			lcd.setCursor(6,1);
			lcd.print("--");
		} else {
			lcd.setCursor(9,1);
			lcd.print("--");
		}	break;
		}
		default: {
			Serial.println("Unsupported bottle_option in switch in newBottleView");
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case buttonRight: {
		if ( m_bottle_option == bottleSpoons ) {
			m_bottle_option = bottleVolume;
		} else if ( m_bottle_option == bottleVolume ) {
			m_bottle_option = bottleTime;
		} else if ( m_bottle_option == bottleTime &&
					m_set_time_state == 0 ) {
			m_set_time_state = 1;
		} else {
			m_crate.addBottle( m_bottle_spoons,
							   m_bottle_volume,
							   m_bottle_time );
			m_displayed_bottle = m_crate.getCurrentBottleIdx();
			restoreDefaults();
			m_current_view = viewHistory;
		}
		m_update_display = true;
		break;
	}
	case buttonLeft: {
		if ( m_bottle_option == 0 ) {
			restoreDefaults();
			// Abort and return to current bottle view.
			m_current_view = viewHistory;
		} else if ( m_bottle_option == bottleVolume ) {
			m_bottle_option = bottleSpoons;
		} else {
			if ( m_set_time_state == 0 ) {
				m_bottle_option = bottleVolume;
			} else {
				m_set_time_state = 0;
			}
		}
		m_update_display = true;
		break;
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
			if ( m_set_time_state == 0 ) {
				m_bottle_time = m_bottle_time + TimeSpan(0,1,0,0);
			} else {
				m_bottle_time = m_bottle_time + TimeSpan(0,0,1,0);
			}
		}
		default: {
			Serial.println("Unsupported BottleItem for up in newBottleView");
			break;
		}
		}
		m_update_display = true;
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
			if ( m_set_time_state == 0 ) {
				m_bottle_time = m_bottle_time - TimeSpan(0,1,0,0);
			} else {
				m_bottle_time = m_bottle_time - TimeSpan(0,0,1,0);
			}
			break;
		}
		default: {
			Serial.println("Unsupported BottleItem for up in newBottleView");
			break;
		}
		}
		m_update_display = true;
		break;
	}
	}

}

/* Displays the total consumption in volume and number of spoons within the last 24 hours.*/
void Interface::consumptionView()
{
	
	// Ensure this view is only drawn once (since it's not possible to
	// change the amount of remaining volume once the user entered
	// this view).
	if ( m_update_display ){
		m_update_display = false;

		int consumed_volume;
		float consumed_powder;
		m_crate.totalConsumption(CONSUMPTION_TIME_SPAN, &consumed_volume, &consumed_powder);

		lcd.clear();
		lcd.setCursor(0,0);

		lcd.print("Volume:");
		if ( consumed_volume > 1000 ) {
			lcd.setCursor(9,0);
		} else if ( consumed_volume > 100  ) {
			lcd.setCursor(10,0);
		} else {
			lcd.setCursor(11,0);
		}
		lcd.print(consumed_volume);
		lcd.setCursor(13,0);
		lcd.print("ml");

		lcd.setCursor(0,1);
		lcd.print("Powder:");
		lcd.setCursor(8,1);
		lcd.print(consumed_powder);
		lcd.setCursor(13,1);
		lcd.print("g");
	}

	int lcd_key = readButtons();

	if ( lcd_key == buttonLeft ) {
		restoreDefaults();
		// Abort and return to current bottle view.
		m_current_view = viewHistory;
		m_displayed_bottle = m_crate.getCurrentBottleIdx();
		m_update_display = true;
	}
}

/* Displays all bottles stored in memory with latest first. Uses the currentBottleView() for a particular pair of bottles.*/
void Interface::historyView()
{
	bottleView(m_displayed_bottle);
}

#ifndef CUSTOM_SHIELD
/* Allows the user to insert a reference time.*/
void Interface::setTimeView()
{
	if ( m_update_display ) {
		m_update_display = false;
		lcd.clear();
		
		lcd.print("Time:");
		if ( custom_time.hour() < 10 ) {
			lcd.setCursor(6,0);
			lcd.print("0");
			lcd.setCursor(7,0);
		} else {
			lcd.setCursor(6,0);
		}
		lcd.print(custom_time.hour());
		lcd.setCursor(8,0);
		lcd.print(":");
		if ( custom_time.minute() < 10 ) {
			lcd.setCursor(9,0);
			lcd.print("0");
			lcd.setCursor(10,0);
		} else {
			lcd.setCursor(9,0);
		}
		lcd.print(custom_time.minute());

		if ( m_set_time_state == 0 ) {
			lcd.setCursor(6,1);
			lcd.print("--");
		} else {
			lcd.setCursor(9,1);
			lcd.print("--");
		}
	}
		
	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case buttonRight: {
		if ( m_set_time_state == 0 ) {
			m_set_time_state = 1;
		} else {
			restoreDefaults();
			m_current_view = viewHistory;
		}
		m_update_display = true;
		break;
	}
	case buttonLeft: {
		if ( m_set_time_state == 0 ) {
			restoreDefaults();
			// Abort and return to current bottle view.
			m_current_view = viewHistory;
		} else {
			m_set_time_state = 0;
		}
		m_update_display = true;
		break;
	}
	case buttonUp: {
		if ( m_set_time_state == 0 ) {
			custom_time = custom_time + TimeSpan(0,1,0,0);
		} else {
			custom_time = custom_time + TimeSpan(0,0,1,0);
		}
		m_update_display = true;
		break;
	}
	case buttonDown: {
		if ( m_set_time_state == 0 ) {
			custom_time = custom_time - TimeSpan(0,1,0,0);
		} else {
			custom_time = custom_time - TimeSpan(0,0,1,0);
		}
		m_update_display = true;
		break;
	}
	}

}
#endif

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
#ifndef CUSTOM_SHIELD
		case viewTime: {
			setTimeView();
			break;
		}
#endif
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
				m_menu_item = viewNewBottle;
				m_update_display = true;
			}
		} else {
			toggleDisplay( true );
		}
	} else {
		// If no button was hit we will check if it is already time to
		// automatically turn off the display.
		toggleDisplay( false );
	}
}

Interface app;

void setup(){
	Serial.begin(9600);

#ifndef CUSTOM_SHIELD
	if ( !rtc.begin() ) {
		lcd.println("Couldn't find RTC");
		Serial.println("Couldn't find RTC");
		while (1);
	}

	if ( !rtc.isrunning() ) { 
		lcd.print("RTC is NOT running!");
		Serial.print("RTC is NOT running!");
	}

	// Use the time of the computer uploading this sketch to set the
	// RTC.
    rtc.adjust( DateTime(F(__DATE__), F(__TIME__)) );
#endif
  
	lcd.begin(16, 2);
	lcd.setCursor(0,0);
	lcd.print("Bottle-Spass");
}

void loop() {
	app.view();
}
