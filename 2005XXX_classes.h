#ifndef CLASSES_H
#define CLASSES_H

#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <GL/glut.h> // Include OpenGL header for rendering

// Define STB_IMAGE_IMPLEMENTATION before including stb_image.h
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct Vector3D {
    double x, y, z;
    Vector3D(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

    // Scalar multiplication
    friend Vector3D operator*(const Vector3D& v, double scalar) {
        return Vector3D(v.x * scalar, v.y * scalar, v.z * scalar);
    }

    // Vector addition
    friend Vector3D operator+(const Vector3D& v1, const Vector3D& v2) {
        return Vector3D(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
    }

    // Vector subtraction
    friend Vector3D operator-(const Vector3D& v1, const Vector3D& v2) {
        return Vector3D(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
    }
};

class Ray {
public:
    Vector3D start;
    Vector3D dir; // Must be normalized

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
    double coEfficients[4]; // ambient, diffuse, specular, reflection coefficients
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
        return -1.0; // Default implementation
    }
    virtual ~Object() {}
};

// Move PointLight and SpotLight classes above extern declarations
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

// Declare extern variables for pointLights and objects
extern std::vector<PointLight> pointLights;
extern std::vector<SpotLight> spotLights;
extern std::vector<Object*> objects;

// Declare extern variable for recursionLevel
extern int recursionLevel;

// Declare extern variables for camera properties
extern float cameraRadius;
extern float cameraAngle;
extern float cameraHeight;

// Sphere intersection
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
        glutSolidSphere(length, 50, 50); // Render sphere
        glPopMatrix();
    }

    double intersect(Ray* ray, double* color, int level) override {
        Vector3D oc = {ray->start.x - reference_point.x, ray->start.y - reference_point.y, ray->start.z - reference_point.z};
        double a = 1.0; // Since ray->dir is normalized
        double b = 2.0 * (oc.x * ray->dir.x + oc.y * ray->dir.y + oc.z * ray->dir.z);
        double c = oc.x * oc.x + oc.y * oc.y + oc.z * oc.z - length * length;
        double discriminant = b * b - 4 * a * c;

        if (discriminant < 0) return -1.0;

        double t1 = (-b - sqrt(discriminant)) / (2.0 * a);
        double t2 = (-b + sqrt(discriminant)) / (2.0 * a);

        double t = (t1 > 0) ? t1 : ((t2 > 0) ? t2 : -1.0);
        if (t < 0) return -1.0;

        if (level == 0) return t;

        // Compute intersection point
        Vector3D intersectionPoint = ray->start + ray->dir * t;

        // Compute normal at intersection point
        Vector3D normal = intersectionPoint - reference_point;
        double magnitude = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        normal.x /= magnitude;
        normal.y /= magnitude;
        normal.z /= magnitude;

        // Initialize color with ambient component
        color[0] = coEfficients[0] * this->color[0];
        color[1] = coEfficients[0] * this->color[1];
        color[2] = coEfficients[0] * this->color[2];

        // Add diffuse and specular components for each light source
        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            // Check for shadows
            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                if (obj->intersect(&shadowRay, color, 0) > 0 && obj->intersect(&shadowRay, color, 0) < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            // Diffuse component
            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            // Specular component
            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        // PBR-based lighting
        double metallic = 0.5; // Example value, can be parameterized
        double roughness = 0.5; // Example value, can be parameterized

        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            // Fresnel effect
            double fresnel = pow(1.0 - std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z), 5.0);
            fresnel = fresnel * (1.0 - metallic) + metallic;

            // Specular reflection
            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double specular = pow(std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z)), 1.0 / roughness);

            color[0] += fresnel * specular * light.color[0];
            color[1] += fresnel * specular * light.color[1];
            color[2] += fresnel * specular * light.color[2];
        }

        if (level >= recursionLevel) return t;

        // Compute reflection ray
        Vector3D reflectDir = ray->dir - normal * (2.0 * (ray->dir.x * normal.x + ray->dir.y * normal.y + ray->dir.z * normal.z));
        Ray reflectedRay(intersectionPoint + reflectDir * 1e-6, reflectDir);

        // Trace the reflected ray
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

// Implement intersect method for Triangle class
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

        if (fabs(a) < 1e-6) return -1.0; // Ray is parallel to the triangle

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
        if (t < 0) return -1.0; // Intersection is behind the ray's origin

        if (level == 0) return t;

        // Compute intersection point and normal
        Vector3D intersectionPoint = ray->start + ray->dir * t;
        Vector3D normal = {edge1.y * edge2.z - edge1.z * edge2.y,
                           edge1.z * edge2.x - edge1.x * edge2.z,
                           edge1.x * edge2.y - edge1.y * edge2.x};
        double magnitude = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
        normal.x /= magnitude;
        normal.y /= magnitude;
        normal.z /= magnitude;

        // Initialize color with ambient component
        color[0] = coEfficients[0] * this->color[0];
        color[1] = coEfficients[0] * this->color[1];
        color[2] = coEfficients[0] * this->color[2];

        // Add diffuse and specular components for each light source
        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            // Check for shadows
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

            // Diffuse component
            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            // Specular component
            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        // Handle spotlights
        for (const auto& light : spotLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            // Check spotlight cutoff angle
            double cosTheta = -(lightDir.x * light.light_direction.x + lightDir.y * light.light_direction.y + lightDir.z * light.light_direction.z);
            if (acos(cosTheta) * 180.0 / M_PI > light.cutoff_angle) continue;

            // Check for shadows
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

            // Diffuse component
            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            // Specular component
            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        if (level >= recursionLevel) return t;

        // Compute reflection ray
        Vector3D reflectDir = ray->dir - normal * (2.0 * (ray->dir.x * normal.x + ray->dir.y * normal.y + ray->dir.z * normal.z));
        Ray reflectedRay(intersectionPoint + reflectDir * 1e-6, reflectDir);

        // Trace the reflected ray
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

// Implement intersect method for Floor class
class Floor : public Object {
public:
    double floorWidth, tileWidth;
    bool useTexture;
    unsigned char* textureData;
    int textureWidth, textureHeight, textureChannels;

    Floor(double floorWidth, double tileWidth, const std::string& textureFile) {
        this->floorWidth = floorWidth;
        this->tileWidth = tileWidth;
        this->useTexture = false; // Default to checkerboard

        // Load texture
        textureData = stbi_load(textureFile.c_str(), &textureWidth, &textureHeight, &textureChannels, 0);
        if (!textureData) {
            std::cerr << "Error: Could not load texture file " << textureFile << std::endl;
        }
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

                    int texX = static_cast<int>(u * (textureWidth - 1));
                    int texY = static_cast<int>(v * (textureHeight - 1));
                    int index = (texY * textureWidth + texX) * textureChannels;

                    glColor3f(textureData[index] / 255.0, textureData[index + 1] / 255.0, textureData[index + 2] / 255.0);
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
            stbi_image_free(textureData);
        }
    }

    double intersect(Ray* ray, double* color, int level) override {
        // Ray-plane intersection
        if (fabs(ray->dir.z) < 1e-6) return -1.0; // Parallel to the floor

        double t = -ray->start.z / ray->dir.z;
        if (t < 0) return -1.0; // Intersection behind the ray's origin

        Vector3D intersectionPoint = ray->start + ray->dir * t;

        // Check if the intersection point lies within the floor bounds
        if (intersectionPoint.x < -floorWidth / 2 || intersectionPoint.x > floorWidth / 2 ||
            intersectionPoint.y < -floorWidth / 2 || intersectionPoint.y > floorWidth / 2) {
            return -1.0;
        }

        if (level == 0) return t;

        // Determine floor color (checkerboard pattern)
        Vector3D intersectionPointColor;
        if (useTexture && textureData) {
            double u = (intersectionPoint.x + floorWidth / 2) / floorWidth;
            double v = (intersectionPoint.y + floorWidth / 2) / floorWidth;

            int texX = static_cast<int>(u * (textureWidth - 1));
            int texY = static_cast<int>(v * (textureHeight - 1));
            int index = (texY * textureWidth + texX) * textureChannels;

            intersectionPointColor.x = textureData[index] / 255.0;
            intersectionPointColor.y = textureData[index + 1] / 255.0;
            intersectionPointColor.z = textureData[index + 2] / 255.0;
        } else {
            bool isWhite = (static_cast<int>((intersectionPoint.x + floorWidth / 2) / tileWidth) + 
                           static_cast<int>((intersectionPoint.y + floorWidth / 2) / tileWidth)) % 2 == 0;
            intersectionPointColor.x = intersectionPointColor.y = intersectionPointColor.z = isWhite ? 1.0 : 0.0;
        }

        Vector3D normal(0, 0, 1); // Floor normal is always (0, 0, 1)

        // Initialize color with ambient component
        color[0] = coEfficients[0] * intersectionPointColor.x;
        color[1] = coEfficients[0] * intersectionPointColor.y;
        color[2] = coEfficients[0] * intersectionPointColor.z;

        // Add diffuse and specular components for each light source
        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            // Check for shadows
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

            // Diffuse component
            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * intersectionPointColor.x;
            color[1] += coEfficients[1] * light.color[1] * lambert * intersectionPointColor.y;
            color[2] += coEfficients[1] * light.color[2] * lambert * intersectionPointColor.z;

            // Specular component
            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        // Handle spotlights
        for (const auto& light : spotLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            // Check spotlight cutoff angle
            double cosTheta = -(lightDir.x * light.light_direction.x + lightDir.y * light.light_direction.y + lightDir.z * light.light_direction.z);
            if (acos(cosTheta) * 180.0 / M_PI > light.cutoff_angle) continue;

            // Check for shadows
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

            // Diffuse component
            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * intersectionPointColor.x;
            color[1] += coEfficients[1] * light.color[1] * lambert * intersectionPointColor.y;
            color[2] += coEfficients[1] * light.color[2] * lambert * intersectionPointColor.z;

            // Specular component
            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        if (level >= recursionLevel) return t;

        // Compute reflection ray
        Vector3D reflectDir = ray->dir - normal * (2.0 * (ray->dir.x * normal.x + ray->dir.y * normal.y + ray->dir.z * normal.z));
        Ray reflectedRay(intersectionPoint + reflectDir * 1e-6, reflectDir);

        // Trace the reflected ray
        double reflectedColor[3] = {0, 0, 0};
        double tMin = 1e9;
        Object* nearestObject = nullptr;

        for (const auto& obj : objects) {
            double t = obj->intersect(&reflectedRay, reflectedColor, 0);
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
// Declare extern variable for spotLights
// extern std::vector<SpotLight> spotLights;
// Implement General class for quadric surfaces
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
        // Render the bounding box of the general object
        glPushMatrix();
        glColor3f(color[0], color[1], color[2]);
        glTranslatef(cubeReferencePoint.x, cubeReferencePoint.y, cubeReferencePoint.z);
        glScalef(length, width, height);
        glutWireCube(1.0); // Render as a wireframe cube
        glPopMatrix();
    }

    double intersect(Ray* ray, double* color, int level) override {
        double dx = ray->dir.x, dy = ray->dir.y, dz = ray->dir.z;
        double ox = ray->start.x, oy = ray->start.y, oz = ray->start.z;

        // Coefficients of the quadratic equation
        double a = A * dx * dx + B * dy * dy + C * dz * dz + D * dx * dy + E * dx * dz + F * dy * dz;
        double b = 2 * (A * ox * dx + B * oy * dy + C * oz * dz) + D * (ox * dy + oy * dx) + E * (ox * dz + oz * dx) + F * (oy * dz + oz * dy) + G * dx + H * dy + I * dz;
        double c = A * ox * ox + B * oy * oy + C * oz * oz + D * ox * oy + E * ox * oz + F * oy * oz + G * ox + H * oy + I * oz + J;

        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return -1.0; // No intersection

        double t1 = (-b - sqrt(discriminant)) / (2.0 * a);
        double t2 = (-b + sqrt(discriminant)) / (2.0 * a);

        // Check if t1 or t2 lies within the bounding box
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

        // Compute intersection point and normal
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

        // Initialize color with ambient component
        color[0] = coEfficients[0] * this->color[0];
        color[1] = coEfficients[0] * this->color[1];
        color[2] = coEfficients[0] * this->color[2];

        // Add diffuse and specular components for each light source
        for (const auto& light : pointLights) {
            Vector3D lightDir = light.light_pos - intersectionPoint;
            double lightDist = sqrt(lightDir.x * lightDir.x + lightDir.y * lightDir.y + lightDir.z * lightDir.z);
            lightDir.x /= lightDist;
            lightDir.y /= lightDist;
            lightDir.z /= lightDist;

            // Check for shadows
            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                if (obj->intersect(&shadowRay, color, 0) > 0 && obj->intersect(&shadowRay, color, 0) < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            // Diffuse component
            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            // Specular component
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

            // Check spotlight cutoff angle
            double cosTheta = lightDir.x * light.light_direction.x + lightDir.y * light.light_direction.y + lightDir.z * light.light_direction.z;
            if (acos(cosTheta) * 180.0 / M_PI > light.cutoff_angle) continue;

            // Check for shadows
            Ray shadowRay(intersectionPoint + lightDir * 1e-6, lightDir);
            bool inShadow = false;
            for (const auto& obj : objects) {
                if (obj->intersect(&shadowRay, color, 0) > 0 && obj->intersect(&shadowRay, color, 0) < lightDist) {
                    inShadow = true;
                    break;
                }
            }

            if (inShadow) continue;

            // Diffuse component
            double lambert = std::max(0.0, normal.x * lightDir.x + normal.y * lightDir.y + normal.z * lightDir.z);
            color[0] += coEfficients[1] * light.color[0] * lambert * this->color[0];
            color[1] += coEfficients[1] * light.color[1] * lambert * this->color[1];
            color[2] += coEfficients[1] * light.color[2] * lambert * this->color[2];

            // Specular component
            Vector3D reflectDir = lightDir - normal * (2.0 * (lightDir.x * normal.x + lightDir.y * normal.y + lightDir.z * normal.z));
            double phong = std::max(0.0, -(ray->dir.x * reflectDir.x + ray->dir.y * reflectDir.y + ray->dir.z * reflectDir.z));
            phong = pow(phong, shine);
            color[0] += coEfficients[2] * light.color[0] * phong;
            color[1] += coEfficients[2] * light.color[1] * phong;
            color[2] += coEfficients[2] * light.color[2] * phong;
        }

        return t;
    }
};



#endif // CLASSES_H