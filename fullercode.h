#ifndef FULLERCODE_H
#define FULLERCODE_H

#include <stdint.h>

/*
 * fullergeocoding - encode geographic coordinates to a fullercode string.
 *
 * lat_deg : latitude in decimal degrees  [-90, 90]
 * lon_deg : longitude in decimal degrees [-180, 180]
 * len     : number of characters to generate [1, 20]
 * out     : caller-supplied buffer of at least (len + 1) bytes
 *
 * Returns 0 on success, -1 on invalid input.
 *
 * The fullercode alphabet is:
 *   first character : one of 20 icosahedron face IDs  (CMPX9V8TR7M3FA2H5NSJK)
 *   subsequent chars: one of 16 subdivision IDs       (CM3FA2H5PX9V8TR7)
 * Each additional character multiplies precision by ~4.
 */
int fullergeocoding(double lat_deg, double lon_deg, uint16_t len, char *out);

/*
 * fullerNATOcoding - encode geographic coordinates to a 7-character NATO fullercode.
 *
 * lat_deg : latitude in decimal degrees  [-90, 90]
 * lon_deg : longitude in decimal degrees [-180, 180]
 * out     : caller-supplied buffer of at least 8 bytes
 *
 * Returns 0 on success, -1 on invalid input.
 */
int fullerNATOcoding(double lat_deg, double lon_deg, char *out);

/*
 * fullerNATOdecoding - decode a NATO fullercode string to geographic coordinates.
 *
 * code    : null-terminated NATO fullercode string (1–11 characters)
 * lat_deg : output latitude in decimal degrees
 * lon_deg : output longitude in decimal degrees
 *
 * Returns 0 on success, -1 on invalid code.
 */
int fullerNATOdecoding(const char *code, double *lat_deg, double *lon_deg);

/*
 * fullergeodecoding - decode a fullercode string to geographic coordinates.
 *
 * code    : null-terminated fullercode string (1–20 characters)
 * lat_deg : output latitude in decimal degrees
 * lon_deg : output longitude in decimal degrees
 *
 * Returns 0 on success, -1 on invalid code.
 * The decoded position is the centroid of the encoded triangle.
 */
int fullergeodecoding(const char *code, double *lat_deg, double *lon_deg);

/*
 * fullerbingeocoding - encode coordinates to a 64-bit binary format.
 * Returns the 64-bit code, or 0 on error (invalid coordinates).
 */
uint64_t fullerbingeocoding(double lat_deg, double lon_deg);

/*
 * fullerbingeodecoding - decode from a 64-bit binary format.
 * bin     : 64-bit fullercode
 * lat_deg : output latitude
 * lon_deg : output longitude
 * Returns 0 on success, -1 on invalid face ID.
 */
int fullerbingeodecoding(uint64_t bin, double *lat_deg, double *lon_deg);

/**
 * Converts a DMS position to decimal degrees.
 *
 * @param[in]  dms      Null-terminated string containing exactly 22 characters
 *                      in the format
 *                      "DD MM SS H DDD MM SS H".
 *                      Latitude uses two digits and N or S; longitude uses
 *                      three digits and E or W.
 *                      Example: "79 58 59 N 034 12 34 W".
 * @param[out] lat_deg  Latitude in decimal degrees, in [-90, 90].
 * @param[out] lon_deg  Longitude in decimal degrees, in [-180, 180].
 *
 * @return 0 on success, or -1 if a pointer or the format is invalid.
 */
int degminsecToDeg(const char *dms, double *lat_deg, double *lon_deg);

/**
 * Converts decimal degrees to a DMS position.
 *
 * @param[in]  lat_deg  Latitude in decimal degrees, in [-90, 90].
 * @param[in]  lon_deg  Longitude in decimal degrees, in [-180, 180].
 * @param[out] out      Buffer of at least 23 bytes. The function writes
 *                      22 characters in the format "DD MM SS H DDD MM SS H",
 *                      followed by '\0'. Seconds are rounded to the nearest
 *                      integer.
 *
 * @return 0 on success, or -1 if an argument is invalid.
 */
int degTodegminsec(double lat_deg, double lon_deg, char out[23]);

/**
 * Encodes a position as a compact 48-bit Fuller code.
 *
 * Bits 47 through 40 contain the initial face identifier. Bits 39 through 0
 * contain ten 4-bit sub-triangle indices. The upper 16 bits of the returned
 * uint64_t are always zero.
 *
 * @param[in] lat_deg  Latitude in decimal degrees, in [-90, 90].
 * @param[in] lon_deg  Longitude in decimal degrees, in [-180, 180].
 * @return The code in the lower 48 bits, or 0 on error.
 */
uint64_t fuller48bitcoding(double lat_deg, double lon_deg);

/**
 * Decodes a compact 48-bit Fuller code to decimal degrees.
 *
 * @param[in]  code     Fuller code; the upper 16 bits must be zero.
 * @param[out] lat_deg  Decoded latitude in decimal degrees.
 * @param[out] lon_deg  Decoded longitude in decimal degrees.
 * @return 0 on success, or -1 if the code or a pointer is invalid.
 */
int fuller48bitdecoding(uint64_t code, double *lat_deg, double *lon_deg);

/**
 * Converts 48 bits to eight RFC 4648 Base64 characters without '=' padding.
 *
 * @param[in]  code  Value for which only the lower 48 bits may be set.
 * @param[out] out   Buffer of at least 9 bytes: 8 characters followed by '\0'.
 * @return 0 on success, or -1 if an argument is invalid.
 */
int binToB64(uint64_t code, char out[9]);

/**
 * Converts eight RFC 4648 Base64 characters to a 48-bit value.
 *
 * @param[in]  b64   Null-terminated string containing exactly 8 characters,
 *                   without '=' padding.
 * @param[out] code  Decoded binary value in the lower 48 bits.
 * @return 0 on success, or -1 if the string or a pointer is invalid.
 */
int b64ToBin(const char b64[9], uint64_t *code);

/**
 * Encodes a DMS position directly as a Base64 Fuller code.
 *
 * @param[in]  dms  22-character DMS string in the format
 *                  "DD MM SS H DDD MM SS H", followed by '\0'.
 * @param[out] out  Buffer of at least 9 bytes: 8 Base64 characters then '\0'.
 * @return 0 on success, or -1 if an argument or the format is invalid.
 */
int fullerB64coding(const char *dms, char out[9]);

/**
 * Decodes a Base64 Fuller code directly to a DMS position.
 *
 * @param[in]  b64  String of exactly 8 Base64 characters followed by '\0'.
 * @param[out] dms  Buffer of at least 23 bytes. The function writes the position
 *                  in the format "DD MM SS H DDD MM SS H" (22 characters),
 *                  followed by '\0'.
 * @return 0 on success, or -1 if an argument or the code is invalid.
 */
int fullerB64decoding(const char *b64, char dms[23]);

#endif /* FULLERCODE_H */
