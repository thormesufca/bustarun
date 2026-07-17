#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <GL/glut.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

struct Vector3 {
    float x, y, z;
};

struct Vector2 {
    float u, v;
};

struct FaceVertex {
    int vIndex;
    int tIndex;
    int nIndex;
};

struct Face {
    std::vector<FaceVertex> vertices;
};

class OBJModel {
private:
    std::vector<Vector3> vertices;
    std::vector<Vector2> texCoords;
    std::vector<Vector3> normals;
    std::vector<Face> faces;
    bool hasTexCoords;
    bool hasNormals;

public:
    OBJModel() : hasTexCoords(false), hasNormals(false) {}

    // Carrega o arquivo .obj
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Erro ao abrir o arquivo OBJ: " << filename << std::endl;
            return false;
        }

        vertices.clear();
        texCoords.clear();
        normals.clear();
        faces.clear();
        hasTexCoords = false;
        hasNormals = false;

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "v") {
                Vector3 v;
                ss >> v.x >> v.y >> v.z;
                vertices.push_back(v);
            } else if (type == "vt") {
                Vector2 vt;
                ss >> vt.u >> vt.v;
                texCoords.push_back(vt);
                hasTexCoords = true;
            } else if (type == "vn") {
                Vector3 vn;
                ss >> vn.x >> vn.y >> vn.z;
                normals.push_back(vn);
                hasNormals = true;
            } else if (type == "f") {
                Face face;
                std::string vertexStr;
                while (ss >> vertexStr) {
                    FaceVertex fv = {-1, -1, -1};
                    std::stringstream vss(vertexStr);
                    std::string vIdx, tIdx, nIdx;

                    // Divide o formato v/vt/vn
                    std::getline(vss, vIdx, '/');
                    std::getline(vss, tIdx, '/');
                    std::getline(vss, nIdx, '/');

                    if (!vIdx.empty()) fv.vIndex = std::stoi(vIdx) - 1;
                    if (!tIdx.empty()) fv.tIndex = std::stoi(tIdx) - 1;
                    if (!nIdx.empty()) fv.nIndex = std::stoi(nIdx) - 1;

                    face.vertices.push_back(fv);
                }
                faces.push_back(face);
            }
        }
        file.close();
        return true;
    }

    // Desenha o modelo na tela aplicando as texturas e normais
    void draw() const {
        for (const auto& face : faces) {
            // Define o tipo de primitiva primitiva baseada no número de vértices da face
            if (face.vertices.size() == 3) {
                glBegin(GL_TRIANGLES);
            } else if (face.vertices.size() == 4) {
                glBegin(GL_QUADS);
            } else {
                glBegin(GL_POLYGON);
            }

            for (const auto& fv : face.vertices) {
                // Aplica a Normal (essencial para a iluminação do modelo ficar correta)
                if (hasNormals && fv.nIndex >= 0 && fv.nIndex < (int)normals.size()) {
                    glNormal3f(normals[fv.nIndex].x, normals[fv.nIndex].y, normals[fv.nIndex].z);
                }
                // Aplica a Coordenada de Textura (mapeia o JPG/PNG no modelo)
                if (hasTexCoords && fv.tIndex >= 0 && fv.tIndex < (int)texCoords.size()) {
                    glTexCoord2f(texCoords[fv.tIndex].u, texCoords[fv.tIndex].v);
                }
                // Desenha o Vértice
                if (fv.vIndex >= 0 && fv.vIndex < (int)vertices.size()) {
                    glVertex3f(vertices[fv.vIndex].x, vertices[fv.vIndex].y, vertices[fv.vIndex].z);
                }
            }
            glEnd();
        }
    }
};

#endif // OBJ_LOADER_H