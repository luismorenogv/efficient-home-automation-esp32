#define MS_TO_US 1000


RTC_DATA_ATTR uint32_t wake_up_period_ms = 5000; 
RTC_DATA_ATTR uint32_t time_awake_ms = 500;

unsigned long wake_up_instant = millis();

void setup() {
  Serial.begin(115200);
  Serial.println("Wake up");

}

void loop() {
  /*unsigned long elapsed_time = millis() - wake_up_instant;
     while (elapsed_time < time_awake_ms){
      elapsed_time = millis() - wake_up_instant;
    } 

    // Calculate sleep duration
    unsigned long sleep_duration = wake_up_period_ms - (millis() - wake_up_instant);
    if (sleep_duration < 0) sleep_duration = 0; // Error handling

    Serial.print("Sleeping for: ");
    Serial.print(sleep_duration / 1000);
    Serial.println(" seconds.");
    */

    // Configure next wake up
    esp_sleep_enable_timer_wakeup(50000);

    Serial.println("Entering Deep Sleep");
    esp_deep_sleep_start();

}
