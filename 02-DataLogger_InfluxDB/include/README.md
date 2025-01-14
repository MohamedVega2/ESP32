
# Weather Monitoring Using ESP32 and InfluxDB

[Blog Post Link](http://embeddedlaboratory.blogspot.com/2022/12/weather-monitoring-using-esp32-and.html)  

In this post, I will demonstrate how we can use the ESP32 module to sense the temperature and humidity from the DHT11 sensor and send this data to the InfluxDB cloud database, and then using InfluxDB and Grafana Dashboard, we can visualize this data in the form of graphs and gauges. Below is the image of the dashboard created in InfluxDB.

[![](https://img.youtube.com/vi/LatI7kNxGmc/0.jpg)](https://www.youtube.com/watch?v=LatI7kNxGmc)
<!-- [![](../../Support/78-WeatherMonitoringUsingESP32.png)](https://www.youtube.com/watch?v=LatI7kNxGmc) -->

Note: Grafana Dashboard post will be a separate topic else this will be a very long post.

![Dashboard Using InfluxDB](https://raw.githubusercontent.com/xpress-embedo/ESP32/master/Support/HomeDataLogger_InfluxDB.png)

## Prerequisite for this post
* ESP32 Board
* DHT11 Sensor
* Some Cables

Note: I prefer PlatformIO with VSCode for my software development, but the same project will work on the Arduino platform also. In PlatformIO you have to click on Libraries and then search for the library and then add that library to the project, while in Arduino you have to use "Library Manager" and search for the library and install it. Both are really simple methods and I assume you all are very much familiar with them.

As a first step, we will use the ESP32 to capture the temperature and humidity information from the DHT11 temperature sensor. Fortunately, the software library is already available for this purpose from Adafruit.

Install the DHT sensor library if using PlatformIO or Arduino as shown in the image below.

![Install Adafruit DHT Library for PlatformIO](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEgrCCQJZckl7nkNIZpDWCiWwd6ayHaMHowGPsYZlP4T9fBEc-lvSDZf8KXzpG_Gl3_f7FBNblCTTwDglhGvQhx31U0H7SLAxr1tHC4ZBhC2stuuJ_Xz7eqFjaAiUJ1hUbf3ZsuQZL6XxWQ1sYr3SLYugr_rkgln6sZF-kdeEgIuaT2hIxlTw8atk2y5Dg/s1189/Install_DHT11_Library_PlatformIO.png)


![Install Adafruit DHT Library for Arduino](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEiVCdtY2Am0DGTQN_tWFm6wtqkscycvPwf1hTt6Y-ZdwYbSwZC04VkzRQHdavmNiDddPQ0VhznJdTcvQzH-t_TMttmGH4Dti9GBC6rr-FPk31wn4fUwM95kAOYu-how54kZ9wjNJy2NW7cb2XG0AhRXsBiOJReGwyvo_VVQjiOQjRwUppdoV2VLzwDweQ/s999/Install_DHT11_Arduino.png)

Install the InfluxDB library if using PlatformIO or Arduino as shown in the image, this library has the name ESP8266 but don't worry about that, it works very well with ESP32 also.

![Install InfluxDB ESP8266 Library for PlatformIO](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEhwu29xsov6CN4rMrFquDmOwi8WKYtM-I2wY80eBkfDU5iSHK8biMFh5pslBih2akRsvTCKKwBkWiOpf0vsdD90X8P-1sytA138RQN9d0sWhVbCGFFpQCnPovbrJTzsLP3YLtKm2uAO6vZyJv9nXNI-85U9IxQ7LMSLmLX7GM4pqdYF8PB7sXK8SlMiMw/s1189/InstallInfluxDB_LibraryPlatformIO.png)


![Install InfluxDB ESP8266 Library for Arduino](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEgdMinuoR-_yWKUJY_pVRCy9ldOFoct0CLWP5CHF_Njlck8sU6Zm8mkpBQ77jeqOIyEkfM_YauAljmwkl3vz6AV8YdvKnQxJ60Y2Mp_dUIFdqSyIyEvRyccjWlh5bCRyd0_VL7kYXsho6VBQoqobaEILEMQSgL1Y6XL2yHweIczViIv8Dcoh0Aamnrfuw/s999/InstallInfluxDB_Arduino.png)

## DHT11 Sensor Interfacing
As a first step, we will test the DHT11 sensor to make sure it is working, this is basically a modular approach that helps the software development easier.
Let's start our coding, first, we need to include the DHT sensor include files in the project as shown below.
```C
#include <DHT.h>
#include <DHT_U.h>
```
 
The next step is to select the DHT sensor type and the pin it is connected to, here it is connected to ESP32 pin 12, we have also created a macro "DHT11_REFRESH_TIME", this macro is used to trigger the measurement from the DHT11 sensor, this we will see below.
```C
#define DHTPIN                          (12)
#define DHTTYPE                         (DHT11)

#define DHT11_REFRESH_TIME              (5000u)
```
 
The next step is to create the DHT sensor object and define some variables to store temperature and humidity data, another variable is created named `dht_refresh_timestamp`, this variable along with macro `DHT11_REFRESH_TIME` is used to get the temperature and humidity after every 5 seconds, which is programmed in `DHT11_REFRESH_TIME` macro.
```C
static uint8_t dht11_temperature = 0;
static uint8_t dht11_humidity = 0u;
DHT dht(DHTPIN, DHTTYPE);
```
 
Below are the two main important functions, one is the `DHT11_TaskInit` function and another is the `DHT11_TaskMng()` function.

### DHT11_TaskInit
This function is called inside the `setup` function and it basically setup the sensor pins and sets pull timings, also initializes the `dht_refresh_timestamp` variable with the current values millisecond value using `millis()` function.

### DHT11_TaskMng
This function is called inside the `loop` function and it basically triggers the temperature and humidity measurements and prints the information on the serial port and also saves the data into the variables `dht11_temperature` and `dht11_humidity`.
The content of both of these functions are as below.
```C
static void DHT11_TaskInit( void )
{
  dht.begin();
  // delay(2000);
  dht_refresh_timestamp = millis();
}
 
static void DHT11_TaskMng( void )
{
  uint32_t now = millis();
  float temperature, humidity;
  if( now - dht_refresh_timestamp >= DHT11_REFRESH_TIME )
  {
    dht_refresh_timestamp = now;
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    humidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    temperature = dht.readTemperature();
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperature) ) 
    {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    else
    {
      Serial.print(F("Humidity: "));
      Serial.print(humidity);
      Serial.print(F("%  Temperature: "));
      Serial.print(temperature);
      Serial.println(F("°C "));
      // store this in the global variables
      dht11_humidity = (uint8_t)humidity;
      dht11_temperature = (uint8_t)temperature;
    }
  }
}
 
void setup()
{
  System_Init();
  DHT11_TaskInit();
}

void loop()
{
  DHT11_TaskMng();
}
```
We will test this much and if this works we will proceed further with the connection to the InfluxDB cloud server.

## Creating InfluxDB Database
With the above piece of code, we can get the temperature and humidity data from the DHT11 sensor, and as a next step we have to store this data on the InfluxDB database ,we need an account for that, which can be created for free of cost for primary usage.

To create a free account, register with an email id on this link. And once you are registered you have the possibility to select from the following three options.

* AWS Amazon Web Services
* Google Cloud
* Microsoft Azure

You can select anyone of them, and opt for `Free Account`, which looks like this.

![InfluxDB Account](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEgl6A7tBicrd_WhAzy4uY-hReZbLJyKlGIDv3bmwvMz1RPVjWCVkzf8L2VD6wCBmIMz4__zEOy6EA_RcpS8fl_o8dOqMBJygySAYmtYVoWIbHnBzJoqf0FWUzfBouqaLAe7wdIe7Tir24QpmMc3jza5il1didtj0NFaz3s2VNfnA0tAJTJNcG3Eh-9yiQ/s1024/InfluxDB%2BPlanType.webp)

### InfluxDB Database Connection Details
InfluxDB has already done some work for us in interfacing Arduino-based devices, so we can take advantage of it, this is shown below where we get the option of `Client Libraries`, one can click on `Arduino` icon and follow along.

![InfluxDB Examples Client Libraries](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEhtq8p7R0MBMDPvJ-QMCVii3Wt0t-odvIrvTqTOCkKWqO1N-n5z5MKzh8J-MwvgDQDJQEgiyoB37j8Hzs8C6c-cLHHoOL7s1QEVsmrp9XUSaBst88GTJnG0ZPaSAPC1i7X6rKQ8CZ89EhF1JaXs18E3G-5v33ODL3m4hg6wvzr3kPM14hF8wimBxF4iCw/s1298/Screenshot%20from%202022-12-20%2016-34-08.png)

But let's do it differently here, although it means the same thing, we need two things here, one is `Bucket` and the other thing is `API Tokens` by which the other application can access the InfluxDB database.

Click on **Generate API Token** button to generate token.

![Generate API Tocken](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEiW4feY-puiOOk4m_HU_TW7mNeq7iBkExCNbOTBXDeiqSmhl5moX7vvaT2gS6yYPE9JMkyMz2kJ2KXcHVB1sg83rmTEGCsFTJLtTkmm58uhtYn5CuMMhIYRRXeysc7e01oAB25T5ij0T3AqsrejBqIRtyBKOYAovEdLZBPN1LIdUpWuIqnoo4w-nEXdxw/s867/GenerateAPIToken.png)


Click on **Create Bucket** button to create a bucket. The purpose of bucket is to collect all data

![Create Bucket](https://blogger.googleusercontent.com/img/b/R29vZ2xl/AVvXsEjnPGrSfBa7aNHVozn7tBfqpAjxAhI1m91i2vQAOc3kvPgVHZigCMHskHfyMlqd_LWa7Sla66ePLY4VleEu5VsAjwqVgEcVXgq0S35U1mzXjMHkTImzKzvcHIh-x9B6oY0zKJE64ektvxIK3NBvMVzd-AaC4DQYOPHhEKqPOQw6EhGJJT5NkZdrwJZMTg/s867/CreateBucket.png)

And now next step is to update our program to connect with the InfluxDB server and send data to the InfluxDB database. And for this we will include the InfluxDB-related header files and also the WiFiMulti header file which will handle all WiFi-related operations for the ESP32 chip, so now our header file inclusion will look like this.
 
```C
#include <Arduino.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <DHT.h>
#include <DHT_U.h>
 ```

The next step is to fill in the WiFi Router's SSID and Password information so that our ESP32 module can be connected to the router and the internet.

```C
#define WIFI_SSID             "Enter SSID Here"
#define WIFI_PASSWORD         "Enter Password Here"
 ```

After this, we must enter the InfluxDB database-related information, as mentioned below.
 
```C
#define INFLUXDB_URL          "????????????"
#define INFLUXDB_TOKEN        "????????????"
#define INFLUXDB_ORG          "????????????"
#define INFLUXDB_BUCKET       "????????????"
```
 
All of this information we can get from the InfluxDB account, like for example we just created the `Bucket` and also generated the `API Token`, so that information needs to be entered here, similarly URL and also the organization, which is just your email id.
 

The next step is to set the time zone and this is done using the following statements.
Here I have specified the Central European Time i.e. CET.

```C
// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
```
 
Once this is done we need to define the Device Name, and also a macro named `INFLUXDB_SEND_TIME` to send data periodically to InfluxDB database.

```C
#define DEVICE                          "ESP32"
#define INFLUXDB_SEND_TIME              (10000u)
```
 
After this, I will create a WiFi object, a Point object which stores the data for the bucket, and an InfluxDB client object for connecting and sending data to the InfluxDB database, and this can be done using the following line of code.

```C
// InfluxDB client instance with preconfigured InfluxCloud Certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("Sensor_Data");

WiFiMulti wifiMulti;
```
 
Now let's start working on writing our important function, just like we did for the DHT11 sensor, this part also includes three important functions, first `WiFi_Setup`, the second one is `InfluxDB_TaskInit()` and the last and most important one `InfluxDB_TaskMng()`

### WiFi_Setup
This function connects the ESP32 with the WiFi router and connects it to the internet, here first configure the ESP32 in station mode and then connect it to the router, as shown below.

```C
static void WiFi_Setup( void )
{
  // Connect WiFi
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
}
```
 
### InfluxDB_TaskInit
This function synchronizes time and connects with the InfluxDB database as shown with the help of the below piece of code, we also added a "Tag" named device, to identify that this data is coming from this device which in this case is "ESP32"
 
```C
static void InfluxDB_TaskInit( void )
{
  // Add constant tags - only once
  sensor.addTag("device", DEVICE);

  // Accurate time is necessary for certificate validation & writing in batches
  // For the fastest time sync find NTP servers in your area: 
  // https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) 
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
```
 
### InfluxDB_TaskMng
This function is the most important function and this function, and this function sends the WiFi RSSI value of the WiFi, with Temperature and Humidity data to the InfluxDb database. As shown in the below piece of source code.

This function executes every 10 seconds, but I would suggest it changed to every minute as temperature and humidity values don't change that quickly.

In this function, the following things are done.
* Clear All Fields for the Point
* Then add fields, one is RSSI, then DHT11 Temperature, and then DHT11 Humidity
* Once the above step is done we send this to Serial for debugging purposes to see if everything looks fine or not.
* The next step is that check if the WiFi is still connected or not.
* If connected we will write the Point data to the InfluxDB server

```C
static void InfluxDB_TaskMng( void )
{
  uint32_t now = millis();
  if( now - influxdb_send_timestamp >= INFLUXDB_SEND_TIME )
  {
    influxdb_send_timestamp = now;
    sensor.clearFields();
    // Report RSSI of currently connected network
    sensor.addField( "rssi", WiFi.RSSI() );
    // add temperature and humidity values also
    sensor.addField( "temperature", dht11_temperature );
    sensor.addField( "humidity", dht11_humidity );
    
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(client.pointToLineProtocol(sensor));
    // If no Wifi signal, try to reconnect it
    if (wifiMulti.run() != WL_CONNECTED)
    {
      Serial.println("Wifi connection lost");
    }
    // Write point
    if (!client.writePoint(sensor))
    {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }
  }
}
```

The complete code is present inside the `main.cpp` file.

## Some Important Information about InfluxDB
### InfluxDB Key Terms
[Complete Glossary from InfluxDB Website](https://docs.influxdata.com/influxdb/v2.6/reference/glossary/)

In **InfluxDB** a `bucket` is a named location where the time series data is stored. All `buckets` have a retention period, which defines the duration of time that a bucket retains data, on free cloud accounts on InfluxDB website the maximum retention time allowed is 30 days only.

Data in InfluxDB is stored in tables within rows and columns. A set of data is known as `Point`. Each `Point` has a `mesurement`, a `tag set` and a `field`, a `field value` and a `timestamp`.

Columns stores `tag sets` (indexed) and `field sets`. The only required column is `time`, which stores `timestamps` and is included in all InfluxDB tables.

* **tag**: the key-value pair in InfluxDB’s data structure that records metadata. Tags are an optional part of InfluxDB’s data structure but they are useful for storing commonly-queried metadata; tags are indexed so queries on tags are performant.
* **tag key**: tag keys are strings and store metadata. Tag keys are indexed so queries on tag keys are processed quickly.
* **tag value**: tag values are strings and they store metadata. Tag values are indexed so queries on tag values are processed quickly.
* **field**: the key-value pair in InfluxDB’s data structure that records metadata and the actual data value. Fields are required in InfluxDB’s data structure and they are not indexed – queries on field values scan all points that match the specified time range and, as a result, are not performant relative to tags.
* **field key**: the key of the key-value pair. Field keys are strings and they store metadata.
* **field value**: the value of a key-value pair. Field values are the actual data; they can be strings, floats, integers, or booleans. A field value is always associated with a timestamp. Field values are not indexed – queries on field values scan all points that match the specified time range and, as a result, are not performant.
* **measurement**: the part of InfluxDB’s structure that describes the data stored in the associated fields.