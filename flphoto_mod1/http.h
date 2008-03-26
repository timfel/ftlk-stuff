//
// "$Id: http.h 321 2005-01-23 03:52:44Z easysw $"
//
// Hyper-Text Transport Protocol class definitions for flPhoto.
//
// Copyright 2002-2005 by Michael Sweet.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#ifndef _HTTP_H_
#  define _HTTP_H_

//
// Include necessary headers...
//

#  include <time.h>
#  if defined(WIN32) && !defined(__CYGWIN__)
#    include <winsock.h>
#  else
#    include <unistd.h>
#    include <sys/types.h>
#    include <sys/time.h>
#    include <sys/socket.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <arpa/inet.h>
#    include <netinet/in_systm.h>
#    include <netinet/ip.h>
#    include <netinet/tcp.h>
#  endif // WIN32 && !__CYGWIN__

#  include "md5.h"


//
// Limits...
//

#  define HTTP_MAX_URI	1024	// Max length of URI string
#  define HTTP_MAX_HOST	256	// Max length of hostname string
#  define HTTP_MAX_BUFFER	2048	// Max length of data buffer
#  define HTTP_MAX_VALUE	256	// Max header field value length


//
// HTTP state values...
//

typedef enum			// States are server-oriented
{
  HTTP_WAITING,			// Waiting for command
  HTTP_OPTIONS,			// OPTIONS command, waiting for blank line
  HTTP_GET,			// GET command, waiting for blank line
  HTTP_GET_SEND,		// GET command, sending data
  HTTP_HEAD,			// HEAD command, waiting for blank line
  HTTP_POST,			// POST command, waiting for blank line
  HTTP_POST_RECV,		// POST command, receiving data
  HTTP_POST_SEND,		// POST command, sending data
  HTTP_PUT,			// PUT command, waiting for blank line
  HTTP_PUT_RECV,		// PUT command, receiving data
  HTTP_DELETE,			// DELETE command, waiting for blank line
  HTTP_TRACE,			// TRACE command, waiting for blank line
  HTTP_MKCOL,			// MKCOL command, waiting for blank line
  HTTP_STATUS			// Command complete, sending status
} HTTPState;


//
// HTTP version numbers...
//

typedef enum
{
  HTTP_0_9 = 9,			// HTTP/0.9
  HTTP_1_0 = 100,		// HTTP/1.0
  HTTP_1_1 = 101		// HTTP/1.1
} HTTPVersion;


//
// HTTP keep-alive values...
//

typedef enum
{
  HTTP_KEEPALIVE_OFF = 0,
  HTTP_KEEPALIVE_ON
} HTTPKeepAlive;


//
// HTTP transfer encoding values...
//

typedef enum
{
  HTTP_ENCODE_LENGTH,		// Data is sent with Content-Length
  HTTP_ENCODE_CHUNKED		// Data is chunked
} HTTPEncoding;


//
// HTTP encryption values...
//

typedef enum
{
  HTTP_ENCRYPT_IF_REQUESTED,	// Encrypt if requested (TLS upgrade)
  HTTP_ENCRYPT_NEVER,		// Never encrypt
  HTTP_ENCRYPT_REQUIRED,	// Encryption is required (TLS upgrade)
  HTTP_ENCRYPT_ALWAYS		// Always encrypt (SSL)
} HTTPEncryption;


//
// HTTP authentication types...
//

typedef enum
{
  HTTP_AUTH_NONE,		// No authentication in use
  HTTP_AUTH_BASIC,		// Basic authentication in use
  HTTP_AUTH_MD5,		// Digest authentication in use
  HTTP_AUTH_MD5_SESS,		// MD5-session authentication in use
  HTTP_AUTH_MD5_INT,		// Digest authentication in use for body
  HTTP_AUTH_MD5_SESS_INT	// MD5-session authentication in use for body
} HTTPAuth;


//
// HTTP status codes...
//

typedef enum
{
  HTTP_ERROR = -1,		// An error response from httpXxxx()

  HTTP_CONTINUE = 100,		// Everything OK, keep going...
  HTTP_SWITCHING_PROTOCOLS,	// HTTP upgrade to TLS/SSL
  HTTP_PROCESSING,		// Processing

  HTTP_OK = 200,		// OPTIONS/GET/HEAD/POST/TRACE command was successful
  HTTP_CREATED,			// PUT command was successful
  HTTP_ACCEPTED,		// DELETE command was successful
  HTTP_NOT_AUTHORITATIVE,	// Information isn't authoritative
  HTTP_NO_CONTENT,		// Successful command, no new data
  HTTP_RESET_CONTENT,		// Content was reset/recreated
  HTTP_PARTIAL_CONTENT,		// Only a partial file was recieved/sent
  HTTP_MULTI_STATUS,		// Multi-status

  HTTP_MULTIPLE_CHOICES = 300,	// Multiple files match request
  HTTP_MOVED_PERMANENTLY,	// Document has moved permanently
  HTTP_MOVED_TEMPORARILY,	// Document has moved temporarily
  HTTP_SEE_OTHER,		// See this other link...
  HTTP_NOT_MODIFIED,		// File not modified
  HTTP_USE_PROXY,		// Must use a proxy to access this URI

  HTTP_BAD_REQUEST = 400,	// Bad request
  HTTP_UNAUTHORIZED,		// Unauthorized to access host
  HTTP_PAYMENT_REQUIRED,	// Payment required
  HTTP_FORBIDDEN,		// Forbidden to access this URI
  HTTP_NOT_FOUND,		// URI was not found
  HTTP_METHOD_NOT_ALLOWED,	// Method is not allowed
  HTTP_NOT_ACCEPTABLE,		// Not Acceptable
  HTTP_PROXY_AUTHENTICATION,	// Proxy Authentication is Required
  HTTP_REQUEST_TIMEOUT,		// Request timed out
  HTTP_CONFLICT,		// Request is self-conflicting
  HTTP_GONE,			// Server has gone away
  HTTP_LENGTH_REQUIRED,		// A content length or encoding is required
  HTTP_PRECONDITION,		// Precondition failed
  HTTP_REQUEST_TOO_LARGE,	// Request entity too large
  HTTP_URI_TOO_LONG,		// URI too long
  HTTP_UNSUPPORTED_MEDIATYPE,	// The requested media type is unsupported
  HTTP_UNPROCESSABLE_ENTITY = 422,
				// Unprocessable entity
  HTTP_LOCKED,			// Locked
  HTTP_FAILED_DEPENDENCY,	// Failed dependency
  HTTP_UPGRADE_REQUIRED = 426,	// Upgrade to SSL/TLS required

  HTTP_SERVER_ERROR = 500,	// Internal server error
  HTTP_NOT_IMPLEMENTED,		// Feature not implemented
  HTTP_BAD_GATEWAY,		// Bad gateway
  HTTP_SERVICE_UNAVAILABLE,	// Service is unavailable
  HTTP_GATEWAY_TIMEOUT,		// Gateway connection timed out
  HTTP_NOT_SUPPORTED,		// HTTP version not supported
  HTTP_INSUFFICIENT_STORAGE = 507
				// Insufficient storage space
} HTTPStatus;


//
// HTTP field names...
//

typedef enum
{
  HTTP_FIELD_UNKNOWN = -1,
  HTTP_FIELD_ACCEPT_LANGUAGE,
  HTTP_FIELD_ACCEPT_RANGES,
  HTTP_FIELD_AUTHORIZATION,
  HTTP_FIELD_CONNECTION,
  HTTP_FIELD_CONTENT_ENCODING,
  HTTP_FIELD_CONTENT_LANGUAGE,
  HTTP_FIELD_CONTENT_LENGTH,
  HTTP_FIELD_CONTENT_LOCATION,
  HTTP_FIELD_CONTENT_MD5,
  HTTP_FIELD_CONTENT_RANGE,
  HTTP_FIELD_CONTENT_TYPE,
  HTTP_FIELD_CONTENT_VERSION,
  HTTP_FIELD_COOKIE,
  HTTP_FIELD_DATE,
  HTTP_FIELD_EXPECT,
  HTTP_FIELD_HOST,
  HTTP_FIELD_IF_MODIFIED_SINCE,
  HTTP_FIELD_IF_UNMODIFIED_SINCE,
  HTTP_FIELD_KEEP_ALIVE,
  HTTP_FIELD_LAST_MODIFIED,
  HTTP_FIELD_LINK,
  HTTP_FIELD_LOCATION,
  HTTP_FIELD_RANGE,
  HTTP_FIELD_REFERER,
  HTTP_FIELD_RETRY_AFTER,
  HTTP_FIELD_SET_COOKIE,
  HTTP_FIELD_TRANSFER_ENCODING,
  HTTP_FIELD_UPGRADE,
  HTTP_FIELD_USER_AGENT,
  HTTP_FIELD_WWW_AUTHENTICATE,
  HTTP_FIELD_MAX
} HTTPField;
  

//
// HTTP connection structure...
//

class HTTP
{
  int			fd;		// File descriptor for this socket
  int			blocking;	// To block or not to block
  int			error;		// Last error on read
  time_t		activity;	// Time since last read/write
  HTTPState		state;		// State of client
  HTTPStatus		status;		// Status of last request
  HTTPVersion		version;	// Protocol version
  HTTPKeepAlive		keep_alive;	// Keep-alive supported?
  struct sockaddr_in	hostaddr;	// Address of connected host
  char			hostname[HTTP_MAX_HOST],
  					// Name of connected host
			fields[HTTP_FIELD_MAX][HTTP_MAX_VALUE];
					// Field values
  char			*data;		// Pointer to data buffer
  HTTPEncoding		data_encoding;	// Chunked or not
  int			data_remaining;	// Number of bytes left
  int			used;		// Number of bytes used in buffer
  char			buffer[HTTP_MAX_BUFFER];
					// Buffer for messages
  int			authtype;	// Authentication in use
  MD5			md5_state;	// MD5 state
  char			nonce[HTTP_MAX_VALUE];
					// Nonce value
  int			nonce_count;	// Nonce count
  void			*tls;		// TLS state information
  HTTPEncryption	encryption;	// Encryption requirements

  int			send(HTTPState request, const char *uri);
  int			upgrade();

  static void		initialize();

  public:

  HTTP(const char *h, int port = 80, HTTPEncryption e = HTTP_ENCRYPT_IF_REQUESTED);
  ~HTTP();

  int			check();
  void			clear_fields();
  void			flush();
  int			get_blocking() { return blocking; }
  HTTPEncryption	get_encryption() { return encryption; }
  int			get_error() { return error; }
  int			get_fd() { return fd; }
  const char		*get_field(HTTPField field) { return fields[field]; }
  int			get_content_length();
  const char		*get_hostname() { return hostname; }
  HTTPState		get_state() { return state; }
  HTTPStatus		get_status() { return status; }
  char			*get_sub_field(HTTPField field, const char *name,
			               char *value, int length);
  char			*gets(char *line, int length);
  int			printf(const char *format, ...);
  int			read(char *buffer, int length);
  int			reconnect();
  int			send_delete(const char *uri);
  int			send_get(const char *uri);
  int			send_head(const char *uri);
  int			send_mkcol(const char *uri);
  int			send_options(const char *uri);
  int			send_post(const char *uri);
  int			send_put(const char *uri);
  int			send_trace(const char *uri);
  void			set_blocking(int b) { blocking = b; }
  int			set_encryption(HTTPEncryption e);
  void			set_field(HTTPField field, const char *value);
  HTTPStatus		update();
  int			write(const char *buffer, int length);

  static char		*decode64(char *out, int outlen, const char *in);
  static char		*encode64(char *out, int outlen, const char *in);
  static HTTPField	field_number(const char *name);
  static const char	*get_date_string(time_t t);
  static time_t		get_date_time(const char *s);
  static char		*md5(const char *username, const char *realm,
			     const char *passwd, char *s, int slen);
  static char		*md5_final(const char *nonce, const char *scheme,
			           const char *resource, char *s, int slen);
  static char		*md5_string(const MD5Byte *sum, char *s, int slen);
  static void		separate(const char *uri, char *scheme, int schemelen,
			         char *username, int usernamelen,
				 char *host, int hostlen,
				 int *port,
				 char *resource, int resourcelen);
  static const char	*status_string(HTTPStatus status);
};


#endif // !_HTTP_H_

//
// End of "$Id: http.h 321 2005-01-23 03:52:44Z easysw $".
//
