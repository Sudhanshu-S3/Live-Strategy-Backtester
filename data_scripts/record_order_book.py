import asyncio
import json
import websockets
import logging
from datetime import datetime
import os

# --- Configuration ---
SYMBOL = "btcusdt"
WEBSOCKET_URI = f"wss://stream.binance.com:9443/ws/{SYMBOL}@depth@100ms"
OUTPUT_DIR = "orderbook_data"

# --- Logging Setup ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

async def process_order_book(symbol, data):
    """Processes a single order book update without database storage."""
    timestamp = datetime.fromtimestamp(data['E']/1000.0).isoformat()
    
    # Log a summary of the order book update
    bid_count = len(data.get('b', []))
    ask_count = len(data.get('a', []))
    logging.info(f"Order book update for {symbol} at {timestamp}: {bid_count} bids, {ask_count} asks")
    
    return {
        "symbol": symbol,
        "timestamp": timestamp,
        "last_update_id": data.get('u', 0),
        "bids": data.get('b', []),
        "asks": data.get('a', [])
    }

async def record_data():
    """Connects to WebSocket and processes data without database storage."""
    in_memory_cache = []
    last_save_time = datetime.now()
    
    async with websockets.connect(WEBSOCKET_URI) as websocket:
        logging.info(f"Connected to WebSocket stream for {SYMBOL}.")
        while True:
            try:
                message = await websocket.recv()
                data = json.loads(message)
                
                # Process order book update
                if 'e' in data and data['e'] == 'depthUpdate':
                    processed = await process_order_book(SYMBOL, data)
                    in_memory_cache.append(processed)
                    
                    # Save data to file every 100 updates or 60 seconds
                    now = datetime.now()
                    if len(in_memory_cache) >= 100 or (now - last_save_time).seconds >= 60:
                        if not os.path.exists(OUTPUT_DIR):
                            os.makedirs(OUTPUT_DIR)
                        
                        output_file = os.path.join(OUTPUT_DIR, f"orderbook_{now.strftime('%Y%m%d_%H%M%S')}.json")
                        with open(output_file, 'w') as f:
                            json.dump(in_memory_cache, f)
                        logging.info(f"Saved {len(in_memory_cache)} order book updates to {output_file}")
                        
                        in_memory_cache = []  # Clear cache after saving
                        last_save_time = now

            except websockets.exceptions.ConnectionClosed:
                logging.warning("WebSocket connection closed. Reconnecting...")
                break
            except Exception as e:
                logging.error(f"An error occurred: {e}")

if __name__ == "__main__":
    while True:
        try:
            asyncio.run(record_data())
        except KeyboardInterrupt:
            logging.info("Recording stopped by user.")
            break
        except Exception as e:
            logging.error(f"Unhandled exception: {e}")
            logging.info("Restarting in 5 seconds...")
            asyncio.sleep(5)