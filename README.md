## Live Strategy Backtester

First, you need to install PostgreSQL and TimescaleDB. Then, connect to your database (e.g., using psql) and run the following SQL commands to create your tables and convert them into TimescaleDB "hypertables," which are optimized for time-series data.

SQL

-- 1. Enable the TimescaleDB extension
CREATE EXTENSION IF NOT EXISTS timescaledb;

-- 2. Create a table for trade data
CREATE TABLE trades (
time TIMESTAMPTZ NOT NULL,
symbol VARCHAR(20) NOT NULL,
price DOUBLE PRECISION,
quantity DOUBLE PRECISION
);

-- 3. Create a table for order book data
-- We use JSONB to efficiently store the list of bids and asks
CREATE TABLE order_books (
time TIMESTAMPTZ NOT NULL,
symbol VARCHAR(20) NOT NULL,
bids JSONB,
asks JSONB
);

-- 4. Turn the regular tables into TimescaleDB hypertables, partitioned by time
SELECT create_hypertable('trades', 'time');
SELECT create_hypertable('order_books', 'time');

-- 5. Create indexes for faster queries
CREATE INDEX ON trades (symbol, time DESC);
CREATE INDEX ON order_books (symbol, time DESC);
