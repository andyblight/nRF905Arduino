/*
 * Project: nRF905 AVR/Arduino Library/Driver (Timed Transmit example)
 * Author: Andy Blight
 * Copyright: (C) 2020 Andy Blight.
 * License: GNU GPL v3 (see License.txt)
 */

/*
 * Ping server
 *
 * Listen for packets and send them back
 *
 * 5 -> LED toggles when packet received.
 * 7 -> CE
 * 8 -> PWR
 * 9 -> TXE
 * 4 -> CD
 * 3 -> DR
 * 2 -> AM
 * 10 -> CSN
 * 12 -> SO
 * 11 -> SI
 * 13 -> SCK
 */

#include <nRF905.h>

#define RXADDR 0x586F2E10 // Address of this device
#define TXADDR 0xFE4CA6E5 // Address of device to send to

#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2

#define LED_PIN 5

static volatile uint8_t packetStatus;

void NRF905_CB_RXCOMPLETE(void)
{
	packetStatus = PACKET_OK;
	nRF905_standby();
}

void NRF905_CB_RXINVALID(void)
{
	packetStatus = PACKET_INVALID;
	nRF905_standby();
}

void toggleLed()
{
	// Toggle LED
	static uint8_t ledState = 0;
	ledState = !ledState;
	digitalWrite(LED_PIN, ledState ? HIGH : LOW);
}

void printMsg(uint8_t buffer[])
{
	// Print out the message in the buffer.
	int packet_number = buffer[0];
	int packet_length = buffer[1];
	Serial.print(F("Received: "));
	Serial.print(packet_number);
	Serial.print(F(", "));
	Serial.print(packet_length);
	Serial.print(F(", '"));
	for (int index = 0; index < packet_length; ++index)
	{
		Serial.print((char)buffer[index + 2]);
	}
	Serial.print(F("'"));
	Serial.println();
}

void setup()
{
	Serial.begin(115200);
	Serial.println(F("Server started"));

	pinMode(LED_PIN, OUTPUT);
	toggleLed();

	// Start up
	nRF905_init();

	// Set address of this device
	nRF905_setListenAddress(RXADDR);

	// Set freqenucy to 434.2MHz.  Band = 0, channel = 118
	nRF905_setBand(NRF905_BAND_433);
	nRF905_setChannel(118);

	// Put into receive mode
	nRF905_RX();
}

void loop()
{
	static uint32_t pings;
	static uint32_t invalids;
	static uint32_t tx_busy;

	Serial.println(F("Waiting for ping..."));

	// Wait for data
	while(packetStatus == PACKET_NONE);

	if(packetStatus != PACKET_OK)
	{
		invalids++;
		Serial.println(F("Invalid packet!"));
		packetStatus = PACKET_NONE;
		nRF905_RX();
	}
	else
	{
		pings++;
		packetStatus = PACKET_NONE;

		// Make buffer for data
		uint8_t buffer[NRF905_MAX_PAYLOAD];
		nRF905_read(buffer, sizeof(buffer));
		printMsg(buffer);
		Serial.println(F("Sending reply..."));

		// Delay makes reply reliable.  No delay means no receive.  No idea why!
		delay(25);

		// Send back the data, once the transmission has completed go into receive mode
		bool sent = false;
		while(!sent)
		{
			sent = nRF905_TX(TXADDR, buffer, sizeof(buffer), NRF905_NEXTMODE_RX);
			if (!sent)
			{
				++tx_busy;
			}
		}

		Serial.println(F("Reply sent"));

		toggleLed();

	}

	Serial.print(F("Totals: "));
	Serial.print(pings);
	Serial.print(F(" Pings, "));
	Serial.print(invalids);
	Serial.println(F(" Invalid"));
	Serial.print(tx_busy);
	Serial.println(F(" tx busy"));
	Serial.println(F("------"));
}
