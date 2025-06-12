import xml.etree.ElementTree as ET
from datetime import datetime

# Load XML
tree = ET.parse('books.xml')
root = tree.getroot()

# Set threshold date
cutoff_date = datetime(1990, 1, 1)

for book in root.findall('book'):
    title = book.find('title').text
    date_str = book.find('publish_date').text

    # Convert to datetime
    publish_date = datetime.strptime(date_str, '%Y-%m-%d')

    if publish_date > cutoff_date:
        print(f"{title} published on {publish_date.date()}")
