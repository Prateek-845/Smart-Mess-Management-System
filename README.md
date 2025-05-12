# Smart Mess Management System – IoT Based Mess Management System

A complete IoT project for real-time monitoring and management of a mess (dining hall) using ESP32, sensors, and a Python Flask dashboard.

## Features

- **Occupancy Tracking:** Counts people entering and exiting using PIR sensors.
- **Environmental Monitoring:** Measures temperature and humidity using a DHT22 sensor.
- **Waste Bin Monitoring:** Detects bin fill level with an ultrasonic sensor and alerts when full.
- **Live Dashboard:** Real-time data visualization and historical data via a Flask web app.
- **RESTful API:** ESP32 sends data to the server using HTTP POST in JSON format.
- **User Authentication:** Login system for accessing and saving historical records.
- **Cross-Platform:** Designed for deployment on Linux (tested in VMware and Raspberry Pi OS).

## Technologies Used

- ESP32 (Arduino IDE, C++)
- Sensors: PIR, DHT22, Ultrasonic (HC-SR04)
- Python 3, Flask, SQLite
- HTML, CSS, JavaScript
- Wi-Fi, HTTP/REST

## How It Works

1. **ESP32** collects sensor data and sends it via Wi-Fi to the Flask server.
2. The **Flask server** stores and displays the data on a web dashboard.
3. **Users** can view live mess stats, historical data, and receive alerts for full bins.
