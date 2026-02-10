# CNT5106C - P2P File Sharing Service.
Per the project description:
```
In this project, you are asked to write a P2P file sharing software similar to BitTorrent.
```

## Message Formats
### Handshake Message
Sent after establishing an intial TCP connection between peers.

**Structure (32 bytes):**
+------------------------------------------------------------------------+
| Handshake Header (18 bytes) | Zero Bits (10 bytes) | Peer ID (4 bytes) |
+------------------------------------------------------------------------+

### Actual Message
After 'Handshake' messages are sent between peer pairs, a never-ending stream of messages follow.
These messages are variable in length of 5+ bytes.

**Structure (5+ bytes):**
+--------------------------------------------------------------------------------+
| Message Length (4 bytes) | Message Type (1 byte) | Message Payload (0+ bytes)  |
+--------------------------------------------------------------------------------+

#### Message Types
There are 8 unique message types sent in the message stream following the handshake.
+----------------+-----+-------------------------------------------------------------------------------+
| Message Type   | ID  | Payload Status + Description                                                  |
+----------------+-----+-------------------------------------------------------------------------------+
| Choke          |  0  | (NO PAYLOAD)                                                                  |
+----------------+-----+-------------------------------------------------------------------------------+
| Unchoke        |  1  | (NO PAYLOAD)                                                                  |
+----------------+-----+-------------------------------------------------------------------------------+
| Interested     |  2  | (NO PAYLOAD)                                                                  |
+----------------+-----+-------------------------------------------------------------------------------+
| Not Interested |  3  | (NO PAYLOAD)                                                                  |
+----------------+-----+-------------------------------------------------------------------------------+
| Have           |  4  | (PAYLOAD: 4 byte piece index field)                                           |
+----------------+-----+-------------------------------------------------------------------------------+
| Bitfield       |  5  | (PAYLOAD: bitfield as payload; first byte corresponds to indices 0-7, etc)    |
+----------------+-----+-------------------------------------------------------------------------------+
| Request        |  6  | (PAYLOAD: 4 byte piece index field)                                           |
+----------------+-----+-------------------------------------------------------------------------------+
| Piece          |  7  | (PAYLOAD: 4 byte piece index field + content of piece)                        |
+----------------+-----+-------------------------------------------------------------------------------+

