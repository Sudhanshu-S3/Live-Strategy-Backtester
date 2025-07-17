import requests
import psycopg2
import logging
from datetime import datetime, timezone

# --- Configuration ---
# Replace with your actual NewsAPI key
NEWS_API_KEY = ""
# The topic or stock you are interested in
QUERY = "bitcoin" 
# The symbol to tag this news with in your database
SYMBOL_TAG = "btcusdt"
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

def insert_article(cursor, symbol, article):
    """Inserts a single news article into the database."""
    # NewsAPI provides time in ISO 8601 format with Zulu time (UTC)
    ts = datetime.fromisoformat(article['publishedAt'].replace('Z', '+00:00'))

    # Placeholder for sentiment analysis. In a real system, you would
    # run a sentiment analysis model on `article['title']` or `article['content']`.
    # Let's generate a placeholder score between -1 and 1.
    sentiment_score = (len(article['title']) % 200 - 100) / 100.0

    sql = """
        INSERT INTO news_articles (time, symbol, headline, source, url, sentiment_score)
        VALUES (%s, %s, %s, %s, %s, %s)
        ON CONFLICT DO NOTHING; -- Avoid inserting duplicate articles based on URL or headline
    """
    try:
        headline = article.get('title', 'No Title')
        source = article.get('source', {}).get('name', 'Unknown')
        url = article.get('url', '')
        
        cursor.execute(sql, (ts, symbol, headline, source, url, sentiment_score))
        logging.info(f"Inserted article: {headline}")
    except Exception as e:
        logging.error(f"Failed to insert article data: {e}")

def fetch_and_store_news():
    """Fetches news from NewsAPI and stores it in the database."""
    conn = get_db_connection()
    if not conn:
        return
        
    cursor = conn.cursor()
    
    url = f"https://newsapi.org/v2/everything?q={QUERY}&apiKey={NEWS_API_KEY}"
    
    try:
        response = requests.get(url)
        response.raise_for_status() # Raises an HTTPError for bad responses
        data = response.json()
        
        if data['status'] == 'ok':
            for article in data['articles']:
                insert_article(cursor, SYMBOL_TAG, article)
            conn.commit()
            logging.info(f"Successfully processed {data['totalResults']} articles.")
        else:
            logging.error(f"NewsAPI returned an error: {data.get('message')}")

    except requests.exceptions.RequestException as e:
        logging.error(f"Failed to fetch news from API: {e}")
        conn.rollback()
    except Exception as e:
        logging.error(f"An unexpected error occurred: {e}")
        conn.rollback()
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    fetch_and_store_news()