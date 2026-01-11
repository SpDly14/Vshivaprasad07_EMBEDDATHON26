#!/usr/bin/env python3

import threading
import json
import base64
import io
import numpy as np
import os
from PIL import Image, ImageFilter, ImageEnhance
from skimage.metrics import structural_similarity as ssim
from scipy.ndimage import gaussian_filter
from scipy.optimize import linear_sum_assignment
from joblib import Parallel, delayed
import paho.mqtt.client as mqtt

# ================= CONFIG =================
BROKER = "broker.mqttdashboard.com"
PORT = 1883
SOURCE_TOPIC = "coralcrib/img"

TEAM_ID_REEF_ID = "shivaprasadvshivaprasad07_manojkumar10b35vshivaprasad07"
CLIENT_ID = "Task5_Pixel_Sculptor_Final"

TARGET_IMAGE_PATH = "target_image.jpg"
IMG_SIZE = (128, 64)
BLOCK = 8
# ==========================================

source_image = None
target_image = None
target_ready = threading.Event()

# =============== PHASE 1 ==================
def load_target_image_nonblocking():
    global target_image
    try:
        if not os.path.exists(TARGET_IMAGE_PATH):
            raise FileNotFoundError(TARGET_IMAGE_PATH)

        img = Image.open(TARGET_IMAGE_PATH).convert("RGB")
        img = img.resize(IMG_SIZE, Image.Resampling.LANCZOS)
        target_image = img
        target_ready.set()

        print("[✓] Target image loaded from disk")

    except Exception as e:
        print("[!] Target image load failed:", e)

threading.Thread(target=load_target_image_nonblocking, daemon=True).start()

# =============== ADVANCED TECHNIQUES ==================

def histogram_matching(source, target):
    """Match histogram of source to target for better color distribution"""
    src_arr = np.array(source)
    tgt_arr = np.array(target)
    
    matched = np.zeros_like(src_arr)
    
    for channel in range(3):
        src_channel = src_arr[:, :, channel].flatten()
        tgt_channel = tgt_arr[:, :, channel].flatten()
        
        src_values, src_counts = np.unique(src_channel, return_counts=True)
        tgt_values, tgt_counts = np.unique(tgt_channel, return_counts=True)
        
        src_quantiles = np.cumsum(src_counts).astype(float) / src_channel.size
        tgt_quantiles = np.cumsum(tgt_counts).astype(float) / tgt_channel.size
        
        interp_values = np.interp(src_quantiles, tgt_quantiles, tgt_values)
        
        matched_channel = np.interp(src_channel, src_values, interp_values)
        matched[:, :, channel] = matched_channel.reshape(src_arr.shape[:2])
    
    return Image.fromarray(matched.astype(np.uint8))

def compute_feature_map(img_arr):
    """Compute gradient-based features for structure preservation"""
    gray = np.mean(img_arr, axis=2)
    
    # Gradient features
    gy, gx = np.gradient(gray)
    gradient_mag = np.sqrt(gx**2 + gy**2)
    
    return gradient_mag

def process_block_advanced(s_blk, t_blk, s_feat, t_feat):
    """Advanced block processing with Hungarian algorithm and feature matching"""
    sh, sw, _ = s_blk.shape
    
    # Build cost matrix
    cost_matrix = np.zeros((sh * sw, sh * sw))
    
    for i in range(sh * sw):
        ty, tx = i // sw, i % sw
        
        for j in range(sh * sw):
            sy, sx = j // sw, j % sw
            
            # Color distance in LAB space for perceptual accuracy
            color_cost = np.linalg.norm(
                s_blk[sy, sx].astype(np.float32) - 
                t_blk[ty, tx].astype(np.float32)
            )
            
            # Spatial distance
            spatial_cost = np.sqrt((sy - ty)**2 + (sx - tx)**2)
            
            # Feature similarity (gradient matching)
            feature_cost = abs(s_feat[sy, sx] - t_feat[ty, tx])
            
            # Combined cost with weights
            cost_matrix[i, j] = color_cost + 3.0 * spatial_cost + 1.5 * feature_cost
    
    # Hungarian algorithm for optimal assignment
    row_ind, col_ind = linear_sum_assignment(cost_matrix)
    
    # Build output block
    out = np.zeros_like(s_blk)
    for i, j in zip(row_ind, col_ind):
        ty, tx = i // sw, i % sw
        sy, sx = j // sw, j % sw
        out[ty, tx] = s_blk[sy, sx]
    
    return out

def advanced_optimal_transport(source, target):
    """Enhanced optimal transport with multiple refinements"""
    src = np.array(source)
    tgt = np.array(target)
    H, W, _ = src.shape
    
    # Compute feature maps
    src_feat = compute_feature_map(src)
    tgt_feat = compute_feature_map(tgt)
    
    out = np.zeros_like(src)
    jobs, coords = [], []
    
    for y in range(0, H, BLOCK):
        for x in range(0, W, BLOCK):
            jobs.append((
                src[y:y+BLOCK, x:x+BLOCK],
                tgt[y:y+BLOCK, x:x+BLOCK],
                src_feat[y:y+BLOCK, x:x+BLOCK],
                tgt_feat[y:y+BLOCK, x:x+BLOCK]
            ))
            coords.append((y, x))
    
    results = Parallel(n_jobs=-1)(
        delayed(process_block_advanced)(s, t, sf, tf) for s, t, sf, tf in jobs
    )
    
    for (y, x), blk in zip(coords, results):
        out[y:y+BLOCK, x:x+BLOCK] = blk
    
    return Image.fromarray(out.astype(np.uint8))

def edge_preserving_smooth(img, sigma=0.5):
    """Apply gentle smoothing while preserving edges"""
    arr = np.array(img).astype(np.float32)
    smoothed = np.zeros_like(arr)
    
    for c in range(3):
        smoothed[:, :, c] = gaussian_filter(arr[:, :, c], sigma=sigma)
    
    return Image.fromarray(smoothed.astype(np.uint8))

def local_contrast_enhancement(img, factor=1.1):
    """Enhance local contrast to improve structure visibility"""
    enhancer = ImageEnhance.Contrast(img)
    return enhancer.enhance(factor)

def multi_scale_transform(source, target):
    """Apply transformation at multiple scales and blend"""
    # Scale 1: Full resolution
    result_full = advanced_optimal_transport(source, target)
    
    # Scale 2: Half resolution
    src_half = source.resize((IMG_SIZE[0]//2, IMG_SIZE[1]//2), Image.Resampling.LANCZOS)
    tgt_half = target.resize((IMG_SIZE[0]//2, IMG_SIZE[1]//2), Image.Resampling.LANCZOS)
    result_half = advanced_optimal_transport(src_half, tgt_half)
    result_half = result_half.resize(IMG_SIZE, Image.Resampling.LANCZOS)
    
    # Blend scales
    arr_full = np.array(result_full).astype(np.float32)
    arr_half = np.array(result_half).astype(np.float32)
    blended = (0.7 * arr_full + 0.3 * arr_half).astype(np.uint8)
    
    return Image.fromarray(blended)

# =============== PHASE 5 ==================
def compute_ssim(img1, img2):
    g1 = np.array(img1.convert("L"))
    g2 = np.array(img2.convert("L"))
    return ssim(g1, g2, data_range=255)

# =============== PHASE 6 ==================
def publish_image(client, img):
    buf = io.BytesIO()
    img.save(buf, format="PNG")
    encoded = base64.b64encode(buf.getvalue()).decode()

    payload = json.dumps({
        "transformed_image": encoded
    })

    client.publish(TEAM_ID_REEF_ID, payload)
    print("[✓] Transformed image published")

# =============== MQTT =====================
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[✓] MQTT connected")
        client.subscribe(SOURCE_TOPIC)
    else:
        print("[!] MQTT connection failed")

def on_message(client, userdata, msg):
    global source_image

    try:
        data = json.loads(msg.payload.decode())
        img_bytes = base64.b64decode(data["data"])
    except:
        img_bytes = msg.payload

    source_image = Image.open(io.BytesIO(img_bytes)).convert("RGB")
    source_image = source_image.resize(IMG_SIZE, Image.Resampling.LANCZOS)

    print("[✓] Source image received")

    if target_ready.is_set():
        run_pipeline(client)
    else:
        print("[!] Waiting for target image")

def run_pipeline(client):
    print("[*] Running advanced transformation pipeline")

    # Step 1: Histogram matching for color distribution
    print("[*] Step 1: Histogram matching")
    matched = histogram_matching(source_image, target_image)
    
    # Step 2: Multi-scale optimal transport
    print("[*] Step 2: Multi-scale optimal transport")
    transformed = multi_scale_transform(matched, target_image)
    
    # Step 3: Edge-preserving smoothing
    print("[*] Step 3: Edge-preserving smoothing")
    smoothed = edge_preserving_smooth(transformed, sigma=0.4)
    
    # Step 4: Local contrast enhancement
    print("[*] Step 4: Contrast enhancement")
    final = local_contrast_enhancement(smoothed, factor=1.05)
    
    # Compute SSIM
    score = compute_ssim(final, target_image)
    print(f"[SSIM] {score:.4f}")

    if score >= 0.70:
        publish_image(client, final)
    else:
        print(f"[!] SSIM below threshold: {score:.4f}")
        # Publish anyway for debugging
        publish_image(client, final)

# =============== MAIN ======================
def main():
    client = mqtt.Client(
        client_id=CLIENT_ID,
        callback_api_version=mqtt.CallbackAPIVersion.VERSION1
    )

    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(BROKER, PORT, 60)
    client.loop_forever()

if __name__ == "__main__":
    main()
