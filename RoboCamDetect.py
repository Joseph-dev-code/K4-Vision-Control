'''import cv2
from ultralytics import YOLO

# Initialize the YOLO model with the custom weights
model = YOLO('/home/anem/anaconda3/envs/yolov8_custom/runs/detect/train4/weights/best.pt')

# Open the video feed

video_stream_url = 'tcp://192.168.1.101:5000'
cap = cv2.VideoCapture(video_stream_url)

if not cap.isOpened():
    print("Error: Failed to open video feed.")
    exit()

while True:
    # Read a frame from the video feed
    ret, frame = cap.read()

    if not ret:
        print("Error: Failed to read frame from video feed.")
        break

    # Perform object detection on the frame
    results = model.predict(frame)

    # Draw the detection results on the frame
    for box in results[0].boxes:
        x1, y1, x2, y2 = map(int, box.xyxy[0])
        class_id = model.names[int(box.cls[0])]
        conf = round(box.conf[0].item(), 2)
        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.putText(frame, f"{class_id} {conf}", (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    # Display the frame
    cv2.imshow("Object Detection", frame)

    # Exit if the user presses the 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release the video feed and destroy all windows
cap.release()
cv2.destroyAllWindows()'''


import cv2
import numpy as np
from ultralytics import YOLO

# Initialize the YOLOv8 model with the custom weights
model = YOLO('/home/anem/anaconda3/envs/yolov8_custom/runs/detect/MAIN_train5/weights/best.pt')

# Define the distance finder function
def distance_finder(focal_length_pixels, object_height_pixels, objects_height_cm):
    distance = (objects_height_cm * focal_length_pixels) / object_height_pixels
    return distance

# Define the camera parameters
active_imager_width_mm = 4.51
active_imager_height_mm = 2.88
active_pixels_width = 384
active_pixels_height = 240
default_lens_focal_length_mm = 2.1

# Calculate the focal length in pixels
focal_length_pixels_width = (default_lens_focal_length_mm * active_pixels_width) / active_imager_width_mm
focal_length_pixels_height = (default_lens_focal_length_mm * active_pixels_height) / active_imager_height_mm

# Define the objects to detect and their heights in cm
objects_height_cm = {
    "stop": 8,
    "right": 7,
    "left": 7,
    "thirty": 6.5,
    "sixty": 6.5
    
}

# Open the video feed
video_stream_url = 'tcp://192.168.1.101:5000'
cap = cv2.VideoCapture(video_stream_url)

if not cap.isOpened():
    print("Error: Failed to open video feed.")
    exit()

while True:
    # Read a frame from the video feed
    ret, frame = cap.read()

    if not ret:
        print("Error: Failed to read frame from video feed.")
        break

    # Perform object detection on the frame
    results = model.predict(frame)

    # Draw the detection results on the frame
    for box in results[0].boxes:
        x1, y1, x2, y2 = map(int, box.xyxy[0])
        class_id = model.names[int(box.cls[0])]
        conf = round(box.conf[0].item(), 2)
        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.putText(frame, f"{class_id} {conf}", (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

        # Calculate the distance to the camera for the detected object
        object_height_pixels = y2 - y1
        distance = distance_finder(focal_length_pixels_height, object_height_pixels, objects_height_cm[class_id])
        cv2.putText(frame, f"Distance: {int(distance)} CM", (x1, y2 + 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    # Display the frame
    cv2.imshow("Object Detection", frame)

    # Exit if the user presses the 'q' key
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release the video feed and destroy all windows
cap.release()
cv2.destroyAllWindows()
