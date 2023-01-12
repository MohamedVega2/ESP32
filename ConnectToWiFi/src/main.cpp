#include <lvgl.h>
#include <WiFi.h>
#include "main.h"
#include "display_mng.h"

// Macros
#define LVGL_REFRESH_TIME                   (5u)      // 5 milliseconds
#define WIFI_MAX_SSID                       (5u)

// Private Variables
static uint32_t lvgl_refresh_timestamp = 0u;
static char wifi_dd_list[WIFI_MAX_SSID*20] = { 0 };

// Private functions
static void WiFi_Init( void );
static void WiFi_ScanSSID( void );
static void LVGL_TaskInit( void );
static void LVGL_TaskMng( void );

void setup()
{
  Serial.begin(115200);
  WiFi_Init();
  WiFi_ScanSSID();

  LVGL_TaskInit();
  Display_Init();
}

void loop()
{
  Display_Mng();
  LVGL_TaskMng();
}

/**
 * @brief This function returns the WiFi SSID Names appended together which is 
 *        suitable for LVGL drop down widget
 * @param  none
 * @return pointer to wifi ssid drop down list
 */
char *Get_WiFiSSID_DD_List( void )
{
  return wifi_dd_list;
}

// Private Function Definitions
static void WiFi_Init( void )
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(2000);
}
static void WiFi_ScanSSID( void )
{
  String ssid_name;
  Serial.println("Start Scanning");
  int n = WiFi.scanNetworks();
  Serial.println("Scanning Done");
  if( n == 0 )
  {
    Serial.println("No Networks Found");
  }
  else
  {
    // I am restricting n to max 5 value
    n = n <= 5 ? n : 5;
    for (int i = 0; i < n; i++) 
    {
      if( i == 0 )
      {
        ssid_name = WiFi.SSID(i);
      }
      else
      {
        ssid_name = ssid_name + WiFi.SSID(i);
      }
      ssid_name = ssid_name + '\n';
      delay(10);
    }
    strcpy( wifi_dd_list, ssid_name.c_str() );
    Serial.println(wifi_dd_list);
  }
  Serial.println("Scanning Completed");
}

static void LVGL_TaskInit( void )
{
  lv_init();
  lvgl_refresh_timestamp = millis();
}

static void LVGL_TaskMng( void )
{
  uint32_t now = millis();
  // LVGL Refresh Timed Task
  if( (now - lvgl_refresh_timestamp) >= LVGL_REFRESH_TIME )
  {
    lvgl_refresh_timestamp = now;
    // let the GUI does work
    lv_timer_handler();
  }
}