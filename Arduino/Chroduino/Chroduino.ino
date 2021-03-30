// Chroduino, an almost functional chronograph by Dirk de la Hunt 2021
// ================== CONFIG ==================== //

// Feature toggle
#define FEATURE_SCREEN   // SSD1306 OLED screen toggle
#define FEATURE_BLE      // Bluetooth Low Energy toggle
#define FEATURE_SERIAL   // serial output toggle

// Timing values
#define MEASUREMENT_TIMEOUT_MS   1000   // how long before a measurement times out (milliseconds)
#define DUPLICATE_EPSILON_MPS    0.01   // the smallest difference between readings to be considered unique (meters per second)
//TODO: MEASUREMENT_DISTANCE_MM, it measures 100mm right now, but make it configurable

// These are the pins the IR LEDs are connected to.
// NOTE: these have to be interrupt pins. If you're unsure, use pin 2 and 3.
// (see the pins for your board here: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/)
#define SENSORPIN_1   2
#define SENSORPIN_2   3

// ================== CODE ====================== //

#ifdef FEATURE_SCREEN
  #include <SPI.h>
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>

  #define SCREEN_WIDTH   128
  #define SCREEN_HEIGHT  32
  #define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
  
  Adafruit_SSD1306 g_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

#ifdef FEATURE_BLE
  #include <ArduinoBLE.h>
  
  BLEService             g_chrono_service("19B10010-E8F2-537E-4F6C-D104768A1213");
  BLEFloatCharacteristic g_speed_characteristic("19B10012-E8F2-537E-4F6C-D104768A1213", BLERead | BLENotify);
#endif
 
volatile unsigned long g_us1           = 0;   // sensor 1 time (microseconds)
volatile unsigned long g_us2           = 0;   // sensor 2 time (microseconds)
long                   g_start_ms      = 0;   // first time any sensor got a reading, used for timeouts (millisecond)
unsigned int           g_count         = 0;   // how many successful measurements taken so far
float                  g_running_total = 0.0; // the running total (meters per second)
float                  g_previous      = 0.0; // the previous reading, used for duplicates (meters per second)

#ifdef FEATURE_SCREEN
void init_screen()
{
#if SCREEN_HEIGHT == 64  
  if(!g_display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) // Address 0x3D for 128x64
#elif SCREEN_HEIGHT == 32
  if(!g_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) // Address 0x3C for 128x32
#else
  #error Unknown screen resolution, can't determine default address
#endif    
  {
    Serial.println(F("SSD1306 OLED allocation failed"));
    while(1);
  }
  g_display.clearDisplay();
  g_display.setTextSize(1);
  g_display.setTextColor(SSD1306_WHITE);
  g_display.setCursor(0, 0);
  g_display.println("Waiting...");
  g_display.display();    
}
#endif    

#ifdef FEATURE_BLE
void init_ble()
{
  if (!BLE.begin())
  {
    Serial.println("starting BLE failed!");
    #ifdef FEATURE_SCREEN
      g_display.setCursor(0,0);
      g_display.println("Could not start BLE");
      g_display.display();    
    #endif
    //while(1); // not the end of the world?
  }

  BLE.setLocalName("Chroduino");
  BLE.setAdvertisedService(g_chrono_service);

  g_chrono_service.addCharacteristic(g_speed_characteristic);

  BLE.addService(g_chrono_service);

  g_speed_characteristic.writeValue(0.0);

  BLE.advertise();
}
#endif

void update_sensor1()
{
  if (!g_us1) // ignore duplicate readings
    g_us1 = micros();
}

void update_sensor2()
{
  if (!g_us2) // ignore duplicate readings
    g_us2 = micros();
}

void setup()
{
#ifdef FEATURE_SERIAL
  Serial.begin(115200);
  Serial.println("Hi");
#endif  
  
  // initialize the sensor pins as an input:
  pinMode(SENSORPIN_1, INPUT);     
  digitalWrite(SENSORPIN_1, HIGH); // turn on the pullup
  pinMode(SENSORPIN_2, INPUT);     
  digitalWrite(SENSORPIN_2, HIGH); // turn on the pullup

  attachInterrupt(digitalPinToInterrupt(SENSORPIN_1), update_sensor1, LOW); //maybe CHANGE instead?
  attachInterrupt(digitalPinToInterrupt(SENSORPIN_2), update_sensor2, LOW);

#ifdef FEATURE_SCREEN
  // initialiaze the screen first, so we can print error messages on it
  init_screen();
#endif
#ifdef FEATURE_BLE
  init_ble();
#endif
}

void reset_timers()
{
  g_start_ms = 0;
  g_us1      = 0;
  g_us2      = 0;
}
 
void loop()
{
  unsigned long now = millis();
  
#ifdef FEATURE_BLE
  BLE.poll();
#endif
  
  if (g_us1 || g_us2)
  {
    if (!g_start_ms)
      g_start_ms = now;

    if (now - g_start_ms > MEASUREMENT_TIMEOUT_MS) // timeout for both beams to be broken
    {
      reset_timers();
#ifdef FEATURE_SERIAL      
      Serial.println("ERROR: Only got 1 trigger, aborting...");
#endif      
#ifdef FEATURE_SCREEN
      {
        g_display.clearDisplay();
        g_display.setCursor(0,0);               // Start at top-left corner
        g_display.println("*** ERROR ***");
      }
#endif          
      return;
    }

    if (g_us1 && g_us2) 
    {
      int           duplicate          = g_count++; // shot 0 can't be a duplicate
      unsigned long trigger_difference = (g_us1 - g_us2);
      float         speed_mps          = (100000000 / ((float)trigger_difference * 1000.0)); // meters per second
      
#ifdef FEATURE_SERIAL      
      Serial.println("Results:");
      Serial.print("Trigger difference (microsecond): ");
      Serial.println(trigger_difference);
      Serial.print("meters/second: ");
      Serial.println(speed_mps);
#endif

      g_running_total += speed_mps;
      duplicate        = duplicate && ( fabs(speed_mps - g_previous) < DUPLICATE_EPSILON_MPS );
      g_previous       = speed_mps;

#ifdef FEATURE_SCREEN
      {
        g_display.clearDisplay();
        g_display.setCursor(0,0);
        g_display.print("speed: ");
        g_display.print(speed_mps);
        g_display.println(" m/s");
        if (duplicate)
          g_display.println("(duplicate)");
        g_display.print("average: ");
        g_display.print(g_running_total / (float)g_count);
        g_display.println(" m/s");
        g_display.print("count: ");
        g_display.println(g_count);
        g_display.display();
      }
#endif        
#ifdef FEATURE_BLE
      g_speed_characteristic.writeValue(speed_mps);
#endif  

      reset_timers();
      delay(500); // take a break buddy, you earned it.
    }
  }
}
