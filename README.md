# Efficient Home Automation with ESP32
A modular home automation system built showcasing how to integrate ESP32 microcontrollers for wireless sensor networks, tradicional home devices control via IR and RF, and simple web dashboards.

## What It Does
- **Environmental Monitoring**: Battery-powered SensorNodes measure temperature and humidity on configurable intervals and report to a central hub.
- **Room Control**: RoomNodes detect presence, manage lighting and air conditioning via RF and IR commands, and adjust behavior based on schedules.
- **Central Hub (MasterDevice)**: Aggregates data, hosts a lightweight web interface with live updates and history charts, and sends commands back to nodes.

## Key Concepts
- **ESP-NOW & Wi‑Fi Hybrid**: Low‑power ESP‑NOW for node‑to‑hub messages; Wi‑Fi for dashboard hosting and NTP sync.
- **Deep Sleep**: SensorNodes use deep sleep to extend battery life, waking only to sample or reconfigure.
- **FreeRTOS Tasks**: Concurrent tasks handle messaging, sensor polling, and control loops, ensuring smooth operation.
- **Auto‑Discovery & Reliable Messaging**: Nodes announce via JOIN; custom ACK‑and‑retry layer ensures robust ESP‑NOW delivery.
