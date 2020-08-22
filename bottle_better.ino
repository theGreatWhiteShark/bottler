//Sample using LiquidCrystal library
#include <LiquidCrystal.h>

/* Amount of microseconds the program will pause after detecting a button event. This is necessary since several button events per second would be detected otherwise.*/
#define DELAY_TIME 100
#define DEFAULT_VOLUME 140
#define DEFAULT_SPOONS 3.5
#define MAX_BOTTLES 256

enum Button { right, up, down, left, select, none };
enum Views { history, menu, newBottle, consumption, time, none };
enum BottleItem { spoons, volume, remaining, time, none };

class Interface {

public:

	void view();

private:

	Interface();
	~Interface();

	void bottleView();
	void historyView();
	void menuView();
	void newBottleView();
	void consumptionView();
	void setTimeView();

	int readButtons() static;
	// Resets the global variables to their initial state (done before entering the view).
	void restoreDefaults();

	Crate m_crate;

	int m_current_view;
	
	// Specifies which bottle in bottles is the most recent one.
	unsigned m_displayed_bottle;
	
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
	int m_bottle_time;
	int m_old_bottle_time;
	int m_old_displayed_bottle;

};


struct Bottle {
	float spoons;
	int volume;
	int remaining;
	int time;
};

class Crate {
public:
	Crate();
	~Crate();
	/* Adds a new bottle to the global bottles array and increments
	   current_bottle. This array will be filled in a cyclic fashion while
	   overwriting the oldest entry.

	   Upon adding a bottle is considered full.*/
	void addBottle(float spoons, int volume, int time);

	int totalVolume(int from, int till);
	int totalSpoons(int from, int till);

	byte getCurrentBottleIdx() static;
	Bottle* getBottle(int idx);
	void setBottle(int idx, Bottle b);
	
private:
	Bottle m_bottles[MAX_BOTTLES];

	byte m_current_bottle_idx;
};

inline byte Crate::getCurrentBottleIdx() static {
	return m_current_bottle_idx;
}
inline Bottle* Crate::getBottle(int idx) {
	return m_bottles[idx];
}

Crate::Crate() : m_current_bottle( 0 )
			   , m_bottles {} {
}
Crate::~Crate(){
}


void Crate::addBottle(float spoons, int volume, int time)
{
	Bottle new_bottle = {spoons, volume, volume, time};

	m_current_bottle++;
	if ( m_current_bottle >= MAX_BOTTLES ) {
		m_current_bottle = 0;
	}

	m_bottles[m_current_bottle] = new_bottle;
}

Interface::Interface() : m_current_view( Views::history )
					   , m_menu_item( Views::newBottle )
					   , m_old_menu_item( Views::none )
					   , m_bottle_option( BottleItem::spoons )
					   , m_old_bottle_option( BottleItem::none )
					   , m_bottle_spoons( DEFAULT_SPOONS )
					   , m_old_bottle_spoons( -1.0 )
					   , m_bottle_volume( DEFAULT_VOLUME )
					   , m_old_bottle_volume( 1 )
					   , m_bottle_time( 12 )
					   , m_old_bottle_time( -1 )
					   , m_displayed_bottle( 0 )
					   , m_old_displayed_bottle( -1 )
					   , m_crate{}{
}

Interface::~Interface(){
}

int Interface::readButtons() static {
	int adc_key_in = analogRead(0);      // read the value from the sensor

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
	menu_item = Views::newBottle;
	m_old_menu_item = Views::none;

	bottle_option = BottleItem::spoons;
	m_old_bottle_option = BottleItem::none;
	bottle_spoons = DEFAULT_SPOONS;
	m_old_bottle_spoons = -1.0;
	bottle_volume = DEFAULT_VOLUME;
	m_old_bottle_volume = -1;
	bottle_time = 12;
	m_old_bottle_time = -1;

	m_displayed_bottle = m_crate.getCurrentBottleIdx;
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

	if ( m_bottle_option == BottleItem::remaining &&
		 m_bottle_volume != m_old_bottle_volume ) {
		m_old_bottle_volume = m_bottle_volume;
		update_display = true;
	}

	if ( update_display ) {
		lcd.clear();
		lcd.setCursor(0,0);

		// There is no concurrency. The bottle pointed to be the
		// return value of the function is guaranteed to persist till
		// the end of this function's scope.
		Bottle* p_current_bottle = m_crate.getBottle( index );

		switch ( m_bottle_option ) {
		case BottleItem::remaining: {
			// Specify what's left of the bottle.
			lcd.print("Remaining:");
			if ( m_bottle_volume >= 100 ) {
				lcd.setCursor(0,1);
			}
			else {
				lcd.setCursor(1,1);
			}
	
			lcd.print(min(m_bottle_volume,
						  p_current_bottle.volume));
			lcd.setCursor(3,1);
			lcd.print("ml");
			break;
		}

		default: {
			// Default view showing the details of the particular
			// bottle.
      
			lcd.setCursor( 0, 0 );
			lcd.print( p_current_bottle.spoons );

			if ( p_current_bottle.remaining < 100 ) {
				lcd.setCursor( 8, 0 );
			} else {
				lcd.setCursor( 7, 0 );
			}
			lcd.print(p_current_bottle.remaining);
	
			lcd.setCursor(10, 0);
			lcd.print("/");

			if (p_current_bottle.volume < 100) {
				lcd.setCursor(12, 0);
			} else {
				lcd.setCursor(11, 0);
			}
			lcd.print(p_current_bottle.volume);
			lcd.setCursor(14, 0);
			lcd.print("ml");
	  

			lcd.setCursor(3, 1);
			lcd.print(p_current_bottle.time);
			lcd.setCursor(5,1);
			lcd.print(":");
			lcd.setCursor(6,1);
			lcd.print(p_current_bottle.time);
	  
	
			break;
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case Button::right: {
		if ( m_bottle_option != BottleItem::remaining ) {
			m_bottle_option = BottleItem::remaining;
			break;
		} else {
			p_current_bottle.remaining = m_bottle_volume;
			m_bottle_option = BottleItem::spoons;
			break;
		}
	}
	case Button::left: {
		if ( m_bottle_option != BottleItem::remaining ) {
			// Abort and return to current bottle view.
			view();
			break;
		} else {
			m_bottle_option = BottleItem::spoons;
			break;
		}
	}
	case Button::up: {
		if ( m_bottle_option != BottleItem::remaining ) {
			if ( m_displayed_bottle != MAX_BOTTLES - 1 ){
				m_displayed_bottle++;
			} else {
				m_displayed_bottle = 0;
			}
		} else {
			if ( m_bottle_volume < p_current_bottle.volume ) {
				m_bottle_volume = m_bottle_volume + 10;
			}
			break;
		}
		break;
	}
	case Button::down: {
		if ( m_bottle_option != BottleItem::remaining ) {
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
	default: {
		Serial.println("Unknown button in bottleView");
		break;
	}
	}
}

/* Main navigation utility allowing the user to reach all the different views and options. */
void menuView() 
{
	if ( menu_item != m_old_menu_item ){
		m_old_menu_item = m_menu_item;

		lcd.clear();
		lcd.setCursor(0,0);
		switch ( menu_item ) {

		case Views::newBottle: {
			lcd.print("> New bottle");
			lcd.setCursor(2,1);
			lcd.print("Total consumption");
			break;
		}
		case Views::consumption: {
			lcd.print("> Total consumption");
			lcd.setCursor(2,1);
			lcd.print("History");
			break;
		}
		case Views::time: {
			lcd.print("> Set time");
			lcd.setCursor(2,1);
			lcd.print("New bottle");
			break;
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case Button::right: {
		// Restore the default values for all options within the particular views.
		restoreDefaults();

		// Enter the selected view.
		m_current_view = m_menu_item;
		break;
	}
	case Button::left {
		restoreDefaults();
		// Abort and return to current bottle view.
		m_current_view = Views::history;
		m_displayed_bottle = m_crate.getCurrentBottleIdx;
		break;
	}
	case Button::up: {
		if ( menu_item == Views::newBottle ) {
			menu_item = Views::time;
		} else if ( menu_item == Views::consumption ) {
			menu_item = Views::newBottle;
		} else if ( menu_item == Views::time ) {
			menu_item = Views::consumption;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
		break;
	}
	case Button::down: {
		if ( menu_item == Views::newBottle ) {
			menu_item = Views::consumption;
		} else if ( menu_item == Views::consumption ) {
			menu_item = Views::time;
		} else if ( menu_item == Views::time ) {
			menu_item = Views::newBottle;
		} else {
			Serial.println("Unsupported menu_item in menuView");
		}
		break;
	}
	default: {
		Serial.println("Unsupported button in menuView");
	}
	}
}

/* Allows the user to add a new bottle.*/
void newBottleView()
{
	bool update_display = false;
	if ( m_bottle_option != m_old_bottle_option ) {
		m_old_bottle_option = m_bottle_option;
		update_display = true;
	}

	if ( m_bottle_option == BottleItem::spoons &&
		 m_bottle_spoons != m_old_bottle_spoons) {
		m_old_bottle_spoons = m_bottle_spoons;
		update_display = true;
	}
	else if ( m_bottle_option == BottleItem::volume &&
			  m_bottle_volume != m_old_bottle_volume ) {
		m_old_bottle_volume = m_bottle_volume;
		update_display = true;
	}
	else if ( m_bottle_option == BottleItem::time &&
			  m_bottle_time != m_old_bottle_time ) {
		m_old_bottle_time = m_bottle_time;
		update_display = true;
	} else {
		Serial.println("Unsupported bottle_option in newBottleView");
	}

	if ( update_display ) {
		lcd.clear();
		lcd.setCursor(0,0);
		switch ( bottle_option ) {
		case BottleItem::spoons: {
			lcd.print("Spoons:");
			lcd.setCursor(8,0);
			lcd.print( m_bottle_spoons );
			break;
		}
		case BottleItem::volume: {
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
		case BottleItem::time: {
			lcd.print("Time:");
			lcd.setCursor(6,0);
			lcd.print( m_bottle_time );
			break;
		}
		default: {
			Serial.println("Unsupported bottle_option in switch in newBottleView");
		}
		}
	}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case Button::right: {
		if ( m_bottle_option == 0 ) {
			m_bottle_option = BottleItem::volume;
			break;
		} else if ( m_bottle_option == 1 ) {
			m_bottle_option = BottleItem::time;
			break;
		} else {
			addBottle(
					  m_bottle_spoons,
					  m_bottle_volume,
					  m_bottle_time);
			m_displayed_bottle = m_create.getCurrentBottleIdx();
			restoreDefaults();
			m_current_view = VIEW_HISTORY;
			break;
		}
	}
	case Button::left: {
		if ( m_bottle_option == 0 ) {
			restoreDefaults();
			// Abort and return to current bottle view.
			m_current_view = Views::history;
			break;
		} else if ( m_bottle_option == 1 ) {
			m_bottle_option = BottleItem::spoons:
			break;
		} else {
			m_bottle_option = BottleItem::volume;
			break;
		}
	}
	case Button::up: {
		switch ( m_bottle_option ) {
		case BottleItem::spoons: {
			m_bottle_spoons = m_bottle_spoons + 0.25;
			break;
		}
		case BottleItem::volume: {
			// There is a maximum value of a bottle.
			if ( m_bottle_volume < 250 ) {
				m_bottle_volume = m_bottle_volume + 10;
			}
			break;
		}
		case BottleItem::time: {
			m_bottle_time++;
			break;
		}
		default: {
			Serial.println("Unsupported BottleItem for up in newBottleView");
			break;
		}
		}
		break;
	}
	case Button::down: {
		switch ( m_bottle_option ) {
		case BottleItem::spoons: {
			if ( m_bottle_spoons > 0 ) {
				m_bottle_spoons = m_bottle_spoons - 0.25;
			}
			break;
		}
		case BottleItem::volume: {
			if ( m_bottle_volume > 0 ) {
				m_bottle_volume = m_bottle_volume - 10;
			}
			break;
		}
		case BottleItem::time: {
			m_bottle_time--;
			break;
		}
		default: {
			Serial.println("Unsupported BottleItem for up in newBottleView");
			break;
		}
		}
		break;
	}
	default: {
		Serial.println("Unsupported button in newBottleView");
		break;
	}
	}

}

/* Displays the total consumption in volume and number of spoons within the last 24 hours.*/
void consumptionView()
{
}

/* Displays all bottles stored in memory with latest first. Uses the currentBottleView() for a particular pair of bottles.*/
void historyView()
{
	bottleView(m_displayed_bottle);
}

/* Allows the user to insert a reference time.*/
void setTimeView()
{
}

/* Displays a particular view. */
void startView()
{
	switch ( m_current_view ) {
		case Views::history: {
			historyView();
			break;
		}
		case Views::menu: {
			menuView();
			break;
		}
		case Views::newBottle: {
			newBottleView();
			break;
		}
		case Views::consumption: {
			consumptionView();
			break;
		}
		case Views::time: {
			setTimeView();
			break;
		}
		default: {
			historyView();
			break;
		}
		}

	int lcd_key = readButtons();

	switch ( lcd_key ) {
	case Button::select: {
		m_current_view = Views::menu;
		break;
	}
	}
}

// Specify the pins used by the LCD
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
Interface app;

void setup(){
	
	lcd.begin(16, 2);
	lcd.setCursor(0,0);
	lcd.print("Bottle-Spass");
	Serial.begin(9600);
}

void loop()
{
	app::view();
}









