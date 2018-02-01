/*
 Name:		Personel_Gewicht.ino
 Created:	31.01.2018 18:00:48
 Author:	Anil Adiguzel
*/




#include <HX711.h>
#include <SPI.h>
#include <SFE_CC3000.h>
#include <SFE_CC3000_Client.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <LiquidCrystal.h>
#include <Countimer.h>





#define CC3000_INT      21   
#define CC3000_EN       49  
#define CC3000_CS       53  
#define IP_ADDR_LEN     4   

// Constants
          
unsigned int ap_security = WLAN_SEC_WPA2; 
unsigned int timeout = 40000;             
char server[] = "gewichtsdatenbank.herokuapp.com";

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#define DOUT 6
#define CLK  7
Countimer tUp;
HX711 scale(DOUT, CLK);

float calibration_factor = 18750.00;
float units;
int timesup = 0;

char wifi_ssid_private[32];
char wifi_password_private[32];
SFE_CC3000 wifi = SFE_CC3000(CC3000_INT, CC3000_EN, CC3000_CS);
SFE_CC3000_Client client = SFE_CC3000_Client(wifi);



void writeEEPROM(int startAdr, int laenge, char* writeString) {
	EEPROM.begin(); //Max bytes of eeprom to use
	yield();
	Serial.println();
	Serial.print("writing EEPROM: ");
	//write to eeprom 
	for (int i = 0; i < laenge; i++)
	{
		EEPROM.write(startAdr + i, writeString[i]);
		Serial.print(writeString[i]);
	}

	EEPROM.end();
}

void readEEPROM(int startAdr, int maxLength, char* dest) {
	EEPROM.begin();
	delay(10);
	for (int i = 0; i < maxLength; i++)
	{
		dest[i] = char(EEPROM.read(startAdr + i));
	}
	EEPROM.end();
	Serial.print("ready reading EEPROM:");
	Serial.println(dest);
}


void setup() {


	ConnectionInfo connection_info;
	int i;

	char *ap_ssid = "";
	char *ap_password = "";

	Serial.begin(9600);
	Serial1.begin(9600);

	readEEPROM(0, 32, wifi_ssid_private);
	readEEPROM(32, 32, wifi_password_private);
	
	//EEPROM.get(0, ap_ssid);
	//EEPROM.get(200, ap_password);
	
	Serial.println(wifi_ssid_private);
	Serial.println(wifi_password_private);

	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("die smart Waage");
	scale.set_scale();

	scale.tare();
	long zero_factor = scale.read_average();


	Serial1.println();
	Serial1.println("---------------------------");
	Serial1.println("Internet Connection");
	Serial1.println("---------------------------");

	// Initialize CC3000 (configure SPI communications)
	if (wifi.init()) {
		Serial1.println("Wireless initialization complete");
	}
	else {
		Serial1.println("Something went wrong during Connection init!");
	}

	// Connect using DHCP
	Serial1.print("Connecting to SSID: ");
	Serial1.println(wifi_ssid_private);
	Serial1.print("Connecting......");
	if (!wifi.connect(wifi_ssid_private, ap_security, wifi_password_private, timeout)) {
		Serial1.println("Error: Could not connect to AP");
	}

	// Gather connection details and print IP address
	if (!wifi.getConnectionInfo(connection_info)) {
		Serial1.println("Error: Could not obtain connection details");
	}
	else {
		Serial1.print("IP Address: ");
		for (i = 0; i < IP_ADDR_LEN; i++) {
			Serial1.print(connection_info.ip_address[i]);
			if (i < IP_ADDR_LEN - 1) {
				Serial1.print(".");
			}
		}
		Serial1.println();
		Serial1.println("Connected");
	}

	tUp.setCounter(0, 0, 5, tUp.COUNT_UP, tUpComplete);
	tUp.setInterval(print_time1, 1000);
	tUp.start();
}

void loop() {




	scale.set_scale(calibration_factor); //Adjust to this calibration factor
	units = scale.get_units();

	if (units < 1) {
		units = 0.00;
	
		tUp.restart();

	}


	if (units>1)
	{

		tUp.run();

	}


	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("die smart Waage");
	lcd.setCursor(5, 1);
	lcd.print(units);
	lcd.print(" kg");




	if (client.available()) {
		char c = client.read();
		Serial.print(c);


	}




	if (timesup == 1)
	{
		SendData("Gewicht", String(units));
		timesup = 0;

	}


}



void print_time1()
{
	Serial.print("tUp: ");
	Serial.println(tUp.getCurrentTime());
}


void tUpComplete()
{
	timesup = 1;
	


}


void SendData(String t, String m)
{
	String datas = "topic=" + t + "&message=" + m;
	// Make a TCP connection to remote host
	Serial.print("Performing HTTP GET of: ");
	Serial.println(server);
	if (!client.connect(server, 80)) {
		Serial.println("Error: Could not make a TCP connection");
	}
	

	// Make a HTTP GET request
	client.println("GET /hinzufeugen?"+datas+" HTTP/1.1");
	client.print("Host: ");
	client.println(server);
	client.println("User-Agent: test");
	client.println("Connection: close");
	client.println();
	Serial.println();
	
}

void serialEvent1()
{
	String income = Serial1.readStringUntil('\n');
	String j = income.substring(3, income.indexOf('|'));
	String k = income.substring(income.indexOf('|') + 1, income.lastIndexOf('|'));
	Serial.println("Writing....");
	j.toCharArray(wifi_ssid_private, 32);
	k.toCharArray(wifi_password_private, 32);
	//strcat(wifi_ssid_private,j.c_str());
	//strcat(wifi_password_private, k.c_str);
	writeEEPROM(0, 32, wifi_ssid_private);
	writeEEPROM(32, 32, wifi_password_private);
	//EEPROM.put(0, j.c_str());
	//EEPROM.put(200, k.c_str());
	Serial.println(j);
	Serial.println(k);
	Serial.println("wrote");
	Serial.println("Completed");
	j = "";
	k = "";

}



