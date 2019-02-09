# Nagios check to check availability and time to send MQTT data
## Preface
A Nagios check to check the availability of a MQTT broker and measures the
time to send and receive MQTT data. The time between sending and receiving
the payload is reported as performance data `mqtt_rtt`.

Anonymous connections, user/password authentications and SSL client certificates can be used to authenticate at the MQTT broker.

## Requirements

* `libuuid` which is provides by the `util-linux` package
* `libmosquitto` from https://mosquitto.org/
* Linux kernel >= 2.6 for high precision time measurement (`clock_gettime`)

## Build requirements

* `cmake`
* development files of the libraries listed above in the "Requirements" paragraph

## Command line parameters

* `-h` / `--help` - Help text
* `-H <host>` / `--host=<host>` - Name or address of the MQTT broker (**mandatory**)
* `-p <port>` / `--port=<port>` - Port of the MQTT broker (Default: 1883)
* `-c <cert>` / `--cert=<cert>` - File containing the public key of the client certificate
* `-k <key>` / `--key=<key>` - File containing the (*unencrypted*) private key of the client certificate
* `-C <ca>` / `--ca=<ca>` - File containing the public key of the CA certificate signing the MQTT broker SSL certificate
* `-D <cadir>` / `--cadir=<dir>` - Directory with public keys of CA certificates which also cotains the CA certificate of the CA signing the MQTT broker SSL certificate. (Default: `/etc/ssl/certs`)
* `-i` / `--insecure` - Don't validate SSL certificate of the MQTT broker
* `-Q <qos>` / `--qos=<qos>` - MQTT QoS to use for messages (see [MQTT Essentials Part 6: Quality of Service 0, 1 & 2](https://www.hivemq.com/blog/mqtt-essentials-part-6-mqtt-quality-of-service-levels)). Allowed values: 0, 1 or 2 (Default: 0)
* `-T <topic>` / `--topic=<topic>` - MQTT topic to send message to (Default: `nagios/check_mqtt`), make sure ACLs are set correctly (readwrite)
* `-t <sec>` / `--timeout=<sec>` - Timeout for connection setup, message send and receival (Default: 15 sec.)
* `-s` / `--ssl` - Connect to the MQTT broker using a SSL/TLS encrypted connection
* `-u <user>` / `--user=<user>` - Authenticate as <user>
* `-P <password>` / `--password=<password>` - Authenticate with <password>
* `-f <file>` / `--password-file=<file>` - Read <password> from <file> to prevent the password to be displayed in the process table. The first line found in <file> is threated as the password to use for authentication
* `-w <ms>` / `--warn=<ms>` - Warning threshold for time between sending and receiving the test payload (Default: 250ms)
* `-W <ms>` / `--critical=<ms>` - Critical threshold for time between sending and receiving the test payload (Default: 500ms)
* `-K <sec>` / `--keepalive=<sec>` - Interval to send MQTT PING probes after no MQTT messages are exchanged between client and server

**Note:** If SSL/TLS connection is used (`--ssl`) the CA certificate of the MQTT broker *MUST* be found. Either in the file provided by `-C` / `--ca` or in the CA directory (`-D` / `--cadir`).

## License
This program is licenses under [GLPv3](http://www.gnu.org/copyleft/gpl.html).

