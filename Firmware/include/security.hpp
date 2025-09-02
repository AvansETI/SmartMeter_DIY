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

  This library implements the functionality to add security to the project that
  is able to validaty the client to the server and if required perform full
  end2end encryption of the messages send over MQTT. MQTT itself does not
  require to use encryption.

  Enhanced ECDH Security Library for ESP8266/ESP32S2
  Improvements: Better error handling, memory management, security hardening

  V1.0  Initial version
  -------------------------------------------------------------------------*/
#include <Crypto.h>
#include <SHA256.h>
#include <AES.h>
#include <CTR.h>
#include <uECC.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// Configuration constants
const int CONNECTION_TIMEOUT = 10000;  // 10 seconds
const int RESPONSE_TIMEOUT = 5000;     // 5 seconds
const int MAX_RETRIES = 3;

// Security constants
const size_t KEY_SIZE = 32;
const size_t IV_SIZE = 16;
const size_t COORDINATE_SIZE = 32;
const size_t PUBLIC_KEY_SIZE = 64;

// Error codes enum for better error handling
enum ECDHError {
    SUCCESS = 0,
    CONNECTION_FAILED = 1,
    TIMEOUT_ERROR = 2,
    JSON_PARSE_ERROR = 3,
    KEY_PARSE_ERROR = 4,
    SHARED_SECRET_ERROR = 5,
    KEY_VERIFICATION_ERROR = 6,
    EXCHANGE_FAILED = 7,
    NO_CONFIRMATION = 8,
    MEMORY_ERROR = 9,
    CRYPTO_ERROR = 10
};

// Utility functions with improved error handling
bool hexStringToBytes(const String& hexString, uint8_t* bytes, size_t expectedLength) {
    if (!bytes) return false;
    
    String cleanHex = hexString;
    if (cleanHex.startsWith("0x")) {
        cleanHex = cleanHex.substring(2);
    }
    
    if (cleanHex.length() != expectedLength * 2) {
        //Serial.printf("Invalid hex length: expected %u, got %u\n", 
        //             expectedLength * 2, cleanHex.length());
        return false;
    }
    
    for (size_t i = 0; i < expectedLength; i++) {
        String byteString = cleanHex.substring(i * 2, i * 2 + 2);
        char* endPtr;
        long value = strtol(byteString.c_str(), &endPtr, 16);
        
        if (*endPtr != '\0' || value < 0 || value > 255) {
            //Serial.printf("Invalid hex byte at position %u: %s\n", i, byteString.c_str());
            return false;
        }
        
        bytes[i] = (uint8_t)value;
    }
    
    return true;
}

String bytesToHexString(const uint8_t* bytes, size_t length, bool includePrefix = true) {
    if (!bytes) return "";
    
    String hexString = includePrefix ? "0x" : "";
    hexString.reserve(hexString.length() + length * 2);
    
    for (size_t i = 0; i < length; i++) {
        if (bytes[i] < 16) hexString += "0";
        hexString += String(bytes[i], HEX);
    }
    return hexString;
}

void printHex(const uint8_t* data, size_t length, const String& label = "") {
    if (!data) return;
    
    if (label.length() > 0) {
        Serial.print(label);
        Serial.print(": ");
    }
    
    for (size_t i = 0; i < length; i++) {
        if (data[i] < 16) Serial.print("0");
        Serial.print(data[i], HEX);
        if (i < length - 1 && (i + 1) % 16 == 0) Serial.print(" ");
    }
    Serial.println();
}

// Secure memory management
class SecureBuffer {
private:
    uint8_t* buffer;
    size_t size;
    
public:
    SecureBuffer(size_t bufferSize) : size(bufferSize) {
        buffer = (uint8_t*)malloc(bufferSize);
        if (buffer) {
            memset(buffer, 0, bufferSize);
        }
    }
    
    ~SecureBuffer() {
        if (buffer) {
            // Securely wipe memory before freeing
            memset(buffer, 0, size);
            free(buffer);
        }
    }
    
    uint8_t* get() { return buffer; }
    size_t getSize() const { return size; }
    bool isValid() const { return buffer != nullptr; }
};

// Enhanced HMAC with better security
String generateHMAC(const String& message, const uint8_t* key, size_t keySize = KEY_SIZE) {
    if (!key || keySize == 0) return "";
    
    SHA256 hasher;
    hasher.reset();
    hasher.update(key, keySize);
    hasher.update(message.c_str(), message.length());
    
    uint8_t hash[KEY_SIZE];
    hasher.finalize(hash, KEY_SIZE);
    
    return bytesToHexString(hash, KEY_SIZE, false);
}

bool verifyHMAC(const String& message, const String& receivedHMAC, const uint8_t* key) {
    String calculatedHMAC = generateHMAC(message, key);
    
    // Constant-time comparison to prevent timing attacks
    if (calculatedHMAC.length() != receivedHMAC.length()) {
        return false;
    }
    
    uint8_t result = 0;
    for (size_t i = 0; i < calculatedHMAC.length(); i++) {
        result |= calculatedHMAC[i] ^ receivedHMAC[i];
    }
    
    return result == 0;
}

class ECDHKeyExchange {
private:
    char serverIp[80];
    int serverPort;

    uint8_t private_key[KEY_SIZE];
    uint8_t public_key[PUBLIC_KEY_SIZE];
    uint8_t shared_secret[KEY_SIZE];
    bool keys_generated;
    
    void secureWipe() {
        memset(private_key, 0, KEY_SIZE);
        memset(shared_secret, 0, KEY_SIZE);
    }
    
public:
    ECDHKeyExchange(char* serverIp, int serverPort) : keys_generated(false) {
        strcpy(this->serverIp, serverIp);
        this->serverPort = serverPort;

        uECC_set_rng(&rng);
        // Better random seed using multiple sources
        randomSeed(analogRead(0) ^ micros() ^ ESP.getCycleCount());
        secureWipe();
    }
    
    ~ECDHKeyExchange() {
        secureWipe();
    }
    
    static int rng(uint8_t *dest, unsigned size) {
        if (!dest) return 0;
        
        // Enhanced randomness using multiple ESP8266 sources
        for (unsigned i = 0; i < size; i++) {
            uint32_t random_val = RANDOM_REG32;
            random_val ^= (ESP.getCycleCount() << (i & 7));
            random_val ^= (micros() << ((i + 4) & 7));
            dest[i] = (uint8_t)(random_val & 0xFF);
        }
        return 1;
    }
    
    bool generateKeyPair() {
        const struct uECC_Curve_t* curve = uECC_secp256r1();
        
        // Clear previous keys
        secureWipe();
        
        if (!uECC_make_key(public_key, private_key, curve)) {
            //Serial.println("ERROR: Failed to generate ECDH key pair");
            return false;
        }
        
        keys_generated = true;
        //Serial.println("SUCCESS: ECDH key pair generated");
        return true;
    }
    
    void getPublicKeyCoordinates(uint8_t* x_coord, uint8_t* y_coord) const {
        if (!keys_generated || !x_coord || !y_coord) return;
        
        memcpy(x_coord, public_key, COORDINATE_SIZE);
        memcpy(y_coord, public_key + COORDINATE_SIZE, COORDINATE_SIZE);
    }
    
    bool computeSharedSecret(const uint8_t* peer_x, const uint8_t* peer_y) {
        if (!keys_generated || !peer_x || !peer_y) return false;
        
        uint8_t peer_public[PUBLIC_KEY_SIZE];
        memcpy(peer_public, peer_x, COORDINATE_SIZE);
        memcpy(peer_public + COORDINATE_SIZE, peer_y, COORDINATE_SIZE);
        
        const struct uECC_Curve_t* curve = uECC_secp256r1();
        
        if (!uECC_shared_secret(peer_public, private_key, shared_secret, curve)) {
            //Serial.println("ERROR: Failed to compute shared secret");
            return false;
        }
        
        Serial.println("SUCCESS: Shared secret computed");
        return true;
    }
    
    void deriveEncryptionKey(uint8_t* derived_key) const {
        if (!derived_key) return;
        
        SHA256 sha256;
        sha256.update(shared_secret, KEY_SIZE);
        sha256.finalize(derived_key, KEY_SIZE);
    }
    
    void printPublicKey() const {
        if (!keys_generated) {
            //Serial.println("No key pair generated");
            return;
        }
        printHex(public_key, PUBLIC_KEY_SIZE, "Public Key");
    }
    
    ECDHError executeKeyExchange(uint8_t encryption_key[KEY_SIZE], String id) {
        if (!encryption_key) return MEMORY_ERROR;
        
        // Generate key pair with retry logic
        int retries = 0;
        while (!generateKeyPair() && retries < MAX_RETRIES) {
            retries++;
            delay(100);
        }
        
        if (!keys_generated) {
            //Serial.println("FATAL: Could not generate key pair after retries");
            return CRYPTO_ERROR;
        }
        
        //printPublicKey();
        
        WiFiClient client;
        //Serial.printf("Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);
        
        if (!client.connect(this->serverIp, this->serverPort)) {
            //Serial.println("ERROR: Connection to server failed");
            return CONNECTION_FAILED;
        }
        
        //Serial.println("SUCCESS: Connected to server");
        
        // Wait for server's public key with timeout
        unsigned long timeout = millis() + CONNECTION_TIMEOUT;
        while (!client.available() && millis() < timeout) {
            delay(50);
        }
        
        if (!client.available()) {
            //Serial.println("ERROR: Timeout waiting for server response");
            client.stop();
            return TIMEOUT_ERROR;
        }
        
        // Read and parse server response
        String response = client.readStringUntil('\n');
        response.trim();
        //Serial.printf("Server response: %s\n", response.c_str());
        
        JsonDocument serverDoc;
        DeserializationError error = deserializeJson(serverDoc, response);
        
        if (error) {
            //Serial.printf("ERROR: JSON parsing failed: %s\n", error.c_str());
            client.stop();
            return JSON_PARSE_ERROR;
        }
        
        // Validate and extract server's public key
        if (!serverDoc.containsKey("server_public_x") || 
            !serverDoc.containsKey("server_public_y")) {
            //Serial.println("ERROR: Missing server public key fields");
            client.stop();
            return KEY_PARSE_ERROR;
        }
        
        String server_x_hex = serverDoc["server_public_x"];
        String server_y_hex = serverDoc["server_public_y"];
        
        uint8_t server_x[COORDINATE_SIZE], server_y[COORDINATE_SIZE];
        if (!hexStringToBytes(server_x_hex, server_x, COORDINATE_SIZE) || 
            !hexStringToBytes(server_y_hex, server_y, COORDINATE_SIZE)) {
            //Serial.println("ERROR: Failed to parse server public key");
            client.stop();
            return KEY_PARSE_ERROR;
        }
        
        // Compute shared secret
        if (!computeSharedSecret(server_x, server_y)) {
            client.stop();
            return SHARED_SECRET_ERROR;
        }
        
        // Send our public key
        uint8_t our_x[COORDINATE_SIZE], our_y[COORDINATE_SIZE];
        getPublicKeyCoordinates(our_x, our_y);
        
        JsonDocument clientDoc;
        clientDoc["client_public_x"] = bytesToHexString(our_x, COORDINATE_SIZE);
        clientDoc["client_public_y"] = bytesToHexString(our_y, COORDINATE_SIZE);
        clientDoc["id"]              = id;
        
        String clientMessage;
        serializeJson(clientDoc, clientMessage);
        clientMessage += "\n";
        
        client.print(clientMessage);
        //Serial.println("SUCCESS: Sent public key to server");
        
        // Wait for confirmation
        timeout = millis() + RESPONSE_TIMEOUT;
        while (!client.available() && millis() < timeout) {
            delay(50);
        }
        
        if (!client.available()) {
            //Serial.println("ERROR: No confirmation received");
            client.stop();
            return NO_CONFIRMATION;
        }
        
        String confirmation = client.readStringUntil('\n');
        confirmation.trim();
        //Serial.printf("Server confirmation: %s\n", confirmation.c_str());
        
        JsonDocument confirmDoc;
        if (deserializeJson(confirmDoc, confirmation) != DeserializationError::Ok) {
            //Serial.println("ERROR: Failed to parse confirmation");
            client.stop();
            return JSON_PARSE_ERROR;
        }
        
        if (confirmDoc["status"] != "success") {
            //Serial.println("ERROR: Key exchange failed");
            client.stop();
            return EXCHANGE_FAILED;
        }
        
        // Derive and verify encryption key
        deriveEncryptionKey(encryption_key);
        //printHex(encryption_key, KEY_SIZE, "Derived encryption key");
        
        // Verify key hash if provided
        if (confirmDoc.containsKey("key_hash")) {
            String server_hash = confirmDoc["key_hash"];
            
            SHA256 sha256;
            sha256.update(encryption_key, KEY_SIZE);
            uint8_t hash[KEY_SIZE];
            sha256.finalize(hash, KEY_SIZE);
            
            String our_hash = bytesToHexString(hash, 8, false); // First 8 bytes
            
            if (!our_hash.equalsIgnoreCase(server_hash)) {
                //Serial.println("ERROR: Key verification failed");
                client.stop();
                return KEY_VERIFICATION_ERROR;
            }
            
            //Serial.println("SUCCESS: Key verification passed");
        } else {
            //Serial.println("ERROR: Key verification failed hash not provided");
            client.stop();
            return KEY_VERIFICATION_ERROR;
        }
        
        client.stop();
        //Serial.println("=== ECDH KEY EXCHANGE COMPLETED SUCCESSFULLY ===");
        return SUCCESS;
    }
    
    static String hashKey (uint8_t key[32], uint8_t s) {
        SHA256 sha256;
        sha256.update(key, 32);
        uint8_t hash[32];
        sha256.finalize(hash, 32);

        String h = bytesToHexString(hash, 32, false);
        
        return h;
    }
    
    static String encryptMessage(const uint8_t* sharedKey, const String& plaintext) {
        if (!sharedKey || plaintext.length() == 0) return "";
        
        // Generate cryptographically strong IV
        SecureBuffer ivBuffer(IV_SIZE);
        if (!ivBuffer.isValid()) return "";
        
        uint8_t* iv = ivBuffer.get();
        rng(iv, IV_SIZE);
        
        // Calculate padded length for PKCS7
        size_t paddedLen = ((plaintext.length() / 16) + 1) * 16;
        SecureBuffer paddedBuffer(paddedLen);
        SecureBuffer cipherBuffer(paddedLen);
        
        if (!paddedBuffer.isValid() || !cipherBuffer.isValid()) {
            return "";
        }
        
        uint8_t* paddedText = paddedBuffer.get();
        uint8_t* ciphertext = cipherBuffer.get();
        
        // Apply PKCS7 padding
        memcpy(paddedText, plaintext.c_str(), plaintext.length());
        uint8_t padValue = paddedLen - plaintext.length();
        for (size_t i = plaintext.length(); i < paddedLen; i++) {
            paddedText[i] = padValue;
        }
        
        // Encrypt using AES-256-CTR
        CTR<AES256> ctr;
        ctr.setKey(sharedKey, KEY_SIZE);
        ctr.setIV(iv, IV_SIZE);
        ctr.encrypt(ciphertext, paddedText, paddedLen);
        
        // Combine IV + ciphertext
        String result = bytesToHexString(iv, IV_SIZE, false) + 
                        bytesToHexString(ciphertext, paddedLen, false);
        
        return result;
    }
    
    static String decryptMessage(const uint8_t* sharedKey, const String& encryptedData) {
        if (!sharedKey || encryptedData.length() < IV_SIZE * 2) return "";
        
        // Extract IV and ciphertext
        String ivHex = encryptedData.substring(0, IV_SIZE * 2);
        String cipherHex = encryptedData.substring(IV_SIZE * 2);
        
        if (cipherHex.length() % 32 != 0) return ""; // Must be 16-byte aligned
        
        size_t cipherLen = cipherHex.length() / 2;
        SecureBuffer ivBuffer(IV_SIZE);
        SecureBuffer cipherBuffer(cipherLen);
        SecureBuffer plainBuffer(cipherLen);
        
        if (!ivBuffer.isValid() || !cipherBuffer.isValid() || !plainBuffer.isValid()) {
            return "";
        }
        
        uint8_t* iv = ivBuffer.get();
        uint8_t* ciphertext = cipherBuffer.get();
        uint8_t* plaintext = plainBuffer.get();
        
        // Convert hex to bytes
        if (!hexStringToBytes(ivHex, iv, IV_SIZE) ||
            !hexStringToBytes(cipherHex, ciphertext, cipherLen)) {
            return "";
        }
        
        // Decrypt
        CTR<AES256> ctr;
        ctr.setKey(sharedKey, KEY_SIZE);
        ctr.setIV(iv, IV_SIZE);
        ctr.decrypt(plaintext, ciphertext, cipherLen);
        
        // Remove PKCS7 padding
        uint8_t padValue = plaintext[cipherLen - 1];
        if (padValue > 16 || padValue == 0) return "";
        
        size_t actualLen = cipherLen - padValue;
        
        // Verify padding
        for (size_t i = actualLen; i < cipherLen; i++) {
            if (plaintext[i] != padValue) return "";
        }
        
        // Create result string
        char* result = (char*)malloc(actualLen + 1);
        if (!result) return "";
        
        memcpy(result, plaintext, actualLen);
        result[actualLen] = '\0';
        
        String decrypted = String(result);
        free(result);
        
        return decrypted;
    }
};