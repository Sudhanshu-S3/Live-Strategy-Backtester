import asyncio
import json
import websockets
import psycopg2
from psycopg2.extras import Json
import logging
from datetime import datetime

# --- Configuration ---
SYMBOL = "btcusdt"
WEBSOCKET_URI = f"wss://stream.binance.com:9443/ws/{SYMBOL}@depth@100ms"
DB_CONNECTION_STRING = "dbname=your_db user=your_user password=your_pass host=localhost"

# --- Logging Setup ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def get_db_connection():
    """Establishes and returns a database connection."""
    try:
        conn = psycopg2.connect(DB_CONNECTION_STRING)
        logging.info("Successfully connected to the database.")
        return conn
    except psycopg2.OperationalError as e:
        logging.error(f"Could not connect to the database: {e}")
        return None

def insert_order_book(cursor, symbol, data):
    """Inserts a single order book snapshot into the database."""
    # Binance sends timestamps in milliseconds, convert to seconds for PostgreSQL
    ts = datetime.fromtimestamp(data['E'] / 1000.0)
    
    # Use psycopg2.extras.Json to wrap the lists, which handles JSONB conversion
    bids = Json(data['b'])
    asks = Json(data['a'])
    
    sql = """
        INSERT INTO order_books (time, symbol, bids, asks)
        VALUES (%s, %s, %s, %s);
    """
    try:
        cursor.execute(sql, (ts, symbol, bids, asks))
    except Exception as e:
        logging.error(f"Failed to insert order book data: {e}")


async def record_data():
    """Connects to WebSocket and records data to the database."""
    conn = get_db_connection()
    if not conn:
        return
        
    cursor = conn.cursor()
    
    async with websockets.connect(WEBSOCKET_URI) as websocket:
        logging.info(f"Connected to WebSocket stream for {SYMBOL}.")
        while True:
            try:
                message = await websocket.recv()
                data = json.loads(message)
                
                # 'E' is the event time from Binance API
                if 'e' in data and data['e'] == 'depthUpdate':
                    insert_order_book(cursor, SYMBOL, data)
                    conn.commit() # Commit after each insert
                    logging.info(f"Recorded order book for {SYMBOL} at {data['E']}")

            except websockets.exceptions.ConnectionClosed:
                logging.warning("WebSocket connection closed. Reconnecting...")
                break
            except Exception as e:
                logging.error(f"An error occurred: {e}")
                conn.rollback() # Rollback transaction on error

    cursor.close()
    conn.close()

if __name__ == "__main__":
    while True:
        try:
            asyncio.run(record_data())
        except KeyboardInterrupt:
            logging.info("Recording stopped by user.")
            break
        except Exception as e:
            logging.error(f"Main loop failed, restarting... Error: {e}")
            # Optional: add a delay before restarting
            # import time
            # time.sleep(5)