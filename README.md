# MySQL UDF Socket

Simple MySQL/MariaDB plugin for sending data to TCP socket and UNIX domain socket.

## Installation

To compile plugin, MySQL client library has to be available.
On Debian/Ubuntu it can be installed with following command.

```sh
apt install libmysqlclient-dev
```

Compile and install.

```sh
make
make install
```

Create SQL functions.

```sql
DROP FUNCTION IF EXISTS mysql_udf_socket_info;
DROP FUNCTION IF EXISTS mysql_udf_socket_send;

CREATE FUNCTION mysql_udf_socket_info RETURNS string SONAME 'mysql_udf_socket.so';
CREATE FUNCTION mysql_udf_socket_send RETURNS string SONAME 'mysql_udf_socket.so';
```

## Usage

```sql
-- print plugin info
SELECT mysql_udf_socket_info();
-- send data to tcp socket
SELECT mysql_udf_socket_send("tcp://127.0.0.1:8080", "hello");
-- send data to unix domain socket
SELECT mysql_udf_socket_send("unix:///tmp/test.sock", "hello");
```

## Restrictions

- maximum data length is 128 characters
- request/response timeout is 2 seconds

## Testing

TCP socket.

```sh
echo -ne "OK" | netcat -lp 8080 -q 1
```

UNIX domain socket.

```sh
socat -v UNIX-LISTEN:/tmp/test_socket.sock,mode=777,reuseaddr EXEC:"echo -n OK"
```

## License

Licensed under MIT License.