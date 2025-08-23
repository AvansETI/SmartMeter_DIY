#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Crypto.h>
#include <SHA256.h>
#include <uECC.h>  // Micro-ECC library for elliptic curve operations

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Server details
const char* serverIP = "192.168.1.100";  // Replace with your server IP
const int serverPort = 8888;

class ECDHKeyExchange {
private:
    uint8_t private_key[32];
    uint8_t public_key[64];  // Uncompressed format: 32 bytes X + 32 bytes Y
    uint8_t shared_secret[32];
    
public:
    ECDHKeyExchange() {
        // Initialize micro-ECC
        uECC_set_rng(&rng);
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
};

WiFiClient client;
ECDHKeyExchange ecdh;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP8266 ECDH Key Exchange Client ===");
    Serial.println("Using elliptic curve: secp256r1 (P-256)");
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());
    
    // Initialize ECDH and perform key exchange
    if (ecdh.generateKeyPair()) {
        ecdh.printPublicKey();
        performECDHKeyExchange();
    } else {
        Serial.println("Failed to initialize ECDH");
    }
}

void performECDHKeyExchange() {
    Serial.println("\nStarting ECDH key exchange...");
    
    if (!client.connect(serverIP, serverPort)) {
        Serial.println("Connection to server failed!");
        return;
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
        return;
    }
    
    // Read server response
    String response = client.readStringUntil('\n');
    Serial.println("Received from server:");
    Serial.println(response);
    
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        client.stop();
        return;
    }
    
    // Extract server's public key coordinates
    String server_x_hex = doc["server_public_x"];
    String server_y_hex = doc["server_public_y"];
    
    // Convert hex strings to bytes
    uint8_t server_x[32], server_y[32];
    if (!hexStringToBytes(server_x_hex, server_x, 32) || 
        !hexStringToBytes(server_y_hex, server_y, 32)) {
        Serial.println("Failed to parse server public key");
        client.stop();
        return;
    }
    
    Serial.println("Server public key received and parsed");
    
    // Compute shared secret
    if (!ecdh.computeSharedSecret(server_x, server_y)) {
        Serial.println("Failed to compute shared secret");
        client.stop();
        return;
    }
    
    // Get our public key coordinates
    uint8_t our_x[32], our_y[32];
    ecdh.getPublicKeyCoordinates(our_x, our_y);
    
    // Send our public key to server
    DynamicJsonDocument clientDoc(512);
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
        DynamicJsonDocument confirmDoc(512);
        if (deserializeJson(confirmDoc, confirmation) == DeserializationError::Ok) {
            if (confirmDoc["status"] == "success") {
                Serial.println("\n=== ECDH KEY EXCHANGE SUCCESSFUL ===");
                
                // Derive encryption key
                uint8_t encryption_key[32];
                ecdh.deriveEncryptionKey(encryption_key);
                
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
                }
                
                Serial.println("=== READY FOR ENCRYPTED COMMUNICATION ===");
            } else {
                Serial.println("Key exchange failed!");
            }
        }
    } else {
        Serial.println("No confirmation received from server");
    }
    
    client.stop();
}

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

void loop() {
    // Main loop - you can add encrypted communication here
    delay(1000);
    
    // Example: Perform key exchange every 5 minutes
    static unsigned long lastExchange = 0;
    if (millis() - lastExchange > 300000) {
        Serial.println("\nPerforming periodic ECDH key exchange...");
        if (ecdh.generateKeyPair()) {
            performECDHKeyExchange();
        }
        lastExchange = millis();
    }
}
