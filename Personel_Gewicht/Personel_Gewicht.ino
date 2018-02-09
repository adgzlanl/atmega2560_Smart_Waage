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
#include <LiquidCrystal.h>
#include <Countimer.h>





#define CC3000_INT      21   
#define CC3000_EN       49  
#define CC3000_CS       53  
#define IP_ADDR_LEN     4   

// Constants
          
unsigned int ap_security = WLAN_SEC_WPA2; 
unsigned int timeout = 30000;             
char server[] = "gewichtsdatenbank.herokuapp.com";
char queryString[256] = { 0 };
char IPadress[] = "";

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#define DOUT 6
#define CLK  7
Countimer tUp;
HX711 scale(DOUT, CLK);
int wifiStatus = 0;
int sendStatus = 0;
float calibration_factor = 18750.00;
float units=0;
int timesup = 0;
String readString;
char wifi_ssid_private[32];
char wifi_password_private[32];
char email[32];
char password[32];
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



	Serial.begin(9600);
	Serial1.begin(9600);

	readEEPROM(0, 32, wifi_ssid_private);
	readEEPROM(32, 32, wifi_password_private);
	readEEPROM(64, 32, email);
	readEEPROM(96, 32, password);
	
	
	Serial.println(wifi_ssid_private);
	Serial.println(wifi_password_private);
	Serial.println(email);
	Serial.println(password);
	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("die smart Waage");
	scale.set_scale();

	scale.tare();
	long zero_factor = scale.read_average();


	Serial.println();
	Serial.println("---------------------------");
	Serial.println("Internet Connection");
	Serial.println("---------------------------");
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Internet");
	lcd.setCursor(0, 1);
	lcd.print("Connection");
	delay(2000);

	// Initialize CC3000 (configure SPI communications)
	if (wifi.init()) {
	
		Serial.println("Wireless initialization complete");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Init");
		lcd.setCursor(0, 1);
		lcd.print("Wireless");
		delay(2000);
	}
	else {
		
		Serial.println("Something went wrong during Connection init!");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Connection init");
		lcd.setCursor(0, 1);
		lcd.print("Error");
		delay(2000);
	}

	// Connect using DHCP

	Serial.print("Connecting to SSID: ");
	Serial.println(wifi_ssid_private);
	Serial.print("Connecting......");
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("ConnectingtoSSID");
	lcd.setCursor(0, 1);
	lcd.print(wifi_ssid_private);


	if (!wifi.connect(wifi_ssid_private, ap_security, wifi_password_private, timeout)) {

		Serial.println("Error: Could not connect to AP");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Error:");
		lcd.setCursor(0, 1);
		lcd.print("Connection to AP");
		delay(2000);
		wifiStatus = 0;
	}

	// Gather connection details and print IP address
	if (!wifi.getConnectionInfo(connection_info)) {
	
		Serial.println("Error: Could not obtain connection details");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Error:");
		lcd.setCursor(0, 1);
		lcd.print("Connection");
		delay(2000);
		wifiStatus = 0;
	}
	else {
		
		Serial.print("IP Address: ");
		for (i = 0; i < IP_ADDR_LEN; i++) {
			IPadress[i]=connection_info.ip_address[i];
			if (i < IP_ADDR_LEN - 1) {
				Serial.print(".");
			}
		}
		Serial.println(IPadress);
		Serial.println("Connected");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Wireless");
		lcd.setCursor(0, 1);
		lcd.print("Connected...");
		delay(3000);
		wifiStatus = 1;

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


	if (wifiStatus == 0 && sendStatus==0)
	{
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("die smart Waage");
		lcd.setCursor(0, 1);
		lcd.print(units);
		lcd.print(" kg");
		lcd.print(" Wifi not");
	}


	else if (wifiStatus == 1 && sendStatus==0)
	{
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("die smart Waage");
		lcd.setCursor(0, 1);
		lcd.print(units);
		lcd.print(" kg");
		lcd.print(" Wifi ok");
	}

	if (timesup == 1 && wifiStatus == 1)
	{
		SendData(email, password, units);
		timesup = 0;


	}

	if (wifiStatus == 1 && sendStatus == 1)
	{

		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Value Sent");
		delay(4000);
		sendStatus = 0;

	}



	if (client.available()>0) {
		char c = client.read();
				
		readString+=c;

		if (c == '\n')
		{
				
			Serial.print(readString);

						if (readString.equals("HTTP/1.1 200 OK\r\n"))
						{
							
							sendStatus = 1;
							client.close();
							
						}

					readString = "";
		
		}
		
		




	}


	while (Serial1.available() > 0)
	{
		String income = Serial1.readString();

		if (income.substring(0, 3) == "CON")
		{
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

		if (income.substring(0, 2) == "ID")
		{
			String j = income.substring(2, income.indexOf('|'));
			String k = income.substring(income.indexOf('|') + 1, income.lastIndexOf('|'));
			Serial.println("Writing....");
			j.toCharArray(email, 32);
			k.toCharArray(password, 32);
			//strcat(wifi_ssid_private,j.c_str());
			//strcat(wifi_password_private, k.c_str);
			writeEEPROM(64, 32, email);
			writeEEPROM(96, 32, password);
			//EEPROM.put(0, j.c_str());
			//EEPROM.put(200, k.c_str());
			Serial.println(j);
			Serial.println(k);
			Serial.println("wrote");
			Serial.println("Completed");
			j = "";
			k = "";
		}

		if (income.substring(0, 5) == "CLEAR")
		{
			for (int i = 0; i < EEPROM.length(); i++) {
				EEPROM.write(i, 0);
			}
		}

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


void SendData(char e[],char p[], float m)
{

	
	String datas = "email=" + String(e) + "&password=" + String(p) +"&topic=Gewicht" + "&message=" + String(m);
	

	
	// Make a TCP connection to remote host
	Serial.print("Performing HTTP Post of: ");
	Serial.println(server);

	if (!client.connect(server, 80)) {
		Serial.println("Error: Could not make a TCP connection");
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("TCP Error");
		lcd.setCursor(0, 1);
		lcd.print("Connection");
		delay(4000);
	}
	

	// Make a HTTP GET request
	client.println("POST /hinzufeugen HTTP/1.1");
	client.print("Host: ");
	client.println(server);
	client.println("User-Agent: SmartWaage");
	client.println("Connection: close");
	client.println("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
	client.print("Content-Length: ");
	client.println(datas.length());
	client.println();
	client.println(datas);
	

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Sending....");
	lcd.setCursor(0, 1);
	lcd.print(String(units) + " kg");
	delay(10000);
	
	
}




