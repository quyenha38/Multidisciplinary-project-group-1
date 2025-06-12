import tensorflow as tf
from tensorflow.keras.applications import MobileNet
from tensorflow.keras.layers import Dense, GlobalAveragePooling2D
from tensorflow.keras.models import Model
from tensorflow.keras.optimizers import Adam
# Imports & configuration
import os
from pathlib import Path
from typing import Tuple, List

import cv2
import numpy as np
from tqdm import tqdm

from skimage.restoration import denoise_tv_chambolle   # Total‑variation
from skimage.filters.rank import median as sk_median     # Adaptive median
from skimage.morphology import disk

# Paths
INPUT_ROOT  = Path("./Original Dataset")
OUTPUT_ROOT = Path("./Processed dataset")

# Resize target
RESIZE_WH = (256, 256)  # (w, h)

# Hyper‑parameters
CLAHE_PARAMS = {"clipLimit": 2.0, "tileGridSize": (8, 8)}
GAMMA_PARAMS = {"gamma": 1.2}  # > 1 brightens, < 1 darkens

AMF_PARAMS = {"selem_size": 3}         # Adaptive Median
MF_PARAMS  = {"ksize": 3}              # Median blur
TVF_PARAMS = {"weight": 0.1, "eps": 1e-3}  # Total Variation
GDF_PARAMS = {"ksize": (3, 3), "sigma": 0} # Gaussian blur

# Decide which filters to run
FILTER_KEYS: List[str] = ["amf", "mf", "tvf", "gdf"]
# Download latest version



def ensure_dir(path: Path):
    path.mkdir(parents=True, exist_ok=True)

# Background removal

def remove_background(img_bgr: np.ndarray, margin: float = 0.05, iterations: int = 5) -> np.ndarray:
    """GrabCut with rectangular init; non‑leaf pixels painted white."""
    h, w = img_bgr.shape[:2]
    rect = (
        int(margin * w),
        int(margin * h),
        int((1 - 2 * margin) * w),
        int((1 - 2 * margin) * h),
    )
    mask = np.zeros((h, w), np.uint8)
    bgdModel = np.zeros((1, 65), np.float64)
    fgdModel = np.zeros((1, 65), np.float64)
    cv2.grabCut(img_bgr, mask, rect, bgdModel, fgdModel, iterations, cv2.GC_INIT_WITH_RECT)
    mask_fg = np.where((mask == 2) | (mask == 0), 0, 1).astype("uint8")
    white = np.full_like(img_bgr, 255)
    return np.where(mask_fg[:, :, None] == 0, white, img_bgr)

# Filters

def adaptive_median(img_bgr: np.ndarray, selem_size: int = 3) -> np.ndarray:
    footprint = disk(selem_size)
    out_ch = [sk_median(ch, footprint=footprint) for ch in cv2.split(img_bgr)]
    return cv2.merge(out_ch)


def median_filter(img_bgr: np.ndarray, ksize: int = 3) -> np.ndarray:
    return cv2.medianBlur(img_bgr, ksize)


def total_variation(img_bgr: np.ndarray, weight: float = 0.1, eps: float = 1e-3) -> np.ndarray:
    float_img = img_bgr.astype(np.float32) / 255.0
    den = denoise_tv_chambolle(float_img, weight=weight, eps=eps, channel_axis=-1)
    return (den * 255).astype(np.uint8)


def gaussian_denoise(img_bgr: np.ndarray, ksize: Tuple[int, int] = (3, 3), sigma: float = 0):
    return cv2.GaussianBlur(img_bgr, ksize, sigma)

FILTER_FUNCS = {
    "amf": (adaptive_median, AMF_PARAMS),
    "mf":  (median_filter,   MF_PARAMS),
    "tvf": (total_variation, TVF_PARAMS),
    "gdf": (gaussian_denoise, GDF_PARAMS),
}

# Gamma correction

def gamma_correction(img_rgb: np.ndarray, gamma: float = 1.2) -> np.ndarray:
    invG = 1.0 / gamma
    table = np.array([(i / 255.0) ** invG * 255 for i in range(256)]).astype("uint8")
    return cv2.LUT(img_rgb, table)


def process_image(img_path: Path, filter_key: str):
    img_bgr = cv2.imread(str(img_path))
    if img_bgr is None:
        print(f"[Warning] {img_path} unreadable")
        return

    # Resize
    img_bgr = cv2.resize(img_bgr, RESIZE_WH, interpolation=cv2.INTER_AREA)

    # Background removal
    img_bgr = remove_background(img_bgr)

    # BGR → Lab & CLAHE on L
    lab = cv2.cvtColor(img_bgr, cv2.COLOR_BGR2LAB)
    l, a, b = cv2.split(lab)
    l_eq = cv2.createCLAHE(**CLAHE_PARAMS).apply(l)
    img_bgr = cv2.cvtColor(cv2.merge([l_eq, a, b]), cv2.COLOR_LAB2BGR)

    # Noise filter
    func, params = FILTER_FUNCS[filter_key]
    img_bgr = func(img_bgr, **params)

    # Gamma correction in RGB
    img_rgb = gamma_correction(cv2.cvtColor(img_bgr, cv2.COLOR_BGR2RGB), **GAMMA_PARAMS)

    # Save
    out_bgr = cv2.cvtColor(img_rgb, cv2.COLOR_RGB2BGR)
    rel = img_path.relative_to(INPUT_ROOT)
    out_dir = OUTPUT_ROOT / f"no_bg_clahe_{filter_key}" / rel.parent
    ensure_dir(out_dir)
    cv2.imwrite(str(out_dir / rel.name), out_bgr)

def preprocess_data():
    image_ext = {".png", ".jpg", ".jpeg", ".bmp"}
    img_paths = [p for p in INPUT_ROOT.rglob("*") if p.suffix.lower() in image_ext]
    print(f"Found {len(img_paths)} images in {INPUT_ROOT}")

    for flt in FILTER_KEYS:
        print(f"\n▶︎ Running: CLAHE + {flt.upper()} filter")
        for img_path in tqdm(img_paths, desc=f"clahe_{flt}"):
            process_image(img_path, flt)

#preprocess_data()

from tensorflow.keras.preprocessing import image_dataset_from_directory
dataset_dir ='Original Dataset'

batch_size = 32
img_size = (224, 224)


full_dataset = tf.keras.preprocessing.image_dataset_from_directory(
    dataset_dir,
    image_size=img_size,
    batch_size=batch_size,
    label_mode='categorical',
    shuffle=True,
    seed=7
)
batch_size = 1
img_size = (224, 224)
model = tf.keras.models.load_model('my_saved_model.keras')

#loss, accuracy = model.evaluate(full_dataset)
#print(f"Test Accuracy: {accuracy * 100:.2f}%")

import numpy as np
from tensorflow.keras.preprocessing import image
from tensorflow.keras.applications.mobilenet import preprocess_input

img_path = './Original Dataset/Anthracnose/Anthracnose00001.jpg'
img = image.load_img(img_path, target_size=img_size)

img_array = image.img_to_array(img)

# Expand dims to create batch of 1 and preprocess for MobileNet
img_array = np.expand_dims(img_array, axis=0)
img_array = preprocess_input(img_array)


# Make prediction
preds = model.predict(img_array)

# Get predicted class index
pred = tf.argmax(preds[0]).numpy()
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

print(f"Predicted class: {class_names[pred]}")
