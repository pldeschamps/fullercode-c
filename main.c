#include "fullercode.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

/* Haversine distance in metres between two lat/lon points. */
static double haversine_m(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371010.0;
    const double to_rad = 3.14159265358979323846 / 180.0;
    double dlat = (lat2 - lat1) * to_rad;
    double dlon = (lon2 - lon1) * to_rad;
    double a = sin(dlat/2)*sin(dlat/2)
             + cos(lat1*to_rad)*cos(lat2*to_rad)*sin(dlon/2)*sin(dlon/2);
    return 2.0 * R * asin(sqrt(a));
}

static void run_test(const char *label, double lat, double lon, int len) {
    char code[32];
    double dec_lat, dec_lon;

    if (fullergeocoding(lat, lon, (unsigned short)len, code) != 0) {
        printf("%-30s  ENCODE ERROR\n", label);
        return;
    }
    if (fullergeodecoding(code, &dec_lat, &dec_lon) != 0) {
        printf("%-30s  code=%-14s  DECODE ERROR\n", label, code);
        return;
    }
    double err_m = haversine_m(lat, lon, dec_lat, dec_lon);
    printf("%-30s  code=%-14s  decoded=(%.6f, %.6f)  err=%.1f m\n",
           label, code, dec_lat, dec_lon, err_m);
}

static void run_NATO_test(const char *label, double lat, double lon) {
    char code[32];
    double dec_lat, dec_lon;

    if (fullerNATOcoding(lat, lon, code) != 0) {
        printf("%-30s  ENCODE ERROR\n", label);
        return;
    }
    if (fullerNATOdecoding(code, &dec_lat, &dec_lon) != 0) {
        printf("%-30s  code=%-14s  DECODE ERROR\n", label, code);
        return;
    }
    double err_m = haversine_m(lat, lon, dec_lat, dec_lon);
    printf("%-30s  code=%-14s  decoded=(%.6f, %.6f)  err=%.1f m\n",
           label, code, dec_lat, dec_lon, err_m);
}

static void run_bin_test(const char *label, double lat, double lon) {
    uint64_t bin = fullerbingeocoding(lat, lon);
    if (bin == 0) {
        printf("%-30s  BIN ENCODE ERROR\n", label);
        return;
    }
    double dec_lat, dec_lon;
    if (fullerbingeodecoding(bin, &dec_lat, &dec_lon) != 0) {
        printf("%-30s  bin=0x%016llx  BIN DECODE ERROR\n", label, (unsigned long long)bin);
        return;
    }
    double err_m = haversine_m(lat, lon, dec_lat, dec_lon);
    printf("%-30s  bin=0x%016llx  decoded=(%.6f, %.6f)  err=%.1f m\n",
           label, (unsigned long long)bin, dec_lat, dec_lon, err_m);
    printf("lat-lat=%.9f deg, lon-lon=%.9f deg\n", lat - dec_lat, lon - dec_lon);
    printf("lat-lat=%.9f mm, lon-lon=%.9f mm\n", (lat - dec_lat) * 111120000, (lon - dec_lon) * 111120000 * cos(lat * (3.14159265358979323846 / 180.0)));
}

static int run_b64_no_collision_test(int latitude) {
    char dms[23], code[9];
    int second, failures = 0;

    for (second = 0; second < 30; second++) {
        char decoded_dms[23];
        snprintf(dms, sizeof dms, "%02d 00 00 N 034 12 %02d W", latitude, second);
        if (fullerB64coding(dms, code) != 0 ||
            fullerB64decoding(code, decoded_dms) != 0) {
            printf("  latitude %d: ERROR for %s\n", latitude, dms);
            failures++;
            continue;
        }
        if (strcmp(dms, decoded_dms) != 0) {
            printf("  latitude %d: FAIL %s -> %s -> %s\n",
                   latitude, dms, code, decoded_dms);
            failures++;
        }
    }
    printf("  latitude %d deg: %s (30 longitudes successives)\n",
           latitude, failures ? "ECHEC" : "OK");
    return failures;
}

int main(void) {
    printf("=== fullercode geocoding tests ===\n\n");

    /* Basic round-trip for well-known locations */
    run_test("Paris (6 chars)",         48.8566,   2.3522,   6);
    run_test("Paris (10 chars)",        48.8566,   2.3522,  10);
    run_test("Paris (12 chars)",        48.8566,   2.3522,  12);
    run_test("New York (8 chars)",      40.7128,  -74.0060,  8);
    run_test("Tokyo (8 chars)",         35.6895,  139.6917,  8);
    run_test("Sydney (8 chars)",       -33.8688,  151.2093,  8);
    run_test("North Pole (6 chars)",    90.0,       0.0,     6);
    run_test("South Pole (6 chars)",   -90.0,       0.0,     6);
    run_test("Null Island (6 chars)",    0.0,       0.0,     6);
    run_test("Antimeridian E (6 c)",     0.0,     180.0,     6);
    run_test("Antimeridian W (6 c)",     0.0,    -180.0,     6);

    /* NATO encoding smoke test */
    printf("\n=== NATO smoke test ===\n");
    {
        char nato_code[32];
        if (fullerNATOcoding(48.8566, 2.3522, nato_code) == 0) {
            printf("  NATO Paris -> %s\n", nato_code);
        } else {
            printf("  NATO Paris -> ERROR\n");
        }
    }
    run_NATO_test("Paris",        48.8566,   2.3522);
    run_NATO_test("New York",     40.7128,  -74.0060);
    run_NATO_test("Tokyo",        35.6895,  139.6917);
    run_NATO_test("Sydney",      -33.8688,  151.2093);
    run_NATO_test("North Pole",   90.0,       0.0);

    printf("\n=== fullercode binary geocoding tests (14 levels) ===\n\n");
    run_bin_test("Paris (Lat+, Lon+)",    48.8566,   2.3522);
    run_bin_test("New York (Lat+, Lon-)",  40.7128, -74.0060);
    run_bin_test("Sydney (Lat-, Lon+)",   -33.8688, 151.2093);
    run_bin_test("Santiago (Lat-, Lon-)", -33.4489, -70.6693);
    run_bin_test("Faa'a (Lat-, Lon-)",  -17.5453265, -149.5946393);
    run_bin_test("Forbidden City (Lat+, Lon+)",  39.9169465, 116.3971213);
    run_bin_test("1 cm", 0.00000009, 0.00000009); /* should be ~1 cm from (0,0) */
    run_bin_test("1 cm", 40.00000009, 30.00000009); /* should be ~1 cm from (0,0) */
    run_bin_test("1 cm", 70.00000009, -179.9999999); /* should be ~1 cm from (0,0) */
    run_bin_test("North pole (Lat+, Lon+)", 90.0, 0.0);
    run_bin_test("South pole (Lat+, Lon-)", -90.0, 0.0);
    run_bin_test("Null island (Lat+, Lon+)", 0.0, 0.0);
    run_bin_test("Antimeridian E (Lat+, Lon+)", 0.0, 180.0);

    printf("\n=== Fuller 48 bits / Base64: test sans collision ===\n\n");
    {
        char sample[9], sample_dms[23];
        if (fullerB64coding("79 58 59 N 034 12 34 W", sample) == 0 &&
            fullerB64decoding(sample, sample_dms) == 0) {
            printf("  exemple: 79 58 59 N 034 12 34 W -> %s -> %s\n",
                   sample, sample_dms);
        }
    }
    run_b64_no_collision_test(70);
    run_b64_no_collision_test(75);
    run_b64_no_collision_test(80);
    run_b64_no_collision_test(81);
    run_b64_no_collision_test(82);
    run_b64_no_collision_test(83);
    run_b64_no_collision_test(84);
    run_b64_no_collision_test(85);

    /* Decode-only test of a known code */
    printf("\n=== decode only ===\n");
    const char *known_codes[] = { "C", "CM", "CM3", "CM3F", "CM3FA2", NULL };
    for (int i = 0; known_codes[i]; i++) {
        double lat, lon;
        if (fullergeodecoding(known_codes[i], &lat, &lon) == 0)
            printf("  decode(\"%s\") -> (%.6f, %.6f)\n", known_codes[i], lat, lon);
        else
            printf("  decode(\"%s\") -> ERROR\n", known_codes[i]);
    }

    /* Error cases */
    printf("\n=== error cases ===\n");
    char buf[32];
    printf("  encode len=0:  %d (expected -1)\n", fullergeocoding(0,0,0,buf));
    printf("  encode lat>90: %d (expected -1)\n", fullergeocoding(91,0,6,buf));
    printf("  decode \"\": %d (expected -1)\n",   fullergeodecoding("", &(double){0}, &(double){0}));
    printf("  decode \"?\": %d (expected -1)\n",  fullergeodecoding("?", &(double){0}, &(double){0}));

    return 0;
}
