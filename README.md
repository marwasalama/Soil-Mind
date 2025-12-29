# Plant Health Prediction System

An IoT-based plant health monitoring and automated irrigation system using Machine Learning and ESP microcontrollers.

## Overview

This project combines edge computing with cloud-based analytics to monitor plant health in real-time and automate irrigation decisions. The system uses TinyML for on-device inference and cloud-deployed ML models for comprehensive health prediction and irrigation scheduling.

## System Architecture

### Hardware Components

- **ESP32 #1 (Data Acquisition & TinyML)**
  - Collects sensor data from plants
  - Runs TinyML model for edge inference
  - Transmits data to cloud infrastructure

- **ESP32 #2 (Irrigation Control)**
  - Controls irrigation valve
  - Receives commands based on ML predictions
  - Executes automated watering schedules

### Software Stack

#### Cloud Infrastructure
- **Deployment:** AWS Cloud
- **Data Pipeline:** TIG Stack (Docker containerized)
  - **Telegraf:** Data collection and metrics aggregation
  - **InfluxDB:** Time-series database for sensor data
  - **Grafana:** Data visualization and monitoring dashboards

#### Machine Learning Models
1. **Health Prediction Model**
   - Analyzes plant health status
   - [More details to be added]

2. **Irrigation Scheduling Model**
   - Optimizes watering schedules
   - [More details to be added]

## Project Status

🚧 **Currently in Development**

This README will be updated as the project progresses with:
- Detailed setup instructions
- Sensor specifications
- ML model architectures and performance metrics
- API documentation
- Deployment guides
- Usage examples

## Features (Planned)

- Real-time plant health monitoring
- Predictive analytics for plant diseases
- Automated irrigation based on ML recommendations
- Historical data visualization
- Alerts and notifications

## Getting Started

Setup instructions will be added as development progresses.

