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
#define MAX_STEER 30.0f   /* Dusuk hizdaki azami on tekerlek acisi */
#define HIGH_SPEED_STEER 12.0f /* Son hizda izin verilen daha dar direksiyon acisi */
#define MAX_FORWARD_SPEED 2.15f
#define MAX_REVERSE_SPEED 1.05f
#define VEHICLE_WHEELBASE 1.24f
#define VEHICLE_TRACK 1.24f

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
static float steerAngle = 0.0f; /* On teker + direksiyon simidi ortak acisi */
static float drivetrainAngle = 0.0f; /* Diferansiyel/disli animasyonu */
static float carTravelX = 0.0f;  /* Arabanin yol uzerindeki yatay konumu */
static float carTravelZ = 0.0f;  /* Arabanin sabit yol uzerindeki konumu */
static float carHeading = 0.0f;   /* Arabanin dunya uzayindaki yonu */
static float carSpeed = 0.0f;     /* Ileri pozitif, geri negatif (dunya birimi/s) */
static int lastTimerMs = 0;       /* Kare hizindan bagimsiz fizik zamanlayicisi */
static float chaseEyeX = 0.0f;    /* Yumusak takip kamerası: goz X */
static float chaseEyeZ = 0.0f;    /* Yumusak takip kamerası: goz Z */
static float chaseCenterX = 0.0f; /* Yumusak takip kamerası: merkez X */
static float chaseCenterZ = 0.0f; /* Yumusak takip kamerası: merkez Z */
static int chaseCameraReady = 0;  /* Ilk karede takibi hedefe oturt */

static int keyLeft = 0;  /* A tusuna basili mi */
static int keyRight = 0; /* D tusuna basili mi */
static int keyForward = 0;  /* W / yukari ok */
static int keyBackward = 0; /* S / asagi ok */

/* --- Kamera ve render modu --- */
static int cameraMode = 6;        /* Suruste varsayilan: araci izleyen Kamera 7 */
static int solidLightEnabled = 1; /* 1=bonus (solid+isik), 0=yalnizca tel */
static int headlightsEnabled = 1; /* F tusu ile farlar ac/kapa */
static int showHelpPanel = 1;     /* H tusu ile ac/kapa */
static int fourWheelDriveEnabled = 1; /* X tusu: 1=4x4, 0=arka cekis */
static int mainWinW = 950;
static int mainWinH = 700;

#define MAIN_CAMERA_COUNT 7

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
    {  0.0f,  1.8f, -5.2f,  0.0f, 0.45f, 1.4f, 0.0f, 1.0f,  0.0f, "Takip Kamerasi (Kamera 7)" }
};

int mainWindow;

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

/* Tek ana surus penceresini yeniden cizer. */
static void requestRedrawBoth(void)
{
    glutSetWindow(mainWindow);
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
 * wheelSteerAngle on tekerleklerde Ackermann acisini tasir; arkada sifirdir.
 *   1) glTranslatef  — tekerlegi arabanin koselerine tasir
 *   2) steer (Y)     — yalniz on tekerlerde direksiyon acisi
 *   3) yatir (Y 90)  — torus eksenini yatay konuma getirir
 *   4) spin (Z)      — wheelAngle ile donme animasyonu
 */
static void drawWheel(float x, float y, float z, float wheelSteerAngle)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    if (fabsf(wheelSteerAngle) > 0.001f)
        glRotatef(wheelSteerAngle, 0.0f, 1.0f, 0.0f);

    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-wheelAngle, 0.0f, 0.0f, 1.0f);

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
 * Acik duzlem: yol cizgisi yok, sadece genis zemin, ısikli grid ve ufuk siluetleri var.
 */
static void drawRoad(void)
{
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    /* Ana zemin: koyu, genis ve bos bir duzlem. */
    glColor3f(0.05f, 0.06f, 0.08f);
    glBegin(GL_QUADS);
    glVertex3f(-34.0f, -0.54f, -140.0f);
    glVertex3f(34.0f, -0.54f, -140.0f);
    glVertex3f(34.0f, -0.54f, 140.0f);
    glVertex3f(-34.0f, -0.54f, 140.0f);
    glEnd();

    /* Hafif grid: parlak ama yol cizgisi gibi olmayan bir referans yuzeyi. */
    glColor4f(0.10f, 0.78f, 1.0f, 0.10f);
    glLineWidth(2.2f);
    glBegin(GL_LINES);
    for (float x = -32.0f; x <= 32.0f; x += 2.0f)
    {
        glVertex3f(x, -0.529f, -140.0f);
        glVertex3f(x, -0.529f, 140.0f);
    }
    for (float z = -140.0f; z <= 140.0f; z += 3.5f)
    {
        glVertex3f(-34.0f, -0.529f, z);
        glVertex3f(34.0f, -0.529f, z);
    }
    glEnd();

    glColor4f(0.55f, 0.95f, 1.0f, 0.30f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -32.0f; x <= 32.0f; x += 4.0f)
    {
        glVertex3f(x, -0.529f, -140.0f);
        glVertex3f(x, -0.529f, 140.0f);
    }
    for (float z = -140.0f; z <= 140.0f; z += 8.0f)
    {
        glVertex3f(-34.0f, -0.529f, z);
        glVertex3f(34.0f, -0.529f, z);
    }
    glEnd();

    /* Kenar referanslari: yol hissi vermeden derinlik katar. */
    glColor4f(0.35f, 0.80f, 0.95f, 0.36f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (float z = -126.0f; z <= 126.0f; z += 12.0f)
    {
        glVertex3f(-27.0f, -0.53f, z);
        glVertex3f(-27.0f, -0.08f, z);
        glVertex3f(27.0f, -0.53f, z);
        glVertex3f(27.0f, -0.08f, z);
    }
    glEnd();

    /* Ufuk siluetleri: bloklar ve hafif tepe formu. */
    glColor3f(0.10f, 0.12f, 0.15f);
    for (float x = -30.0f; x <= 30.0f; x += 6.0f)
    {
        float peak = 1.1f + fabsf(sinf(x * 0.17f)) * 2.4f;
        glPushMatrix();
        glTranslatef(x, -0.54f + peak * 0.5f, -132.0f);
        glScalef(2.2f, peak, 3.0f);
        glutSolidCube(1.0);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(x, -0.54f + (peak * 0.65f), 132.0f);
        glScalef(2.2f, peak * 0.8f, 3.0f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    glColor3f(0.85f, 0.92f, 1.0f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 80; ++i)
    {
        float sx = -33.0f + (float)(i % 16) * 4.0f;
        float sy = 10.0f + (float)(i % 5) * 1.8f;
        float sz = -120.0f + (float)(i / 16) * 60.0f;
        glVertex3f(sx, sy, sz);
    }
    glEnd();

    glLineWidth(1.0f);
    glPointSize(1.0f);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
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

/*
 * Ackermann geometrisi: virajin icindeki on tekerlek dis tekerlekten daha
 * fazla doner. Bu sayede iki teker ayni merkezin etrafinda yuvarlanir ve
 * arac gorsel olarak yana surukleniyormus gibi durmaz.
 */
static float getFrontWheelSteer(float wheelX)
{
    float absSteer = fabsf(steerAngle);
    float turnRadius;
    float innerDenominator;
    float innerAngle;
    float outerAngle;
    int isInnerWheel;

    if (absSteer < 0.01f)
        return 0.0f;

    turnRadius = VEHICLE_WHEELBASE / tanf(absSteer * PI / 180.0f);
    innerDenominator = turnRadius - VEHICLE_TRACK * 0.5f;
    if (innerDenominator < 0.05f)
        innerDenominator = 0.05f;

    innerAngle = atanf(VEHICLE_WHEELBASE / innerDenominator) * 180.0f / PI;
    outerAngle = atanf(VEHICLE_WHEELBASE /
        (turnRadius + VEHICLE_TRACK * 0.5f)) * 180.0f / PI;

    /* Pozitif direksiyon sola (+X), negatif direksiyon saga donustur. */
    isInnerWheel = (steerAngle > 0.0f && wheelX > 0.0f) ||
        (steerAngle < 0.0f && wheelX < 0.0f);

    return (steerAngle > 0.0f ? 1.0f : -1.0f) *
        (isInnerWheel ? innerAngle : outerAngle);
}

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
static void drawWireGear(float cx, float cy, float cz, float radius, int teeth, float phaseDeg)
{
    float step = 2.0f * PI / (float)(teeth * 2);
    float phase = phaseDeg * PI / 180.0f;
    int i;

    glBegin(GL_LINE_LOOP);
    for (i = 0; i < teeth * 2; i++)
    {
        float ang = i * step + phase;
        float r = (i % 2 == 0) ? radius : radius * 0.72f;
        glVertex3f(cx + r * cosf(ang), cy + r * sinf(ang), cz);
    }
    glEnd();

    glBegin(GL_LINES);
    glVertex3f(cx + cosf(phase) * radius * 0.55f, cy + sinf(phase) * radius * 0.55f, cz);
    glVertex3f(cx - cosf(phase) * radius * 0.55f, cy - sinf(phase) * radius * 0.55f, cz);
    glVertex3f(cx + cosf(phase + PI * 0.5f) * radius * 0.55f, cy + sinf(phase + PI * 0.5f) * radius * 0.55f, cz);
    glVertex3f(cx - cosf(phase + PI * 0.5f) * radius * 0.55f, cy - sinf(phase + PI * 0.5f) * radius * 0.55f, cz);
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
    glColor3f(fourWheelDriveEnabled ? 0.15f : 0.45f,
        fourWheelDriveEnabled ? 0.95f : 0.45f,
        fourWheelDriveEnabled ? 1.0f : 0.45f);

    /* On aks + saft + teker hub cizgileri */
    glBegin(GL_LINES);
    glVertex3f(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z); glVertex3f(WHEEL_X, AXLE_Y, FRONT_AXLE_Z);
    if (fourWheelDriveEnabled)
    {
        glVertex3f(0.0f, AXLE_Y, FRONT_AXLE_Z); glVertex3f(0.0f, CENTER_Y, 0.06f);
    }
    glVertex3f(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z); glVertex3f(-WHEEL_X, AXLE_Y + 0.05f, FRONT_AXLE_Z);
    glVertex3f(WHEEL_X, AXLE_Y, FRONT_AXLE_Z);  glVertex3f(WHEEL_X, AXLE_Y + 0.05f, FRONT_AXLE_Z);
    glEnd();
    drawWireBox(0.0f, AXLE_Y, FRONT_AXLE_Z, 0.045f, 0.03f, 0.035f);
    drawWireGear(0.0f, AXLE_Y, FRONT_AXLE_Z + 0.036f, 0.040f, 7,
        fourWheelDriveEnabled ? drivetrainAngle : 0.0f);

    /* Arka aks + saft + teker hub cizgileri */
    glColor3f(1.0f, 0.68f, 0.12f);
    glBegin(GL_LINES);
    glVertex3f(-WHEEL_X, AXLE_Y, REAR_AXLE_Z); glVertex3f(WHEEL_X, AXLE_Y, REAR_AXLE_Z);
    glVertex3f(0.0f, AXLE_Y, REAR_AXLE_Z);     glVertex3f(0.0f, CENTER_Y, CENTER_Z);
    glVertex3f(-WHEEL_X, AXLE_Y, REAR_AXLE_Z); glVertex3f(-WHEEL_X, AXLE_Y + 0.05f, REAR_AXLE_Z);
    glVertex3f(WHEEL_X, AXLE_Y, REAR_AXLE_Z);  glVertex3f(WHEEL_X, AXLE_Y + 0.05f, REAR_AXLE_Z);
    glEnd();
    drawWireBox(0.0f, AXLE_Y, REAR_AXLE_Z, 0.045f, 0.03f, 0.035f);
    drawWireGear(0.0f, AXLE_Y, REAR_AXLE_Z + 0.036f, 0.040f, 7, drivetrainAngle);

    /* Transfer kutusu: 4x4 acikken on ve arka aksa ayni anda guc dagitir. */
    glColor3f(fourWheelDriveEnabled ? 0.20f : 1.0f,
        fourWheelDriveEnabled ? 1.0f : 0.68f,
        fourWheelDriveEnabled ? 0.35f : 0.12f);
    drawWireGear(0.0f, CENTER_Y, CENTER_Z - 0.01f, 0.045f, 6, drivetrainAngle);
    drawWireBox(0.0f, CENTER_Y, CENTER_Z - 0.01f, 0.022f, 0.022f, 0.014f);
    drawWireGear(0.0f, CENTER_Y, 0.06f, 0.035f, 6,
        fourWheelDriveEnabled ? -drivetrainAngle : 0.0f);

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
    glRotatef(-steerAngle * 3.0f, 0.0f, 0.0f, 1.0f);
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

    drawWheel(WHEEL_X, AXLE_Y, FRONT_AXLE_Z, getFrontWheelSteer(WHEEL_X));
    drawWheel(-WHEEL_X, AXLE_Y, FRONT_AXLE_Z, getFrontWheelSteer(-WHEEL_X));
    drawWheel(WHEEL_X, AXLE_Y, REAR_AXLE_Z, 0.0f);
    drawWheel(-WHEEL_X, AXLE_Y, REAR_AXLE_Z, 0.0f);

    drawWindows();
}

/* Yol yerinde kalir; arabanin tamami Z ekseninde gercekten ilerler. */
static void drawMovingCar(void)
{
    glPushMatrix();
    glTranslatef(carTravelX, 0.0f, carTravelZ);
    glRotatef(carHeading, 0.0f, 1.0f, 0.0f);
    drawCar();
    glPopMatrix();
}

static void applyCameraPreset(const CameraPreset* cam)
{
    /* Kamera aracla ayni dunya ofsetini tasir; chase kamera aracin yonune de uyar. */
    if (cam == &mainCameras[6])
    {
        float rad = carHeading * PI / 180.0f;
        float sinH = sinf(rad);
        float cosH = cosf(rad);
        float targetEyeX = cam->eyeX * cosH + cam->eyeZ * sinH + carTravelX;
        float targetEyeZ = -cam->eyeX * sinH + cam->eyeZ * cosH + carTravelZ;
        float targetCenterX = cam->centerX * cosH + cam->centerZ * sinH + carTravelX;
        float targetCenterZ = -cam->centerX * sinH + cam->centerZ * cosH + carTravelZ;

        if (!chaseCameraReady)
        {
            chaseEyeX = targetEyeX;
            chaseEyeZ = targetEyeZ;
            chaseCenterX = targetCenterX;
            chaseCenterZ = targetCenterZ;
            chaseCameraReady = 1;
        }
        else
        {
            chaseEyeX += (targetEyeX - chaseEyeX) * 0.12f;
            chaseEyeZ += (targetEyeZ - chaseEyeZ) * 0.12f;
            chaseCenterX += (targetCenterX - chaseCenterX) * 0.16f;
            chaseCenterZ += (targetCenterZ - chaseCenterZ) * 0.16f;
        }

        gluLookAt(chaseEyeX, cam->eyeY, chaseEyeZ,
            chaseCenterX, cam->centerY, chaseCenterZ,
            cam->upX, cam->upY, cam->upZ);
        return;
    }

    chaseCameraReady = 0;
    gluLookAt(cam->eyeX + carTravelX, cam->eyeY, cam->eyeZ + carTravelZ,
        cam->centerX + carTravelX, cam->centerY, cam->centerZ + carTravelZ,
        cam->upX, cam->upY, cam->upZ);
}

static void setCamera(void)
{
    if (cameraMode < 0 || cameraMode >= MAIN_CAMERA_COUNT)
        cameraMode = 6;
    applyCameraPreset(&mainCameras[cameraMode]);
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
    const float lastBase = firstBase - 12.0f * lineStep;
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
    drawBitmapText(textX, y, "W/S veya Yukari/Asagi .. Ileri/geri");
    y -= lineStep;
    drawBitmapText(textX, y, "A/D veya Sol/Sag ok .... Direksiyon");
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
    drawBitmapText(textX, y, "X .................... 4x4 modu ac/kapa");
    y -= lineStep;
    drawBitmapText(textX, y, "Sag tik .............. Kamera menusu");
    y -= lineStep;
    drawBitmapText(textX, y, "ESC .................. Cikis");
    y -= lineStep;

    glColor3f(0.55f, 0.75f, 0.55f);
    drawBitmapText(textX, y, solidLightEnabled ? "Mod: Kaplama + isik ACIK" : "Mod: Tel (isik kapali)");
    y -= lineStep;
    glColor3f(fourWheelDriveEnabled ? 0.20f : 1.0f,
        fourWheelDriveEnabled ? 1.0f : 0.65f,
        fourWheelDriveEnabled ? 0.35f : 0.20f);
    drawBitmapText(textX, y, fourWheelDriveEnabled ? "CEKIS: 4x4 AKTIF (4 tekerden cekis)" : "CEKIS: 4x4 PASIF (arka cekis)");
    y -= lineStep;
    glColor3f(0.55f, 0.75f, 0.55f);
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
    drawMovingCar();
    drawHelpPanel();

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
 * Direksiyon fizigi: A sola, D saga. Hiz arttikca azami direksiyon acisi
 * daralir; tus birakilinca tekerlekler yumusakca merkeze gelir.
 */
static float moveTowards(float current, float target, float maxChange)
{
    if (current < target)
        return (current + maxChange > target) ? target : current + maxChange;
    if (current > target)
        return (current - maxChange < target) ? target : current - maxChange;
    return target;
}

static void updateSteering(float dt)
{
    float steerInput = 0.0f;
    float speedRatio = fabsf(carSpeed) / MAX_FORWARD_SPEED;
    float allowedSteer;
    float targetSteer;
    float steeringRate;

    if (speedRatio > 1.0f)
        speedRatio = 1.0f;

    if (keyLeft && !keyRight)
        steerInput = 1.0f;
    else if (keyRight && !keyLeft)
        steerInput = -1.0f;

    allowedSteer = MAX_STEER - (MAX_STEER - HIGH_SPEED_STEER) * speedRatio;
    targetSteer = steerInput * allowedSteer;

    /* Surucu cevirirken kontrollu, biraktiginda biraz daha hizli merkezleme. */
    steeringRate = (steerInput == 0.0f) ? 125.0f : 92.0f;
    steerAngle = moveTowards(steerAngle, targetSteer, steeringRate * dt);

    if (fabsf(steerAngle) < 0.03f)
        steerAngle = 0.0f;
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
    if (key == 'd' || key == 'D')
        keyRight = 1;
    if (key == 'w' || key == 'W')
        keyForward = 1;
    if (key == 's' || key == 'S')
        keyBackward = 1;

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

    if (key == 'x' || key == 'X')
    {
        fourWheelDriveEnabled = !fourWheelDriveEnabled;
        requestRedrawBoth();
    }

    if (key == 'h' || key == 'H')
    {
        showHelpPanel = !showHelpPanel;
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
    if (key == 'd' || key == 'D')
        keyRight = 0;
    if (key == 'w' || key == 'W')
        keyForward = 0;
    if (key == 's' || key == 'S')
        keyBackward = 0;
}

void specialKeyDown(int key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == GLUT_KEY_LEFT)
        keyLeft = 1;
    if (key == GLUT_KEY_RIGHT)
        keyRight = 1;
    if (key == GLUT_KEY_UP)
        keyForward = 1;
    if (key == GLUT_KEY_DOWN)
        keyBackward = 1;
}

void specialKeyUp(int key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == GLUT_KEY_LEFT)
        keyLeft = 0;
    if (key == GLUT_KEY_RIGHT)
        keyRight = 0;
    if (key == GLUT_KEY_UP)
        keyForward = 0;
    if (key == GLUT_KEY_DOWN)
        keyBackward = 0;
}

/*
 * Direksiyon, tekerlek donusu ve yol cizgisi
 * guncellenir; her iki pencere yeniden cizilir.
 */
void timer(int value)
{
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt;
    float movement;
    float acceleration;
    float headingChange;
    float middleHeadingRad;
    float nextX;
    float nextZ;
    int hitBoundary = 0;
    (void)value;

    if (lastTimerMs == 0)
        dt = 0.016f;
    else
        dt = (nowMs - lastTimerMs) / 1000.0f;
    lastTimerMs = nowMs;

    /* Pencere tasinirken veya debugger dururken fizik bir anda sicrama yapmasin. */
    if (dt < 0.001f) dt = 0.001f;
    if (dt > 0.050f) dt = 0.050f;

    updateSteering(dt);

    acceleration = fourWheelDriveEnabled ? 1.90f : 1.65f;

    if (keyForward && !keyBackward)
    {
        if (carSpeed < -0.02f)
            carSpeed = moveTowards(carSpeed, 0.0f, 3.80f * dt);
        else
            carSpeed = moveTowards(carSpeed, MAX_FORWARD_SPEED, acceleration * dt);
    }
    else if (keyBackward && !keyForward)
    {
        if (carSpeed > 0.02f)
            carSpeed = moveTowards(carSpeed, 0.0f, 3.80f * dt);
        else
            carSpeed = moveTowards(carSpeed, -MAX_REVERSE_SPEED, 1.35f * dt);
    }
    else
    {
        /* Gaz birakilinca arac aniden durmaz; yuvarlanma direnciyle yavaslar. */
        carSpeed = moveTowards(carSpeed, 0.0f, 0.82f * dt);
    }

    if (fabsf(carSpeed) < 0.002f)
        carSpeed = 0.0f;

    movement = carSpeed * dt;

    if (fabsf(movement) > 0.000001f)
    {
        /* Kinematik bisiklet modeli: yon degisimi hiz, dingil mesafesi ve
           gercek on teker acisindan hesaplanir. Geri giderken yon dogal terslenir. */
        headingChange = (movement / VEHICLE_WHEELBASE) *
            tanf(steerAngle * PI / 180.0f) * 180.0f / PI;
        middleHeadingRad = (carHeading + headingChange * 0.5f) * PI / 180.0f;
        nextX = carTravelX + sinf(middleHeadingRad) * movement;
        nextZ = carTravelZ + cosf(middleHeadingRad) * movement;

        carHeading += headingChange;
        while (carHeading > 180.0f) carHeading -= 360.0f;
        while (carHeading < -180.0f) carHeading += 360.0f;

        if (nextZ > 105.0f) { nextZ = 105.0f; hitBoundary = 1; }
        if (nextZ < -105.0f) { nextZ = -105.0f; hitBoundary = 1; }
        if (nextX > 20.0f) { nextX = 20.0f; hitBoundary = 1; }
        if (nextX < -20.0f) { nextX = -20.0f; hitBoundary = 1; }

        carTravelX = nextX;
        carTravelZ = nextZ;
        if (hitBoundary)
            carSpeed = 0.0f;

        wheelAngle += movement * (360.0f / (2.0f * PI * 0.16f));
        drivetrainAngle += movement * (fourWheelDriveEnabled ? 200.0f : 145.0f);
        if (wheelAngle > 360.0f) wheelAngle -= 360.0f;
        if (wheelAngle < -360.0f) wheelAngle += 360.0f;
        if (drivetrainAngle > 360.0f) drivetrainAngle -= 360.0f;
        if (drivetrainAngle < -360.0f) drivetrainAngle += 360.0f;
    }

    requestRedrawBoth();
    glutTimerFunc(16, timer, 0);
}

/* Ana surus penceresinin viewport ve perspektif ayari. */
void reshapeMain(int w, int h)
{
    if (h == 0)
        h = 1;

    glViewport(0, 0, w, h);
    mainWinW = w;
    mainWinH = h;
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

/* Program girisi: tek genis surus penceresi, menu, timer ve klavye. */
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(950, 700);

    mainWindow = glutCreateWindow("4x4WireCar [X 4x4 | C renk | V kaplama | F far | H panel]");
    initShared();
    glutDisplayFunc(displayMain);
    glutReshapeFunc(reshapeMain);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    createMenu();

    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
