/*-------------------------------------------------------------------------
  The MIT License (MIT)
  Copyright © 2025 Avans Hogeschool Lectoraat Smart Energy
  
  Permission is hereby granted, free of charge, to any person obtaining a 
  copy of this software and associated documentation files (the “Software”), 
  to deal in the Software without restriction, including without limitation 
  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the 
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
  THE SOFTWARE.

  -------------------------------------------------------------------------

  This is a inline header file that contains both declarations and implemenation
  of the library and it is part of the DIY SMARTMETER project. It is developed
  for both the ESP8266 (Wemos D1 mini) and ESP32S2 (Lolin S2 mini). Configuration
  of this library will be done in this library in the top of the file itself.

  ...

  V1.0  Initial version
  -------------------------------------------------------------------------*/
#include <Crypto.h> // Crypto library
#include <SHA256.h>
#include <AES.h>
#include <CTR.h>
#include <uECC.h>  // Micro-ECC library for elliptic curve operations
#include <WiFiClient.h>
#include <ArduinoJson.h>

// Server details
const char* serverIP = "51.77.215.199";  // Replace with your server IP
const int serverPort = 8888;

bool hexStringToBytes(String hexString, uint8_t* bytes, int expectedLength) {
    // Remove "0x" prefix if present
    if (hexString.startsWith("0x")) {
        hexString = hexString.substring(2);
    }
    
    if (hexString.length() != expectedLength * 2) {
        Serial.println("Invalid hex string length");
        return false;
    }
    
    for (int i = 0; i < expectedLength; i++) {
        String byteString = hexString.substring(i * 2, i * 2 + 2);
        bytes[i] = (uint8_t)strtol(byteString.c_str(), NULL, 16);
    }
    
    return true;
}

String bytesToHexString(const uint8_t* bytes, int length) {
    String hexString = "0x";
    for (int i = 0; i < length; i++) {
        if (bytes[i] < 16) hexString += "0";
        hexString += String(bytes[i], HEX);
    }
    return hexString;
}

void printHex(const uint8_t* data, int length) {
    for (int i = 0; i < length; i++) {
        if (data[i] < 16) Serial.print("0");
        Serial.print(data[i], HEX);
    }
    Serial.println();
}

String bytesToHex(uint8_t* data, size_t len) {
  String hex = "";
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 16) hex += "0";
    hex += String(data[i], HEX);
  }
  return hex;
}

void hexToBytes(String hex, uint8_t* output) {
  for (size_t i = 0; i < hex.length(); i += 2) {
    output[i/2] = strtol(hex.substring(i, i+2).c_str(), NULL, 16);
  }
}

// Generate HMAC for message authentication
String generateHMAC(String message, uint8_t* key) {
  SHA256 hasher;
  hasher.reset();
  hasher.update(key, 32);
  hasher.update(message.c_str(), message.length());
  
  uint8_t hash[32];
  hasher.finalize(hash, 32);
  
  return bytesToHex(hash, 32);
}

bool verifyHMAC(String message, String receivedHMAC, uint8_t* key) {
  String calculatedHMAC = generateHMAC(message, key);
  return calculatedHMAC.equalsIgnoreCase(receivedHMAC);
}

class ECDHKeyExchange {
private:
    uint8_t private_key[32];
    uint8_t public_key[64];  // Uncompressed format: 32 bytes X + 32 bytes Y
    uint8_t shared_secret[32];
    
public:
    ECDHKeyExchange() {
        // Initialize micro-ECC
        uECC_set_rng(&rng);
        // Initialize random seed
        randomSeed(analogRead(0));      
    }
    
    // Random number generator for uECC
    static int rng(uint8_t *dest, unsigned size) {
        // Use ESP8266's hardware random number generator
        for (unsigned i = 0; i < size; i++) {
            dest[i] = (uint8_t)RANDOM_REG32;
        }
        return 1;
    }
    
    bool generateKeyPair() {
        // Generate ECDH key pair using secp256r1 (P-256)
        const struct uECC_Curve_t * curve = uECC_secp256r1();
        
        if (!uECC_make_key(public_key, private_key, curve)) {
            Serial.println("Failed to generate ECDH key pair");
            return false;
        }
        
        Serial.println("ECDH key pair generated successfully");
        return true;
    }
    
    void getPublicKeyCoordinates(uint8_t* x_coord, uint8_t* y_coord) {
        // Public key is stored as [X coordinate (32 bytes)][Y coordinate (32 bytes)]
        memcpy(x_coord, public_key, 32);
        memcpy(y_coord, public_key + 32, 32);
    }
    
    bool computeSharedSecret(const uint8_t* peer_x, const uint8_t* peer_y) {
        // Reconstruct peer's public key
        uint8_t peer_public[64];
        memcpy(peer_public, peer_x, 32);
        memcpy(peer_public + 32, peer_y, 32);
        
        const struct uECC_Curve_t * curve = uECC_secp256r1();
        
        // Compute shared secret
        if (!uECC_shared_secret(peer_public, private_key, shared_secret, curve)) {
            Serial.println("Failed to compute shared secret");
            return false;
        }
        
        Serial.println("Shared secret computed successfully");
        return true;
    }
    
    void deriveEncryptionKey(uint8_t* derived_key) {
        // Use SHA-256 to derive encryption key from shared secret
        SHA256 sha256;
        sha256.update(shared_secret, 32);
        sha256.finalize(derived_key, 32);
    }
    
    void printPublicKey() {
        Serial.print("Public key (X,Y): ");
        for (int i = 0; i < 64; i++) {
            if (public_key[i] < 16) Serial.print("0");
            Serial.print(public_key[i], HEX);
        }
        Serial.println();
    }
    
    void printSharedSecret() {
        Serial.print("Shared secret: ");
        for (int i = 0; i < 32; i++) {
            if (shared_secret[i] < 16) Serial.print("0");
            Serial.print(shared_secret[i], HEX);
        }
        Serial.println();
    }

    int executeKeyExchange (uint8_t encryption_key[32]) {
        if ( !this->generateKeyPair() ) { // Initialize ECDH and perform key exchange
            Serial.println("Failed to initialize ECDH");
            return false;
        }

        this->printPublicKey();
        WiFiClient client;

        Serial.println("\nStarting ECDH key exchange...");
    
        if (!client.connect(serverIP, serverPort)) {
            Serial.println("Connection to server failed!");
            return 1;
        }
        
        Serial.println("Connected to server");
        
        // Wait for server's public key
        Serial.println("Waiting for server public key...");
        
        unsigned long timeout = millis() + 10000; // 10 second timeout
        while (!client.available() && millis() < timeout) {
            delay(100);
        }
        
        if (!client.available()) {
            Serial.println("Timeout waiting for server response");
            client.stop();
            return 2;
        }
        
        // Read server response
        String response = client.readStringUntil('\n');
        Serial.println("Received from server:");
        Serial.println(response);
        
        // Parse JSON response
        JsonDocument serverDoc;  // Use different variable name to avoid conflicts
        DeserializationError error = deserializeJson(serverDoc, response.c_str());
        
        if (error) {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            client.stop();
            return 3;
        }

        // Extract server's public key coordinates
        String server_x_hex = serverDoc["server_public_x"];
        String server_y_hex = serverDoc["server_public_y"];
        
        // Convert hex strings to bytes
        uint8_t server_x[32], server_y[32];
        if (!hexStringToBytes(server_x_hex, server_x, 32) || 
            !hexStringToBytes(server_y_hex, server_y, 32)) {
            Serial.println("Failed to parse server public key");
            client.stop();
            return 4;
        }
        
        Serial.println("Server public key received and parsed");
        
        // Compute shared secret
        if (!this->computeSharedSecret(server_x, server_y)) {
            Serial.println("Failed to compute shared secret");
            client.stop();
            return 5;
        }
        
        // Get our public key coordinates
        uint8_t our_x[32], our_y[32];
        this->getPublicKeyCoordinates(our_x, our_y);
        
        // Send our public key to server
        JsonDocument clientDoc;
        clientDoc["client_public_x"] = bytesToHexString(our_x, 32);
        clientDoc["client_public_y"] = bytesToHexString(our_y, 32);
        
        String clientMessage;
        serializeJson(clientDoc, clientMessage);
        clientMessage += "\n";
        
        client.print(clientMessage);
        Serial.println("Sent our public key to server");
        
        // Wait for server confirmation
        timeout = millis() + 5000; // 5 second timeout
        while (!client.available() && millis() < timeout) {
            delay(100);
        }
        
        if (client.available()) {
            String confirmation = client.readStringUntil('\n');
            Serial.println("Server confirmation:");
            Serial.println(confirmation);
            
            // Parse confirmation
            JsonDocument confirmDoc;
            if (deserializeJson(confirmDoc, confirmation.c_str()) == DeserializationError::Ok) {
                if (confirmDoc["status"] == "success") {
                    Serial.println("\n=== ECDH KEY EXCHANGE SUCCESSFUL ===");
                    
                    // Derive encryption key
                    //uint8_t encryption_key[32];
                    this->deriveEncryptionKey(encryption_key);
                    
                    Serial.print("Derived encryption key: ");
                    printHex(encryption_key, 32);
                    
                    // Verify key hash
                    String server_hash = confirmDoc["key_hash"];
                    Serial.print("Server key hash: ");
                    Serial.println(server_hash);
                    
                    // Compute our hash for verification
                    SHA256 sha256;
                    sha256.update(encryption_key, 32);
                    uint8_t hash[32];
                    sha256.finalize(hash, 32);
                    
                    String our_hash = "";
                    for (int i = 0; i < 8; i++) { // First 16 hex chars
                        if (hash[i] < 16) our_hash += "0";
                        our_hash += String(hash[i], HEX);
                    }
                    
                    Serial.print("Our key hash: ");
                    Serial.println(our_hash);
                    
                    if (our_hash.equalsIgnoreCase(server_hash)) {
                        Serial.println("✓ Key verification successful!");
                    } else {
                        Serial.println("✗ Key verification failed!");
                        return 6;
                    }
                    
                    Serial.println("=== READY FOR ENCRYPTED COMMUNICATION ===");
                } else {
                    Serial.println("Key exchange failed!");
                    return 7;
                }
            }
        } else {
            Serial.println("No confirmation received from server");
            return 8;
        }
        
        client.stop();

        return 0;
    }

    String encryptMessage(uint8_t sharedKey[32], String plaintext) {
        // Generate random IV (16 bytes for AES)
        uint8_t iv[16];
        for (int i = 0; i < 16; i++) {
            iv[i] = random(256);
        }
        
        // Pad message to 16-byte boundary
        size_t paddedLen = ((plaintext.length() / 16) + 1) * 16;
        uint8_t* paddedText = new uint8_t[paddedLen];
        memset(paddedText, 0, paddedLen);
        memcpy(paddedText, plaintext.c_str(), plaintext.length());
        
        // Add PKCS7 padding
        uint8_t padValue = paddedLen - plaintext.length();
        for (size_t i = plaintext.length(); i < paddedLen; i++) {
            paddedText[i] = padValue;
        }
        
        // Create cipher buffer
        uint8_t* ciphertext = new uint8_t[paddedLen];
        
        // Encrypt using AES-256-CTR
        CTR<AES256> ctr;
        ctr.setKey(sharedKey, 32);
        ctr.setIV(iv, 16);
        ctr.encrypt(ciphertext, paddedText, paddedLen);
        
        // Create message with IV + ciphertext
        String ivHex = bytesToHex(iv, 16);
        String cipherHex = bytesToHex(ciphertext, paddedLen);
        String combined = ivHex + cipherHex;
        
        // Generate HMAC for authentication
        //String hmac = generateHMAC(combined, sharedKey);
        
        // Cleanup
        delete[] paddedText;
        delete[] ciphertext;
        
        // Return format: HMAC:IV:CIPHERTEXT
        //return hmac + ":" + combined;
        return combined;
    }

    String decryptMessage(uint8_t sharedKey[32], String encryptedData) {
        // Parse format: HMAC:IV:CIPHERTEXT
        // Parse format: IV:CIPHERTEXT
        //int firstColon = encryptedData.indexOf(':');
        //if (firstColon == -1) return "";
        
        //String receivedHMAC = encryptedData.substring(0, firstColon);
        String combined = encryptedData;
        
        // Verify HMAC
        //if (!verifyHMAC(combined, receivedHMAC, sharedKey)) {
        //    Serial.println("HMAC verification failed!");
        //    return "";
        //}
        
        // Extract IV and ciphertext
        if (combined.length() < 32) return ""; // At least 16 bytes IV
        
        String ivHex = combined.substring(0, 32);  // 16 bytes = 32 hex chars
        String cipherHex = combined.substring(32);
        
        if (cipherHex.length() % 32 != 0) return ""; // Must be multiple of 16 bytes
        
        uint8_t iv[16];
        hexToBytes(ivHex, iv);
        
        size_t cipherLen = cipherHex.length() / 2;
        uint8_t* ciphertext = new uint8_t[cipherLen];
        uint8_t* plaintext = new uint8_t[cipherLen];
        hexToBytes(cipherHex, ciphertext);
        
        // Decrypt using AES-256-CTR
        CTR<AES256> ctr;
        ctr.setKey(sharedKey, 32);
        ctr.setIV(iv, 16);
        ctr.decrypt(plaintext, ciphertext, cipherLen);
        
        // Remove PKCS7 padding
        uint8_t padValue = plaintext[cipherLen - 1];
        if (padValue > 16) {
            delete[] ciphertext;
            delete[] plaintext;
            return "";
        }
        
        size_t actualLen = cipherLen - padValue;
        
        // Convert to string
        char* result = new char[actualLen + 1];
        memcpy(result, plaintext, actualLen);
        result[actualLen] = '\0';
        
        String decrypted = String(result);
        
        // Cleanup
        delete[] ciphertext;
        delete[] plaintext;
        delete[] result;
        
        return decrypted;
    }
};


/* Post Quantum

My Recommendation: Option 1 - Hybrid Approach
Since ML-KEM is resource-intensive and may not fit on ESP8266, I recommend the hybrid approach:

ESP8266 does: Lightweight ECDH key exchange
Server does: ML-KEM heavy computation
Combined result: Post-quantum secure key

Benefits:

ESP8266 stays lightweight
You get full post-quantum security
Backward compatible with existing infrastructure
Server handles the computational load

The flow would be:

ESP8266 generates ECDH keypair (existing code)
Server generates ML-KEM keypair and combines with ECDH
Final key = ECDH_key ⊕ ML-KEM_key (or hash combination)

This gives you post-quantum security without overwhelming your ESP8266's 
limited resources. The server-side can use robust ML-KEM implementations 
like wolfSSL's Kyber implementation The Impact of Quantum Computing on 
Present Cryptography that are designed for more powerful devices.

*/