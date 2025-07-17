import requests
import pandas as pd
import time
from datetime import datetime, timedelta

# --- Configuration ---
SYMBOL = "BTCUSDT"
DAYS_OF_DATA = 7
API_ENDPOINT = "https://api.binance.com/api/v3/historicalTrades"
OUTPUT_FOLDER = "../data/"
OUTPUT_FILENAME = f"{OUTPUT_FOLDER}{SYMBOL}_trades_{DAYS_OF_DATA}days.csv"
REQUEST_LIMIT = 1000 # Binance API limit per request

def get_historical_trades(symbol):
    """
    Downloads historical trade data from Binance and saves it to a CSV file.

    It fetches data in chunks to respect the API's 1000-trade limit per request,
    iterating backwards in time until the desired date range is covered.
    """
    print(f"Starting download for {symbol}...")

    # Calculate start time (N days ago in UTC)
    start_date = datetime.utcnow() - timedelta(days=DAYS_OF_DATA)
    start_timestamp_ms = int(start_date.timestamp() * 1000)
    
    all_trades = []
    from_id = None

    while True:
        try:
            params = {
                "symbol": symbol,
                "limit": REQUEST_LIMIT
            }
            if from_id:
                params["fromId"] = from_id

            print(f"Fetching trades from ID: {from_id or 'Most Recent'}")
            response = requests.get(API_ENDPOINT, params=params)
            response.raise_for_status()  # Raise an exception for bad status codes (4xx or 5xx)
            
            trades = response.json()

            if not trades:
                print("No more trades to fetch.")
                break

            all_trades.extend(trades)

            # The earliest trade comes first in the response list.
            # We get its ID and subtract 1 to get the next chunk of older trades.
            oldest_trade_id = trades[0]['id']
            oldest_trade_time_ms = trades[0]['time']
            from_id = oldest_trade_id - 1

            # Get the timestamp of the oldest trade we've fetched so far
            oldest_datetime = datetime.utcfromtimestamp(oldest_trade_time_ms / 1000)
            print(f"Fetched {len(trades)} trades. Oldest trade in batch is from: {oldest_datetime.strftime('%Y-%m-%d %H:%M:%S')}")

            # Stop if we've gone past our target start date
            if oldest_trade_time_ms < start_timestamp_ms:
                print("Reached the target start date. Finishing download.")
                break
            
            # Respect API rate limits
            time.sleep(0.5) 

        except requests.exceptions.RequestException as e:
            print(f"An error occurred: {e}")
            # Optional: Implement retry logic here
            break
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
            break

    if not all_trades:
        print("Could not download any trade data.")
        return

    # Create a DataFrame
    df = pd.DataFrame(all_trades)
    
    # Select and rename columns for clarity
    df = df[['id', 'price', 'qty', 'quoteQty', 'time', 'isBuyerMaker']]
    df.rename(columns={'id': 'trade_id', 'qty': 'quantity'}, inplace=True)

    # Convert timestamp to a readable datetime format (and set as index)
    df['datetime'] = pd.to_datetime(df['time'], unit='ms')
    
    # Filter out trades older than our start date
    df = df[df['datetime'] >= start_date]

    # Sort data chronologically (oldest first)
    df.sort_values(by='time', inplace=True)
    df.set_index('datetime', inplace=True)

    # Save to CSV
    print(f"\nDownloaded a total of {len(df)} trades.")
    print(f"Saving data to {OUTPUT_FILENAME}...")
    
    # Ensure the output directory exists
    import os
    os.makedirs(OUTPUT_FOLDER, exist_ok=True)
    
    df.to_csv(OUTPUT_FILENAME)
    print("Download and save complete!")


if __name__ == "__main__":
    get_historical_trades(SYMBOL)