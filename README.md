# Smart Farm IoT System

An IoT-based plant health monitoring and automated irrigation system using TinyML, ESP32 microcontroller, and cloud-deployed data pipeline.

<img width="784" height="447" alt="Screenshot 2025-12-30 140324" src="https://github.com/user-attachments/assets/7fe054b2-6896-408a-9c07-5cd9db7cc6b1" />


## Overview

This project combines edge computing with cloud-based analytics to monitor plant health in real-time and automate irrigation decisions. The system uses TinyML models running on FreeRTOS for on-device inference, while sensor data flows through an MQTT-based pipeline to a cloud-hosted monitoring dashboard accessible from anywhere in the world.

## System Architecture

![System Architecture](docs/flowchart.drawio.png "Architecture Diagram")


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

**Cloud Infrastructure (Docker Containerized)**

The entire data pipeline is containerized using Docker and deployed on cloud platforms (AWS / Azure / Hostinger) enabling remote access to the dashboard from anywhere in the world.

| Component | Function |
|-----------|----------|
| **Mosquitto** | MQTT broker for receiving sensor data from ESP32 |
| **Telegraf** | Data collection agent and metrics aggregation |
| **InfluxDB** | Time-series database for storing sensor data |
| **Grafana** | Real-time visualization and monitoring dashboards |

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

The Docker containerized stack (Mosquitto → Telegraf → InfluxDB → Grafana) can be deployed on:
- **AWS** (EC2 / ECS)
- **Azure** (Container Instances / AKS)
- **Hostinger** (VPS)

This enables accessing the Grafana dashboard from any location worldwide via web browser.

## Getting Started

Setup instructions will be added as development progresses, including:
- Docker Compose configuration
- ESP32 firmware flashing guide
- Cloud deployment steps
- Grafana dashboard import

## Project Status

🚧 **Currently in Development**
