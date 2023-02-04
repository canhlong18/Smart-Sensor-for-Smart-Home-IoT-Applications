#include "BH1750.h"
#include "Wire.h"
#include <Ticker.h>

BH1750 bh1750_a;
BH1750 bh1750_b;

float light_level_a;
float light_level_b;
float lux;

/** Task handle for the light value read task */
TaskHandle_t TaskHandleLight = NULL;

/** Ticker for light reading */
Ticker lightTicker;

/** Flags for light readings finished */
bool gotNewLight = false;

/* Flag if main loop is running */
bool tasksEnabledLight = false;


float getLight(BH1750 sensorName) {
  float light_level;
  if (sensorName.measurementReady()) {
    light_level = sensorName.readLightLevel();
  }
  return light_level;
}

void TaskLight(void *pvParameters) {
	Serial.println("TaskLight loop started");
	while (1) //  loop
	{
		if (tasksEnabledLight && !gotNewLight) {
			light_level_a = getLight(bh1750_a);	// Read values from sensor a
			light_level_b = getLight(bh1750_b);	// Read values from sensor b
			gotNewLight = true;
		}
    // Got sleep again
		vTaskSuspend(NULL);
	}
}

void triggerGetLight() {
	if (TaskHandleLight != NULL) {
		 xTaskResumeFromISR(TaskHandleLight);
	}
}

void setup() {
  Serial.begin(115200);
  Wire.begin(18, 19);
  Wire1.begin(21, 22);
  bh1750_a.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire);
  bh1750_b.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire1);

  xTaskCreatePinnedToCore(
			TaskLight,											/* Function to implement the task */
			"Task Light ",									/* Name of the task */
			4000,													/* Stack size in words */
			NULL,													/* Task input parameter */
			4,														/* Priority of the task */
			&TaskHandleLight,							/* Task handle. */
			1);

  if (TaskHandleLight == NULL) {
		Serial.println("[ERROR] Failed to start task for light intensity update");
	} else {
		// Start update of light data every N seconds
		lightTicker.attach(1, triggerGetLight);
	}
	// Signal end of setup() to tasks
	tasksEnabledLight = true;
}

void loop() {
  if (gotNewLight) {

    Serial.printf("A: %.0f lux   B: %.0f lux", 
                  light_level_a,  
                  light_level_b);
    lux = (light_level_a + light_level_b) / 2;
    Serial.printf("Avr_lux: %.0f \n", lux);
    
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
            Serial.println(F(
                "Setting MTReg to default value for normal light environment"));
          }
          else {
            Serial.println(F("Error setting MTReg to default value for normal "
                            "light environment"));
          }
        }
        else {
          if (lux <= 10.0) {
            // very low light environment
            if (bh1750_a.setMTreg(138)) {
              Serial.println(
                  F("Setting MTReg to high value for low light environment"));
            }
            else {
              Serial.println(F("Error setting MTReg to high value for low "
                              "light environment"));
            }
          }
        }
      }
    }

    gotNewLight = false;
  }
}
