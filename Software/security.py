#!/usr/bin/env python3
#-------------------------------------------------------------------------
#  The MIT License (MIT)
#  Copyright © 2025 Avans Hogeschool Lectoraat Smart Energy
#  
#  Permission is hereby granted, free of charge, to any person obtaining a 
#  copy of this software and associated documentation files (the “Software”), 
#  to deal in the Software without restriction, including without limitation 
#  the rights to use, copy, modify, merge, publish, distribute, sublicense, 
#  and/or sell copies of the Software, and to permit persons to whom the 
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in 
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
#  THE SOFTWARE.
#
#  -------------------------------------------------------------------------
#
#  This software is part of the DIY SMARTMETER project. It is developed to enable
#  security for the project. It implements a key exchange server that the firmware
#  uses to execute key exchange if required to get a shared key on both the ESP
#  and the server. When the shared key has been exchanged, encryption and/or message
#  validity can be done.
#
#  -------------------------------------------------------------------------
import socket
import hashlib
import secrets
import json
from threading import Thread
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.backends import default_backend

class ECDHKeyExchange:
    def __init__(self):
        self.private_key = None
        self.public_key = None
        self.shared_secret = None
        self.curve = ec.SECP256R1()  # P-256 curve
      
    def generate_keypair(self):
        """Generate ECDH private and public key pair"""
        self.private_key = ec.generate_private_key(self.curve, default_backend())
        self.public_key = self.private_key.public_key()
        return self.public_key
        
    def get_public_key_bytes(self):
        """Get public key as uncompressed bytes (65 bytes for P-256)"""
        return self.public_key.public_numbers().x, self.public_key.public_numbers().y
        
    def compute_shared_secret(self, peer_public_x, peer_public_y):
        """Compute shared secret from peer's public key coordinates"""
        # Reconstruct the peer's public key from coordinates
        peer_public_numbers = ec.EllipticCurvePublicNumbers(
            x=peer_public_x,
            y=peer_public_y,
            curve=self.curve
        )
        peer_public_key = peer_public_numbers.public_key(default_backend())
        
        # Perform ECDH
        shared_key = self.private_key.exchange(ec.ECDH(), peer_public_key)
        
        # Derive AES key from shared secret using SHA-256
        digest = hashes.Hash(hashes.SHA256(), backend=default_backend())
        digest.update(shared_key)
        self.shared_secret = digest.finalize()
        
        return self.shared_secret

class ECDHKeyExchangeServer:
    def __init__(self, host='0.0.0.0', port=8888):
        self.host = host
        self.port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.key_vault = {} # Store the keys with key as ID and value as key
        self.read_vault()
        
    def read_vault(self):
        self.key_vault = {}
        try:
            f = open('vault.dat', 'rt', encoding='utf-8')
            for line in f:   ## iterates over the lines of the file
                r = line.rstrip().split(":=")
                if ( len(r) == 2 ):
                    print(f"Read key of id {r[0]}")
                    self.key_vault[r[0]] = r[1]
                else:
                    print("Parsing error '{line}")
            f.close()
        except:
            print("Could not read the key vault, so creating new one.")
            self.write_vault()

    def write_vault(self):
        try:
            f = open('vault.dat', 'wt', encoding='utf-8')
            for id in self.key_vault:
                f.write(f"{id}:={self.key_vault[id]}\n")
            f.close()
        except:
            print("Could not read the key vault")

    def start(self):
        """Start the server"""
        self.socket.bind((self.host, self.port))
        self.socket.listen(5)
        print(f"ECDH Key Exchange Server listening on {self.host}:{self.port}")
        print("Using elliptic curve: secp256r1 (P-256)")
        
        while True:
            try:
                client_socket, address = self.socket.accept()
                print(f"Connection from {address}")
                
                # Handle each client in a separate thread
                client_thread = Thread(target=self.handle_client, args=(client_socket, address))
                client_thread.daemon = True
                client_thread.start()
                
            except Exception as e:
                print(f"Error accepting connection: {e}")

    def handle_client(self, client_socket, address):
        """Handle ECDH key exchange with a client"""
        try:
            # Initialize ECDH
            ecdh = ECDHKeyExchange()
            server_public_key = ecdh.generate_keypair()
            
            # Get public key coordinates
            pub_x, pub_y = ecdh.get_public_key_bytes()
            
            # Send server's public key coordinates
            key_data = {
                'curve': 'secp256r1',
                'server_public_x': hex(pub_x),
                'server_public_y': hex(pub_y),
                'key_size': 256
            }
            
            message = json.dumps(key_data) + '\n'
            client_socket.send(message.encode())
            print(f"Sent ECDH public key to {address}")
            
            # Receive client's public key
            response = client_socket.recv(4096).decode().strip()
            if not response:
                print(f"No response from {address}")
                return
                
            client_data = json.loads(response)
            client_public_x = int(client_data['client_public_x'], 16)
            client_public_y = int(client_data['client_public_y'], 16)
            id              = str(client_data['id'])
            
            print(f"Received client public key from {address} with id {id}")
            
            # Compute shared secret
            shared_key = ecdh.compute_shared_secret(client_public_x, client_public_y)
            
            # Send confirmation with key hash for verification
            confirmation = {
                'status': 'success',
                'key_hash': hashlib.sha256(shared_key).hexdigest()[:16],
                'algorithm': 'ECDH-P256-SHA256'
            }
            
            client_socket.send((json.dumps(confirmation) + '\n').encode())
            
            print(f"ECDH key exchange completed with {address}")
            print(f"Shared key hash: {confirmation['key_hash']}")
            print(f"Full shared key (32 bytes): {shared_key.hex()}")

            # Save shared key
            if ( id not in self.key_vault ):
                self.key_vault[id] = shared_key.hex()
                self.write_vault()
                print("Successfully saved shared key!")
            else:
                print("ID already stored, key not accepted!")
            
            print("-" * 60)
            
        except json.JSONDecodeError as e:
            print(f"JSON decode error from {address}: {e}")
        except ValueError as e:
            print(f"Invalid key data from {address}: {e}")
        except Exception as e:
            print(f"Error handling client {address}: {e}")
        finally:
            client_socket.close()

    def stop(self):
        """Stop the server"""
        self.socket.close()

if __name__ == "__main__":
    print("=== ECDH Key Exchange Server ===")
    print("Make sure to install cryptography library:")
    print("pip install cryptography")
    print()
    
    try:
        from cryptography.hazmat.primitives.asymmetric import ec
        print("Cryptography library found ✓")
    except ImportError:
        print("ERROR: cryptography library not found!")
        print("Install it with: pip install cryptography")
        exit(1)
    
    server = ECDHKeyExchangeServer()
    try:
        server.start()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        server.stop()
