import numpy as np
import tensorflow as tf
from flask import Flask, request, jsonify
from tensorflow.keras.preprocessing import image
from tensorflow.keras.applications.mobilenet import preprocess_input
from PIL import Image
import io
from flask_cors import CORS
import csv
import pandas as pd
# Configuration
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg'}
IMG_SIZE = (224, 224)

# Class names (from your dataset)
class_names = [
    'Anthracnose',
    'Bacterial Blight',
    'Citrus Canker',
    'Curl Virus',
    'Deficiency Leaf',
    'Dry Leaf',
    'Healthy Leaf',
    'Sooty Mould',
    'Spider Mites'
]

# Load the model once at startup
model = tf.keras.models.load_model('my_saved_model.keras')

# Create Flask app
app = Flask(__name__)
CORS(app)
# Utility: Check allowed file extensions
def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

# Predict image class from memory
def predict_image(file_stream):
    img = Image.open(file_stream).convert('RGB')  # Ensure it's RGB
    img = img.resize(IMG_SIZE)
    x = image.img_to_array(img)
    x = np.expand_dims(x, axis=0)
    x = preprocess_input(x)
    preds = model.predict(x)
    pred_index = tf.argmax(preds[0]).numpy()
    return class_names[pred_index]
CSV_FILE = 'thingsboard.csv'

@app.route('/log')
def log():
    try:
        df = pd.read_csv('thingsboard.csv', sep=';')
        df.rename(columns={
            'Timestamp': 'timestamp',
            'Humidity': 'humid',
            'Temperature': 'temp'
        }, inplace=True)
        df = df.dropna(axis='index', how='all')
        data = df[['timestamp', 'humid', 'temp']].to_dict(orient='records')
        print(data)
        return jsonify(data)  # <-- THIS must be used!
    except Exception as e:
        print("[ERROR] Failed to process log:", e)
        return jsonify({'error': str(e)}), 500

    

@app.route('/predict', methods=['POST'])
def predict():
    if 'file' not in request.files:
        return jsonify({'error': 'No file part'}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400

    if file and allowed_file(file.filename):
        predicted_class = predict_image(file.stream)
        return jsonify({'predicted_class': predicted_class})

    return jsonify({'error': 'Invalid file type. Only png, jpg, jpeg allowed.'}), 400

# Run the app
if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0')
