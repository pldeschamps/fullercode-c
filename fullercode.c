#include "fullercode.h"

#include <math.h>
#include <string.h>

#define RADIUS     6371010.0
#define TRANSITION 18    /* levels 0..(TRANSITION-1) use 3D cross-product tests;
                            level TRANSITION and above use 2D barycentric subdivision */
#define MAX_LEN    20    /* practical precision limit (beyond ~15 float64 noise dominates) */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ── 3-D vector ─────────────────────────────────────────────────────────── */

typedef struct { double x, y, z; } Vec3;

static Vec3 cross3(Vec3 a, Vec3 b) {
    Vec3 r = { a.y*b.z - a.z*b.y,
               a.z*b.x - a.x*b.z,
               a.x*b.y - a.y*b.x };
    return r;
}

static double dot3(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

/* Average two points then project back onto the sphere scaled by RADIUS. */
static Vec3 sph_mid(Vec3 p, Vec3 q) {
    double x = p.x + q.x, y = p.y + q.y, z = p.z + q.z;
    double L = sqrt(x*x + y*y + z*z);
    Vec3 r = { x*RADIUS/L, y*RADIUS/L, z*RADIUS/L };
    return r;
}

/* Centroid of three points projected onto the sphere scaled by RADIUS. */
static Vec3 sph_center(Vec3 a, Vec3 b, Vec3 c) {
    double x = a.x+b.x+c.x, y = a.y+b.y+c.y, z = a.z+b.z+c.z;
    double L = sqrt(x*x + y*y + z*z);
    Vec3 r = { x*RADIUS/L, y*RADIUS/L, z*RADIUS/L };
    return r;
}

/* ── Icosahedron data ───────────────────────────────────────────────────── */

/* Unit-sphere coordinates of the 12 vertices, indexed by vertex id (1-12).
 * Slot 0 is unused. Coordinates are from icosahedron.json. */
static const double RAW_VERTS[13][3] = {
    { 0.0,                   0.0,                   0.0                  }, /* 0 – unused */
    {-0.414682220851542,      0.655962408702304,      0.630675807431286   }, /* 1  */
    { 0.420152428828912,      0.078145249043253,      0.904082549660778   }, /* 2  */
    { 0.5188367348275258,     0.8354203781459196,     0.1813318349657353  }, /* 3  */
    { 0.9950094390590583,    -0.09134780014578021,    0.04014717414595104 }, /* 4  */
    { 0.3557813991100285,    -0.8435800034654891,     0.40223422753474947 }, /* 5  */
    {-0.5154559603719798,    -0.3817168942575905,     0.7672009942351089  }, /* 6  */
    { 0.414682220851542,     -0.655962408702304,     -0.630675807431286   }, /* 7  */
    {-0.420152428828912,     -0.078145249043253,     -0.904082549660778   }, /* 8  */
    {-0.5188367348275258,    -0.8354203781459196,    -0.1813318349657353  }, /* 9  */
    {-0.9950094390590583,     0.09134780014578021,   -0.04014717414595104 }, /* 10 */
    {-0.3557813991100285,     0.8435800034654891,    -0.40223422753474947 }, /* 11 */
    { 0.5154559603719798,     0.3817168942575905,    -0.7672009942351089  }, /* 12 */
};

static Vec3 ico_vert(int id) {
    Vec3 v = { RAW_VERTS[id][0]*RADIUS,
               RAW_VERTS[id][1]*RADIUS,
               RAW_VERTS[id][2]*RADIUS };
    return v;
}

/* 20 icosahedron face definitions (from icosahedron.json). */
typedef struct {
    char id;
    int  v[3];       /* vertex IDs */
    char stids[17];  /* 16-char subtrianglesIds string + null */
} IcoFace;

static const IcoFace ICO_FACES[20] = {
    {'P', {2,5,6},   "C5PX9V8TR7M3FA2H"},
    {'M', {2,4,5},   "CA2H5PX9V8TR7M3F"},
    {'X', {2,3,4},   "CM3FA2H5PX9V8TR7"},
    {'C', {2,1,3},   "CTR7M3FA2H5PX9V8"},
    {'N', {2,6,1},   "C9V8TR7M3FA2H5PX"},
    {'V', {12,4,3},  "C2AF3M7RT8V9XP5H"},
    {'5', {4,12,7},  "CA2H5PX9V8TR7M3F"},
    {'F', {7,5,4},   "CP5H2AF3M7RT8V9X"},
    {'S', {8,7,12},  "CP5H2AF3M7RT8V9X"},
    {'A', {5,7,9},   "C5PX9V8TR7M3FA2H"},
    {'J', {1,10,11}, "CTR7M3FA2H5PX9V8"},
    {'9', {6,9,10},  "C9V8TR7M3FA2H5PX"},
    {'H', {9,6,5},   "CV9XP5H2AF3M7RT8"},
    {'R', {8,9,7},   "CV9XP5H2AF3M7RT8"},
    {'3', {3,11,12}, "CM3FA2H5PX9V8TR7"},
    {'T', {11,3,1},  "C3M7RT8V9XP5H2AF"},
    {'K', {8,11,10}, "C3M7RT8V9XP5H2AF"},
    {'7', {10,1,6},  "CRT8V9XP5H2AF3M7"},
    {'8', {8,10,9},  "CRT8V9XP5H2AF3M7"},
    {'2', {8,12,11}, "C2AF3M7RT8V9XP5H"},
};
static const IcoFace ICO_FACES_NATO[20] = {
    {'P', {2,5,6},   "CWPXGVBTRYMEFAZH"},
    {'M', {2,4,5},   "CAZHWPXGVBTRYMEF"},
    {'X', {2,3,4},   "CMEFAZHWPXGVBTRY"},
    {'C', {2,1,3},   "CTRYMEFAZHWPXGVB"},
    {'N', {2,6,1},   "CGVBTRYMEFAZHWPX"},
    {'V', {12,4,3},  "CZAFEMYRTBVGXPWH"},
    {'W', {4,12,7},  "CAZHWPXGVBTRYMEF"},
    {'F', {7,5,4},   "CPWHZAFEMYRTBVGX"},
    {'S', {8,7,12},  "CPWHZAFEMYRTBVGX"},
    {'A', {5,7,9},   "CWPXGVBTRYMEFAZH"},
    {'J', {1,10,11}, "CTRYMEFAZHWPXGVB"},
    {'G', {6,9,10},  "CGVBTRYMEFAZHWPX"},
    {'H', {9,6,5},   "CVGXPWHZAFEMYRTB"},
    {'R', {8,9,7},   "CVGXPWHZAFEMYRTB"},
    {'E', {3,11,12}, "CMEFAZHWPXGVBTRY"},
    {'T', {11,3,1},  "CEMYRTBVGXPWHZAF"},
    {'K', {8,11,10}, "CEMYRTBVGXPWHZAF"},
    {'Y', {10,1,6},  "CRTBVGXPWHZAFEMY"},
    {'B', {8,10,9},  "CRTBVGXPWHZAFEMY"},
    {'Z', {8,12,11}, "CZAFEMYRTBVGXPWH"},
};

/* Binary face mapping order: M=0x10, X=0x11, ... S=0x23 */
static const char BIN_FACE_ORDER[] = "MXCNPFVT7H53J9A2KR8S";

/* ── Face state ─────────────────────────────────────────────────────────── */

/*
 * A Face holds the triangle state at one level of the hierarchy.
 * sp[0..14] and ids[0..15] are computed lazily by face_subdivide().
 *
 * Naming of the 15 subdivision points (matches Subtriangles.ts v[] array):
 *   sp[0] = a,      sp[1] = b,      sp[2] = c
 *   sp[3] = ab,     sp[4] = bc,     sp[5] = ac
 *   sp[6] = a_ab,   sp[7] = ac_ab,  sp[8] = ac_a
 *   sp[9] = b_bc,   sp[10]= ab_bc,  sp[11]= ab_b
 *   sp[12]= c_ac,   sp[13]= bc_ac,  sp[14]= bc_c
 */
typedef struct {
    Vec3 verts[3];   /* triangle vertices (RADIUS-scaled) */
    char stids[17];  /* subtrianglesIds (inherited from icosahedron face) */
    int  up;         /* parentOrientation: 1=true, 0=false */
    int  depth;      /* 0 = initial icosahedron face; 1+ = sub-face level */
    Vec3 sp[15];     /* subdivision points, filled by face_subdivide() */
    char ids[16];    /* character mapped to each of the 16 sub-triangles */
    int  ready;      /* 1 once sp and ids have been computed */
} Face;

/* Permutation applied to stids when depth > 0 and orientation is down.
 * Mirrors the pBox array from Subtriangles.ts. */
static const int PBOX[16] = {0,2,1,8,9,10,7,6,13,14,15,12,11,3,4,5};

/* sp[] indices of the three corners for each of the 16 sub-triangles.
 * Row k gives [sp_a, sp_b, sp_c] for sub-face k. */
static const int SF_VERTS[16][3] = {
    { 7,10,13}, /*  0: [ac_ab, ab_bc, bc_ac]  */
    { 0, 6, 8}, /*  1: [a,     a_ab,  ac_a ]  */
    { 7, 8, 6}, /*  2: [ac_ab, ac_a,  a_ab ]  */
    { 6, 3, 7}, /*  3: [a_ab,  ab,    ac_ab]  */
    {10, 7, 3}, /*  4: [ab_bc, ac_ab, ab   ]  */
    { 3,11,10}, /*  5: [ab,    ab_b,  ab_bc]  */
    {11, 1, 9}, /*  6: [ab_b,  b,     b_bc ]  */
    { 9,10,11}, /*  7: [b_bc,  ab_bc, ab_b ]  */
    {10, 9, 4}, /*  8: [ab_bc, b_bc,  bc   ]  */
    { 4,13,10}, /*  9: [bc,    bc_ac, ab_bc]  */
    {13, 4,14}, /* 10: [bc_ac, bc,    bc_c ]  */
    {12,14, 2}, /* 11: [c_ac,  bc_c,  c    ]  */
    {14,12,13}, /* 12: [bc_c,  c_ac,  bc_ac]  */
    { 5,13,12}, /* 13: [ac,    bc_ac, c_ac ]  */
    {13, 5, 7}, /* 14: [bc_ac, ac,    ac_ab]  */
    { 8, 7, 5}, /* 15: [ac_a,  ac_ab, ac   ]  */
};

/* 1 = sub-face orientation is the same as the parent; 0 = flipped. */
static const int SF_SAME[16] = {1,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1};

/* Compute the 15 subdivision points and the 16 character IDs for face f. */
static void face_subdivide(Face *f) {
    if (f->ready) return;

    Vec3 a = f->verts[0], b = f->verts[1], c = f->verts[2];
    Vec3 ab   = sph_mid(a, b);
    Vec3 bc   = sph_mid(b, c);
    Vec3 ac   = sph_mid(a, c);
    Vec3 acab = sph_mid(ac, ab);
    Vec3 abbc = sph_mid(ab, bc);
    Vec3 bcac = sph_mid(bc, ac);
    Vec3 aab  = sph_mid(a,  ab);
    Vec3 abb  = sph_mid(ab, b);
    Vec3 bbc  = sph_mid(b,  bc);
    Vec3 bcc  = sph_mid(bc, c);
    Vec3 cac  = sph_mid(c,  ac);
    Vec3 aca  = sph_mid(ac, a);

    f->sp[0]  = a;
    f->sp[1]  = b;
    f->sp[2]  = c;
    f->sp[3]  = ab;
    f->sp[4]  = bc;
    f->sp[5]  = ac;
    f->sp[6]  = aab;
    f->sp[7]  = acab;
    f->sp[8]  = aca;
    f->sp[9]  = bbc;
    f->sp[10] = abbc;
    f->sp[11] = abb;
    f->sp[12] = cac;
    f->sp[13] = bcac;
    f->sp[14] = bcc;

    /* Map sub-triangle indices to characters. */
    if (f->depth > 0 && !f->up) {
        int i;
        for (i = 0; i < 16; i++) f->ids[i] = f->stids[PBOX[i]];
    } else {
        int i;
        for (i = 0; i < 16; i++) f->ids[i] = f->stids[i];
    }

    f->ready = 1;
}

/* Return sub-face idx of the current face as a new Face. */
static Face make_subface(const Face *parent, int idx) {
    Face sf;
    sf.verts[0] = parent->sp[SF_VERTS[idx][0]];
    sf.verts[1] = parent->sp[SF_VERTS[idx][1]];
    sf.verts[2] = parent->sp[SF_VERTS[idx][2]];
    memcpy(sf.stids, parent->stids, 17);
    sf.up    = SF_SAME[idx] ? parent->up : !parent->up;
    sf.depth = parent->depth + 1;
    sf.ready = 0;
    return sf;
}

/* ── findSubtriangle3D ──────────────────────────────────────────────────── */

/*
 * Use the cross-product/dot-product plane test to locate which of the 16
 * sub-triangles contains the direction q (must already have called
 * face_subdivide on f). Mirrors fullercode.ts findSubtriangle3D().
 */
static int find_sub3d(const Face *f, Vec3 q) {
    Vec3 ab   = f->sp[3],  bc   = f->sp[4],  ac   = f->sp[5];
    Vec3 aab  = f->sp[6],  acab = f->sp[7],  aca  = f->sp[8];
    Vec3 bbc  = f->sp[9],  abbc = f->sp[10], abb  = f->sp[11];
    Vec3 cac  = f->sp[12], bcac = f->sp[13], bcc  = f->sp[14];

    Vec3 cp; double dp;

    cp = cross3(ab, bc);   dp = dot3(cp, q);
    if (dp > 0) {
        cp = cross3(abbc, abb); dp = dot3(cp, q); if (dp > 0) return 5;
        cp = cross3(abb,  bbc); dp = dot3(cp, q); if (dp > 0) return 6;
        cp = cross3(bbc,  abbc); dp = dot3(cp, q); return dp > 0 ? 8 : 7;
    }

    cp = cross3(ac, ab);   dp = dot3(cp, q);
    if (dp > 0) {
        cp = cross3(aca,  aab);  dp = dot3(cp, q); if (dp > 0) return 1;
        cp = cross3(aab,  acab); dp = dot3(cp, q); if (dp > 0) return 3;
        cp = cross3(acab, aca);  dp = dot3(cp, q); return dp > 0 ? 15 : 2;
    }

    cp = cross3(bc, ac);   dp = dot3(cp, q);
    if (dp > 0) {
        cp = cross3(cac,  bcac); dp = dot3(cp, q); if (dp > 0) return 13;
        cp = cross3(bcac, bcc);  dp = dot3(cp, q); if (dp > 0) return 10;
        cp = cross3(bcc,  cac);  dp = dot3(cp, q); return dp > 0 ? 11 : 12;
    }

    cp = cross3(acab, abbc); dp = dot3(cp, q); if (dp > 0) return 4;
    cp = cross3(abbc, bcac); dp = dot3(cp, q); if (dp > 0) return 9;
    cp = cross3(bcac, acab); dp = dot3(cp, q); return dp > 0 ? 14 : 0;
}

/* ── projectToTriangle ──────────────────────────────────────────────────── */

/*
 * Project surface point pt onto the 2-D local coordinate system of triangle f.
 * Returns (Xc, Yc) barycentric-like coordinates in [0, 1].
 * Mirrors fullercode.ts projectToTriangle().
 */
static void project2d(const Face *f, Vec3 pt, double *Xc, double *Yc) {
    Vec3 o  = f->verts[0];
    Vec3 b1 = { f->verts[1].x - o.x, f->verts[1].y - o.y, f->verts[1].z - o.z };
    Vec3 b2 = { f->verts[2].x - o.x, f->verts[2].y - o.y, f->verts[2].z - o.z };
    Vec3 pv = { pt.x - o.x,          pt.y - o.y,          pt.z - o.z           };
    Vec3 n  = cross3(b1, b2);
    double nsq = dot3(n, n);
    if (nsq == 0.0) { *Xc = 0.0; *Yc = 0.0; return; }
    *Xc = dot3(cross3(b1, pv), n) / nsq;
    *Yc = dot3(cross3(pv, b2), n) / nsq;
}

/* ── findSubtriangle2D ──────────────────────────────────────────────────── */

/*
 * Locate the sub-triangle index given 2-D coordinates (Xc, Yc) in [0,1].
 * Updates *Xc and *Yc to local coordinates inside the returned sub-triangle.
 * Mirrors fullercode.ts findSubtriangle2D().
 */
static int find_sub2d(double *Xc, double *Yc) {
    double X = *Xc, Y = *Yc;
    int idx;

    if (Y > 0.5) {
        if      (Y > 0.75)       { idx = 6;  Y -= 0.75; }
        else if (X > 0.25)       { idx = 8;  Y -= 0.5;  X -= 0.25; }
        else if (X + Y < 0.75)   { idx = 5;  Y -= 0.5;  }
        else                     { idx = 7;  Y = 0.75 - Y; X = 0.25 - X; }
    } else if (X > 0.5) {
        if      (X > 0.75)       { idx = 11; X -= 0.75; }
        else if (Y > 0.25)       { idx = 10; X -= 0.5;  Y -= 0.25; }
        else if (Y + X < 0.75)   { idx = 13; X -= 0.5;  }
        else                     { idx = 12; Y = 0.25 - Y; X = 0.75 - X; }
    } else if (X + Y < 0.5) {
        if      (Y > 0.25)       { idx = 3;  Y -= 0.25; }
        else if (X > 0.25)       { idx = 15; X -= 0.25; }
        else if (X + Y < 0.25)   { idx = 1; }
        else                     { idx = 2;  X = 0.5 - X; Y = 0.5 - Y; }
    } else {
        if      (Y < 0.25)       { idx = 14; Y = 0.25 - Y; X = 0.5 - X;  }
        else if (X < 0.25)       { idx = 4;  Y = 0.5  - Y; X = 0.25 - X; }
        else if (X + Y > 0.75)   { idx = 9;  Y = 0.5  - Y; X = 0.5  - X; }
        else                     { idx = 0;  X -= 0.25; Y -= 0.25; }
    }

    *Xc = X * 4.0;
    *Yc = Y * 4.0;
    return idx;
}

/* ── Closest initial face ───────────────────────────────────────────────── */

static int find_closest_ico_face(Vec3 q, const IcoFace *faces) {
    double min_dsq = 1e300;
    int best = 0, i;
    for (i = 0; i < 20; i++) {
        Vec3 v0  = ico_vert(faces[i].v[0]);
        Vec3 v1  = ico_vert(faces[i].v[1]);
        Vec3 v2  = ico_vert(faces[i].v[2]);
        Vec3 ctr = sph_center(v0, v1, v2);
        double dx = q.x - ctr.x, dy = q.y - ctr.y, dz = q.z - ctr.z;
        double d  = dx*dx + dy*dy + dz*dz;
        if (d < min_dsq) { min_dsq = d; best = i; }
    }
    return best;
}

static int encode_with_faces(double lat_deg, double lon_deg, uint16_t len, char *out, const IcoFace *faces) {
    double lat, lon, cos_lat;
    Vec3 q;
    int fi, i;
    Face f;
    double Xc, Yc;

    if (!out || !faces || len < 1 || len > MAX_LEN) return -1;
    if (lat_deg < -90.0 || lat_deg > 90.0) return -1;
    if (lon_deg < -180.0 || lon_deg > 180.0) return -1;

    lat     = lat_deg * (M_PI / 180.0);
    lon     = lon_deg * (M_PI / 180.0);
    cos_lat = cos(lat);

    q.x = cos_lat * cos(lon) * RADIUS;
    q.y = cos_lat * sin(lon) * RADIUS;
    q.z = sin(lat)           * RADIUS;

    fi = find_closest_ico_face(q, faces);

    f.verts[0] = ico_vert(faces[fi].v[0]);
    f.verts[1] = ico_vert(faces[fi].v[1]);
    f.verts[2] = ico_vert(faces[fi].v[2]);
    memcpy(f.stids, faces[fi].stids, 17);
    f.up    = 1;
    f.depth = 0;
    f.ready = 0;

    out[0] = faces[fi].id;

    Xc = 0.0; Yc = 0.0;

    for (i = 0; i < (int)len - 1; i++) {
        int idx;

        face_subdivide(&f);

        if (i < TRANSITION) {
            idx = find_sub3d(&f, q);
        } else {
            if (i == TRANSITION) {
                project2d(&f, q, &Xc, &Yc);
            }
            idx = find_sub2d(&Xc, &Yc);
        }

        out[i + 1] = f.ids[idx];
        f = make_subface(&f, idx);
    }

    out[len] = '\0';
    return 0;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

int fullergeocoding(double lat_deg, double lon_deg, uint16_t len, char *out) {
    return encode_with_faces(lat_deg, lon_deg, len, out, ICO_FACES);
}

int fullerNATOcoding(double lat_deg, double lon_deg, char *out) {
    return encode_with_faces(lat_deg, lon_deg, 11, out, ICO_FACES_NATO);
}


static int decode_with_faces(const char *code, double *lat_deg, double *lon_deg, const IcoFace *faces, int max_len) {
    int n, fi, i, k;
    Face f;
    Vec3 ctr;
    double radius;

    if (!code || !lat_deg || !lon_deg) return -1;

    n = (int)strlen(code);
    if (n < 1 || n > max_len) return -1;

    /* Locate initial icosahedron face. */
    fi = -1;
    for (i = 0; i < 20; i++) {
        if (faces[i].id == code[0]) { fi = i; break; }
    }
    if (fi < 0) return -1;

    f.verts[0] = ico_vert(faces[fi].v[0]);
    f.verts[1] = ico_vert(faces[fi].v[1]);
    f.verts[2] = ico_vert(faces[fi].v[2]);
    memcpy(f.stids, faces[fi].stids, 17);
    f.up    = 1;
    f.depth = 0;
    f.ready = 0;

    for (i = 1; i < n; i++) {
        char c = code[i];
        int idx = -1;

        face_subdivide(&f);

        for (k = 0; k < 16; k++) {
            if (f.ids[k] == c) { idx = k; break; }
        }
        if (idx < 0) return -1;

        f = make_subface(&f, idx);
    }

    /* Return the centroid of the final triangle projected onto the sphere. */
    ctr    = sph_center(f.verts[0], f.verts[1], f.verts[2]);
    radius = sqrt(ctr.x*ctr.x + ctr.y*ctr.y + ctr.z*ctr.z);

    *lat_deg = asin(ctr.z / radius)      * (180.0 / M_PI);
    *lon_deg = atan2(ctr.y, ctr.x)       * (180.0 / M_PI);

    return 0;
}

int fullerNATOdecoding(const char *code, double *lat_deg, double *lon_deg) {
    return decode_with_faces(code, lat_deg, lon_deg, ICO_FACES_NATO, 11);
}

int fullergeodecoding(const char *code, double *lat_deg, double *lon_deg) {
    return decode_with_faces(code, lat_deg, lon_deg, ICO_FACES, MAX_LEN);
}

uint64_t fullerbingeocoding(double lat_deg, double lon_deg) {
    double lat, lon, cos_lat;
    Vec3 q;
    int fi, i, bin_face = -1;
    Face f;
    uint64_t result = 0;

    if (lat_deg < -90.0 || lat_deg > 90.0) return 0;
    if (lon_deg < -180.0 || lon_deg > 180.0) return 0;

    lat = lat_deg * (M_PI / 180.0);
    lon = lon_deg * (M_PI / 180.0);
    cos_lat = cos(lat);
    q.x = cos_lat * cos(lon) * RADIUS;
    q.y = cos_lat * sin(lon) * RADIUS;
    q.z = sin(lat) * RADIUS;

    fi = find_closest_ico_face(q, ICO_FACES);
    char id = ICO_FACES[fi].id;
    for (i = 0; i < 20; i++) {
        if (BIN_FACE_ORDER[i] == id) { bin_face = 0x10 + i; break; }
    }
    
    if (bin_face == -1) return 0;

    result = (uint64_t)bin_face << 56;

    f.verts[0] = ico_vert(ICO_FACES[fi].v[0]);
    f.verts[1] = ico_vert(ICO_FACES[fi].v[1]);
    f.verts[2] = ico_vert(ICO_FACES[fi].v[2]);
    memcpy(f.stids, ICO_FACES[fi].stids, 17);
    f.up = 1; f.depth = 0; f.ready = 0;

    for (i = 0; i < 14; i++) {
        face_subdivide(&f);
        int idx = find_sub3d(&f, q);
        result |= (uint64_t)idx << (52 - (i * 4));
        f = make_subface(&f, idx);
    }

    return result;
}

int fullerbingeodecoding(uint64_t bin, double *lat_deg, double *lon_deg) {
    int i, fi = -1;
    Face f;
    Vec3 ctr;
    double radius;

    if (!lat_deg || !lon_deg) return -1;

    int bin_face = (int)((bin >> 56) & 0xFF);
    int order_idx = bin_face - 0x10;
    if (order_idx < 0 || order_idx >= 20) return -1;

    char target_id = BIN_FACE_ORDER[order_idx];
    for (i = 0; i < 20; i++) {
        if (ICO_FACES[i].id == target_id) { fi = i; break; }
    }
    if (fi == -1) return -1;

    f.verts[0] = ico_vert(ICO_FACES[fi].v[0]);
    f.verts[1] = ico_vert(ICO_FACES[fi].v[1]);
    f.verts[2] = ico_vert(ICO_FACES[fi].v[2]);
    memcpy(f.stids, ICO_FACES[fi].stids, 17);
    f.up = 1; f.depth = 0; f.ready = 0;

    for (i = 0; i < 14; i++) {
        int idx = (int)((bin >> (52 - (i * 4))) & 0x0F);
        face_subdivide(&f);
        /* Note: SF_VERTS table provides vertex indices for sub-triangles */
        f = make_subface(&f, idx);
    }

    ctr = sph_center(f.verts[0], f.verts[1], f.verts[2]);
    radius = sqrt(ctr.x * ctr.x + ctr.y * ctr.y + ctr.z * ctr.z);

    *lat_deg = asin(ctr.z / radius) * (180.0 / M_PI);
    *lon_deg = atan2(ctr.y, ctr.x) * (180.0 / M_PI);

    return 0;
}
