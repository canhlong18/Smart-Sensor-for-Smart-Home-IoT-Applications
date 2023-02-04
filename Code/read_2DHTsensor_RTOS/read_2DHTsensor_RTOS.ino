#include <Arduino.h>
#include <Ticker.h>
#include "DHTesp.h"

#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif


/** Pin number definition for DHT sensor 1, 2, 3 data pin */
int dhtPin1 = 15;   //DHT11
int dhtPin2 = 4;    //DHT22
int dhtPin3 = 5;    //DHT11

/** Variable definition for DHT sensor 1, 2, 3 */
DHTesp dhtSensor1;
DHTesp dhtSensor2;
DHTesp dhtSensor3;

/** Data storage variables from sensor 1, 2, 3 */
TempAndHumidity sensor1Data;
TempAndHumidity sensor2Data;
TempAndHumidity sensor3Data;

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;

/** Flag if main loop is running */
bool tasksEnabled = false;
/** Flags for temperature and humidity readings finished */
bool gotNewTemperature = false;


/**
 * Task to reads temperature from DHT11 sensor
 * @param pvParameters
 *		pointer to task parameters
 */
void tempTask(void *pvParameters) {
	Serial.println("tempTask loop started");
	while (1) // tempTask loop
	{
		if (tasksEnabled && !gotNewTemperature) {
      // Read temperature only if old data was processed already
			// Reading temperature & humidity takes about 250 milliseconds
			// Sensor readings may also be up to 1(DHT11) or 2(DHT22) seconds
			sensor1Data = dhtSensor1.getTempAndHumidity();	// Read values from sensor 1
			sensor2Data = dhtSensor2.getTempAndHumidity();	// Read values from sensor 1
			sensor3Data = dhtSensor3.getTempAndHumidity();	// Read values from sensor 1
			gotNewTemperature = true;
		}
    // Got sleep again
		vTaskSuspend(NULL);
	}
}


/**
 * - triggerGetTemp
 * - Sets flag dhtUpdated to true for handling in loop()
 * - called by Ticker tempTicker
 */
void triggerGetTemp() {
	if (tempTaskHandle != NULL) {
		 xTaskResumeFromISR(tempTaskHandle);
	}
}


/**
 * Arduino setup function (called once after boot/reboot)
 */
void setup() {
	Serial.begin(115200);
	Serial.println("Measuring for 3 DHT11/22 sensors");

	// Initialize temperature sensor 1, 2, 3
	dhtSensor1.setup(dhtPin1, DHTesp::DHT11);
	dhtSensor2.setup(dhtPin2, DHTesp::DHT22);
	dhtSensor3.setup(dhtPin3, DHTesp::DHT11);

	// Start task to get temperature
	xTaskCreatePinnedToCore(
			tempTask,											/* Function to implement the task */
			"tempTask ",									/* Name of the task */
			4000,													/* Stack size in words */
			NULL,													/* Task input parameter */
			5,														/* Priority of the task */
			&tempTaskHandle,							/* Task handle. */
			1);														/* Core where the task should run */

	if (tempTaskHandle == NULL) {
		Serial.println("[ERROR] Failed to start task for temperature and humidity update");
	} else {
		// Start update of temperature and humidity data every N seconds
		tempTicker.attach(2, triggerGetTemp);
	}
	// Signal end of setup() to tasks
	tasksEnabled = true;
} // End of setup.


/**
 * loop
 * Arduino loop function, called once 'setup' is complete (your own code
 * should go here)
 */ 
void loop() {
	if (gotNewTemperature) {
		Serial.println("Sensor 1: "
                   "Temp= " + String(sensor1Data.temperature, 1) +
                   "C Humidity= " + String(sensor1Data.humidity + 16.6, 1) + "%");

		Serial.println("Sensor 2: "
                   "Temp= " + String(sensor2Data.temperature, 1) +
                   "C Humidity= " + String(sensor2Data.humidity, 1) + "%");
                   
		Serial.println("Sensor 3: "
                   "Temp= " + String(sensor3Data.temperature, 1) +
                   "C Humidity= " + String(sensor3Data.humidity + 15.7, 1) + "%");
		
    Serial.println("---------------------------------------------");
    gotNewTemperature = false;
	}
} // End of loop

