#ifndef CLASSES_H
#define CLASSES_H

#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <GL/glut.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Vector3D {
    double x, y, z;
    Vector3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    friend Vector3D operator*(const Vector3D& v, double scalar) {
        return Vector3D(v.x * scalar, v.y * scalar, v.z * scalar);
    }

    friend Vector3D operator+(const Vector3D& v1, const Vector3D& v2) {
        return Vector3D(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
    }

    friend Vector3D operator-(const Vector3D& v1, const Vector3D& v2) {
        return Vector3D(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
    }
};

class Ray {
public:
    Vector3D start;
    Vector3D dir;

    Ray(Vector3D start, Vector3D dir) : start(start), dir(dir) {
        double magnitude = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        this->dir.x /= magnitude;
        this->dir.y /= magnitude;
        this->dir.z /= magnitude;
    }
};

class Object {
public:
    Vector3D reference_point;
    double height, width, length;
    double color[3];
    double coEfficients[4];
    int shine;

    Object() : height(0), width(0), length(0), shine(0) {
        color[0] = color[1] = color[2] = 0;
        coEfficients[0] = coEfficients[1] = coEfficients[2] = coEfficients[3] = 0;
    }

    virtual void draw() {}
    virtual void setColor(double r, double g, double b) {
        color[0] = r; color[1] = g; color[2] = b;
    }
    virtual void setShine(int s) { shine = s; }
    virtual void setCoEfficients(double ambient, double diffuse, double specular, double reflection) {
        coEfficients[0] = ambient;
        coEfficients[1] = diffuse;
        coEfficients[2] = specular;
        coEfficients[3] = reflection;
    }
    virtual double intersect(Ray* ray, double* color, int level) {
        return -1.0;
    }
    virtual ~Object() {}
};

class PointLight {
public:
    Vector3D light_pos;
    double color[3];

    PointLight(Vector3D pos, double r, double g, double b) {
        light_pos = pos;
        color[0] = r; color[1] = g; color[2] = b;
    }
};

class SpotLight : public PointLight {
public:
    Vector3D light_direction;
    double cutoff_angle;

    SpotLight(Vector3D pos, double r, double g, double b, Vector3D dir, double cutoff)
        : PointLight(pos, r, g, b), light_direction(dir), cutoff_angle(cutoff) {}
};

extern std::vector<PointLight> pointLights;
extern std::vector<SpotLight> spotLights;
extern std::vector<Object*> objects;
extern class Floor* globalFloor;

extern int recursionLevel;

extern float cameraRadius;
extern float cameraAngle;
extern float cameraHeight;

class Sphere : public Object {
public:
    Sphere(Vector3D center, double radius) {
        reference_point = center;
        length = radius;
    }

    void draw() override {
        glPushMatrix();
        glColor3f(color[0], color[1], color[2]);
        glTranslatef(reference_point.x, reference_point.y, reference_point.z);
        glutSolidSphere(length, 50, 50);
        glPopMatrix();
    }

    double intersect(Ray* ray, double* color, int level) override {
        Vector3D oc = {ray->start.x - reference_point.x, ray->start.y - reference_point.y, ray->start.z - reference_point.z};
        double a = 1.0;
        double b = 2.0 * (oc.x * ray->dir.x + oc.y * ray->dir.y + oc.z * ray->dir.z);
        double c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - length * length;
        double discriminant = b * b - 4 * a * c;

        if (discriminant < 0) return -1.0;

        double t1 = (-b - sqrt(discriminant)) / (2.0 * a);
        double t2 = (-b + sqrt(discriminant)) / (2.0 * a);

        double t = (t1 > 0) ? t1 : ((t2 > 0) ? t2 : -1.0);
        if (t < 0) return -1.0;

        if (level == 0) return t;

        Vector3D intersectionPoint = ray->start + ray->dir * t;

        Vector3D normal = intersectionPoint - reference_point;
        double magnitude = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        normal.x /= magnitude;
        normal.y /= magnitude;
        normal.z /= magnitude;

        color[0] = coEfficients[0] * this->color[0];
        color[1] = coEfficients[0] * this->color[1];
        color[2] = coEfficients[0] * this->color[2];

        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                if (obj->intersect(&shadowRay, color, 0) > 0 && obj->intersect(&shadowRay, color, 0) < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        double metallic = 0.5;
        double roughness = 0.5;

        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            double fresnel = pow(1.0 - std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z), 5.0);
            fresnel = fresnel * (1.0 - metallic) + metallic;

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double specular = pow(std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z)), 1.0 / roughness);

            color[0] += fresnel * specular * light.color[0];
            color[1] += fresnel * specular * light.color[1];
            color[2] += fresnel * specular * light.color[2];
        }

        if (level >= recursionLevel) return t;

        Vector3D reflectDir = ray->dir - normal * (2.0 * (ray->dir.x * normal.x + ray->dir.y * normal.y + ray->dir.z * normal.z));
        Ray reflectedRay(intersectionPoint + reflectDir * 1e-6, reflectDir);

        double reflectedColor[3] = {0, 0, 0};
        double tMin = 1e9;
        Object* nearestObject = nullptr;

        for (const auto& obj : objects) {
            double t = obj->intersect(&reflectedRay, reflectedColor, level + 1);
            if (t > 0 && t < tMin) {
                tMin = t;
                nearestObject = obj;
            }
        }

        if (nearestObject) {
            nearestObject->intersect(&reflectedRay, reflectedColor, level + 1);
            color[0] += reflectedColor[0] * coEfficients[3];
            color[1] += reflectedColor[1] * coEfficients[3];
            color[2] += reflectedColor[2] * coEfficients[3];
        }

        return t;
    }
};

class Triangle : public Object {
public:
    Vector3D points[3];
    Triangle(Vector3D p1, Vector3D p2, Vector3D p3) {
        points[0] = p1; points[1] = p2; points[2] = p3;
    }

    void draw() override {
        glBegin(GL_TRIANGLES);
        glColor3f(color[0], color[1], color[2]);
        glVertex3f(points[0].x, points[0].y, points[0].z);
        glVertex3f(points[1].x, points[1].y, points[1].z);
        glVertex3f(points[2].x, points[2].y, points[2].z);
        glEnd();
    }

    double intersect(Ray* ray, double* color, int level) override {
        Vector3D edge1 = points[1] - points[0];
        Vector3D edge2 = points[2] - points[0];
        Vector3D h = {ray->dir.y * edge2.z - ray->dir.z * edge2.y,
                      ray->dir.z * edge2.x - ray->dir.x * edge2.z,
                      ray->dir.x * edge2.y - ray->dir.y * edge2.x};
        double a = edge1.x * h.x + edge1.y * h.y + edge1.z * h.z;

        if (fabs(a) < 1e-6) return -1.0;

        double f = 1.0 / a;
        Vector3D s = ray->start - points[0];
        double u = f * (s.x * h.x + s.y * h.y + s.z * h.z);
        if (u < 0.0 || u > 1.0) return -1.0;

        Vector3D q = {s.y * edge1.z - s.z * edge1.y,
                      s.z * edge1.x - s.x * edge1.z,
                      s.x * edge1.y - s.y * edge1.x};
        double v = f * (ray->dir.x * q.x + ray->dir.y * q.y + ray->dir.z * q.z);
        if (v < 0.0 || u + v > 1.0) return -1.0;

        double t = f * (edge2.x * q.x + edge2.y * q.y + edge2.z * q.z);
        if (t < 0) return -1.0;

        if (level == 0) return t;

        Vector3D intersectionPoint = ray->start + ray->dir * t;
        Vector3D normal = {edge1.y * edge2.z - edge1.z * edge2.y,
                           edge1.z * edge2.x - edge1.x * edge2.z,
                           edge1.x * edge2.y - edge1.y * edge2.x};
        double magnitude = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        normal.x /= magnitude;
        normal.y /= magnitude;
        normal.z /= magnitude;

        color[0] = coEfficients[0] * this->color[0];
        color[1] = coEfficients[0] * this->color[1];
        color[2] = coEfficients[0] * this->color[2];

        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                double shadowT = obj->intersect(&shadowRay, nullptr, 0);
                if (shadowT > 0 && shadowT < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        for (const auto& light : spotLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            double cosTheta = -(lightDir.x * light.light_direction.x + lightDir.y * light.light_direction.y + lightDir.z * light.light_direction.z);
            if (acos(cosTheta) * 180.0 / M_PI > light.cutoff_angle) continue;

            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                double shadowT = obj->intersect(&shadowRay, nullptr, 0);
                if (shadowT > 0 && shadowT < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        if (level >= recursionLevel) return t;

        Vector3D reflectDir = ray->dir - normal * (2.0 * (ray->dir.x * normal.x + ray->dir.y * normal.y + ray->dir.z * normal.z));
        Ray reflectedRay(intersectionPoint + reflectDir * 1e-6, reflectDir);

        double reflectedColor[3] = {0, 0, 0};
        double tMin = 1e9;
        Object* nearestObject = nullptr;

        for (const auto& obj : objects) {
            double t = obj->intersect(&reflectedRay, reflectedColor, level + 1);
            if (t > 0 && t < tMin) {
                tMin = t;
                nearestObject = obj;
            }
        }

        if (nearestObject) {
            nearestObject->intersect(&reflectedRay, reflectedColor, level + 1);
            color[0] += reflectedColor[0] * coEfficients[3];
            color[1] += reflectedColor[1] * coEfficients[3];
            color[2] += reflectedColor[2] * coEfficients[3];
        }

        return t;
    }
};

class Floor : public Object {
public:
    double floorWidth, tileWidth;
    bool useTexture;
    unsigned char* textureData;
    int textureWidth, textureHeight, textureChannels;

    Floor(double floorWidth, double tileWidth, const std::string& textureFile = "") {
        this->floorWidth = floorWidth;
        this->tileWidth = tileWidth;
        this->useTexture = false;

        loadTextureFromFile();
    }

    void loadTextureFromFile() {
        textureData = stbi_load("texture2.bmp", &textureWidth, &textureHeight, &textureChannels, 0);
        
        if (!textureData) {
            std::cerr << "Warning: Could not load texture2.bmp, generating fallback texture" << std::endl;
            generateFallbackTexture();
        } else {
            std::cout << "Successfully loaded texture2.bmp (" << textureWidth << "x" << textureHeight << ", " << textureChannels << " channels)" << std::endl;
        }
    }

    void generateFallbackTexture() {
        textureWidth = 512;
        textureHeight = 512;
        textureChannels = 3;
        
        textureData = new unsigned char[textureWidth * textureHeight * textureChannels];
        
        for (int y = 0; y < textureHeight; y++) {
            for (int x = 0; x < textureWidth; x++) {
                int index = (y * textureWidth + x) * textureChannels;
                
                int brickWidth = 64;
                int brickHeight = 32;
                int mortarWidth = 4;
                
                int brickX = x % (brickWidth + mortarWidth);
                int brickY = y % (brickHeight + mortarWidth);
                
                if ((y / (brickHeight + mortarWidth)) % 2 == 1) {
                    brickX = (x + brickWidth / 2) % (brickWidth + mortarWidth);
                }
                
                bool isMortar = (brickX >= brickWidth) || (brickY >= brickHeight);
                
                if (isMortar) {
                    textureData[index] = 200;
                    textureData[index + 1] = 200;
                    textureData[index + 2] = 200;
                } else {
                    double noise = sin(x * 0.1) * cos(y * 0.1) * 20;
                    textureData[index] = (unsigned char)std::max(0, std::min(255, (int)(150 + noise)));
                    textureData[index + 1] = (unsigned char)std::max(0, std::min(255, (int)(80 + noise * 0.5)));
                    textureData[index + 2] = (unsigned char)std::max(0, std::min(255, (int)(60 + noise * 0.3)));
                }
            }
        }
    }

    void hsvToRgb(double h, double s, double v, double& r, double& g, double& b) {
        double c = v * s;
        double x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
        double m = v - c;
        
        if (h >= 0 && h < 60) {
            r = c; g = x; b = 0;
        } else if (h >= 60 && h < 120) {
            r = x; g = c; b = 0;
        } else if (h >= 120 && h < 180) {
            r = 0; g = c; b = x;
        } else if (h >= 180 && h < 240) {
            r = 0; g = x; b = c;
        } else if (h >= 240 && h < 300) {
            r = x; g = 0; b = c;
        } else {
            r = c; g = 0; b = x;
        }
        
        r += m;
        g += m;
        b += m;
    }

    Vector3D sampleTexture(double u, double v) {
        if (!textureData || textureWidth <= 0 || textureHeight <= 0) {
            return Vector3D(0.5, 0.5, 0.5);
        }

        double tilesPerFloor = 10.0;
        
        u = fmod(u * tilesPerFloor, 1.0);
        v = fmod(v * tilesPerFloor, 1.0);
        
        if (u < 0) u += 1.0;
        if (v < 0) v += 1.0;

        u = std::max(0.0, std::min(1.0, u));
        v = std::max(0.0, std::min(1.0, v));

        int pixel_x = (int)(u * (textureWidth - 1));
        int pixel_y = (int)((1.0 - v) * (textureHeight - 1));

        pixel_x = std::max(0, std::min(textureWidth - 1, pixel_x));
        pixel_y = std::max(0, std::min(textureHeight - 1, pixel_y));

        int index = (pixel_y * textureWidth + pixel_x) * textureChannels;
        int max_index = textureWidth * textureHeight * textureChannels;
        if (index < 0 || index + 2 >= max_index) {
            return Vector3D(1.0, 0.0, 1.0);
        }

        Vector3D color;
        color.x = textureData[index] / 255.0;

        if (textureChannels >= 2) {
            color.y = textureData[index + 1] / 255.0;
        } else {
            color.y = color.x;
        }

        if (textureChannels >= 3) {
            color.z = textureData[index + 2] / 255.0;
        } else {
            color.z = color.x;
        }

        return color;
    }

    void toggleTexture() {
        useTexture = !useTexture;
    }

    void draw() override {
        glBegin(GL_QUADS);
        for (double x = -floorWidth / 2; x < floorWidth / 2; x += tileWidth) {
            for (double y = -floorWidth / 2; y < floorWidth / 2; y += tileWidth) {
                if (useTexture && textureData) {
                    double u = (x + floorWidth / 2) / floorWidth;
                    double v = (y + floorWidth / 2) / floorWidth;
                    
                    Vector3D texColor = sampleTexture(u, v);
                    glColor3f(texColor.x, texColor.y, texColor.z);
                } else {
                    bool isWhite = (static_cast<int>((x + floorWidth / 2) / tileWidth) + static_cast<int>((y + floorWidth / 2) / tileWidth)) % 2 == 0;
                    glColor3f(isWhite ? 1.0 : 0.0, isWhite ? 1.0 : 0.0, isWhite ? 1.0 : 0.0);
                }

                glVertex3f(x, y, 0);
                glVertex3f(x + tileWidth, y, 0);
                glVertex3f(x + tileWidth, y + tileWidth, 0);
                glVertex3f(x, y + tileWidth, 0);
            }
        }
        glEnd();
    }

    ~Floor() {
        if (textureData) {
            delete[] textureData;
        }
    }

    double intersect(Ray* ray, double* color, int level) override {
        if (fabs(ray->dir.z) < 1e-6) return -1.0;

        double t = -ray->start.z / ray->dir.z;
        if (t < 0) return -1.0;

        Vector3D intersectionPoint = ray->start + ray->dir * t;

        if (intersectionPoint.x < -floorWidth / 2 || intersectionPoint.x > floorWidth / 2 ||
            intersectionPoint.y < -floorWidth / 2 || intersectionPoint.y > floorWidth / 2) {
            return -1.0;
        }

        if (level == 0) return t;

        Vector3D intersectionPointColor;
        if (useTexture && textureData) {
            double u = (intersectionPoint.x + floorWidth / 2) / floorWidth;
            double v = (intersectionPoint.y + floorWidth / 2) / floorWidth;
            
            intersectionPointColor = sampleTexture(u, v);
        } else {
            bool isWhite = (static_cast<int>((intersectionPoint.x + floorWidth / 2) / tileWidth) + 
                           static_cast<int>((intersectionPoint.y + floorWidth / 2) / tileWidth)) % 2 == 0;
            intersectionPointColor.x = intersectionPointColor.y = intersectionPointColor.z = isWhite ? 1.0 : 0.0;
        }

        Vector3D normal(0, 0, 1);

        color[0] = coEfficients[0] * intersectionPointColor.x;
        color[1] = coEfficients[0] * intersectionPointColor.y;
        color[2] = coEfficients[0] * intersectionPointColor.z;

        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                double shadowT = obj->intersect(&shadowRay, nullptr, 0);
                if (shadowT > 0 && shadowT < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * intersectionPointColor.x;
            color[1] += coEfficients[1] * light.color[1] * lambert * intersectionPointColor.y;
            color[2] += coEfficients[1] * light.color[2] * lambert * intersectionPointColor.z;

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        for (const auto& light : spotLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            double cosTheta = -(lightDir.x * light.light_direction.x + lightDir.y * light.light_direction.y + lightDir.z * light.light_direction.z);
            if (acos(cosTheta) * 180.0 / M_PI > light.cutoff_angle) continue;

            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                double shadowT = obj->intersect(&shadowRay, nullptr, 0);
                if (shadowT > 0 && shadowT < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * intersectionPointColor.x;
            color[1] += coEfficients[1] * light.color[1] * lambert * intersectionPointColor.y;
            color[2] += coEfficients[1] * light.color[2] * lambert * intersectionPointColor.z;

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        if (level >= recursionLevel) return t;

        Vector3D reflectDir = ray->dir - normal * (2.0 * (ray->dir.x * normal.x + ray->dir.y * normal.y + ray->dir.z * normal.z));
        Ray reflectedRay(intersectionPoint + reflectDir * 1e-6, reflectDir);

        double reflectedColor[3] = {0, 0, 0};
        double tMin = 1e9;
        Object* nearestObject = nullptr;

        for (const auto& obj : objects) {
            double t = obj->intersect(&reflectedRay, reflectedColor, level + 1);
            if (t > 0 && t < tMin) {
                tMin = t;
                nearestObject = obj;
            }
        }

        if (nearestObject) {
            nearestObject->intersect(&reflectedRay, reflectedColor, level + 1);
            color[0] += reflectedColor[0] * coEfficients[3];
            color[1] += reflectedColor[1] * coEfficients[3];
            color[2] += reflectedColor[2] * coEfficients[3];
        }

        return t;
    }
};

class General : public Object {
public:
    double A, B, C, D, E, F, G, H, I, J;
    Vector3D cubeReferencePoint;
    double length, width, height;

    General(double A, double B, double C, double D, double E, double F, double G, double H, double I, double J,
            Vector3D cubeReferencePoint, double length, double width, double height) {
        this->A = A; this->B = B; this->C = C; this->D = D; this->E = E;
        this->F = F; this->G = G; this->H = H; this->I = I; this->J = J;
        this->cubeReferencePoint = cubeReferencePoint;
        this->length = length;
        this->width = width;
        this->height = height;
    }

    void draw() override {
        glPushMatrix();
        glColor3f(color[0], color[1], color[2]);
        glTranslatef(cubeReferencePoint.x, cubeReferencePoint.y, cubeReferencePoint.z);
        glScalef(length, width, height);
        glutWireCube(1.0);
        glPopMatrix();
    }

    double intersect(Ray* ray, double* color, int level) override {
        double dx = ray->dir.x, dy = ray->dir.y, dz = ray->dir.z;
        double ox = ray->start.x, oy = ray->start.y, oz = ray->start.z;

        double a = A * dx * dx + B * dy * dy + C * dz * dz + D * dx * dy + E * dx * dz + F * dy * dz;
        double b = 2 * (A * ox * dx + B * oy * dy + C * oz * dz) + D * (ox * dy + oy * dx) + E * (ox * dz + oz * dx) + F * (oy * dz + oz * dy) + G * dx + H * dy + I * dz;
        double c = A * ox * ox + B * oy * oy + C * oz * oz + D * ox * oy + E * ox * oz + F * oy * oz + G * ox + H * oy + I * oz + J;

        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return -1.0;

        double t1 = (-b - sqrt(discriminant)) / (2.0 * a);
        double t2 = (-b + sqrt(discriminant)) / (2.0 * a);

        auto isInsideBoundingBox = [&](double t) {
            if (t < 0) return false;
            Vector3D p = ray->start + ray->dir * t;
            if (length > 0 && (p.x < cubeReferencePoint.x || p.x > cubeReferencePoint.x + length)) return false;
            if (width > 0 && (p.y < cubeReferencePoint.y || p.y > cubeReferencePoint.y + width)) return false;
            if (height > 0 && (p.z < cubeReferencePoint.z || p.z > cubeReferencePoint.z + height)) return false;
            return true;
        };

        bool t1Valid = isInsideBoundingBox(t1);
        bool t2Valid = isInsideBoundingBox(t2);

        double t = -1;
        if (t1Valid && t2Valid) t = std::min(t1, t2);
        else if (t1Valid) t = t1;
        else if (t2Valid) t = t2;
        if (t < 0) return -1.0;

        if (level == 0) return t;

        Vector3D intersectionPoint = ray->start + ray->dir * t;
        Vector3D normal = {
            2 * A * intersectionPoint.x + D * intersectionPoint.y + E * intersectionPoint.z + G,
            2 * B * intersectionPoint.y + D * intersectionPoint.x + F * intersectionPoint.z + H,
            2 * C * intersectionPoint.z + E * intersectionPoint.x + F * intersectionPoint.y + I
        };
        double magnitude = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        normal.x /= magnitude;
        normal.y /= magnitude;
        normal.z /= magnitude;

        color[0] = coEfficients[0] * this->color[0];
        color[1] = coEfficients[0] * this->color[1];
        color[2] = coEfficients[0] * this->color[2];

        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                if (obj->intersect(&shadowRay, color, 0) > 0 && obj->intersect(&shadowRay, color, 0) < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        for (const auto& light : spotLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            double cosTheta = -(lightDir.x * light.light_direction.x + lightDir.y * light.light_direction.y + lightDir.z * light.light_direction.z);
            if (acos(cosTheta) * 180.0 / M_PI > light.cutoff_angle) continue;

            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                if (obj->intersect(&shadowRay, color, 0) > 0 && obj->intersect(&shadowRay, color, 0) < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        if (level >= recursionLevel) return t;

        Vector3D reflectDir = ray->dir - normal * (2.0 * (ray->dir.x * normal.x + ray->dir.y * normal.y + ray->dir.z * normal.z));
        Ray reflectedRay(intersectionPoint + reflectDir * 1e-6, reflectDir);

        double reflectedColor[3] = {0, 0, 0};
        double tMin = 1e9;
        Object* nearestObject = nullptr;

        for (const auto& obj : objects) {
            double t = obj->intersect(&reflectedRay, reflectedColor, level + 1);
            if (t > 0 && t < tMin) {
                tMin = t;
                nearestObject = obj;
            }
        }

        if (nearestObject) {
            nearestObject->intersect(&reflectedRay, reflectedColor, level + 1);
            color[0] += reflectedColor[0] * coEfficients[3];
            color[1] += reflectedColor[1] * coEfficients[3];
            color[2] += reflectedColor[2] * coEfficients[3];
        }

        return t;
    }
};

#endif