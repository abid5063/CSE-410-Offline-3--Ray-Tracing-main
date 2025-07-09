#include "2005XXX_classes.h"
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

int recursionLevel; // Define recursionLevel in the main file

// Camera variables
float cameraAngle = 0.0;
float cameraHeight = 200.0;
float cameraRadius = 500.0;

// Additional camera variables for full control
Vector3D eye(0, -500, 200);
Vector3D l(0, 1, 0);  // Look direction
Vector3D r(1, 0, 0);  // Right direction
Vector3D u(0, 0, 1);  // Up direction

void loadData() {
    ifstream sceneFile("scene.txt");
    if (!sceneFile.is_open()) {
        cerr << "Error: Could not open scene.txt" << endl;
        return;
    }

    int imageResolution;
    sceneFile >> recursionLevel >> imageResolution;
    cout << "Recursion Level: " << recursionLevel << endl;
    cout << "Image Resolution: " << imageResolution << endl;

    int numObjects;
    sceneFile >> numObjects;
    cout << "Number of Objects: " << numObjects << endl;

    for (int i = 0; i < numObjects; i++) {
        string objectType;
        sceneFile >> objectType;
        cout << "Object Type: " << objectType << endl;

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

            cout << "  Center: (" << center.x << ", " << center.y << ", " << center.z << ")" << endl;
            cout << "  Radius: " << radius << endl;
            cout << "  Color: (" << color[0] << ", " << color[1] << ", " << color[2] << ")" << endl;
            cout << "  Coefficients: (" << coEfficients[0] << ", " << coEfficients[1] << ", " << coEfficients[2] << ", " << coEfficients[3] << ")" << endl;
            cout << "  Shine: " << shine << endl;

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

            cout << "  Point 1: (" << p1.x << ", " << p1.y << ", " << p1.z << ")" << endl;
            cout << "  Point 2: (" << p2.x << ", " << p2.y << ", " << p2.z << ")" << endl;
            cout << "  Point 3: (" << p3.x << ", " << p3.y << ", " << p3.z << ")" << endl;
            cout << "  Color: (" << color[0] << ", " << color[1] << ", " << color[2] << ")" << endl;
            cout << "  Coefficients: (" << coEfficients[0] << ", " << coEfficients[1] << ", " << coEfficients[2] << ", " << coEfficients[3] << ")" << endl;
            cout << "  Shine: " << shine << endl;

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

            cout << "  Coefficients: (" << A << ", " << B << ", " << C << ", " << D << ", " << E << ", " << F << ", " << G << ", " << H << ", " << I << ", " << J << ")" << endl;
            cout << "  Cube Reference Point: (" << cubeReferencePoint.x << ", " << cubeReferencePoint.y << ", " << cubeReferencePoint.z << ")" << endl;
            cout << "  Dimensions: (" << length << ", " << width << ", " << height << ")" << endl;
            cout << "  Color: (" << color[0] << ", " << color[1] << ", " << color[2] << ")" << endl;
            cout << "  Coefficients: (" << coEfficients[0] << ", " << coEfficients[1] << ", " << coEfficients[2] << ", " << coEfficients[3] << ")" << endl;
            cout << "  Shine: " << shine << endl;

            General* general = new General(A, B, C, D, E, F, G, H, I, J, cubeReferencePoint, length, width, height);
            general->setColor(color[0], color[1], color[2]);
            general->setCoEfficients(coEfficients[0], coEfficients[1], coEfficients[2], coEfficients[3]);
            general->setShine(shine);
            objects.push_back(general);
        }
    }

    int numPointLights;
    sceneFile >> numPointLights;
    cout << "Number of Point Lights: " << numPointLights << endl;

    for (int i = 0; i < numPointLights; i++) {
        Vector3D position;
        double color[3];

        sceneFile >> position.x >> position.y >> position.z;
        sceneFile >> color[0] >> color[1] >> color[2];

        cout << "  Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << endl;
        cout << "  Color: (" << color[0] << ", " << color[1] << ", " << color[2] << ")" << endl;

        PointLight pointLight(position, color[0], color[1], color[2]);
        pointLights.push_back(pointLight);
    }

    int numSpotLights;
    sceneFile >> numSpotLights;
    cout << "Number of Spot Lights: " << numSpotLights << endl;

    for (int i = 0; i < numSpotLights; i++) {
        Vector3D position, direction;
        double color[3], cutoffAngle;

        sceneFile >> position.x >> position.y >> position.z;
        sceneFile >> color[0] >> color[1] >> color[2];
        sceneFile >> direction.x >> direction.y >> direction.z;
        sceneFile >> cutoffAngle;

        cout << "  Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << endl;
        cout << "  Color: (" << color[0] << ", " << color[1] << ", " << color[2] << ")" << endl;
        cout << "  Direction: (" << direction.x << ", " << direction.y << ", " << direction.z << ")" << endl;
        cout << "  Cutoff Angle: " << cutoffAngle << endl;

        SpotLight spotLight(position, color[0], color[1], color[2], direction, cutoffAngle);
        spotLights.push_back(spotLight);
    }

    sceneFile.close();
    
    // Add floor object
    Floor* floor = new Floor(1000, 20, "");
    floor->setColor(1.0, 1.0, 1.0);
    floor->setCoEfficients(0.4, 0.2, 0.2, 0.2);
    floor->setShine(1);
    objects.push_back(floor);
}

void capture() {
    const int imageWidth = 1920;   // Increased from 768 to 1920
    const int imageHeight = 1920;  // Increased from 768 to 1920
    bitmap_image image(imageWidth, imageHeight);
    image.clear();

    // Use the current camera position and orientation
    Vector3D eye(cameraRadius * cos(cameraAngle), cameraRadius * sin(cameraAngle), cameraHeight);
    Vector3D l(-cos(cameraAngle), -sin(cameraAngle), 0); // Look direction
    Vector3D r(-sin(cameraAngle), cos(cameraAngle), 0);  // Right direction
    Vector3D u(0, 0, 1);  // Up direction

    // Calculate viewing window parameters
    double windowWidth = imageWidth;
    double windowHeight = imageHeight;
    double viewAngle = 70.0 * M_PI / 180.0; // 70 degrees in radians
    
    double planeDistance = (windowHeight / 2.0) / tan(viewAngle / 2.0);
    Vector3D topLeft = eye + l * planeDistance - r * (windowWidth / 2.0) + u * (windowHeight / 2.0);

    double du = windowWidth / (double)imageWidth;
    double dv = windowHeight / (double)imageHeight;

    // Choose middle of the grid cell
    topLeft = topLeft + r * (0.5 * du) - u * (0.5 * dv);

    for (int i = 0; i < imageWidth; i++) {
        for (int j = 0; j < imageHeight; j++) {
            Vector3D curPixel = topLeft + r * (i * du) - u * (j * dv);
            Vector3D rayDir = curPixel - eye;
            Ray ray(eye, rayDir);

            double tMin = 1e9;
            Object* nearestObject = nullptr;
            double pixelColor[3] = {0, 0, 0};

            // Find nearest object
            for (Object* obj : objects) {
                double t = obj->intersect(&ray, pixelColor, 0);
                if (t > 0 && t < tMin) {
                    tMin = t;
                    nearestObject = obj;
                }
            }

            if (nearestObject) {
                nearestObject->intersect(&ray, pixelColor, 1);
                
                // Clamp color values to [0, 1]
                pixelColor[0] = std::max(0.0, std::min(1.0, pixelColor[0]));
                pixelColor[1] = std::max(0.0, std::min(1.0, pixelColor[1]));
                pixelColor[2] = std::max(0.0, std::min(1.0, pixelColor[2]));
                
                image.set_pixel(i, j, 
                    (unsigned char)(pixelColor[0] * 255), 
                    (unsigned char)(pixelColor[1] * 255), 
                    (unsigned char)(pixelColor[2] * 255));
            } else {
                image.set_pixel(i, j, 0, 0, 0); // Background color
            }
        }
    }

    static int imageCount = 11; // Start from Output_11.bmp
    std::ostringstream filename;
    filename << "Output_" << imageCount++ << ".bmp";
    image.save_image(filename.str());
    std::cout << "Image saved as " << filename.str() << std::endl;
}

void drawAxes() {
    glBegin(GL_LINES);
    // X-axis
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(-1000, 0, 0);
    glVertex3f(1000, 0, 0);
    // Y-axis
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(0, -1000, 0);
    glVertex3f(0, 1000, 0);
    // Z-axis
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

    // Set camera position
    gluLookAt(cameraRadius * cos(cameraAngle), cameraRadius * sin(cameraAngle), cameraHeight,
              0, 0, 0, 0, 0, 1);

    drawAxes();

    // Draw light sources
    drawLightSources();

    // Draw objects in the scene
    for (Object* obj : objects) {
        obj->draw();
    }

    glutSwapBuffers();
}

void keyboardListener(unsigned char key, int x, int y) {
    switch (key) {
        case 'w':
            cameraHeight += 10;
            break;
        case 's':
            cameraHeight -= 10;
            break;
        case 'a':
            cameraAngle -= 0.1;
            break;
        case 'd':
            cameraAngle += 0.1;
            break;
        case 'c':
            capture();
            break;
        case '1':
            cameraAngle -= 0.1; // Rotate left
            break;
        case '2':
            cameraAngle += 0.1; // Rotate right
            break;
        case '3':
            cameraHeight += 10; // Look up
            break;
        case '4':
            cameraHeight -= 10; // Look down
            break;
        case '5':
            cameraRadius -= 10; // Tilt clockwise
            break;
        case '6':
            cameraRadius += 10; // Tilt counter-clockwise
            break;
        case '0':
            capture(); // Capture image
            break;
        default:
            break;
    }
    glutPostRedisplay();
}

// Add special key handler for arrow keys and PageUp/PageDown
void specialKeyListener(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_UP:
            // Move forward
            eye.x += 10 * cos(cameraAngle);
            eye.y += 10 * sin(cameraAngle);
            break;
        case GLUT_KEY_DOWN:
            // Move backward
            eye.x -= 10 * cos(cameraAngle);
            eye.y -= 10 * sin(cameraAngle);
            break;
        case GLUT_KEY_LEFT:
            // Move left
            eye.x -= 10 * sin(cameraAngle);
            eye.y += 10 * cos(cameraAngle);
            break;
        case GLUT_KEY_RIGHT:
            // Move right
            eye.x += 10 * sin(cameraAngle);
            eye.y -= 10 * cos(cameraAngle);
            break;
        case GLUT_KEY_PAGE_UP:
            // Move up
            eye.z += 10;
            break;
        case GLUT_KEY_PAGE_DOWN:
            // Move down
            eye.z -= 10;
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

    // Clean up dynamically allocated objects
    for (Object* obj : objects) {
        delete obj;
    }
    objects.clear();

    return 0;
}