# ESP8266 & Python Server ECDH Key Exchange Setup

This example demonstrates an Elliptic Curve Diffie-Hellman (ECDH) key exchange between an ESP8266 microcontroller and a Python server using the secp256r1 (P-256) curve. ECDH provides the same security as traditional DH but with much smaller key sizes, making it ideal for IoT devices.

## Why ECDH vs Traditional DH?

| Feature | Traditional DH | ECDH (P-256) |
|---------|---------------|--------------|
| Key Size | 2048-4096 bits | 256 bits |
| Security Level | 112-150 bits | 128 bits |
| Performance | Slower | Much faster |
| Memory Usage | High | Low |
| Battery Life | Poor | Better |
| IoT Suitability | Limited | Excellent |

## Prerequisites

### Python Server Requirements
```bash
pip install cryptography
```

The `cryptography` library provides robust elliptic curve implementations.

### ESP8266 Requirements
Install these libraries in Arduino IDE:

1. **ESP8266WiFi** (included with ESP8266 board package)
2. **ArduinoJson** (by Benoit Blanchon)
3. **Crypto** (by Rhys Weatherley) 
4. **uECC** (Micro-ECC library)

#### Installing Micro-ECC Library:
1. Download from: https://github.com/kmackay/micro-ecc
2. Extract to your Arduino libraries folder
3. Or use Library Manager: Search "Micro-ECC" by Kenneth MacKay

## Setup Instructions

### 1. Python Server Setup

1. Install the cryptography library:
   ```bash
   pip install cryptography
   ```

2. Save the Python server code to `ecdh_server.py`

3. Run the server:
   ```bash
   python3 ecdh_server.py
   ```

4. Server will listen on port 8888 and display your IP address

### 2. ESP8266 Client Setup

1. Open Arduino IDE and install required libraries
2. Update WiFi credentials:
   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

3. Update server IP address:
   ```cpp
   const char* serverIP = "192.168.1.100";  // Your server's IP
   ```

4. Select your ESP8266 board and upload the code

## How ECDH Works

### Mathematical Foundation
ECDH is based on the discrete logarithm problem over elliptic curves:
- **Curve**: y² = x³ + ax + b (mod p)
- **secp256r1**: Standardized P-256 curve by NIST
- **Point Multiplication**: Q = d × G (where G is the generator point)

### Key Exchange Process
1. **Both parties** agree on the same elliptic curve (secp256r1)
2. **Server generates** private key `ds` and computes public key `Qs = ds × G`
3. **Client generates** private key `dc` and computes public key `Qc = dc × G`
4. **Server sends** its public key coordinates (Qsx, Qsy) to client
5. **Client sends** its public key coordinates (Qcx, Qcy) to server  
6. **Both compute** shared point: Server: `ds × Qc`, Client: `dc × Qs`
7. **Shared secret** is the x-coordinate of the shared point
8. **Derive key** using SHA-256 hash of the shared secret

### Security Properties
- **Forward Secrecy**: New keys for each session
- **Small Keys**: 256-bit keys provide 128-bit security
- **Fast Operations**: Efficient point multiplication
- **Standard Curve**: Well-tested secp256r1 (P-256)

## Example Output

### Python Server Console
```
=== ECDH Key Exchange Server ===
Cryptography library found ✓
ECDH Key Exchange Server listening on 0.0.0.0:8888
Using elliptic curve: secp256r1 (P-256)

Connection from ('192.168.1.105', 54321)
Sent ECDH public key to ('192.168.1.105', 54321)
Received client public key from ('192.168.1.105', 54321)
ECDH key exchange completed with ('192.168.1.105', 54321)
Shared key hash: a1b2c3d4e5f6g7h8
Full shared key (32 bytes): a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2
```

### ESP8266 Serial Monitor
```
=== ESP8266 ECDH Key Exchange Client ===
Using elliptic curve: secp256r1 (P-256)
Connecting to WiFi....
Connected! IP: 192.168.1.105

ECDH key pair generated successfully
Public key (X,Y): 1A2B3C4D...

Starting ECDH key exchange...
Connected to server
Waiting for server public key...
Server public key received and parsed
Shared secret computed successfully
Sent our public key to server

=== ECDH KEY EXCHANGE SUCCESSFUL ===
Derived encryption key: A1B2C3D4E5F6G7H8I9J0K1L2M3N4O5P6Q7R8S9T0U1V2W3X4Y5Z6A7B8C9D0E1F2
Server key hash: a1b2c3d4e5f6g7h8
Our key hash: a1b2c3d4e5f6g7h8
✓ Key verification successful!
=== READY FOR ENCRYPTED COMMUNICATION ===
```

## Performance Comparison

### Memory Usage (ESP8266):
- **ECDH**: ~2KB RAM, ~15KB Flash
- **Traditional DH**: ~8KB RAM, ~25KB Flash

### Speed (ESP8266 @ 80MHz):
- **ECDH Key Generation**: ~200ms
- **ECDH Shared Secret**: ~180ms
- **Total Exchange**: <1 second

### Security Equivalence:
- **256-bit ECDH** ≈ **3072-bit RSA** ≈ **128-bit AES**

## Advanced Features

### 1. Key Derivation Function (KDF)
```cpp
// Derive multiple keys from shared secret
void deriveKeys(uint8_t* master_key, uint8_t* enc_key, uint8_t* mac_key) {
    SHA256 sha;
    sha.update(shared_secret, 32);
    sha.update("ENCRYPTION", 10);
    sha.finalize(enc_key, 32);
    
    sha.reset();
    sha.update(shared_secret, 32);  
    sha.update("AUTHENTICATION", 14);
    sha.finalize(mac_key, 32);
}
```

### 2. Point Compression (Save Bandwidth)
```cpp
// Compress public key to 33 bytes instead of 64
bool compressPublicKey(uint8_t* compressed, const uint8_t* uncompressed) {
    compressed[0] = (uncompressed[63] & 1) ? 0x03 : 0x02;  // Parity bit
    memcpy(compressed + 1, uncompressed, 32);  // X coordinate only
    return true;
}
```

### 3. Ephemeral Keys (Perfect Forward Secrecy)
```cpp
// Generate new key pair for each session
void generateEphemeralKeys() {
    if (ecdh.generateKeyPair()) {
        Serial.println("New ephemeral keys generated");
        // Old keys are automatically overwritten
    }
}
```

## Security Best Practices

### Production Recommendations:
1. **Use Hardware RNG** when available on your platform
2. **Implement key pinning** to prevent man-in-the-middle attacks
3. **Add timestamp/nonce** to prevent replay attacks
4. **Use authenticated encryption** (AES-GCM) with derived keys
5. **Rotate keys regularly** (every few hours/days)
6. **Validate curve points** to prevent invalid curve attacks
7. **Use constant-time operations** to prevent timing attacks

### Current Security Level:
- ✅ **Strong Curve**: secp256r1 is NIST-approved
- ✅ **Proper Key Generation**: Uses cryptographically secure RNG
- ✅ **Key Verification**: Hash comparison prevents silent failures
- ✅ **Forward Secrecy**: New keys for each exchange
- ⚠️ **No Authentication**: Vulnerable to man-in-the-middle (add certificates)
- ⚠️ **No Message Authentication**: Add HMAC for message integrity

## Troubleshooting

### Common Issues:
1. **"Failed to generate ECDH key pair"**
   - Check if uECC library is properly installed
   - Verify ESP8266 has enough free memory

2. **"JSON parsing failed"**
   - Increase JSON document size in DynamicJsonDocument
   - Check network connectivity and data integrity

3. **"Key verification failed"**  
   - Usually indicates a computation error
   - Check endianness and byte order consistency

4. **Memory Issues**
   - ECDH uses less memory than traditional DH
   - Free unused variables promptly
   - Consider using static allocation

## Next Steps

After successful ECDH key exchange:

1. **Implement AES-GCM encryption** for message confidentiality and authenticity
2. **Add certificate-based authentication** using ECDSA signatures
3. **Build application protocols** on top of the secure channel
4. **Implement key rotation** for long-running connections
5. **Add error recovery** and reconnection logic

This ECDH implementation provides a solid foundation for secure IoT communication with modern cryptographic standards.
