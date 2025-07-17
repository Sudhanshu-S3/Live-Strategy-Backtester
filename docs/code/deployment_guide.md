# Deployment Guide

## Overview

This guide provides instructions for deploying the Live Strategy Backtester in various environments, from development to production. The deployment process is designed to be flexible, supporting both local development and cloud-based production environments.

## System Requirements

### Minimum Requirements

- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 19.14+)
- CMake 3.12 or higher
- 8GB RAM
- 1GB free disk space

### Recommended Requirements

- C++20 compatible compiler
- CMake 3.20 or higher
- 16GB+ RAM
- SSD storage
- Multi-core CPU

### Operating Systems

- Linux (Ubuntu 20.04+, CentOS 8+)
- Windows 10/11
- macOS 11.0+

## Local Development Deployment

### Step 1: Clone the Repository

```bash
git clone https://github.com/yourusername/Live_Strategy_Backtester.git
cd Live_Strategy_Backtester
```

### Step 2: Initialize Submodules

```bash
git submodule update --init --recursive
```

### Step 3: Create Build Directory

```bash
mkdir build && cd build
```

### Step 4: Configure with CMake

```bash
# For default build
cmake ..

# For debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# For release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# For development with testing enabled
cmake -DBUILD_TESTING=ON ..
```

### Step 5: Build the Project

```bash
# Build with all available cores
cmake --build . --parallel

# Alternatively, use make directly
make -j$(nproc)
```

### Step 6: Run Tests (Optional)

```bash
ctest --output-on-failure
```

### Step 7: Install (Optional)

```bash
# For system-wide installation (requires admin/sudo)
cmake --install .

# For custom installation directory
cmake --install . --prefix /path/to/install/directory
```

## Docker Deployment

### Step 1: Build Docker Image

```bash
# From project root
docker build -t live-strategy-backtester -f deploy/Dockerfile .
```

### Step 2: Run Docker Container

```bash
# Run with local data volume mounted
docker run -v /path/to/local/data:/app/data live-strategy-backtester

# Run interactive shell
docker run -it --entrypoint /bin/bash live-strategy-backtester
```

### Step 3: Docker Compose (Optional)

For more complex setups with multiple services:

```bash
# Start all services
docker-compose -f deploy/docker-compose.yml up

# Stop all services
docker-compose -f deploy/docker-compose.yml down
```

## Cloud Deployment

### AWS Deployment

#### Step 1: Package Application

```bash
# Create deployment package
./scripts/create_aws_package.sh
```

#### Step 2: Deploy to EC2

```bash
# Upload and deploy to EC2 instance
./scripts/deploy_to_ec2.sh your-ec2-instance-id
```

### Azure Deployment

#### Step 1: Create Azure Container Registry

```bash
az acr create --resource-group your-resource-group --name yourregistry --sku Basic
```

#### Step 2: Build and Push Docker Image

```bash
az acr build --registry yourregistry --image live-backtester:latest .
```

#### Step 3: Deploy Container Instance

```bash
az container create --resource-group your-resource-group --name live-backtester \
  --image yourregistry.azurecr.io/live-backtester:latest \
  --registry-login-server yourregistry.azurecr.io \
  --registry-username username --registry-password password \
  --cpu 2 --memory 4 \
  --environment-variables DATA_PATH=/data STRATEGY=MovingAverageCrossover
```

## CI/CD Integration

### GitHub Actions

A GitHub Actions workflow is provided in `.github/workflows/ci.yml` that:

1. Builds the project
2. Runs tests
3. Creates artifacts
4. Deploys to testing/staging environments

### Jenkins

A sample Jenkinsfile is provided in `deploy/Jenkinsfile` that:

1. Builds the project
2. Runs tests
3. Generates documentation
4. Deploys to selected environments

## Performance Optimization

For production deployments, consider these performance optimizations:

### Compiler Optimizations

```bash
# Build with aggressive optimizations
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
```

### Profile-Guided Optimization

```bash
# Step 1: Build with instrumentation
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_GENERATE=ON ..
make

# Step 2: Run representative workloads
./backtester --config large_workload_config.json

# Step 3: Build with profile data
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO_USE=ON ..
make
```

## Configuration

### Configuration Files

The backtester uses JSON configuration files:

```json
{
  "data": {
    "source": "csv",
    "path": "data/BTCUSDT-1s-2025-07-13.csv",
    "format": "time,open,high,low,close,volume"
  },
  "strategy": {
    "name": "MovingAverageCrossover",
    "parameters": {
      "short_period": 20,
      "long_period": 50,
      "position_size": 1.0
    }
  },
  "execution": {
    "model": "market_impact",
    "slippage": 0.001,
    "commission": 0.001
  }
}
```

### Environment Variables

The following environment variables can override configuration:

| Variable         | Description              | Default         |
| ---------------- | ------------------------ | --------------- |
| `BT_DATA_PATH`   | Path to data files       | `./data`        |
| `BT_CONFIG_PATH` | Path to config file      | `./config.json` |
| `BT_LOG_LEVEL`   | Logging verbosity        | `info`          |
| `BT_THREADS`     | Number of worker threads | CPU count       |

## Monitoring and Logging

### Logging Configuration

Logs are written to:

- Console (standard output)
- Log files (`logs/backtester.log`)
- Syslog (in production mode)

Log levels:

- `trace`: Detailed tracing information
- `debug`: Debugging information
- `info`: General information (default)
- `warning`: Warning conditions
- `error`: Error conditions
- `critical`: Critical conditions

### Health Checks

For server deployments, health checks are available at:

- HTTP: `http://hostname:8080/health`
- TCP: Port 8081

## Troubleshooting

### Common Issues

#### CMake Configuration Fails

- Ensure all dependencies are installed
- Check minimum required CMake version
- Verify compiler compatibility

#### Build Errors

- Update submodules: `git submodule update --init --recursive`
- Clear build directory and start fresh
- Check compiler error messages

#### Runtime Errors

- Check data file formats and paths
- Verify configuration file syntax
- Ensure adequate memory and disk space

### Support Resources

- GitHub Issues: https://github.com/yourusername/Live_Strategy_Backtester/issues
- Documentation: https://yourusername.github.io/Live_Strategy_Backtester
- Community Forum: https://forum.example.com/backtester
