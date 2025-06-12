# Use official TensorFlow image as base
FROM tensorflow/tensorflow:2.15.0

# Set working directory
WORKDIR /app

# Copy files to container
COPY . .

# Install dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Expose default Flask port
ENV PORT 8080
ENV PYTHONUNBUFFERED True

# Run the Flask app
CMD ["python", "app.py"]
