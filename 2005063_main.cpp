#include "2005063_classes.h"
#include "bitmap_image.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <GL/glut.h>

using namespace std;

vector<Object*> objects;
vector<PointLight> pointLights;
vector<SpotLight> spotLights;
Floor* globalFloor = nullptr;

int recursionLevel;

Vector3D cameraPos(0, -500, 200);
Vector3D cameraLookDir(0, 1, 0);
Vector3D cameraUp(0, 0, 1);
Vector3D cameraRight(1, 0, 0);
float cameraTilt = 0.0;

void loadData() {
    ifstream sceneFile("scene.txt");
    if (!sceneFile.is_open()) {
        cerr << "Error: Could not open scene.txt" << endl;
        return;
    }

    int imageResolution;
    sceneFile >> recursionLevel >> imageResolution;

    int numObjects;
    sceneFile >> numObjects;

    for (int i = 0; i < numObjects; i++) {
        string objectType;
        sceneFile >> objectType;

        if (objectType == "sphere") {
            Vector3D center;
            double radius;
            double color[3], coEfficients[4];
            int shine;

            sceneFile >> center.x >> center.y >> center.z;
            sceneFile >> radius;
            sceneFile >> color[0] >> color[1] >> color[2];
            sceneFile >> coEfficients[0] >> coEfficients[1] >> coEfficients[2] >> coEfficients[3];
            sceneFile >> shine;

            Sphere* sphere = new Sphere(center, radius);
            sphere->setColor(color[0], color[1], color[2]);
            sphere->setCoEfficients(coEfficients[0], coEfficients[1], coEfficients[2], coEfficients[3]);
            sphere->setShine(shine);
            objects.push_back(sphere);
        } else if (objectType == "triangle") {
            Vector3D p1, p2, p3;
            double color[3], coEfficients[4];
            int shine;

            sceneFile >> p1.x >> p1.y >> p1.z;
            sceneFile >> p2.x >> p2.y >> p2.z;
            sceneFile >> p3.x >> p3.y >> p3.z;
            sceneFile >> color[0] >> color[1] >> color[2];
            sceneFile >> coEfficients[0] >> coEfficients[1] >> coEfficients[2] >> coEfficients[3];
            sceneFile >> shine;

            Triangle* triangle = new Triangle(p1, p2, p3);
            triangle->setColor(color[0], color[1], color[2]);
            triangle->setCoEfficients(coEfficients[0], coEfficients[1], coEfficients[2], coEfficients[3]);
            triangle->setShine(shine);
            objects.push_back(triangle);
        } else if (objectType == "general") {
            double A, B, C, D, E, F, G, H, I, J;
            Vector3D cubeReferencePoint;
            double length, width, height;
            double color[3], coEfficients[4];
            int shine;

            sceneFile >> A >> B >> C >> D >> E >> F >> G >> H >> I >> J;
            sceneFile >> cubeReferencePoint.x >> cubeReferencePoint.y >> cubeReferencePoint.z;
            sceneFile >> length >> width >> height;
            sceneFile >> color[0] >> color[1] >> color[2];
            sceneFile >> coEfficients[0] >> coEfficients[1] >> coEfficients[2] >> coEfficients[3];
            sceneFile >> shine;

            General* general = new General(A, B, C, D, E, F, G, H, I, J, cubeReferencePoint, length, width, height);
            general->setColor(color[0], color[1], color[2]);
            general->setCoEfficients(coEfficients[0], coEfficients[1], coEfficients[2], coEfficients[3]);
            general->setShine(shine);
            objects.push_back(general);
        }
    }

    int numPointLights;
    sceneFile >> numPointLights;

    for (int i = 0; i < numPointLights; i++) {
        Vector3D position;
        double color[3];

        sceneFile >> position.x >> position.y >> position.z;
        sceneFile >> color[0] >> color[1] >> color[2];

        PointLight pointLight(position, color[0], color[1], color[2]);
        pointLights.push_back(pointLight);
    }

    int numSpotLights;
    sceneFile >> numSpotLights;

    for (int i = 0; i < numSpotLights; i++) {
        Vector3D position, direction;
        double color[3], cutoffAngle;

        sceneFile >> position.x >> position.y >> position.z;
        sceneFile >> color[0] >> color[1] >> color[2];
        sceneFile >> direction.x >> direction.y >> direction.z;
        sceneFile >> cutoffAngle;

        SpotLight spotLight(position, color[0], color[1], color[2], direction, cutoffAngle);
        spotLights.push_back(spotLight);
    }

    sceneFile.close();
    
    Floor* floor = new Floor(1000, 20, "");
    floor->setColor(1.0, 1.0, 1.0);
    floor->setCoEfficients(0.4, 0.2, 0.2, 0.2);
    floor->setShine(1);
    objects.push_back(floor);
    globalFloor = floor;
}

void capture() {
    const int imageWidth = 1920;
    const int imageHeight = 1920;
    bitmap_image image(imageWidth, imageHeight);
    image.clear();

    Vector3D eye = cameraPos;
    Vector3D target = cameraPos + cameraLookDir;
    
    Vector3D l = cameraLookDir;
    Vector3D r = cameraRight;   
    Vector3D u = cameraUp;

    double fov = 70.0 * M_PI / 180.0;
    double aspect = 1.0;
    double nearPlane = 1.0;
    
    double halfHeight = nearPlane * tan(fov / 2.0);
    double halfWidth = halfHeight * aspect;
    
    Vector3D center = eye + l * nearPlane;
    Vector3D topLeft = center + u * halfHeight - r * halfWidth;
    
    double pixelWidth = (2.0 * halfWidth) / imageWidth;
    double pixelHeight = (2.0 * halfHeight) / imageHeight;

    for (int i = 0; i < imageWidth; i++) {
        for (int j = 0; j < imageHeight; j++) {
            Vector3D pixelPos = topLeft + r * (i * pixelWidth) - u * (j * pixelHeight);
            
            Vector3D rayDir = pixelPos - eye;
            Ray ray(eye, rayDir);

            double tMin = 1e9;
            Object* nearestObject = nullptr;
            double pixelColor[3] = {0, 0, 0};

            for (Object* obj : objects) {
                double t = obj->intersect(&ray, pixelColor, 0);
                if (t > 0 && t < tMin) {
                    tMin = t;
                    nearestObject = obj;
                }
            }

            if (nearestObject) {
                nearestObject->intersect(&ray, pixelColor, 1);
                
                pixelColor[0] = std::max(0.0, std::min(1.0, pixelColor[0]));
                pixelColor[1] = std::max(0.0, std::min(1.0, pixelColor[1]));
                pixelColor[2] = std::max(0.0, std::min(1.0, pixelColor[2]));
                
                image.set_pixel(i, j, 
                    (unsigned char)(pixelColor[0] * 255), 
                    (unsigned char)(pixelColor[1] * 255), 
                    (unsigned char)(pixelColor[2] * 255));
            } else {
                image.set_pixel(i, j, 0, 0, 0);
            }
        }
    }

    static int imageCount = 11;
    std::ostringstream filename;
    filename << "Output_" << imageCount++ << ".bmp";
    image.save_image(filename.str());
    std::cout << "Image saved as " << filename.str() << std::endl;
}

void drawAxes() {
    glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(-1000, 0, 0);
    glVertex3f(1000, 0, 0);
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0, -1000, 0);
    glVertex3f(0, 1000, 0);
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0, 0, -1000);
    glVertex3f(0, 0, 1000);
    glEnd();
}

void drawLightSources() {
    for (const auto& light : pointLights) {
        glBegin(GL_POINTS);
        glColor3f(light.color[0], light.color[1], light.color[2]);
        glVertex3f(light.light_pos.x, light.light_pos.y, light.light_pos.z);
        glEnd();
    }

    for (const auto& light : spotLights) {
        glBegin(GL_POINTS);
        glColor3f(light.color[0], light.color[1], light.color[2]);
        glVertex3f(light.light_pos.x, light.light_pos.y, light.light_pos.z);
        glEnd();
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vector3D target = cameraPos + cameraLookDir;
    gluLookAt(cameraPos.x, cameraPos.y, cameraPos.z,
              target.x, target.y, target.z,
              cameraUp.x, cameraUp.y, cameraUp.z);

    drawAxes();

    drawLightSources();

    for (Object* obj : objects) {
        obj->draw();
    }

    glutSwapBuffers();
}

void updateCameraVectorsWithTilt() {
    double length = sqrt(cameraLookDir.x * cameraLookDir.x + cameraLookDir.y * cameraLookDir.y + cameraLookDir.z * cameraLookDir.z);
    cameraLookDir.x /= length;
    cameraLookDir.y /= length;
    cameraLookDir.z /= length;
    
    length = sqrt(cameraRight.x * cameraRight.x + cameraRight.y * cameraRight.y + cameraRight.z * cameraRight.z);
    cameraRight.x /= length;
    cameraRight.y /= length;
    cameraRight.z /= length;
    
    length = sqrt(cameraUp.x * cameraUp.x + cameraUp.y * cameraUp.y + cameraUp.z * cameraUp.z);
    cameraUp.x /= length;
    cameraUp.y /= length;
    cameraUp.z /= length;
}

void updateCameraVectors() {
    double length = sqrt(cameraLookDir.x * cameraLookDir.x + cameraLookDir.y * cameraLookDir.y + cameraLookDir.z * cameraLookDir.z);
    cameraLookDir.x /= length;
    cameraLookDir.y /= length;
    cameraLookDir.z /= length;
    
    Vector3D worldUp(0, 0, 1);
    cameraRight.x = cameraLookDir.y * worldUp.z - cameraLookDir.z * worldUp.y;
    cameraRight.y = cameraLookDir.z * worldUp.x - cameraLookDir.x * worldUp.z;
    cameraRight.z = cameraLookDir.x * worldUp.y - cameraLookDir.y * worldUp.x;
    
    length = sqrt(cameraRight.x * cameraRight.x + cameraRight.y * cameraRight.y + cameraRight.z * cameraRight.z);
    cameraRight.x /= length;
    cameraRight.y /= length;
    cameraRight.z /= length;
    
    cameraUp.x = cameraRight.y * cameraLookDir.z - cameraRight.z * cameraLookDir.y;
    cameraUp.y = cameraRight.z * cameraLookDir.x - cameraRight.x * cameraLookDir.z;
    cameraUp.z = cameraRight.x * cameraLookDir.y - cameraRight.y * cameraLookDir.x;
}

void keyboardListener(unsigned char key, int x, int y) {
    const double ROTATE_SPEED = 0.1;
    
    switch (key) {
        case '1':
            {
                double cosAngle = cos(-ROTATE_SPEED);
                double sinAngle = sin(-ROTATE_SPEED);
                double newX = cameraLookDir.x * cosAngle - cameraLookDir.y * sinAngle;
                double newY = cameraLookDir.x * sinAngle + cameraLookDir.y * cosAngle;
                cameraLookDir.x = newX;
                cameraLookDir.y = newY;
                updateCameraVectors();
            }
            break;
        case '2':
            {
                double cosAngle = cos(ROTATE_SPEED);
                double sinAngle = sin(ROTATE_SPEED);
                double newX = cameraLookDir.x * cosAngle - cameraLookDir.y * sinAngle;
                double newY = cameraLookDir.x * sinAngle + cameraLookDir.y * cosAngle;
                cameraLookDir.x = newX;
                cameraLookDir.y = newY;
                updateCameraVectors();
            }
            break;
        case '3':
            {
                Vector3D temp = cameraLookDir + cameraUp * ROTATE_SPEED;
                double length = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z);
                cameraLookDir.x = temp.x / length;
                cameraLookDir.y = temp.y / length;
                cameraLookDir.z = temp.z / length;
                updateCameraVectors();
            }
            break;
        case '4':
            {
                Vector3D temp = cameraLookDir - cameraUp * ROTATE_SPEED;
                double length = sqrt(temp.x * temp.x + temp.y * temp.y + temp.z * temp.z);
                cameraLookDir.x = temp.x / length;
                cameraLookDir.y = temp.y / length;
                cameraLookDir.z = temp.z / length;
                updateCameraVectors();
            }
            break;
        case '5':
            {
                Vector3D newUp = cameraUp * cos(-ROTATE_SPEED) + cameraRight * sin(-ROTATE_SPEED);
                Vector3D newRight = cameraRight * cos(-ROTATE_SPEED) - cameraUp * sin(-ROTATE_SPEED);
                cameraUp = newUp;
                cameraRight = newRight;
                updateCameraVectorsWithTilt();
            }
            break;
        case '6':
            {
                Vector3D newUp = cameraUp * cos(ROTATE_SPEED) + cameraRight * sin(ROTATE_SPEED);
                Vector3D newRight = cameraRight * cos(ROTATE_SPEED) - cameraUp * sin(ROTATE_SPEED);
                cameraUp = newUp;
                cameraRight = newRight;
                updateCameraVectorsWithTilt();
            }
            break;
        case '0':
            capture();
            break;
        case 'c':
            capture();
            break;
        case 't':
            if (globalFloor) {
                globalFloor->toggleTexture();
                cout << "Floor texture toggled. Current mode: " << (globalFloor->useTexture ? "Texture" : "Checkerboard") << endl;
            }
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

void specialKeyListener(int key, int x, int y) {
    const double moveSpeed = 20.0;
    
    switch (key) {
        case GLUT_KEY_UP:
            cameraPos = cameraPos + cameraLookDir * moveSpeed;
            break;
        case GLUT_KEY_DOWN:
            cameraPos = cameraPos - cameraLookDir * moveSpeed;
            break;
        case GLUT_KEY_LEFT:
            cameraPos = cameraPos - cameraRight * moveSpeed;
            break;
        case GLUT_KEY_RIGHT:
            cameraPos = cameraPos + cameraRight * moveSpeed;
            break;
        case GLUT_KEY_PAGE_UP:
            cameraPos.z += moveSpeed;
            break;
        case GLUT_KEY_PAGE_DOWN:
            cameraPos.z -= moveSpeed;
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

void init() {
    glClearColor(0, 0, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70, 1, 0.1, 10000);
}

int main(int argc, char** argv) {
    loadData();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Ray Tracer OpenGL Viewer");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboardListener);
    glutSpecialFunc(specialKeyListener);

    glutMainLoop();

    for (Object* obj : objects) {
        delete obj;
    }
    objects.clear();

    return 0;
}