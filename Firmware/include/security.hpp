/*-------------------------------------------------------------------------
  The MIT License (MIT)
  Copyright © 2025 Avans Hogeschool Lectoraat Smart Energy
  
  Permission is hereby granted, free of charge, to any person obtaining a 
  copy of this software and associated documentation files (the "Software"), 
  to deal in the Software without restriction, including without limitation 
  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
  and/or sell copies of the Software, and to permit persons to whom the 
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
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
  C-String version for reduced memory usage

  V1.1  C-String conversion for memory optimization
  -------------------------------------------------------------------------*/
#include <Crypto.h>
#include <SHA256.h>
#include <AES.h>
#include <CTR.h>
#include <uECC.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include <stdlib.h>

// Configuration constants
const int CONNECTION_TIMEOUT = 10000;  // 10 seconds
const int RESPONSE_TIMEOUT = 5000;     // 5 seconds
const int MAX_RETRIES = 3;

// Security constants
const size_t KEY_SIZE = 32;
const size_t IV_SIZE = 16;
const size_t COORDINATE_SIZE = 32;
const size_t PUBLIC_KEY_SIZE = 64;

// Buffer size constants
const size_t MAX_HEX_STRING = 256;
const size_t MAX_MESSAGE_SIZE = 512;
const size_t MAX_RESPONSE_SIZE = 1024;
const size_t MAX_ID_SIZE = 64;

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
    CRYPTO_ERROR = 10,
    BUFFER_OVERFLOW = 11
};

// Utility functions with improved error handling
bool hexStringToBytes(const char* hexString, uint8_t* bytes, size_t expectedLength) {
    if (!bytes || !hexString) return false;
    
    const char* cleanHex = hexString;
    // Skip "0x" prefix if present
    if (strncmp(hexString, "0x", 2) == 0) {
        cleanHex = hexString + 2;
    }
    
    size_t hexLen = strlen(cleanHex);
    if (hexLen != expectedLength * 2) {
        return false;
    }
    
    for (size_t i = 0; i < expectedLength; i++) {
        char byteString[3];
        byteString[0] = cleanHex[i * 2];
        byteString[1] = cleanHex[i * 2 + 1];
        byteString[2] = '\0';
        
        char* endPtr;
        long value = strtol(byteString, &endPtr, 16);
        
        if (*endPtr != '\0' || value < 0 || value > 255) {
            return false;
        }
        
        bytes[i] = (uint8_t)value;
    }
    
    return true;
}

bool bytesToHexString(const uint8_t* bytes, size_t length, char* output, size_t outputSize, bool includePrefix = true) {
    if (!bytes || !output) return false;
    
    size_t requiredSize = (includePrefix ? 2 : 0) + length * 2 + 1;
    if (outputSize < requiredSize) return false;
    
    char* ptr = output;
    
    if (includePrefix) {
        strcpy(ptr, "0x");
        ptr += 2;
    }
    
    for (size_t i = 0; i < length; i++) {
        sprintf(ptr, "%02x", bytes[i]);
        ptr += 2;
    }
    *ptr = '\0';
    
    return true;
}

void printHex(const uint8_t* data, size_t length, const char* label = "") {
    if (!data) return;
    
    if (strlen(label) > 0) {
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
bool generateHMAC(const char* message, const uint8_t* key, char* output, size_t outputSize, size_t keySize = KEY_SIZE) {
    if (!key || !message || !output || keySize == 0 || outputSize < (KEY_SIZE * 2 + 1)) {
        return false;
    }
    
    SHA256 hasher;
    hasher.reset();
    hasher.update(key, keySize);
    hasher.update(message, strlen(message));
    
    uint8_t hash[KEY_SIZE];
    hasher.finalize(hash, KEY_SIZE);
    
    return bytesToHexString(hash, KEY_SIZE, output, outputSize, false);
}

bool verifyHMAC(const char* message, const char* receivedHMAC, const uint8_t* key) {
    char calculatedHMAC[KEY_SIZE * 2 + 1];
    
    if (!generateHMAC(message, key, calculatedHMAC, sizeof(calculatedHMAC))) {
        return false;
    }
    
    // Constant-time comparison to prevent timing attacks
    size_t calcLen = strlen(calculatedHMAC);
    size_t recvLen = strlen(receivedHMAC);
    
    if (calcLen != recvLen) {
        return false;
    }
    
    uint8_t result = 0;
    for (size_t i = 0; i < calcLen; i++) {
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
    ECDHKeyExchange(const char* serverIp, int serverPort) : keys_generated(false) {
        strncpy(this->serverIp, serverIp, sizeof(this->serverIp) - 1);
        this->serverIp[sizeof(this->serverIp) - 1] = '\0';
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
            return false;
        }
        
        keys_generated = true;
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
            return;
        }
        printHex(public_key, PUBLIC_KEY_SIZE, "Public Key");
    }
    
    ECDHError executeKeyExchange(uint8_t encryption_key[KEY_SIZE], const char* id) {
        if (!encryption_key || !id) return MEMORY_ERROR;
        
        // Generate key pair with retry logic
        int retries = 0;
        while (!generateKeyPair() && retries < MAX_RETRIES) {
            retries++;
            delay(100);
        }
        
        if (!keys_generated) {
            return CRYPTO_ERROR;
        }
        
        WiFiClient client;
        
        if (!client.connect(this->serverIp, this->serverPort)) {
            return CONNECTION_FAILED;
        }
        
        // Wait for server's public key with timeout
        unsigned long timeout = millis() + CONNECTION_TIMEOUT;
        while (!client.available() && millis() < timeout) {
            delay(50);
        }
        
        if (!client.available()) {
            client.stop();
            return TIMEOUT_ERROR;
        }
        
        // Read server response into buffer
        char response[MAX_RESPONSE_SIZE];
        size_t responseLen = 0;
        
        while (client.available() && responseLen < sizeof(response) - 1) {
            char c = client.read();
            if (c == '\n') break;
            response[responseLen++] = c;
        }
        response[responseLen] = '\0';
        
        // Trim whitespace
        while (responseLen > 0 && (response[responseLen-1] == '\r' || response[responseLen-1] == ' ')) {
            response[--responseLen] = '\0';
        }
        
        JsonDocument serverDoc;
        DeserializationError error = deserializeJson(serverDoc, response);
        
        if (error) {
            client.stop();
            return JSON_PARSE_ERROR;
        }
        
        // Validate and extract server's public key
        if (!serverDoc.containsKey("server_public_x") || 
            !serverDoc.containsKey("server_public_y")) {
            client.stop();
            return KEY_PARSE_ERROR;
        }
        
        const char* server_x_hex = serverDoc["server_public_x"];
        const char* server_y_hex = serverDoc["server_public_y"];
        
        uint8_t server_x[COORDINATE_SIZE], server_y[COORDINATE_SIZE];
        if (!hexStringToBytes(server_x_hex, server_x, COORDINATE_SIZE) || 
            !hexStringToBytes(server_y_hex, server_y, COORDINATE_SIZE)) {
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
        
        char our_x_hex[COORDINATE_SIZE * 2 + 1];
        char our_y_hex[COORDINATE_SIZE * 2 + 1];
        
        if (!bytesToHexString(our_x, COORDINATE_SIZE, our_x_hex, sizeof(our_x_hex), false) ||
            !bytesToHexString(our_y, COORDINATE_SIZE, our_y_hex, sizeof(our_y_hex), false)) {
            client.stop();
            return MEMORY_ERROR;
        }
        
        JsonDocument clientDoc;
        clientDoc["client_public_x"] = our_x_hex;
        clientDoc["client_public_y"] = our_y_hex;
        clientDoc["id"] = id;
        
        char clientMessage[MAX_MESSAGE_SIZE];
        size_t messageLen = serializeJson(clientDoc, clientMessage, sizeof(clientMessage) - 2);
        if (messageLen == 0 || messageLen >= sizeof(clientMessage) - 2) {
            client.stop();
            return MEMORY_ERROR;
        }
        
        strcat(clientMessage, "\n");
        client.print(clientMessage);
        
        // Wait for confirmation
        timeout = millis() + RESPONSE_TIMEOUT;
        while (!client.available() && millis() < timeout) {
            delay(50);
        }
        
        if (!client.available()) {
            client.stop();
            return NO_CONFIRMATION;
        }
        
        // Read confirmation response
        responseLen = 0;
        memset(response, 0, sizeof(response));
        
        while (client.available() && responseLen < sizeof(response) - 1) {
            char c = client.read();
            if (c == '\n') break;
            response[responseLen++] = c;
        }
        response[responseLen] = '\0';
        
        // Trim whitespace
        while (responseLen > 0 && (response[responseLen-1] == '\r' || response[responseLen-1] == ' ')) {
            response[--responseLen] = '\0';
        }
        
        JsonDocument confirmDoc;
        if (deserializeJson(confirmDoc, response) != DeserializationError::Ok) {
            client.stop();
            return JSON_PARSE_ERROR;
        }
        
        const char* status = confirmDoc["status"];
        if (!status || strcmp(status, "success") != 0) {
            client.stop();
            return EXCHANGE_FAILED;
        }
        
        // Derive and verify encryption key
        deriveEncryptionKey(encryption_key);
        
        // Verify key hash if provided
        if (confirmDoc.containsKey("key_hash")) {
            const char* server_hash = confirmDoc["key_hash"];
            
            SHA256 sha256;
            sha256.update(encryption_key, KEY_SIZE);
            uint8_t hash[KEY_SIZE];
            sha256.finalize(hash, KEY_SIZE);
            
            char our_hash[17]; // First 8 bytes = 16 hex chars + null
            if (!bytesToHexString(hash, 8, our_hash, sizeof(our_hash), false)) {
                client.stop();
                return MEMORY_ERROR;
            }
            
            // Case-insensitive comparison
            bool hashMatch = true;
            size_t serverHashLen = strlen(server_hash);
            size_t ourHashLen = strlen(our_hash);
            
            if (serverHashLen != ourHashLen) {
                hashMatch = false;
            } else {
                for (size_t i = 0; i < ourHashLen; i++) {
                    char c1 = tolower(server_hash[i]);
                    char c2 = tolower(our_hash[i]);
                    if (c1 != c2) {
                        hashMatch = false;
                        break;
                    }
                }
            }
            
            if (!hashMatch) {
                client.stop();
                return KEY_VERIFICATION_ERROR;
            }
        } else {
            client.stop();
            return KEY_VERIFICATION_ERROR;
        }
        
        client.stop();
        return SUCCESS;
    }
    
    static bool hashKey(const uint8_t key[32], char* output, size_t outputSize) {
        if (!key || !output || outputSize < (32 * 2 + 1)) return false;
        
        SHA256 sha256;
        sha256.update(key, 32);
        uint8_t hash[32];
        sha256.finalize(hash, 32);

        return bytesToHexString(hash, 32, output, outputSize, false);
    }
    
    static bool encryptMessage(const uint8_t* sharedKey, const char* plaintext, char* output, size_t outputSize) {
        if (!sharedKey || !plaintext || !output || strlen(plaintext) == 0) return false;
        
        size_t plaintextLen = strlen(plaintext);
        
        // Generate cryptographically strong IV
        SecureBuffer ivBuffer(IV_SIZE);
        if (!ivBuffer.isValid()) return false;
        
        uint8_t* iv = ivBuffer.get();
        rng(iv, IV_SIZE);
        
        // Calculate padded length for PKCS7
        size_t paddedLen = ((plaintextLen / 16) + 1) * 16;
        SecureBuffer paddedBuffer(paddedLen);
        SecureBuffer cipherBuffer(paddedLen);
        
        if (!paddedBuffer.isValid() || !cipherBuffer.isValid()) {
            return false;
        }
        
        uint8_t* paddedText = paddedBuffer.get();
        uint8_t* ciphertext = cipherBuffer.get();
        
        // Apply PKCS7 padding
        memcpy(paddedText, plaintext, plaintextLen);
        uint8_t padValue = paddedLen - plaintextLen;
        for (size_t i = plaintextLen; i < paddedLen; i++) {
            paddedText[i] = padValue;
        }
        
        // Encrypt using AES-256-CTR
        CTR<AES256> ctr;
        ctr.setKey(sharedKey, KEY_SIZE);
        ctr.setIV(iv, IV_SIZE);
        ctr.encrypt(ciphertext, paddedText, paddedLen);
        
        // Check output buffer size
        size_t requiredSize = (IV_SIZE + paddedLen) * 2 + 1;
        if (outputSize < requiredSize) return false;
        
        // Convert IV to hex
        char ivHex[IV_SIZE * 2 + 1];
        char cipherHex[paddedLen * 2 + 1];
        
        if (!bytesToHexString(iv, IV_SIZE, ivHex, sizeof(ivHex), false) ||
            !bytesToHexString(ciphertext, paddedLen, cipherHex, sizeof(cipherHex), false)) {
            return false;
        }
        
        // Combine IV + ciphertext
        strcpy(output, ivHex);
        strcat(output, cipherHex);
        
        return true;
    }
    
    static bool decryptMessage(const uint8_t* sharedKey, const char* encryptedData, char* output, size_t outputSize) {
        if (!sharedKey || !encryptedData || !output) return false;
        
        size_t encDataLen = strlen(encryptedData);
        if (encDataLen < IV_SIZE * 2) return false;
        
        // Extract IV and ciphertext hex strings
        char ivHex[IV_SIZE * 2 + 1];
        strncpy(ivHex, encryptedData, IV_SIZE * 2);
        ivHex[IV_SIZE * 2] = '\0';
        
        const char* cipherHex = encryptedData + (IV_SIZE * 2);
        size_t cipherHexLen = strlen(cipherHex);
        
        if (cipherHexLen % 32 != 0) return false; // Must be 16-byte aligned
        
        size_t cipherLen = cipherHexLen / 2;
        SecureBuffer ivBuffer(IV_SIZE);
        SecureBuffer cipherBuffer(cipherLen);
        SecureBuffer plainBuffer(cipherLen);
        
        if (!ivBuffer.isValid() || !cipherBuffer.isValid() || !plainBuffer.isValid()) {
            return false;
        }
        
        uint8_t* iv = ivBuffer.get();
        uint8_t* ciphertext = cipherBuffer.get();
        uint8_t* plaintext = plainBuffer.get();
        
        // Convert hex to bytes
        if (!hexStringToBytes(ivHex, iv, IV_SIZE) ||
            !hexStringToBytes(cipherHex, ciphertext, cipherLen)) {
            return false;
        }
        
        // Decrypt
        CTR<AES256> ctr;
        ctr.setKey(sharedKey, KEY_SIZE);
        ctr.setIV(iv, IV_SIZE);
        ctr.decrypt(plaintext, ciphertext, cipherLen);
        
        // Remove PKCS7 padding
        uint8_t padValue = plaintext[cipherLen - 1];
        if (padValue > 16 || padValue == 0) return false;
        
        size_t actualLen = cipherLen - padValue;
        
        // Verify padding
        for (size_t i = actualLen; i < cipherLen; i++) {
            if (plaintext[i] != padValue) return false;
        }
        
        // Check output buffer size
        if (outputSize <= actualLen) return false;
        
        // Copy result
        memcpy(output, plaintext, actualLen);
        output[actualLen] = '\0';
        
        return true;
    }
};