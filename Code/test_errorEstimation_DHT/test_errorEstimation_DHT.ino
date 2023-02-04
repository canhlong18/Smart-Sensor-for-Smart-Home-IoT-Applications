#include <Arduino.h>
#include <Ticker.h>
#include "DHTesp.h"

#ifndef ESP32
#pragma message(THIS PROGRAM IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif


//////////////////////////////////////////////////////////////////////////////
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


//////////////////////////////////////////////////////////////////////////////
/**
 * Task to reads temperature from DHT sensor
 * @param pvParameters
 *		pointer to task parameters
 */
void tempTask(void *pvParameters) {
	Serial.println("tempTask loop started");
	while (1) // tempTask loop
	{
    /**
    * Read temperature only if old data was processed already
    * Reading temperature & humidity takes about 250 milliseconds
    * Sensor readings may also be up to 1(DHT11) or 2(DHT22) seconds
    */
		if (tasksEnabled && !gotNewTemperature) {
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


//////////////////////////////////////////////////////////////////////////////
const int arrSize = 15;
float measuredTempValues[arrSize];
float measuredHumdValues[arrSize];
float meanTemp = 0;
float meanHumd = 0;
float stdTemp = 0;
float stdHump = 0;
int countTemp = 0;
int countHumd = 0;

/**
 *  @brief  Compute sum of array values.
 *  @param  arr //array.
 *  @param  arrSize // size of array arr.
 *	@return float sumary value.
 */
float computeSumArrayValues(float arr[], const int arrSize) {
  float sum = 0;
  for (int i = 0; i < arrSize; i++) {
    sum += arr[i];
  }
  return sum;
}
/**
 *  @brief  Compute standard derivation of array values.
 *  @param  mean // mean of values in array.
 *  @param  arr // array.
 *  @param  arrSize // size of array arr.
 *	@return float standard derivation value.
 */
float computeStdArrayValues(float arr[], const int arrSize, float mean) {
  float temArr[arrSize];
  for (int i = 0; i < arrSize; i++) {
    temArr[i] = pow(arr[i] - mean, 2);
  }
  return 2.12 * sqrt(computeSumArrayValues(temArr, arrSize) / (arrSize * (arrSize - 1)));
}
/**
 *  @brief  Estimate Error of measurement; use in a loop.
 *  @param  arr // temperature array or humidity array.
 *  @param  sensorValue // Value read from sensor.
 *  @param  quantity // char - only fill T = "Temperature" or H = "Humidity".
 *	@return Error estimation result of measurement.
 */
void estimateError(float arr[], float sensorValue, char quantity) {
  if (quantity == 'T') {
    
    if (countTemp == arrSize) {
      // reset index when array is fullfilled.
      countTemp = 0;
    }
    arr[countTemp] = sensorValue;    // fill array values
    countTemp++;
    
    if (arr[arrSize - 1] != 0) {
      // Caculate means and STD only values array is fullfilled.
      meanTemp = computeSumArrayValues(arr, arrSize) / arrSize;
      stdTemp = computeStdArrayValues(arr, arrSize, meanTemp);
    }  
    Serial.println("Temperature Result = " + String(meanTemp) + "C +-" + String(stdTemp) + "C");
  }

  if (quantity == 'H') {
    
    if (countHumd == arrSize) {
      // reset index when array is fullfilled.
      countHumd = 0;
    }
    arr[countHumd] = sensorValue;    // fill array values
    countHumd++;
    
    if (arr[arrSize - 1] != 0) {
      // Caculate means and STD only values array is fullfilled.
      meanHumd = computeSumArrayValues(arr, arrSize) / arrSize;
      stdHump = computeStdArrayValues(arr, arrSize, meanHumd);
    }  
    Serial.println("Humidity Result = " + String(meanHumd) + "H +-" + String(stdHump) + "H");
  }
}


/**
 *  @brief  Converts Celcius to Fahrenheit
 *  @param  c //value in Celcius
 *	@return float value in Fahrenheit
 */
float convtCtoF(float c) {return c*1.8 + 32.0;}
/**
 *  @brief  Converts Fahrenheit to Celcius
 *  @param  f //value in Fahrenheit
 *	@return float value in Celcius
 */
float convtFtoC(float f) {return (f-32.0) * 0.55555;}

/**
 *  @brief  Compute Heat Index using both Rothfusz and Steadman's equations
 *					pref: http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
 *  @param  temperature     // temperature in selected scale
 *  @param  humidity    // humidity in percentage
 *  @param  tempUnitType    // string - only fill  C = "celcius" or F = "fahrenheit"
 *	@return float heat index
 */
float autoAdjustHeatIndex(float temperature, float humidity, char tempUnitType) {
  float heatIndex;

  if (tempUnitType == 'C')
    temperature = convtCtoF(temperature);
  
  // Using Steadman's equation to compute HeatIndex firstly
  heatIndex = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (humidity * 0.094));

  if (heatIndex >= 80.0) { // Using Rothfusz's equation to compute HeatIndex
    heatIndex = -42.379 +
         2.04901523 * temperature +
         10.14333127 * humidity +
         -0.22475541 * temperature * humidity +
         -0.00683783 * pow(temperature, 2) +
         -0.05481717 * pow(humidity, 2) +
         0.00122874 * pow(temperature, 2) * humidity +
         0.00085282 * temperature * pow(humidity, 2) +
         -0.00000199 * pow(temperature, 2) * pow(humidity, 2);

    // Auto adjustment for Heat Index
    if ((humidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      heatIndex -= ((13.0 - humidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if ((humidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      heatIndex += ((humidity - 85.0) * 0.10) * ((87.0 - temperature) * 0.2);
  }
  return tempUnitType == 'F' ? heatIndex : convtFtoC(heatIndex);
}


//////////////////////////////////////////////////////////////////////////////
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
		tempTicker.attach(5, triggerGetTemp);
	}
	// Signal end of setup() to tasks
	tasksEnabled = true;
} // End of setup.


//////////////////////////////////////////////////////////////////////////////
/**
 * loop
 * Arduino loop function, called once 'setup' is complete
 */ 
void loop() {
	if (gotNewTemperature) {
    // float HeatIndex1 = autoAdjustHeatIndex(sensor1Data.temperature, sensor1Data.humidity, 'C');
    // float HeatIndex2 = autoAdjustHeatIndex(sensor2Data.temperature, sensor2Data.humidity, 'C');
    // float HeatIndex3 = autoAdjustHeatIndex(sensor3Data.temperature, sensor3Data.humidity, 'C');

		Serial.println("\n----------------Measured Raw Values--------------");
    Serial.println("Sensor 1: "
                   "Temp= " + String(sensor1Data.temperature, 1) +
                   "C Humidity= " + String(sensor1Data.humidity + 16.6, 1) +
                   "% HeatIndex= " + String(autoAdjustHeatIndex(sensor1Data.temperature, sensor1Data.humidity, 'C'), 2));

		Serial.println("Sensor 2: "
                   "Temp= " + String(sensor2Data.temperature, 1) +
                   "C Humidity= " + String(sensor2Data.humidity, 1) +
                   "% HeatIndex= " + String(autoAdjustHeatIndex(sensor2Data.temperature, sensor2Data.humidity, 'C'), 2));
                   
		Serial.println("Sensor 3: "
                   "Temp= " + String(sensor3Data.temperature, 1) +
                   "C Humidity= " + String(sensor3Data.humidity + 15.7, 1) +
                   "% HeatIndex= " + String(autoAdjustHeatIndex(sensor3Data.temperature, sensor3Data.humidity, 'C'), 2));
		
    Serial.println("--------Measured Results After Adjustment Heat Index & Error Estimation----------");
    
    float tempAvg = (sensor1Data.temperature + sensor2Data.temperature + sensor3Data.temperature) / 3;
    float humdAvg = (sensor1Data.humidity + 16.6 + sensor2Data.humidity + sensor3Data.humidity + 15.7) / 3;
    
    Serial.println("Temperature Avg = " + String(tempAvg, 2) + "C");
    Serial.println("Humidity Avg = " + String(humdAvg, 2) + "H");

    estimateError(measuredTempValues, tempAvg, 'T');
    estimateError(measuredHumdValues, humdAvg, 'H');
    
    for (int i = 0; i < arrSize; i++) {
      Serial.print(measuredTempValues[i]);
      Serial.print("  ");
    }
    Serial.println();
    for (int i = 0; i < arrSize; i++) {
      Serial.print(measuredHumdValues[i]);
      Serial.print("  ");
    }
  
    gotNewTemperature = false;
	}
} // End of loop

