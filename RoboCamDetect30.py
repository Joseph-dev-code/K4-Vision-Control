import cv2
import numpy as np
import socket
import struct
import time
from ultralytics import YOLO

# Camera details
active_imager_width_mm = 4.51
active_imager_height_mm = 2.88
active_pixels_width = 384
active_pixels_height = 240
default_lens_focal_length_mm = 2.1

# Predefined object heights in cm
objects_height_cm = {
    "stop": 8,
    "right": 7,
    "left": 7,
    "thirty": 6.5,
    "sixty": 6.5
}

# Creating server socket (port 5544)
IP = "192.168.1.3"
PORT = 5544
SIZE = 1024

server_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_addr = (IP, PORT)

try:
    server_fd.bind(server_addr)
    server_fd.listen(1)  # Listen for only one connection
    print(f"[LISTENING] Port Number: {PORT}")
    client_fd, client_addr = server_fd.accept()
    print("[CONNECTED] Connection established")
except Exception as e:
    print(f"[ERROR] Failed to create server or accept connection: {e}")
    exit()

# Initialize YOLOv8 model with custom weights
model = YOLO('/home/anem/anaconda3/envs/yolov8_custom/runs/detect/MAIN_train5/weights/best.pt')

# Calculate focal length in pixels
focal_length_pixels_width = (default_lens_focal_length_mm * active_pixels_width) / active_imager_width_mm
focal_length_pixels_height = (default_lens_focal_length_mm * active_pixels_height) / active_imager_height_mm

# Function to find distance
def distance_finder(focal_length_pixels, object_height_pixels, objects_height_cm):
    if object_height_pixels > 0:
        distance = (objects_height_cm * focal_length_pixels) / object_height_pixels
        return distance
    else:
        return None

# Function to send prediction over the socket
def send_prediction(prediction):
    try:
        client_fd.sendall(struct.pack('>I', len(prediction)))
        client_fd.sendall(prediction.encode())  # Encoding the string before sending
        print(f"[SENT] Prediction '{prediction}' successfully sent.")
    except Exception as e:
        print(f"[ERROR] Failed to send prediction: {e}")

# Video stream logic from test.py
video_stream_url = 'tcp://192.168.1.101:5000'
cap = cv2.VideoCapture(video_stream_url)

if not cap.isOpened():
    print("Error: Failed to open video feed.")
    exit()

# Cooldown period after sending a prediction (in seconds)
cooldown_period = 6

# Tolerance level for considering distance as equal to 30 cm
tolerance = 1.0  # Increased tolerance to make it easier to detect

# Dictionary to keep track of the last time each prediction was sent
last_prediction_time = {"stop": 0, "right": 0, "left": 0, "thirty": 0, "sixty": 0}

# Main loop for object detection and tracking
while True:
    # Read a frame from the video stream
    ret, frame = cap.read()
    if not ret:
        print("Error: Failed to read frame from video feed.")
        break

    frame = cv2.resize(frame, (384, 240))

    # Perform object detection on the frame using YOLOv8
    results = model.predict(frame)

    # Iterate over detected objects
    for box in results[0].boxes:
        x1, y1, x2, y2 = map(int, box.xyxy[0])
        class_id = model.names[int(box.cls[0])]
        conf = round(box.conf[0].item(), 2)
        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.putText(frame, f"{class_id} {conf}", (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

        # Calculate the distance to the detected object
        object_height_pixels = y2 - y1
        if class_id in objects_height_cm:
            distance = distance_finder(focal_length_pixels_height, object_height_pixels, objects_height_cm[class_id])
            if distance is not None:
                cv2.putText(frame, f"Distance: {int(distance)} CM", (x1, y2 + 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
                print(f"[DEBUG] Distance to {class_id}: {int(distance)} CM")

                # Check if the distance is within the tolerance of 30 cm and send the prediction
                if abs(distance - 30) < tolerance:
                    if time.time() - last_prediction_time[class_id] >= cooldown_period:
                        print(f"[DEBUG] Sending prediction for {class_id} at {int(distance)} CM")
                        send_prediction(class_id)
                        last_prediction_time[class_id] = time.time()

    # Display the frame with detected objects and tracking
    cv2.imshow("Object Detection", frame)

    # Exit if the user presses the 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release the video capture object and close all windows
cap.release()
client_fd.close()
print("[DISCONNECTED] Connection closed")
cv2.destroyAllWindows()

