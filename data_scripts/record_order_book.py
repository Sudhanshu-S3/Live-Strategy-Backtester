import websocket
import json
import csv
from datetime import datetime
import time

# --- Configuration ---
SYMBOL = "btcusdt"
OUTPUT_FOLDER = "../historical_data/"
OUTPUT_FILENAME = f"{OUTPUT_FOLDER}{SYMBOL}_book_depth_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
WEBSOCKET_URL = f"wss://stream.binance.com:9443/ws/{SYMBOL}@depth20@100ms"

# This is Level 2 data, top 20 bids/asks, updated every 100ms

def on_message(ws, message):
    """
    This function is called every time a new message is received from the WebSocket.
    It processes the message and writes the order book snapshot to a CSV file.
    """
    data = json.loads(message)
    
    # Get the current UTC timestamp for our record
    utc_timestamp = datetime.utcnow().timestamp()

    # The 'bids' and 'asks' are lists of [price, quantity]
    bids = data.get('bids', [])
    asks = data.get('asks', [])

    rows_to_write = []
    # Process bids
    for price, quantity in bids:
        rows_to_write.append([utc_timestamp, 'BID', price, quantity])

    # Process asks
    for price, quantity in asks:
        rows_to_write.append([utc_timestamp, 'ASK', price, quantity])

    # Write all rows for this snapshot to the CSV
    try:
        with open(OUTPUT_FILENAME, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerows(rows_to_write)
    except Exception as e:
        print(f"Error writing to file: {e}")

    # Optional: Print a small confirmation to the console
    print(f"Recorded snapshot at {datetime.utcnow().strftime('%H:%M:%S.%f')[:-3]} with {len(bids)} bids and {len(asks)} asks.")


def on_error(ws, error):
    print(f"Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("### WebSocket closed ###")

def on_open(ws):
    print("### WebSocket opened ###")
    print(f"Recording order book data for {SYMBOL.upper()}...")
    print(f"Data will be saved to: {OUTPUT_FILENAME}")
    # Write the header to the CSV file
    with open(OUTPUT_FILENAME, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['timestamp', 'type', 'price', 'quantity'])

if __name__ == "__main__":
    # Ensure the output directory exists
    import os
    os.makedirs(OUTPUT_FOLDER, exist_ok=True)
    
    # Start the WebSocket connection
    ws = websocket.WebSocketApp(WEBSOCKET_URL,
                              on_open=on_open,
                              on_message=on_message,
                              on_error=on_error,
                              on_close=on_close)

    # Run forever until you manually stop the script (Ctrl+C)
    ws.run_forever()