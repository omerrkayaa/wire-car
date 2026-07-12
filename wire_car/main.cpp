#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

 /* --- Genel sabitler --- */
#define PI 3.14159265f
#define MAX_STEER 30.0f   /* On tekerlek max donus acisi (derece) */
#define ROAD_LOOP 0.8f    /* Yol orta cizgisi tekrar araligi */

/* WireCar 1.1: secilebilir govde renkleri ve kaplama tipleri */
typedef struct {
    float r, g, b;
    const char* name;
} PaintColor;

static const PaintColor paintColors[] = {
    { 0.52f, 0.58f, 0.01f, "Zeytin Yesili" },
    { 0.78f, 0.04f, 0.05f, "Yaris Kirmizisi" },
    { 0.04f, 0.22f, 0.72f, "Gece Mavisi" },
    { 0.92f, 0.92f, 0.94f, "Inci Beyazi" },
    { 0.04f, 0.05f, 0.07f, "Karbon Siyahi" },
    { 0.90f, 0.42f, 0.03f, "Bakir Turuncu" }
};

static const char* finishNames[] = { "Parlak", "Mat", "Metalik" };
static int paintColorIndex = 0;
static int paintFinishIndex = 0;

#define PAINT_COLOR_COUNT 6
#define PAINT_FINISH_COUNT 3
#define BODY_R (paintColors[paintColorIndex].r)
#define BODY_G (paintColors[paintColorIndex].g)
#define BODY_B (paintColors[paintColorIndex].b)

/* Tel cizimlerde glColor3f ile govde rengini uyguladım */
static void setBodyColor(void)
{
    glColor3f(BODY_R, BODY_G, BODY_B);
}

/* --- Animasyon ve etkilesim durumu --- */
static float wheelAngle = 0.0f;   /* Tekerlek donme acisi (timer ile artar) */
static float roadOffset = 0.0f; /* Yol cizgilerinin kayma miktari */
static float steerAngle = 0.0f; /* On teker + direksiyon simidi ortak acisi */

static int keyLeft = 0;  /* A tusuna basili mi */
static int keyRight = 0; /* S tusuna basili mi */

/* --- Kamera ve render modu --- */
static int cameraMode = 0;        /* Ana pencere: 0..6 ( tus 1-7 ) */
static int subCameraMode = 0;     /* Alt pencere: 0..4 ( tus Q-T ) */
static int solidLightEnabled = 1; /* 1=bonus (solid+isik), 0=yalnizca tel */
static int headlightsEnabled = 1; /* F tusu ile farlar ac/kapa */
static int showHelpPanel = 1;     /* H tusu ile ac/kapa */
static int mainWinW = 950;
static int mainWinH = 700;

#define MAIN_CAMERA_COUNT 7
#define SUB_CAMERA_COUNT  5

/*
 * Kamera on ayari: goz konumu, bakis noktasi, yukari vektoru, menu etiketi.
 * gluLookAt(eye -> center, up) ile kullanilir.
 */
typedef struct {
    float eyeX, eyeY, eyeZ;
    float centerX, centerY, centerZ;
    float upX, upY, upZ;
    const char* label;
} CameraPreset;

/* Ana pencere kameralari — tus 1-7 ve sag tik menu */
static const CameraPreset mainCameras[MAIN_CAMERA_COUNT] = {
    {  3.5f,  2.2f, -5.5f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,  0.0f, "Perspektif Gorunus (Kamera 1)" },
    {  0.0f,  7.0f,  0.01f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f, "Kus Bakisi Gorunus (Kamera 2)" },
    {  6.0f,  1.5f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,  0.0f, "Sag Yan Gorunus (Kamera 3)" },
    { -5.0f,  2.0f, -4.0f,  0.0f, 0.0f, 0.0f,  0.0f, 1.0f,  0.0f, "Sol Arka Gorunus (Kamera 4)" },
    {  0.0f,  0.35f, 4.2f,  0.0f, 0.1f, 0.0f,  0.0f, 1.0f,  0.0f, "On Gorunus (Kamera 5)" },
    {  0.0f,  0.35f,-4.2f,  0.0f, 0.1f, 0.0f,  0.0f, 1.0f,  0.0f, "Arka Gorunus (Kamera 6)" },
    {  0.0f,  1.4f, -4.0f,  0.0f, 0.25f, 0.8f, 0.0f, 1.0f,  0.0f, "Takip Kamerasi (Kamera 7)" }
};

/* Alt pencere kameralari — ana pencereden bagimsiz, tus Q-T */
static const CameraPreset subCameras[SUB_CAMERA_COUNT] = {
    {  0.0f, 6.5f,  0.001f, 0.0f, 0.0f,  0.0f,  0.0f, 0.0f, -1.0f, "Ustten Tam (Q)" },
    {  0.0f, 4.5f,  3.5f,   0.0f, 0.0f,  0.3f,  0.0f, 1.0f,  0.0f, "On-Ust Capraz (W)" },
    {  0.0f, 4.5f, -3.5f,   0.0f, 0.0f, -0.3f,  0.0f, 1.0f,  0.0f, "Arka-Ust Capraz (E)" },
    { -4.5f, 4.0f,  0.0f,   0.0f, 0.0f,  0.0f,  0.0f, 1.0f,  0.0f, "Sol-Ust Yan (R)" },
    {  0.0f, 3.2f,  0.001f, 0.0f, 0.0f,  0.0f,  0.0f, 0.0f, -1.0f, "Yakin Ust Zoom (T)" }
};

int mainWindow;
int subWindow;   

static GLUquadric* solidQuadric = NULL; /* gluCylinder icin (far govdesi) */

/*
 * Bonus malzeme ayari: diffuse renk + specular parlaklik.
 * GL_LIGHT0 acikken solid yuzeylerde gölgeleme etkisi olusturur.
 */
static void setMaterial(float dr, float dg, float db,
    float sr, float sg, float sb,
    float shininess, float alpha)
{
    GLfloat ambient[] = { dr * 0.25f, dg * 0.25f, db * 0.25f, alpha };
    GLfloat diffuse[] = { dr, dg, db, alpha };
    GLfloat specular[] = { sr, sg, sb, alpha };

    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess);
}

/* Secili boya tipine gore govde malzemesini ayarlar. */
static void setBodyMaterial(void)
{
    if (paintFinishIndex == 1)       /* Mat */
        setMaterial(BODY_R, BODY_G, BODY_B, 0.08f, 0.08f, 0.08f, 6.0f, 1.0f);
    else if (paintFinishIndex == 2)  /* Metalik */
        setMaterial(BODY_R * 0.72f, BODY_G * 0.72f, BODY_B * 0.72f,
            0.95f, 0.95f, 1.0f, 96.0f, 1.0f);
    else                             /* Parlak */
        setMaterial(BODY_R, BODY_G, BODY_B, 0.72f, 0.72f, 0.72f, 52.0f, 1.0f);
}

/* Merkez (cx,cy,cz) ve yarim boyutlarla solid kutu cizer */
static void drawSolidBox(float cx, float cy, float cz, float hx, float hy, float hz)
{
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glScalef(hx * 2.0f, hy * 2.0f, hz * 2.0f);
    glutSolidCube(1.0);
    glPopMatrix();
}

/* Iki nokta arasini tamamen dolduran, kare kesitli solid kiris cizer. */
static void drawSolidBeam(float x0, float y0, float z0,
    float x1, float y1, float z1, float thickness)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dz = z1 - z0;
    float length = sqrtf(dx * dx + dy * dy + dz * dz);

    if (length <= 0.0001f)
        return;

    glPushMatrix();
    glTranslatef((x0 + x1) * 0.5f, (y0 + y1) * 0.5f, (z0 + z1) * 0.5f);

    /* Varsayilan Z eksenini kirisin yonune cevir. */
    {
        float angle = acosf(dz / length) * 180.0f / PI;
        float axisX = -dy;
        float axisY = dx;
        if (fabsf(axisX) > 0.0001f || fabsf(axisY) > 0.0001f)
            glRotatef(angle, axisX, axisY, 0.0f);
        else if (dz < 0.0f)
            glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    }

    glScalef(thickness, thickness, length);
    glutSolidCube(1.0);
    glPopMatrix();
}

/* Ana ve alt pencereyi ayni anda yeniden cizer (senkron guncelleme) */
static void requestRedrawBoth(void)
{
    glutSetWindow(mainWindow);
    glutPostRedisplay();
    glutSetWindow(subWindow);
    glutPostRedisplay();
}

/* Z ekseni boyunca solid silindir + iki kapak (on far) */
static void drawSolidCylinder(float radius, float height, int slices)
{
    if (solidQuadric == NULL)
        return;

    gluQuadricNormals(solidQuadric, GLU_SMOOTH);
    gluCylinder(solidQuadric, radius, radius, height, slices, 1);
    gluDisk(solidQuadric, 0.0, radius, slices, 1);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, height);
    gluDisk(solidQuadric, 0.0, radius, slices, 1);
    glPopMatrix();
}

/* Tel silindir: ust/alt halka + dikey baglanti cizgileri (far cercevesi) */
static void drawWireCylinder(float radius, float height, int slices)
{
    float step = 2.0f * PI / slices;

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < slices; i++)
        glVertex3f(radius * cosf(i * step), radius * sinf(i * step), 0.0f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < slices; i++)
        glVertex3f(radius * cosf(i * step), radius * sinf(i * step), height);
    glEnd();

    glBegin(GL_LINES);
    for (int i = 0; i < slices; i += 2)
    {
        glVertex3f(radius * cosf(i * step), radius * sinf(i * step), 0.0f);
        glVertex3f(radius * cosf(i * step), radius * sinf(i * step), height);
    }
    glEnd();
}

/*
 * Tekerlek cizer (glutWireTorus + capraz jant cizgileri, zorunlu parca).
 *
 * frontWheel=1 ise steerAngle uygulanir (on tekerlekler A/S ile doner).
 *   1) glTranslatef  — tekerlegi arabanin koselerine tasir
 *   2) steer (Y)     — yalniz on tekerlerde direksiyon acisi
 *   3) yatir (Y 90)  — torus eksenini yatay konuma getirir
 *   4) spin (Z)      — wheelAngle ile donme animasyonu
 */
static void drawWheel(float x, float y, float z, int frontWheel)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    if (frontWheel)
        glRotatef(steerAngle, 0.0f, 1.0f, 0.0f);

    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(wheelAngle, 0.0f, 0.0f, 1.0f);

    if (solidLightEnabled)
    {
        setMaterial(0.18f, 0.18f, 0.18f, 0.65f, 0.65f, 0.65f, 45.0f, 1.0f);
        glutSolidTorus(0.06f, 0.16f, 14, 28); /* Bonus: koyu lastik kaplamasi */
    }

    /* Tel modda da gorunen kisim — proje sarti glutWireTorus */
    glDisable(GL_LIGHTING);
    glutWireTorus(0.06f, 0.16f, 10, 20);
    glColor3f(0.85f, 0.85f, 0.85f);
    glBegin(GL_LINES);
    glVertex3f(-0.15f, 0.0f, 0.0f); glVertex3f(0.15f, 0.0f, 0.0f);
    glVertex3f(0.0f, -0.15f, 0.0f); glVertex3f(0.0f, 0.15f, 0.0f);
    glVertex3f(-0.105f, -0.105f, 0.0f); glVertex3f(0.105f, 0.105f, 0.0f);
    glVertex3f(-0.105f, 0.105f, 0.0f); glVertex3f(0.105f, -0.105f, 0.0f);
    glEnd();
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);

    glPopMatrix();
}

/*
 * Yol: gri kenar cizgileri + sari kesik orta cizgi.
 */
static void drawRoad(void)
{
    glDisable(GL_LIGHTING);
    glColor3f(0.45f, 0.45f, 0.45f);
    glBegin(GL_LINES);
    glVertex3f(-2.5f, -0.52f, -20.0f); glVertex3f(-2.5f, -0.52f, 20.0f);
    glVertex3f(2.5f, -0.52f, -20.0f);  glVertex3f(2.5f, -0.52f, 20.0f);
    glEnd();

    glColor3f(1.0f, 0.15f, 0.12f);
    glBegin(GL_LINES);
    for (float z = -10.0f - roadOffset; z < 10.0f; z += ROAD_LOOP)
    {
        glVertex3f(0.0f, -0.53f, z);
        glVertex3f(0.0f, -0.53f, z + 0.38f);
    }
    glEnd();
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);
}


static void drawBody(void)
{
    if (solidLightEnabled)
    {
        setBodyMaterial();
        glPushMatrix();
        glScalef(1.10f, 0.42f, 1.80f);
        glutSolidCube(1.0);
        glPopMatrix();

        /* Tavan daha once yalnizca tel cizgilerden olusuyordu. Kaplama
           modunda govdeyle ayni malzemeyi kullanan ince bir yuzey ekle. */
        drawSolidBox(0.0f, 0.685f, -0.10f, 0.34f, 0.015f, 0.28f);

        /* Kaputun tel cercevesiyle ayni sinirlarda ince kaplama. */
        drawSolidBox(0.0f, 0.215f, 0.60f, 0.48f, 0.012f, 0.28f);

        /* Direkler tel kabin cercevesinin gercek egimini birebir izler. */
        drawSolidBeam(0.42f, 0.21f, 0.32f, 0.34f, 0.70f, 0.18f, 0.048f);
        drawSolidBeam(-0.42f, 0.21f, 0.32f, -0.34f, 0.70f, 0.18f, 0.048f);
        drawSolidBeam(0.42f, 0.21f, -0.50f, 0.34f, 0.70f, -0.38f, 0.048f);
        drawSolidBeam(-0.42f, 0.21f, -0.50f, -0.34f, 0.70f, -0.38f, 0.048f);
        drawSolidBeam(0.34f, 0.70f, 0.18f, 0.34f, 0.70f, -0.38f, 0.044f);
        drawSolidBeam(-0.34f, 0.70f, 0.18f, -0.34f, 0.70f, -0.38f, 0.044f);
    }

    glDisable(GL_LIGHTING);
    setBodyColor();
    glPushMatrix();
    glScalef(1.10f, 0.42f, 1.80f);
    glutWireCube(1.0);
    glPopMatrix();

    glBegin(GL_LINE_LOOP);
    glVertex3f(0.42f, 0.21f, -0.50f); glVertex3f(0.42f, 0.21f, 0.32f);
    glVertex3f(0.34f, 0.70f, 0.18f);  glVertex3f(0.34f, 0.70f, -0.38f);
    glEnd(); /* Sag yan cam cercevesi */

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.42f, 0.21f, -0.50f); glVertex3f(-0.42f, 0.21f, 0.32f);
    glVertex3f(-0.34f, 0.70f, 0.18f);  glVertex3f(-0.34f, 0.70f, -0.38f);
    glEnd(); /* Sol yan cam cercevesi */

    glBegin(GL_LINES);
    glVertex3f(0.42f, 0.21f, -0.50f); glVertex3f(-0.42f, 0.21f, -0.50f);
    glVertex3f(0.42f, 0.21f, 0.32f);  glVertex3f(-0.42f, 0.21f, 0.32f);
    glVertex3f(0.34f, 0.70f, 0.18f);  glVertex3f(-0.34f, 0.70f, 0.18f);
    glVertex3f(0.34f, 0.70f, -0.38f); glVertex3f(-0.34f, 0.70f, -0.38f);
    glVertex3f(0.0f, 0.22f, -0.50f);  glVertex3f(0.0f, 0.22f, -0.90f);

    /* Kaput uzantisi ve cizgileri */
    glVertex3f(-0.40f, 0.22f, 0.32f); glVertex3f(-0.52f, 0.22f, 0.88f);
    glVertex3f(0.40f, 0.22f, 0.32f);  glVertex3f(0.52f, 0.22f, 0.88f);
    glVertex3f(-0.26f, 0.22f, 0.32f); glVertex3f(-0.34f, 0.22f, 0.88f);
    glVertex3f(0.26f, 0.22f, 0.32f);  glVertex3f(0.34f, 0.22f, 0.88f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.40f, 0.22f, 0.32f); glVertex3f(0.40f, 0.22f, 0.32f);
    glVertex3f(0.52f, 0.22f, 0.88f);  glVertex3f(-0.52f, 0.22f, 0.88f);
    glEnd();
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);
}

/* Dort koseli yari saydam cam yuzeyi (solid modda) */
static void drawWindowQuad(float v[4][3])
{
    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++)
        glVertex3fv(v[i]);
    glEnd();
}

/*
 * Camlar: bonus modda yari saydam mavi quad (GL_BLEND).
 * Her modda mavi tel cerceve cizilir; en sonda cizilir (derinlik icin).
 */
static void drawWindows(void)
{
    float front[4][3] = {
        { -0.32f, 0.24f, 0.34f }, { 0.32f, 0.24f, 0.34f },
        { 0.25f, 0.64f, 0.16f },  { -0.25f, 0.64f, 0.16f }
    };
    float rear[4][3] = {
        { -0.32f, 0.24f, -0.52f }, { 0.32f, 0.24f, -0.52f },
        { 0.25f, 0.64f, -0.38f },  { -0.25f, 0.64f, -0.38f }
    };
    float rightSide[4][3] = {
        { 0.43f, 0.27f, -0.42f }, { 0.43f, 0.27f, 0.22f },
        { 0.35f, 0.62f, 0.10f },  { 0.35f, 0.62f, -0.34f }
    };
    float leftSide[4][3] = {
        { -0.43f, 0.27f, -0.42f }, { -0.43f, 0.27f, 0.22f },
        { -0.35f, 0.62f, 0.10f },  { -0.35f, 0.62f, -0.34f }
    };

    if (solidLightEnabled)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        setMaterial(0.25f, 0.55f, 0.85f, 0.85f, 0.85f, 1.0f, 90.0f, 0.35f);

        drawWindowQuad(front);
        drawWindowQuad(rear);
        drawWindowQuad(rightSide);
        drawWindowQuad(leftSide);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    glDisable(GL_LIGHTING);
    glColor3f(0.55f, 0.75f, 1.0f);

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.32f, 0.24f, 0.34f); glVertex3f(0.32f, 0.24f, 0.34f);
    glVertex3f(0.25f, 0.64f, 0.16f);  glVertex3f(-0.25f, 0.64f, 0.16f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.32f, 0.24f, -0.52f); glVertex3f(0.32f, 0.24f, -0.52f);
    glVertex3f(0.25f, 0.64f, -0.38f);  glVertex3f(-0.25f, 0.64f, -0.38f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(0.43f, 0.27f, -0.42f); glVertex3f(0.43f, 0.27f, 0.22f);
    glVertex3f(0.35f, 0.62f, 0.10f);  glVertex3f(0.35f, 0.62f, -0.34f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.43f, 0.27f, -0.42f); glVertex3f(-0.43f, 0.27f, 0.22f);
    glVertex3f(-0.35f, 0.62f, 0.10f);  glVertex3f(-0.35f, 0.62f, -0.34f);
    glEnd();
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);
}

/*
 * On farlar (sari) + arka stoplar (turuncu).
 * Solid mod: silindir/kup kaplama; tel mod: yalnizca tel cerceve.
 */
static void drawLights(void)
{
    if (solidLightEnabled && headlightsEnabled)
    {
        setMaterial(1.0f, 0.95f, 0.2f, 1.0f, 1.0f, 0.8f, 60.0f, 1.0f);
        glPushMatrix(); glTranslatef(0.32f, 0.10f, 0.92f); drawSolidCylinder(0.075f, 0.05f, 16); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.32f, 0.10f, 0.92f); drawSolidCylinder(0.075f, 0.05f, 16); glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    glColor3f(headlightsEnabled ? 1.0f : 0.35f,
        headlightsEnabled ? 1.0f : 0.35f, 0.0f);
    glPushMatrix(); glTranslatef(0.32f, 0.10f, 0.92f); drawWireCylinder(0.075f, 0.05f, 16); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.32f, 0.10f, 0.92f); drawWireCylinder(0.075f, 0.05f, 16); glPopMatrix();

    if (solidLightEnabled)
    {
        setMaterial(1.0f, 0.45f, 0.0f, 0.6f, 0.45f, 0.15f, 20.0f, 1.0f);
        glEnable(GL_LIGHTING);
        glPushMatrix();
        glTranslatef(-0.335f, 0.105f, -0.92f);
        glScalef(0.17f, 0.15f, 0.02f);
        glutSolidCube(1.0);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.335f, 0.105f, -0.92f);
        glScalef(0.17f, 0.15f, 0.02f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.45f, 0.0f); /* Arka stop tel rengi (turuncu) */
    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.42f, 0.03f, -0.92f); glVertex3f(-0.25f, 0.03f, -0.92f);
    glVertex3f(-0.25f, 0.18f, -0.92f); glVertex3f(-0.42f, 0.18f, -0.92f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(0.25f, 0.03f, -0.92f);  glVertex3f(0.42f, 0.03f, -0.92f);
    glVertex3f(0.42f, 0.18f, -0.92f);  glVertex3f(0.25f, 0.18f, -0.92f);
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(-0.335f, 0.03f, -0.921f); glVertex3f(-0.335f, 0.18f, -0.921f);
    glVertex3f(0.335f, 0.03f, -0.921f);  glVertex3f(0.335f, 0.18f, -0.921f);
    glEnd();
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);
}

/* On ve arka tampon — govde rengiyle ayni kaplama */
static void drawBumpers(void)
{
    if (solidLightEnabled)
    {
        setBodyMaterial();
        glPushMatrix();
        glTranslatef(0.0f, -0.28f, 0.96f);
        glScalef(1.04f, 0.12f, 0.04f);
        glutSolidCube(1.0);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(0.0f, -0.28f, -0.96f);
        glScalef(1.04f, 0.12f, 0.04f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    setBodyColor();
    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.52f, -0.22f, 0.96f); glVertex3f(0.52f, -0.22f, 0.96f);
    glVertex3f(0.52f, -0.34f, 0.96f);  glVertex3f(-0.52f, -0.34f, 0.96f);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.52f, -0.22f, -0.96f); glVertex3f(0.52f, -0.22f, -0.96f);
    glVertex3f(0.52f, -0.34f, -0.96f);  glVertex3f(-0.52f, -0.34f, -0.96f);
    glEnd();
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);
}

/* --- Aktarma organi konum sabitleri (4 tekerlek + merkez diferansiyel) --- */
#define FRONT_AXLE_Z  0.62f   /* On aks Z konumu */
#define REAR_AXLE_Z  -0.62f   /* Arka aks Z konumu */
#define AXLE_Y       -0.34f   /* Aks yuksekligi */
#define CENTER_Y     -0.36f   /* Merkez diferansiyel Y */
#define CENTER_Z      0.0f
#define WHEEL_X       0.62f   /* Tekerlegin X mesafesi (merkeze gore) */

/* Tel kutu cercevesi — aks diferansiyel kutulari icin */
static void drawWireBox(float cx, float cy, float cz, float hx, float hy, float hz)
{
    float x0 = cx - hx, x1 = cx + hx;
    float y0 = cy - hy, y1 = cy + hy;
    float z0 = cz - hz, z1 = cz + hz;

    glBegin(GL_LINE_LOOP);
    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z1);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glEnd();
    glBegin(GL_LINES);
    glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
    glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1);
    glEnd();
}

/* Disli sembolu (tel cark) */
static void drawWireGear(float cx, float cy, float cz, float radius, int teeth)
{
    float step = 2.0f * PI / (float)(teeth * 2);
    int i;

    glBegin(GL_LINE_LOOP);
    for (i = 0; i < teeth * 2; i++)
    {
        float ang = i * step;
        float r = (i % 2 == 0) ? radius : radius * 0.72f;
        glVertex3f(cx + r * cosf(ang), cy + r * sinf(ang), cz);
    }
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(cx - radius * 0.55f, cy, cz); glVertex3f(cx + radius * 0.55f, cy, cz);
    glVertex3f(cx, cy - radius * 0.55f, cz); glVertex3f(cx, cy + radius * 0.55f, cz);
    glEnd();
}

/*
 * Beyaz tel akslar; on/arka tekerleklere uzanan saftlar ve merkez disliler.
 */
static void drawSelectable4x4(void)
{
    if (solidLightEnabled)
    {
        setMaterial(0.72f, 0.72f, 0.72f, 0.85f, 0.85f, 0.85f, 45.0f, 1.0f);
        drawSolidBox(0.0f, AXLE_Y, FRONT_AXLE_Z, 0.045f, 0.03f, 0.035f);
        drawSolidBox(0.0f, AXLE_Y, REAR_AXLE_Z, 0.045f, 0.03f, 0.035f);
        drawSolidBox(0.0f, CENTER_Y, CENTER_Z - 0.01f, 0.022f, 0.022f, 0.014f);
    }

    glDisable(GL_LIGHTING);
    glLineWidth(2.5f);
    glColor3f(1.0f, 1.0f, 1.0f);

    /* On aks + saft + teker hub cizgileri */
    glBegin(GL_LINES);
    glVertex3f(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z); glVertex3f(WHEEL_X, AXLE_Y, FRONT_AXLE_Z);
    glVertex3f(0.0f, AXLE_Y, FRONT_AXLE_Z);    glVertex3f(0.0f, CENTER_Y, 0.06f);
    glVertex3f(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z); glVertex3f(-WHEEL_X, AXLE_Y + 0.05f, FRONT_AXLE_Z);
    glVertex3f(WHEEL_X, AXLE_Y, FRONT_AXLE_Z);  glVertex3f(WHEEL_X, AXLE_Y + 0.05f, FRONT_AXLE_Z);
    glEnd();
    drawWireBox(0.0f, AXLE_Y, FRONT_AXLE_Z, 0.045f, 0.03f, 0.035f);

    /* Arka aks + saft + teker hub cizgileri */
    glBegin(GL_LINES);
    glVertex3f(-WHEEL_X, AXLE_Y, REAR_AXLE_Z); glVertex3f(WHEEL_X, AXLE_Y, REAR_AXLE_Z);
    glVertex3f(0.0f, AXLE_Y, REAR_AXLE_Z);     glVertex3f(0.0f, CENTER_Y, CENTER_Z);
    glVertex3f(-WHEEL_X, AXLE_Y, REAR_AXLE_Z); glVertex3f(-WHEEL_X, AXLE_Y + 0.05f, REAR_AXLE_Z);
    glVertex3f(WHEEL_X, AXLE_Y, REAR_AXLE_Z);  glVertex3f(WHEEL_X, AXLE_Y + 0.05f, REAR_AXLE_Z);
    glEnd();
    drawWireBox(0.0f, AXLE_Y, REAR_AXLE_Z, 0.045f, 0.03f, 0.035f);

    /* Merkez baglanti (kucuk) */
    drawWireGear(0.0f, CENTER_Y, CENTER_Z - 0.01f, 0.045f, 6);
    drawWireBox(0.0f, CENTER_Y, CENTER_Z - 0.01f, 0.022f, 0.022f, 0.014f);
    drawWireGear(0.0f, CENTER_Y, 0.06f, 0.035f, 6);

    glBegin(GL_LINES);
    glVertex3f(0.0f, CENTER_Y, CENTER_Z - 0.01f); glVertex3f(0.0f, CENTER_Y, 0.06f);
    glEnd();

    glLineWidth(1.0f);
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);
}

/*
 * Direksiyon sistemi (zorunlu parcalar):
 *   - glutWireTorus simit (steerAngle ile birlikte doner)
 *   - Direksiyon mili / kolu (GL_LINES)
 *   - Rack cubuklari — on aksa baglanti (on tekerlekleri yönlendirir)
 * Simit matrix: translate -> egik (65 derece X) -> steerAngle (Z)
 */
static void drawSteeringSystem(void)
{
    glPushMatrix();
    glTranslatef(-0.20f, 0.22f, 0.22f);
    glRotatef(65.0f, 1.0f, 0.0f, 0.0f);
    /* Egik simidin gorunen donusu, tekerlek Y donusunun ters isaretidir. */
    glRotatef(-steerAngle, 0.0f, 0.0f, 1.0f);
    if (solidLightEnabled)
    {
        setMaterial(0.85f, 0.80f, 0.10f, 0.55f, 0.55f, 0.20f, 35.0f, 1.0f);
        glutSolidTorus(0.008f, 0.065f, 10, 24);
    }

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f);
    glutWireTorus(0.008f, 0.065f, 8, 20);
    glBegin(GL_LINES);
    glVertex3f(-0.06f, 0.0f, 0.0f); glVertex3f(0.06f, 0.0f, 0.0f);
    glVertex3f(0.0f, -0.06f, 0.0f); glVertex3f(0.0f, 0.06f, 0.0f);
    glEnd();
    glPopMatrix();

    glColor3f(0.75f, 0.75f, 0.75f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex3f(-0.20f, 0.18f, 0.20f);  glVertex3f(-0.20f, -0.02f, 0.45f);
    glVertex3f(-0.20f, 0.22f, 0.22f);  glVertex3f(-0.20f, 0.18f, 0.20f);
    glEnd(); /* Direksiyon mili */
    glLineWidth(1.0f);

    /* Rack: simitten on aksa, oradan sol/sag on tekerleklere */
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex3f(-0.20f, -0.02f, 0.45f); glVertex3f(0.0f, AXLE_Y + 0.02f, FRONT_AXLE_Z);
    glVertex3f(0.0f, AXLE_Y + 0.02f, FRONT_AXLE_Z); glVertex3f(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z);
    glVertex3f(0.0f, AXLE_Y + 0.02f, FRONT_AXLE_Z); glVertex3f(WHEEL_X, AXLE_Y, FRONT_AXLE_Z);
    glEnd();
    glLineWidth(1.0f);
    if (solidLightEnabled)
        glEnable(GL_LIGHTING);
}

/* Tum araba parcalarini cizim sirasiyla birlestirir; camlar en sonda (on plan) */
static void drawCar(void)
{
    drawBody();
    drawLights();
    drawBumpers();
    drawSelectable4x4();
    drawSteeringSystem();

    drawWheel(WHEEL_X, AXLE_Y, FRONT_AXLE_Z, 1);
    drawWheel(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z, 1);
    drawWheel(WHEEL_X, AXLE_Y, REAR_AXLE_Z, 0);
    drawWheel(-WHEEL_X, AXLE_Y, REAR_AXLE_Z, 0);

    drawWindows();
}

static void applyCameraPreset(const CameraPreset* cam)
{
    gluLookAt(cam->eyeX, cam->eyeY, cam->eyeZ,
        cam->centerX, cam->centerY, cam->centerZ,
        cam->upX, cam->upY, cam->upZ);
}

static void setCamera(void)
{
    if (cameraMode < 0 || cameraMode >= MAIN_CAMERA_COUNT)
        cameraMode = 0;
    applyCameraPreset(&mainCameras[cameraMode]);
}

static void setSubCamera(void)
{
    if (subCameraMode < 0 || subCameraMode >= SUB_CAMERA_COUNT)
        subCameraMode = 0;
    applyCameraPreset(&subCameras[subCameraMode]);
}

/* Kamera kurulduktan sonra iki yumusak studyo isigi ve iki far spotu ayarlanir. */
static void configureSceneLights(void)
{
    GLfloat keyPosition[] = { 3.5f, 5.5f, 3.0f, 1.0f };
    GLfloat keyDiffuse[] = { 0.92f, 0.88f, 0.82f, 1.0f };
    GLfloat fillPosition[] = { -4.0f, 2.8f, -2.5f, 1.0f };
    GLfloat fillDiffuse[] = { 0.32f, 0.42f, 0.58f, 1.0f };
    GLfloat fillSpecular[] = { 0.50f, 0.58f, 0.72f, 1.0f };
    GLfloat headColor[] = { 1.0f, 0.92f, 0.68f, 1.0f };
    GLfloat headDirection[] = { 0.0f, -0.12f, 1.0f };
    GLfloat leftHead[] = { -0.32f, 0.10f, 0.96f, 1.0f };
    GLfloat rightHead[] = { 0.32f, 0.10f, 0.96f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, keyPosition);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, keyDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, fillPosition);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, fillDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, fillSpecular);

    glLightfv(GL_LIGHT2, GL_POSITION, leftHead);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, headColor);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, headDirection);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 24.0f);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 18.0f);
    glLightfv(GL_LIGHT3, GL_POSITION, rightHead);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, headColor);
    glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, headDirection);
    glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 24.0f);
    glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 18.0f);

    if (headlightsEnabled && solidLightEnabled) {
        glEnable(GL_LIGHT2);
        glEnable(GL_LIGHT3);
    } else {
        glDisable(GL_LIGHT2);
        glDisable(GL_LIGHT3);
    }
}

static void drawBitmapText(float x, float y, const char* text)
{
    glRasterPos2f(x, y);
    while (*text)
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (int)*text++);
}

/* Sol ust kose: tum klavye kontrolleri (H ile gizlenir) */
static void drawHelpPanel(void)
{
    const float padL = 8.0f;
    const float padT = 6.0f;
    const float padB = 5.0f;
    const float lineStep = 16.0f;
    const float textX = padL + 6.0f;
    const float panelWidth = 410.0f;
    const float boxTop = (float)mainWinH - padT;
    const float firstBase = boxTop - 13.0f;
    const float lastBase = firstBase - 10.0f * lineStep;
    const float boxBottom = lastBase - padB;
    const float boxRight = padL + panelWidth;
    float y;
    char statusText[160];

    if (!showHelpPanel)
        return;

    GLint lightingWasEnabled = glIsEnabled(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, (double)mainWinW, 0.0, (double)mainWinH, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.06f, 0.07f, 0.10f, 0.88f);
    glBegin(GL_QUADS);
    glVertex2f(padL, boxBottom);
    glVertex2f(boxRight, boxBottom);
    glVertex2f(boxRight, boxTop);
    glVertex2f(padL, boxTop);
    glEnd();

    y = firstBase;
    glColor3f(0.95f, 0.90f, 0.35f);
    drawBitmapText(textX, y, "KONTROL PANELI  [H] gizle/goster");
    y -= lineStep;

    glColor3f(0.82f, 0.86f, 0.92f);
    drawBitmapText(textX, y, "Sol/Sag ok veya A/S .... Direksiyon");
    y -= lineStep;
    drawBitmapText(textX, y, "1 - 7 ................ Ana kamera");
    y -= lineStep;
    drawBitmapText(textX, y, "L veya 9 ............. Kaplama + isik");
    y -= lineStep;
    drawBitmapText(textX, y, "C .................... Sonraki govde rengi");
    y -= lineStep;
    drawBitmapText(textX, y, "V .................... Kaplama tipi");
    y -= lineStep;
    drawBitmapText(textX, y, "F .................... Farlari ac/kapa");
    y -= lineStep;
    drawBitmapText(textX, y, "Q W E R T ............ Alt pencere kamera");
    y -= lineStep;
    drawBitmapText(textX, y, "Sag tik .............. Kamera menusu");
    y -= lineStep;
    drawBitmapText(textX, y, "ESC .................. Cikis");
    y -= lineStep;

    glColor3f(0.55f, 0.75f, 0.55f);
    drawBitmapText(textX, y, solidLightEnabled ? "Mod: Kaplama + isik ACIK" : "Mod: Tel (isik kapali)");
    y -= lineStep;
    snprintf(statusText, sizeof(statusText), "Boya: %s | Yuzey: %s | Far: %s",
        paintColors[paintColorIndex].name, finishNames[paintFinishIndex],
        headlightsEnabled ? "ACIK" : "KAPALI");
    drawBitmapText(textX, y, statusText);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    if (lightingWasEnabled && solidLightEnabled)
        glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void toggleSolidLight(void)
{
    solidLightEnabled = !solidLightEnabled;
    requestRedrawBoth();
}

/* Ana pencere cizim callback'i */
void displayMain(void)
{
    glutSetWindow(mainWindow);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    setCamera();
    configureSceneLights();
    drawRoad();
    drawCar();
    drawHelpPanel();

    glutSwapBuffers();
}

/* Alt pencere cizim callback'i — bagimsiz subCameraMode kullanir */
void displaySub(void)
{
    glutSetWindow(subWindow);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    setSubCamera();
    configureSceneLights();

    drawRoad();
    drawCar();

    glutSwapBuffers();
}

/* Sag tik ana menu ve alt menuler */
void menuSelect(int option)
{
    if (option >= 100 && option < 100 + MAIN_CAMERA_COUNT)
        cameraMode = option - 100;
    else if (option == 300)
        headlightsEnabled = !headlightsEnabled;
    else if (option == 301)
        solidLightEnabled = !solidLightEnabled;
    else if (option == 999)
        exit(0);

    requestRedrawBoth();
}

void colorMenuSelect(int option)
{
    if (option >= 0 && option < PAINT_COLOR_COUNT)
        paintColorIndex = option;
    requestRedrawBoth();
}

void finishMenuSelect(int option)
{
    if (option >= 0 && option < PAINT_FINISH_COUNT)
        paintFinishIndex = option;
    requestRedrawBoth();
}

void createMenu(void)
{
    int i;
    int cameraMenu;
    int colorMenu;
    int finishMenu;

    cameraMenu = glutCreateMenu(menuSelect);
    for (i = 0; i < MAIN_CAMERA_COUNT; i++)
        glutAddMenuEntry(mainCameras[i].label, 100 + i);

    colorMenu = glutCreateMenu(colorMenuSelect);
    for (i = 0; i < PAINT_COLOR_COUNT; i++)
        glutAddMenuEntry(paintColors[i].name, i);

    finishMenu = glutCreateMenu(finishMenuSelect);
    for (i = 0; i < PAINT_FINISH_COUNT; i++)
        glutAddMenuEntry(finishNames[i], i);

    glutCreateMenu(menuSelect);
    glutAddSubMenu("Kamera Secimi", cameraMenu);
    glutAddSubMenu("Govde Rengi", colorMenu);
    glutAddSubMenu("Kaplama Tipi", finishMenu);
    glutAddMenuEntry("Farlari Ac / Kapat", 300);
    glutAddMenuEntry("Tel / Kaplama Modu", 301);
    glutAddMenuEntry("Cikis", 999);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/*
 * Direksiyon fizigi: A basili -> sola, S basili -> saga.
 * Tus birakilinca steerAngle yavasca sifira doner (±30 derece sinir).
 * steerAngle hem on tekerleklerde hem direksiyon simidinde kullanilir.
 */
static void updateSteering(void)
{
    if (keyLeft)
    {
        steerAngle += 2.0f;
        if (steerAngle > MAX_STEER)
            steerAngle = MAX_STEER;
    }
    else if (keyRight)
    {
        steerAngle -= 2.0f;
        if (steerAngle < -MAX_STEER)
            steerAngle = -MAX_STEER;
    }
    else
    {
        if (steerAngle > 0.0f)
            steerAngle -= 1.5f;
        if (steerAngle < 0.0f)
            steerAngle += 1.5f;
        if (fabsf(steerAngle) < 1.5f)
            steerAngle = 0.0f;
    }
}

/*
 * Klavye basildi — etkilesim ve kamera kontrolleri.
 * solidLightEnabled toggle (9) kamera acisini degistirmez.
 */
void keyDown(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == 'a' || key == 'A')
        keyLeft = 1;
    if (key == 's' || key == 'S')
        keyRight = 1;

    if (key >= '1' && key <= '7')
    {
        cameraMode = key - '1';
        requestRedrawBoth();
    }

    if (key == '9' || key == 'l' || key == 'L')
    {
        toggleSolidLight();
    }

    if (key == 'c' || key == 'C')
    {
        paintColorIndex = (paintColorIndex + 1) % PAINT_COLOR_COUNT;
        requestRedrawBoth();
    }

    if (key == 'v' || key == 'V')
    {
        paintFinishIndex = (paintFinishIndex + 1) % PAINT_FINISH_COUNT;
        requestRedrawBoth();
    }

    if (key == 'f' || key == 'F')
    {
        headlightsEnabled = !headlightsEnabled;
        requestRedrawBoth();
    }

    if (key == 'h' || key == 'H')
    {
        showHelpPanel = !showHelpPanel;
        requestRedrawBoth();
    }

    /* Alt pencere kameralari (ana pencereden bagimsiz) */
    if (key == 'q' || key == 'Q')
    {
        subCameraMode = 0;
        requestRedrawBoth();
    }
    if (key == 'w' || key == 'W')
    {
        subCameraMode = 1;
        requestRedrawBoth();
    }
    if (key == 'e' || key == 'E')
    {
        subCameraMode = 2;
        requestRedrawBoth();
    }
    if (key == 'r' || key == 'R')
    {
        subCameraMode = 3;
        requestRedrawBoth();
    }
    if (key == 't' || key == 'T')
    {
        subCameraMode = 4;
        requestRedrawBoth();
    }

    if (key == 27)
        exit(0);
}

void keyUp(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == 'a' || key == 'A')
        keyLeft = 0;
    if (key == 's' || key == 'S')
        keyRight = 0;
}

void specialKeyDown(int key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == GLUT_KEY_LEFT)
        keyLeft = 1;
    if (key == GLUT_KEY_RIGHT)
        keyRight = 1;
}

void specialKeyUp(int key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == GLUT_KEY_LEFT)
        keyLeft = 0;
    if (key == GLUT_KEY_RIGHT)
        keyRight = 0;
}

/*
 * Direksiyon, tekerlek donusu ve yol cizgisi
 * guncellenir; her iki pencere yeniden cizilir.
 */
void timer(int value)
{
    (void)value;

    updateSteering();

    wheelAngle += 3.0f;
    if (wheelAngle > 360.0f)
        wheelAngle -= 360.0f;

    roadOffset += 0.02f;
    if (roadOffset > ROAD_LOOP)
        roadOffset = 0.0f;

    requestRedrawBoth();
    glutTimerFunc(16, timer, 0);
}

/* Ana pencere boyutu degisince viewport + alt pencere konumu ayarlanir */
void reshapeMain(int w, int h)
{
    if (h == 0)
        h = 1;

    glutSetWindow(subWindow);
    glutPositionWindow(w - 260, 10);
    glutReshapeWindow(240, 180);

    glutSetWindow(mainWindow);
    glViewport(0, 0, w, h);
    mainWinW = w;
    mainWinH = h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 1.0, 100.0);
}

void reshapeSub(int w, int h)
{
    if (h == 0)
        h = 1;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 1.0, 100.0);
}

/*
 * Her iki pencere icin ortak OpenGL baslangic ayarlari:
 * - Derinlik testi (GL_DEPTH_TEST)
 * - Tek yonlu isik GL_LIGHT0, konum (3, 5, -4)
 * - GL_NORMALIZE: olceklenmis solid yuzeylerde dogru aydinlatma
 */
void initShared(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    GLfloat lightPosition[] = { 3.0f, 5.0f, -4.0f, 1.0f };
    GLfloat lightAmbient[] = { 0.22f, 0.22f, 0.22f, 1.0f };
    GLfloat lightDiffuse[] = { 0.85f, 0.85f, 0.85f, 1.0f };
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    if (solidQuadric == NULL)
        solidQuadric = gluNewQuadric();
}

/* Program girisi: ana + alt pencere, menu, timer, klavye callback'leri */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(950, 700);

    mainWindow = glutCreateWindow("WireCar 1.1 [C renk | V kaplama | F far | H panel]");
    initShared();
    glutDisplayFunc(displayMain);
    glutReshapeFunc(reshapeMain);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    createMenu();

    subWindow = glutCreateSubWindow(mainWindow, 680, 10, 240, 180);
    initShared();
    glutDisplayFunc(displaySub);
    glutReshapeFunc(reshapeSub);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);

    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
