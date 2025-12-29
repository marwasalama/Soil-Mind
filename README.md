# Smart Farm IoT System

An IoT-based plant health monitoring and automated irrigation system using TinyML, ESP32 microcontroller, and cloud-deployed data pipeline.


<img width="1536" height="1024" alt="dashboard01" src="https://github.com/user-attachments/assets/2c8eaf56-55b4-4c94-9045-6251af8d50a3" />


## Overview

This project combines edge computing with cloud-based analytics to monitor plant health in real-time and automate irrigation decisions. The system uses TinyML models running on FreeRTOS for on-device inference, while sensor data flows through an MQTT-based pipeline to a cloud-hosted monitoring dashboard accessible from anywhere in the world.

## System Architecture


<img width="1024" height="1536" alt="ChatGPT Image Dec 29, 2025, 03_55_03 PM" src="https://github.com/user-attachments/assets/6bbe3784-5fc4-49b3-bd23-3060e975c399" />


### Hardware Components

**ESP32 Microcontroller (FreeRTOS)**
- Runs FreeRTOS for real-time task management
- Executes two TinyML models for edge inference
- Collects and processes data from multiple sensors
- Publishes sensor data via MQTT protocol

**Sensors**
| Sensor | Measurements |
|--------|-------------|
| Soil Moisture Sensor | Soil moisture percentage |
| DHT Sensor | Temperature (°C) and Humidity (%) |
| NPK Sensor | Nitrogen, Phosphorus, Potassium levels |
| pH Sensor | Soil pH level |

### Software Stack

**Edge Layer**
- **Operating System:** FreeRTOS
- **ML Framework:** TensorFlow Lite Micro (TinyML)
- **Communication Protocol:** MQTT

**Cloud Infrastructure (AWS Managed Services)**

The entire data pipeline is built using AWS managed cloud services.
Sensor data is securely ingested, stored, and visualized through AWS IoT Core,
Amazon Timestream, and Amazon Managed Grafana, enabling remote access to the dashboard
from anywhere in the world.


| Component | Function |
|-----------|----------|
| **AWS IoT Core** | Managed MQTT broker for securely receiving sensor data from ESP32 or simulators|
| **AWS IoT Rule Engine** | Routes incoming MQTT messages to downstream AWS services |
| **Amazon Timestream** | Managed time-series database for storing sensor telemetry data |
| **Amazon Managed Grafana** |Cloud-based dashboards for real-time visualization and monitoring |

### Machine Learning Models

**1. Plant Health Prediction Model**
- Analyzes sensor data to determine plant health status
- Classifies plant condition as Healthy/Unhealthy
- Runs on-device using TinyML

**2. Irrigation Control Model**
- Predicts optimal irrigation timing based on soil conditions
- Controls irrigation system (ON/OFF/STANDBY)
- Enables automated watering decisions

## Grafana Dashboard

The Smart Farm IoT Dashboard displays real-time monitoring data:

**Sensor Readings**
- Soil Moisture (%) with threshold indicators
- Temperature (°C)
- Humidity (%)
- NPK Levels (Nitrogen, Phosphorus, Potassium)
- pH Level with optimal range gauge

**Visualizations**
- Soil Moisture & Temperature real-time graphs
- NPK Levels bar charts
- NPK Trends time-series graphs
- pH Level gauge with optimal zone indicator

**System Status**
- Irrigation System status (ON/OFF/STANDBY)
- Plant Health status (HEALTHY/UNHEALTHY)
- Active Alerts panel
- Recommendations section

## Features

- ✅ Real-time plant health monitoring
- ✅ On-device ML inference using TinyML
- ✅ Automated irrigation control based on ML predictions
- ✅ Cloud-deployed pipeline for global dashboard access
- ✅ Historical data visualization and trends
- ✅ Configurable thresholds and alerts
- ✅ Recommendations based on sensor readings

## Deployment

The system is deployed using AWS managed cloud services, where each component is provided
as a fully managed service by AWS.

AWS Cloud (Managed Services)
- AWS IoT Core
- AWS IoT Rule Engine
- Amazon Timestream
- Amazon Managed Grafana


This architecture enables secure, remote access to real-time monitoring dashboards
through Amazon Managed Grafana from any location via a web browser.

## Getting Started

Setup instructions will be added as development progresses, including:
- Docker Compose configuration
- ESP32 firmware flashing guide
- Cloud deployment steps
- Grafana dashboard import

## Project Status

🚧 **Currently in Development**
