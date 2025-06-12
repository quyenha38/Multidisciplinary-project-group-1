import requests

# Server URL
url = 'http://127.0.0.1:5000/predict'

# Path to a sample test image
image_path = './Original Dataset/Anthracnose/Anthracnose00001.jpg'

# Send POST request with image
with open(image_path, 'rb') as img_file:
    files = {'file': img_file}
    response = requests.post(url, files=files)

# Output the response
if response.status_code == 200:
    print("Prediction:", response.json()['predicted_class'])
else:
    print("Error:", response.status_code, response.text)
