#!/usr/bin/env python3
"""
Task 4: The Silent Image - Steganography Challenge
Retrieves hidden message from PNG image using LSB steganography
"""

import paho.mqtt.client as mqtt
import json
import base64
import time
from PIL import Image
import io

# MQTT Configuration
BROKER = "broker.mqttdashboard.com"
PORT = 1883
REQUEST_TOPIC = "kelpsaute/steganography"
CLIENT_ID = "Task4_Stego_Solver_001"

# Configuration - UPDATE THESE
HIDDEN_MESSAGE = "REEFING KRILLS :( CORALS BLOOM <3"  # From Task 3
AGENT_ID = "shivaprasadvshivaprasad07"  # Your team ID
CHALLENGE_CODE = "edrft_window"  # From Task 2

# State variables
image_received = True
image_data = None
running = True

# ANSI Colors
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    END = '\033[0m'

def print_header():
    """Print startup header"""
    print(f"\n{Colors.CYAN}{Colors.BOLD}")
    print("╔════════════════════════════════════════════════════╗")
    print("║        TASK 4: THE SILENT IMAGE 🔐                ║")
    print("║      Steganography Challenge Solver                ║")
    print("╚════════════════════════════════════════════════════╝")
    print(f"{Colors.END}")
    print(f"\n{Colors.YELLOW}📡 Configuration:{Colors.END}")
    print(f"   Broker: {BROKER}:{PORT}")
    print(f"   Request Topic: {REQUEST_TOPIC}")
    print(f"   Challenge Topic: {CHALLENGE_CODE}")
    print(f"   Hidden Message: {HIDDEN_MESSAGE}")
    print(f"   Agent ID: {AGENT_ID}")
    print(f"\n{Colors.CYAN}{'─' * 52}{Colors.END}\n")

def extract_lsb_message(image):
    """
    Extract hidden message using LSB (Least Significant Bit) steganography.
    Extracts LSB from R, G, B channels sequentially.
    """
    print(f"\n{Colors.YELLOW}Phase 3: Extracting Hidden Message...{Colors.END}")
    print(f"{Colors.CYAN}Image size: {image.width}x{image.height}{Colors.END}")
    print(f"{Colors.CYAN}Total pixels: {image.width * image.height}{Colors.END}\n")
    
    pixels = image.load()
    binary_message = ""
    
    # Extract LSB from each color channel
    print(f"{Colors.BLUE}[DEBUG] Extracting LSB from each RGB component...{Colors.END}")
    
    for y in range(image.height):
        for x in range(image.width):
            pixel = pixels[x, y]
            
            if isinstance(pixel, tuple) and len(pixel) >= 3:
                r, g, b = pixel[0], pixel[1], pixel[2]
                
                # Extract LSB from each channel
                binary_message += str(r & 1)
                binary_message += str(g & 1)
                binary_message += str(b & 1)
    
    print(f"{Colors.GREEN}✓ Extracted {len(binary_message)} bits{Colors.END}")
    print(f"{Colors.BLUE}[DEBUG] First 128 bits: {binary_message[:128]}{Colors.END}")
    
    # Convert binary to ASCII
    message = ""
    print(f"\n{Colors.YELLOW}Converting binary to ASCII...{Colors.END}")
    
    chars_found = []
    for i in range(0, len(binary_message), 8):
        byte = binary_message[i:i+8]
        if len(byte) == 8:
            char_code = int(byte, 2)
            
            # Debug: show first few bytes
            if i < 200:
                print(f"  Byte {i//8}: {byte} = {char_code} ({chr(char_code) if 32 <= char_code <= 126 else '?'})")
            
            # Stop at null terminator
            if char_code == 0:
                print(f"{Colors.BLUE}[DEBUG] Found null terminator at byte {i//8}{Colors.END}")
                break
            
            # Only add printable ASCII characters
            if 32 <= char_code <= 126 or char_code in [10, 13]:
                char = chr(char_code)
                message += char
                chars_found.append(f"{char_code}:{char}")
            elif char_code < 32 and len(message) > 10:
                # Stop at control characters after we've found some message
                print(f"{Colors.BLUE}[DEBUG] Found control character {char_code} at byte {i//8}, stopping{Colors.END}")
                break
    
    print(f"\n{Colors.GREEN}✓ Extracted message length: {len(message)} characters{Colors.END}")
    print(f"{Colors.BLUE}[DEBUG] Characters found: {chars_found[:20]}{Colors.END}\n")
    
    return message.strip()

def extract_lsb_reverse(image):
    """
    Try extracting LSB in reverse order (from bottom-right to top-left)
    """
    print(f"\n{Colors.YELLOW}Trying reverse extraction...{Colors.END}")
    
    pixels = image.load()
    binary_message = ""
    
    for y in range(image.height - 1, -1, -1):
        for x in range(image.width - 1, -1, -1):
            pixel = pixels[x, y]
            
            if isinstance(pixel, tuple) and len(pixel) >= 3:
                r, g, b = pixel[0], pixel[1], pixel[2]
                binary_message += str(r & 1)
                binary_message += str(g & 1)
                binary_message += str(b & 1)
    
    message = ""
    for i in range(0, len(binary_message), 8):
        byte = binary_message[i:i+8]
        if len(byte) == 8:
            char_code = int(byte, 2)
            if char_code == 0:
                break
            if 32 <= char_code <= 126 or char_code in [10, 13]:
                message += chr(char_code)
            elif char_code < 32 and len(message) > 10:
                break
    
    return message.strip()

def extract_msb_message(image):
    """
    Try extracting MSB (Most Significant Bit) instead of LSB
    """
    print(f"\n{Colors.YELLOW}Trying MSB extraction...{Colors.END}")
    
    pixels = image.load()
    binary_message = ""
    
    for y in range(image.height):
        for x in range(image.width):
            pixel = pixels[x, y]
            
            if isinstance(pixel, tuple) and len(pixel) >= 3:
                r, g, b = pixel[0], pixel[1], pixel[2]
                
                # Extract MSB (bit 7) from each channel
                binary_message += str((r >> 7) & 1)
                binary_message += str((g >> 7) & 1)
                binary_message += str((b >> 7) & 1)
    
    message = ""
    for i in range(0, len(binary_message), 8):
        byte = binary_message[i:i+8]
        if len(byte) == 8:
            char_code = int(byte, 2)
            if char_code == 0:
                break
            if 32 <= char_code <= 126 or char_code in [10, 13]:
                message += chr(char_code)
            elif char_code < 32 and len(message) > 10:
                break
    
    return message.strip()

def analyze_pixel_relationships(image):
    """
    Alternative method: Analyze relationships between RGB components.
    """
    print(f"\n{Colors.YELLOW}Alternative Analysis: Pixel Relationships...{Colors.END}")
    
    pixels = image.load()
    patterns = []
    binary_msg = ""
    
    for y in range(image.height):
        for x in range(image.width):
            pixel = pixels[x, y]
            
            if isinstance(pixel, tuple) and len(pixel) >= 3:
                r, g, b = pixel[0], pixel[1], pixel[2]
                
                # Method 1: Compare R vs G (simple binary)
                if r > g:
                    binary_msg += '1'
                else:
                    binary_msg += '0'
                
                # Method 2: Compare all three
                if r > g and r > b:
                    patterns.append('R')
                elif g > r and g > b:
                    patterns.append('G')
                elif b > r and b > g:
                    patterns.append('B')
                else:
                    patterns.append('E')  # Equal
    
    print(f"{Colors.BLUE}[DEBUG] R>G binary: {binary_msg[:64]}{Colors.END}")
    print(f"{Colors.BLUE}[DEBUG] RGB patterns: {''.join(patterns[:32])}{Colors.END}")
    
    # Try to decode R>G method
    message = ""
    for i in range(0, len(binary_msg), 8):
        byte = binary_msg[i:i+8]
        if len(byte) == 8:
            char_code = int(byte, 2)
            if char_code == 0:
                break
            if 32 <= char_code <= 126:
                message += chr(char_code)
            elif len(message) > 10:
                break
    
    if message:
        print(f"{Colors.GREEN}Found message with R>G method: {message}{Colors.END}\n")
        return message
    
    return ''.join(patterns)

def decode_base64_if_needed(message):
    """Check if message is base64 encoded and decode if so"""
    try:
        # Check if it looks like base64
        if len(message) > 10 and all(c in 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=' for c in message.strip()):
            decoded = base64.b64decode(message).decode('utf-8')
            print(f"{Colors.GREEN}✓ Message was base64 encoded{Colors.END}")
            return decoded
    except Exception as e:
        print(f"{Colors.YELLOW}Base64 decode failed: {e}{Colors.END}")
    return message

def on_connect(client, userdata, flags, rc):
    """Callback when connected to MQTT broker"""
    if rc == 0:
        print(f"{Colors.GREEN}✓ Connected to MQTT Broker!{Colors.END}")
        
        # Subscribe to challenge code topic for image response
        client.subscribe(CHALLENGE_CODE)
        print(f"{Colors.GREEN}✓ Subscribed to: {CHALLENGE_CODE}{Colors.END}\n")
        
        # Phase 1: Send request
        print(f"{Colors.YELLOW}Phase 1: Signaling the Reef...{Colors.END}")
        request_payload = {
            "request": HIDDEN_MESSAGE,
            "agent_id": AGENT_ID
        }
        
        print(f"{Colors.CYAN}Sending request:{Colors.END}")
        print(f"  Topic: {REQUEST_TOPIC}")
        print(f"  Payload: {json.dumps(request_payload, indent=2)}")
        
        result = client.publish(REQUEST_TOPIC, json.dumps(request_payload))
        
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"{Colors.GREEN}✓ Request sent successfully!{Colors.END}")
            print(f"{Colors.YELLOW}Waiting for reef response...{Colors.END}\n")
        else:
            print(f"{Colors.RED}✗ Failed to send request{Colors.END}")
    else:
        print(f"{Colors.RED}✗ Connection failed with code {rc}{Colors.END}")

def on_message(client, userdata, msg):
    """Callback when message received from MQTT broker"""
    global image_received, image_data, running
    
    try:
        print(f"\n{Colors.GREEN}{'═' * 52}{Colors.END}")
        print(f"{Colors.GREEN}Message received on topic: {msg.topic}{Colors.END}")
        print(f"{Colors.GREEN}{'═' * 52}{Colors.END}\n")
        
        # Parse the response
        payload = json.loads(msg.payload.decode())
        
        print(f"{Colors.CYAN}Payload keys: {list(payload.keys())}{Colors.END}")
        
        # Check if this is the image response
        if "data" in payload and "type" in payload:
            print(f"\n{Colors.GREEN}✓ Image payload received!{Colors.END}")
            print(f"  Type: {payload.get('type')}")
            print(f"  Width: {payload.get('width')}")
            print(f"  Height: {payload.get('height')}")
            print(f"  Data length: {len(payload['data'])} characters\n")
            
            # Phase 2: Restore the image
            print(f"{Colors.YELLOW}Phase 2: Restoring Image...{Colors.END}")
            
            # Decode base64 image data
            image_bytes = base64.b64decode(payload['data'])
            print(f"{Colors.GREEN}✓ Decoded base64 data: {len(image_bytes)} bytes{Colors.END}")
            
            # Create PIL Image
            image = Image.open(io.BytesIO(image_bytes))
            print(f"{Colors.GREEN}✓ Image loaded successfully{Colors.END}")
            print(f"  Format: {image.format}")
            print(f"  Size: {image.size}")
            print(f"  Mode: {image.mode}")
            
            # Save the reconstructed image
            image.save("reconstructed_image.png")
            print(f"{Colors.GREEN}✓ Image saved as: reconstructed_image.png{Colors.END}")
            
            # Verify structural integrity
            if image.size == (payload.get('width'), payload.get('height')):
                print(f"{Colors.GREEN}✓ Structural integrity verified!{Colors.END}\n")
            else:
                print(f"{Colors.RED}✗ Size mismatch!{Colors.END}\n")
            
            # Phase 3: Try multiple extraction methods
            hidden_message = None
            
            # Method 1: Standard LSB
            print(f"{Colors.MAGENTA}[Method 1] Standard LSB Extraction{Colors.END}")
            msg1 = extract_lsb_message(image)
            if msg1 and len(msg1) > 5:
                hidden_message = msg1
                print(f"{Colors.GREEN}✓ Found message with standard LSB{Colors.END}")
            
            # Method 2: Reverse order
            if not hidden_message:
                print(f"\n{Colors.MAGENTA}[Method 2] Reverse Order LSB{Colors.END}")
                msg2 = extract_lsb_reverse(image)
                if msg2 and len(msg2) > 5:
                    hidden_message = msg2
                    print(f"{Colors.GREEN}✓ Found message with reverse LSB{Colors.END}")
            
            # Method 3: MSB instead of LSB
            if not hidden_message:
                print(f"\n{Colors.MAGENTA}[Method 3] MSB Extraction{Colors.END}")
                msg3 = extract_msb_message(image)
                if msg3 and len(msg3) > 5:
                    hidden_message = msg3
                    print(f"{Colors.GREEN}✓ Found message with MSB{Colors.END}")
            
            # Method 4: Pixel relationships
            if not hidden_message:
                print(f"\n{Colors.MAGENTA}[Method 4] Pixel Relationship Analysis{Colors.END}")
                msg4 = analyze_pixel_relationships(image)
                if msg4 and len(msg4) > 5:
                    hidden_message = msg4
                    print(f"{Colors.GREEN}✓ Found message with relationship analysis{Colors.END}")
            
            if hidden_message:
                print(f"\n{Colors.GREEN}{'═' * 52}{Colors.END}")
                print(f"{Colors.GREEN}{Colors.BOLD}HIDDEN MESSAGE FOUND:{Colors.END}")
                print(f"{Colors.GREEN}{'═' * 52}{Colors.END}")
                print(f"{Colors.CYAN}{hidden_message}{Colors.END}")
                print(f"{Colors.GREEN}{'═' * 52}{Colors.END}\n")
                
                # Try decoding if it's base64
                decoded = decode_base64_if_needed(hidden_message)
                if decoded != hidden_message:
                    print(f"{Colors.YELLOW}Decoded message:{Colors.END}")
                    print(f"{Colors.CYAN}{decoded}{Colors.END}\n")
                    hidden_message = decoded
                
                # Phase 4: Follow the direction
                print(f"{Colors.YELLOW}Phase 4: Following the Direction...{Colors.END}")
                print(f"{Colors.CYAN}Interpreting message as next step...{Colors.END}\n")
                
                # Try to identify what type of message it is
                if "http" in hidden_message.lower():
                    print(f"{Colors.GREEN}✓ URL detected in message{Colors.END}")
                    print(f"{Colors.YELLOW}📍 Target URL: {hidden_message}{Colors.END}\n")
                    print(f"{Colors.MAGENTA}⚠️  Do not retrieve yet - wait for Task 5{Colors.END}\n")
                    
                    # Publish to the discovered topic if it looks like one
                    response_topic = hidden_message.strip()
                    
                elif "/" in hidden_message:
                    print(f"{Colors.GREEN}✓ MQTT topic detected{Colors.END}")
                    print(f"{Colors.YELLOW}📍 Next topic: {hidden_message}{Colors.END}\n")
                    
                    # Try publishing to this topic or subscribing
                    response_topic = hidden_message.strip()
                    client.subscribe(response_topic)
                    print(f"{Colors.GREEN}✓ Subscribed to: {response_topic}{Colors.END}")
                    
                    # Try publishing acknowledgment
                    ack_msg = {"status": "discovered", "agent_id": AGENT_ID}
                    client.publish(response_topic, json.dumps(ack_msg))
                    print(f"{Colors.GREEN}✓ Published acknowledgment to: {response_topic}{Colors.END}")
                    print(f"{Colors.YELLOW}Waiting for reef acknowledgment...{Colors.END}\n")
                else:
                    print(f"{Colors.YELLOW}Message type: Instruction or encoded data{Colors.END}")
                    print(f"{Colors.CYAN}Raw message: {hidden_message}{Colors.END}\n")
                
            else:
                print(f"{Colors.RED}✗ No hidden message found with any method{Colors.END}")
                print(f"{Colors.YELLOW}Image may use a different steganography technique{Colors.END}\n")
            
            image_received = True
            
        elif "target_image_url" in payload:
            # This is the acknowledgment with next URL
            print(f"\n{Colors.GREEN}{'═' * 52}{Colors.END}")
            print(f"{Colors.GREEN}{Colors.BOLD}REEF ACKNOWLEDGMENT RECEIVED!{Colors.END}")
            print(f"{Colors.GREEN}{'═' * 52}{Colors.END}")
            print(f"{Colors.YELLOW}Target Image URL:{Colors.END}")
            print(f"{Colors.CYAN}{payload['target_image_url']}{Colors.END}")
            print(f"{Colors.GREEN}{'═' * 52}{Colors.END}\n")
            print(f"{Colors.MAGENTA}⚠️  Save this URL for Task 5{Colors.END}\n")
            
        else:
            # Unknown payload format
            print(f"{Colors.CYAN}Full payload:{Colors.END}")
            print(json.dumps(payload, indent=2))
            print()
        
    except json.JSONDecodeError:
        print(f"{Colors.YELLOW}Non-JSON message received:{Colors.END}")
        print(msg.payload.decode())
        print()
    except Exception as e:
        print(f"{Colors.RED}✗ Error processing message: {e}{Colors.END}")
        import traceback
        traceback.print_exc()

def main():
    """Main function"""
    global running
    
    print_header()
    
    if AGENT_ID == "YOUR_TEAM_ID" or CHALLENGE_CODE == "YOUR_CHALLENGE_CODE":
        print(f"{Colors.RED}✗ ERROR: Please update AGENT_ID and CHALLENGE_CODE in the script!{Colors.END}\n")
        return
    
    # Create MQTT client
    try:
        client = mqtt.Client(client_id=CLIENT_ID, callback_api_version=mqtt.CallbackAPIVersion.VERSION1)
    except AttributeError:
        client = mqtt.Client(CLIENT_ID)
    
    client.on_connect = on_connect
    client.on_message = on_message
    
    try:
        # Connect to broker
        print(f"{Colors.YELLOW}Connecting to MQTT broker...{Colors.END}")
        client.connect(BROKER, PORT, 60)
        
        # Start MQTT loop in background
        client.loop_start()
        
        # Keep running until we receive the image
        print(f"{Colors.MAGENTA}Press Ctrl+C to exit{Colors.END}\n")
        
        while running:
            time.sleep(1)
            
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Interrupted by user{Colors.END}")
    except Exception as e:
        print(f"{Colors.RED}Error: {e}{Colors.END}")
    finally:
        running = False
        print(f"\n{Colors.YELLOW}Disconnecting from MQTT broker...{Colors.END}")
        client.loop_stop()
        client.disconnect()
        print(f"{Colors.GREEN}✓ Disconnected. Goodbye!{Colors.END}\n")

if __name__ == "__main__":
    main()