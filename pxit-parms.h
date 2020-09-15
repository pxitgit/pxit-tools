/* pxit-parms.h - defines properties pxit images
 * 
 * Resolution:  720x480 pixels
 * Cell size:     16x16 pixels
 * 
 * Packet size: 337 bytes
     * header:    5 bytes
     * data:    328 bytes
     * checksum:  4 bytes
 */
 
const int width      = 720;
const int height     = 480;
const int cellsize   =  16;
const int packetSize = 337; //size of packet (bytes)
const int csumSize   =   4; //4-byte checksum
const int headerSize =   5; //size of packet header
const int blockSize = packetSize - headerSize - csumSize;


