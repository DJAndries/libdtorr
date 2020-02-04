## TODO

* tcp server
* peer protocol
  * bitfield
  * request / piece
  * cancel
  * choking / interested
* free
  * nodes
  * torrent
* dht
* upnp in another project that consumes this library
* visiblity=hidden
* bsd sock support for network layer
* tracker intervals
* support init for nested dirs
* keepalives
* reassign half done pieces to another peer
* check socket buffer avail before sending

## DONE

* bencoding parsing
* metadata loading
* hashing
* cross-platform networking layer - win first
* tracker requests
* fs init
* fs write / piece map
* peer protocol
  * handshake