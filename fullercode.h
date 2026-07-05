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

#endif /* FULLERCODE_H */
