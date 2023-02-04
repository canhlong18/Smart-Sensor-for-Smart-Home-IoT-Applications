#include <Arduino.h>
#include <Ticker.h>

#include "DHTesp.h"
#include <DHT.h>

#include "BH1750.h"
#include <Wire.h>

#include <MQUnifiedsensor.h>
#include <MQ135.h>
#define MQ135_THRESHOLD_1 500

#include <WiFi.h>
#include "ThingSpeak.h"

#ifndef ESP32
#pragma message(THIS PROGRAM IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

//*****************************************************************************
/**
* Configuration for wifi connection.
*/
const char* ssid = "canhlong";   // your network SSID (name) 
const char* password = "l187871614";   // your network password
WiFiClient  client;

/**
* Configuration for web server.
*/
unsigned long myChannelNumber = 4 ;
const char * myWriteAPIKey = "B0C5LMTWPMO8E1ZI";
// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;


//*****************************************************************************
/**
* Declare all variable for measuring temerature and humidity
*/
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

float HeatIndex1;
float HeatIndex2;
float HeatIndex3;

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Flags for temperature and humidity readings finished */
bool gotNewTemperature = false;
/** Flag if main loop is running */
bool tasksEnabled = false;


/**
* Declare all variable for measuring light intensity
*/
BH1750 bh1750_a;
BH1750 bh1750_b;

float light_level_a;
float light_level_b;
float lux;

/** Task handle for the light value read task */
TaskHandle_t lightTaskHandle = NULL;
/** Ticker for light reading */
Ticker lightTicker;
/** Flags for light readings finished */
bool gotNewLightLevel = false;
// /* Flag if main loop is running */
// bool tasksEnabled = false;


/**
* Declare all variable for measuring gas 
*/
float MQ135_data;
/** Task handle for the value read task */
TaskHandle_t gasTaskHandle = NULL;
/** Ticker for gas concentation reading */
Ticker gasTicker;
/** Flags for gas concentation readings finished */
bool gotNewGas = false;
/* Flag if main loop is running */
// bool tasksEnabled = false;

//*****************************************************************************
/**
 * Task to reads temperature from DHT sensor
 * @param pvParameters
 *		pointer to task parameters
 */
void tempTask(void *pvParameters) {
	Serial.println("tempTask loop started");
	while (1) // tempTask loop
	{
    //Read temperature only if old data was processed already
    //Reading temperature & humidity takes about 250 milliseconds
    //Sensor readings may also be up to 1(DHT11) or 2(DHT22) seconds
		if (tasksEnabled && !gotNewTemperature) {
			sensor1Data = dhtSensor1.getTempAndHumidity();
			sensor2Data = dhtSensor2.getTempAndHumidity();
			sensor3Data = dhtSensor3.getTempAndHumidity();
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
 * Task to reads light intensity from BH1750 sensor
 * @param pvParameters
 *		pointer to task parameters
 */
void lightTask(void *pvParameters) {
	Serial.println("lightTask loop started");
	while (1) //  loop
	{
		if (tasksEnabled && !gotNewLightLevel) {
			light_level_a = getLight(bh1750_a);	// Read values from sensor a
			light_level_b = getLight(bh1750_b);	// Read values from sensor b
			gotNewLightLevel = true;
		}
    // Got sleep again
		vTaskSuspend(NULL);
	}
}

/**
 * - triggerGetLight
 * - Sets flag light level updated to true for handling in loop()
 * - called by Ticker lightTicker
 */
void triggerGetLight() {
	if (lightTaskHandle != NULL) {
		 xTaskResumeFromISR(lightTaskHandle);
	}
}

/**
 * Task to reads gas concentation from DHT11 sensor
 * @param pvParameters
 *		pointer to task parameters
 */
void gasTask(void *pvParameters) {
	Serial.println("gasTask loop started");
	while (1) // gasTask loop
	{
		if (tasksEnabled && !gotNewGas) {
      int MQ135_data = analogRead(A0);
      gotNewGas = true;
		}
		vTaskSuspend(NULL);
	}
}

/**
 * triggerGetGas
 * Sets flag MQ-sensor to true for handling in loop()
 * called by Ticker gasTicker
 */
void triggerGetGas() {
	if (gasTaskHandle != NULL) {
		 xTaskResumeFromISR(gasTaskHandle);
	}
}


//*****************************************************************************
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
  return sqrt(computeSumArrayValues(temArr, arrSize) / (arrSize*(arrSize - 1)));
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
    Serial.println("Temperature Result = " + String(meanTemp, 1) + "C +-" + String(2.12 * stdTemp, 1) + "C");
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
    Serial.println("Humidity Result = " + String(meanHumd, 1) + "H +-" + String(2.12 * stdHump, 1) + "H");
  }
}

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


float getLight(BH1750 sensorName) {
  float light_level;
  if (sensorName.measurementReady()) {
    light_level = sensorName.readLightLevel();
  }
  return light_level;
}


//*****************************************************************************
/**
 * Arduino setup function (called once after boot/reboot)
 */
void setup() {
	Serial.begin(115200);
  WiFi.mode(WIFI_STA);   
  ThingSpeak.begin(client);  // Initialize ThingSpeak

	Serial.println("Measuring All Sensors");

	// Initialize temperature sensor 1, 2, 3
	dhtSensor1.setup(dhtPin1, DHTesp::DHT11);
	dhtSensor2.setup(dhtPin2, DHTesp::DHT22);
	dhtSensor3.setup(dhtPin3, DHTesp::DHT11);

  // Initialize light sensor objects
  Wire.begin(18, 19);
  Wire1.begin(21, 22);
  bh1750_a.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  bh1750_b.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1);

	// Initialize task to get temperature
	xTaskCreatePinnedToCore(
			tempTask,											/* Function to implement the task */
			"tempTask ",									/* Name of the task */
			4000,													/* Stack size in words */
			NULL,													/* Task input parameter */
			15,														/* Priority of the task */
			&tempTaskHandle,							/* Task handle. */
			1);														/* Core where the task should run */

	if (tempTaskHandle == NULL) {
		Serial.println("[ERROR] Failed to start task for temperature and humidity update");
	} else {
		// Start update of temperature and humidity data every N seconds
		tempTicker.attach(5, triggerGetTemp);
	}

	// Initialize task to get light intensity
  xTaskCreatePinnedToCore(
			lightTask,										/* Function to implement the task */
			"lightTask ",							  	/* Name of the task */
			4000,													/* Stack size in words */
			NULL,													/* Task input parameter */
			10,														/* Priority of the task */
			&lightTaskHandle,							/* Task handle. */
			1);

  if (lightTaskHandle == NULL) {
		Serial.println("[ERROR] Failed to start task for light intensity update");
	} else {
		// Start update of light data every N seconds
		lightTicker.attach(15, triggerGetLight);
	}

  // Initialize task to get gas concentration
  xTaskCreatePinnedToCore(
			gasTask,										/* Function to implement the task */
			"gasTask ",							  	/* Name of the task */
			4000,													/* Stack size in words */
			NULL,													/* Task input parameter */
			5,														/* Priority of the task */
			&gasTaskHandle,							/* Task handle. */
			1);

  if (gasTaskHandle == NULL) {
		Serial.println("[ERROR] Failed to start task for gas concentration update");
	} else {
		// Start update of gas concentration data every N seconds
		gasTicker.attach(10, triggerGetGas);
	}

	// Signal end of setup() to tasks
	tasksEnabled = true;
} // End of setup.


//*****************************************************************************
/**
 * loop
 * Arduino loop function, called once 'setup' is complete
 */ 
void loop() {

  if (gotNewTemperature) {
    HeatIndex1 = autoAdjustHeatIndex(sensor1Data.temperature, sensor1Data.humidity + 16.6, 'C');
    HeatIndex2 = autoAdjustHeatIndex(sensor2Data.temperature, sensor2Data.humidity, 'C');
    HeatIndex3 = autoAdjustHeatIndex(sensor3Data.temperature, sensor3Data.humidity  + 15.7, 'C');

		Serial.println("\n----------------Measured Raw Temperature and Humidity Values------------------------------------------------------------------------------");
    Serial.println("Sensor 1: "
                   "Temp= " + String(sensor1Data.temperature, 1) +
                   "C Humidity= " + String(sensor1Data.humidity + 16.6, 1) +
                   "% HeatIndex= " + String(HeatIndex1, 1));

		Serial.println("Sensor 2: "
                   "Temp= " + String(sensor2Data.temperature, 1) +
                   "C Humidity= " + String(sensor2Data.humidity, 1) +
                   "% HeatIndex= " + String(HeatIndex2, 1));
                   
		Serial.println("Sensor 3: "
                   "Temp= " + String(sensor3Data.temperature, 1) +
                   "C Humidity= " + String(sensor3Data.humidity + 15.7, 1) +
                   "% HeatIndex= " + String(HeatIndex3, 1));
		
    Serial.println("----------------Measured Results After Adjustment Heat Index & Error Estimation------------------------------------------------------------------------------");
    
    float tempAvg = (sensor1Data.temperature + sensor2Data.temperature +  sensor3Data.temperature) / 3;
    float humdAvg = (sensor1Data.humidity + 16.6 + sensor2Data.humidity + sensor3Data.humidity + 15.7) / 3;
    
    Serial.println("Temperature Avg = " + String(tempAvg, 1) + "C");
    Serial.println("Humidity Avg = " + String(humdAvg, 1) + "H");

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
    Serial.println();
  
    gotNewTemperature = false;
	}

  if (gotNewLightLevel) {

    Serial.println("\n----------------Measured Light Illuminity Values------------------------------------------------------------------------------");

    Serial.printf("A: %.0f lux   B: %.0f lux    ", 
                  light_level_a,  
                  light_level_b);
    lux = (light_level_a + light_level_b) / 2;
    Serial.printf("Avr_lux: %.0f \n", lux);
    
    // Auto adjust MTreg values for reducing resolution in different environments
    if (lux < 0) {
      Serial.println(F("Error condition detected"));
    }
    else {
      if (lux > 40000.0) {
        // reduce measurement time - needed in direct sun light
        if (bh1750_a.setMTreg(32)) {
          Serial.println(
              F("Setting MTReg to low value for high light environment"));
        }
        else {
          Serial.println(
              F("Error setting MTReg to low value for high light environment"));
        }
      }
      else {
        if (lux > 10.0) {
          // typical light environment
          if (bh1750_a.setMTreg(69)) {
            Serial.println(F("Setting MTReg to default value for normal light environment"));
          }
          else {
            Serial.println(F("Error setting MTReg to default value for normal light environment"));
          }
        }
        else {
          if (lux <= 10.0) {
            // very low light environment
            if (bh1750_a.setMTreg(138)) {
              Serial.println(F("Setting MTReg to high value for low light environment"));
            }
            else {
              Serial.println(F("Error setting MTReg to high value for low light environment"));
            }
          }
        }
      }
    }

    gotNewLightLevel = false;
  }

  if (gotNewGas) {
		Serial.println("----------------Measured Gas Concentration------------------------------------------------------------------------------");
  
    if(MQ135_data < MQ135_THRESHOLD_1) {
      Serial.print("Fresh Air: ");
    }
    else {
      Serial.print("Poor Air: ");
    }      
    Serial.print(MQ135_data); // analog data
    Serial.println(" PPM");
    gotNewGas = false;
	}


  if ((millis() - lastTime) > timerDelay) {
    
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }
  	
    ThingSpeak.setField(1, meanTemp);
    ThingSpeak.setField(2, meanHumd);
    ThingSpeak.setField(3, MQ135_data);
    ThingSpeak.setField(4, lux);
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    if(x == 200){
      Serial.println("Channel update successful.");
    }
    else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
  }      
} // End of loop

