import requests
import logging
from datetime import datetime
import json
import os

# --- Configuration ---
# Replace with your actual NewsAPI key
NEWS_API_KEY = ""
# The topic or stock you are interested in
QUERY = "bitcoin" 
# The symbol to tag this news with
SYMBOL_TAG = "btcusdt"
# Directory to save processed news (optional)
OUTPUT_DIR = "processed_news"

# --- Logging Setup ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def process_article(symbol, article):
    """Processes a single news article without database storage."""
    # NewsAPI provides time in ISO 8601 format with Zulu time (UTC)
    ts = datetime.fromisoformat(article['publishedAt'].replace('Z', '+00:00'))

    # Placeholder for sentiment analysis. In a real system, you would
    # run a sentiment analysis model on `article['title']` or `article['content']`.
    # Let's generate a placeholder score between -1 and 1.
    sentiment_score = (len(article['title']) % 200 - 100) / 100.0

    headline = article.get('title', 'No Title')
    source = article.get('source', {}).get('name', 'Unknown')
    url = article.get('url', '')
    
    # Create a structured representation of the article
    processed_article = {
        "timestamp": ts.isoformat(),
        "symbol": symbol,
        "headline": headline,
        "source": source,
        "url": url,
        "sentiment_score": sentiment_score
    }
    
    # Log the article information instead of storing it
    logging.info(f"Processed article: {headline} | Sentiment: {sentiment_score:.2f}")
    
    return processed_article

def fetch_news():
    """Fetches news from NewsAPI and processes it without database storage."""
    articles_processed = []
    
    url = f"https://newsapi.org/v2/everything?q={QUERY}&apiKey={NEWS_API_KEY}"
    
    try:
        response = requests.get(url)
        response.raise_for_status()  # Raises an HTTPError for bad responses
        data = response.json()
        
        if data['status'] == 'ok':
            for article in data['articles']:
                processed = process_article(SYMBOL_TAG, article)
                articles_processed.append(processed)
            
            logging.info(f"Successfully processed {len(articles_processed)} articles.")
            
            # Optionally save to a JSON file
            if not os.path.exists(OUTPUT_DIR):
                os.makedirs(OUTPUT_DIR)
            
            output_file = os.path.join(OUTPUT_DIR, f"news_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json")
            with open(output_file, 'w') as f:
                json.dump(articles_processed, f, indent=2)
            logging.info(f"Saved processed articles to {output_file}")
                
        else:
            logging.error(f"NewsAPI returned an error: {data.get('message')}")

    except requests.exceptions.RequestException as e:
        logging.error(f"Failed to fetch news from API: {e}")
    except Exception as e:
        logging.error(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    fetch_news()