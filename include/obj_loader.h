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
    int matIndex; // Índice em materials; -1 = face sem material declarado
};

// Material do arquivo .mtl que acompanha o .obj. Cada campo corresponde a uma
// propriedade óptica da superfície:
//   Ka -> quanto o objeto reflete da luz ambiente
//   Kd -> cor difusa (a cor "própria" do objeto sob luz direta)
//   Ks -> cor do brilho especular (o reflexo da fonte de luz)
//   Ns -> concentração desse brilho (quanto maior, menor e mais nítido o ponto)
struct Material {
    std::string name;
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat shininess;

    Material() : name(""),
                 ambient{0.2f, 0.2f, 0.2f, 1.0f},
                 diffuse{0.8f, 0.8f, 0.8f, 1.0f},
                 specular{0.0f, 0.0f, 0.0f, 1.0f},
                 shininess(0.0f) {}
};

class OBJModel {
private:
    std::vector<Vector3> vertices;
    std::vector<Vector2> texCoords;
    std::vector<Vector3> normals;
    std::vector<Face> faces;
    std::vector<Material> materials;
    bool hasTexCoords;
    bool hasNormals;
    Vector3 boundsMin;
    Vector3 boundsMax;

    // Extrai a pasta do caminho do .obj, pra procurar o .mtl ao lado dele
    // (o "mtllib" do arquivo traz só o nome, sem o caminho).
    static std::string diretorioDe(const std::string& filename) {
        size_t corte = filename.find_last_of("/\\");
        if (corte == std::string::npos) return "";
        return filename.substr(0, corte + 1);
    }

    // Carrega o .mtl indicado pelo "mtllib". Falha aqui não é fatal: sem
    // materiais o modelo ainda desenha, só usa o material corrente do OpenGL.
    void loadMTL(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Aviso: nao foi possivel abrir o MTL: " << filename << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "newmtl") {
                Material m;
                ss >> m.name;
                materials.push_back(m);
            } else if (materials.empty()) {
                continue; // Propriedade solta antes de qualquer newmtl
            } else if (type == "Ka") {
                Material& m = materials.back();
                ss >> m.ambient[0] >> m.ambient[1] >> m.ambient[2];
            } else if (type == "Kd") {
                Material& m = materials.back();
                ss >> m.diffuse[0] >> m.diffuse[1] >> m.diffuse[2];
            } else if (type == "Ks") {
                Material& m = materials.back();
                ss >> m.specular[0] >> m.specular[1] >> m.specular[2];
            } else if (type == "Ns") {
                // O .mtl usa a escala 0..1000 do Phong exponent; o OpenGL
                // aceita GL_SHININESS só de 0 a 128.
                float ns = 0.0f;
                ss >> ns;
                float convertido = ns * 128.0f / 1000.0f;
                if (convertido > 128.0f) convertido = 128.0f;
                if (convertido < 0.0f) convertido = 0.0f;
                materials.back().shininess = convertido;
            } else if (type == "d") {
                // Transparência: vale para as 3 componentes de cor
                float alpha = 1.0f;
                ss >> alpha;
                Material& m = materials.back();
                m.ambient[3] = m.diffuse[3] = m.specular[3] = alpha;
            }
        }
        file.close();
    }

    int indiceDoMaterial(const std::string& nome) const {
        for (size_t i = 0; i < materials.size(); i++) {
            if (materials[i].name == nome) return (int)i;
        }
        return -1;
    }

    // Envia um material do .mtl para o OpenGL
    void aplicarMaterial(const Material& m) const {
        glMaterialfv(GL_FRONT, GL_AMBIENT, m.ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, m.diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, m.specular);
        glMaterialf(GL_FRONT, GL_SHININESS, m.shininess);
    }

public:
    OBJModel() : hasTexCoords(false), hasNormals(false),
                 boundsMin{0.0f, 0.0f, 0.0f}, boundsMax{0.0f, 0.0f, 0.0f} {}

    int getNumMaterials() const { return (int)materials.size(); }

    // Centro da bounding box do modelo no espaço local (útil para
    // alinhar cálculos no mundo, como colisão, com a malha visível,
    // já que o .obj pode não ter a origem centralizada)
    Vector3 getCenter() const {
        return {
            (boundsMin.x + boundsMax.x) / 2.0f,
            (boundsMin.y + boundsMax.y) / 2.0f,
            (boundsMin.z + boundsMax.z) / 2.0f
        };
    }

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
        materials.clear();
        hasTexCoords = false;
        hasNormals = false;

        // Material declarado pelo último "usemtl": vale para todas as faces
        // seguintes, até aparecer outro.
        int materialAtual = -1;

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string type;
            ss >> type;

            if (type == "v") {
                Vector3 v;
                ss >> v.x >> v.y >> v.z;
                vertices.push_back(v);

                if (vertices.size() == 1) {
                    boundsMin = v;
                    boundsMax = v;
                } else {
                    if (v.x < boundsMin.x) boundsMin.x = v.x;
                    if (v.y < boundsMin.y) boundsMin.y = v.y;
                    if (v.z < boundsMin.z) boundsMin.z = v.z;
                    if (v.x > boundsMax.x) boundsMax.x = v.x;
                    if (v.y > boundsMax.y) boundsMax.y = v.y;
                    if (v.z > boundsMax.z) boundsMax.z = v.z;
                }
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
            } else if (type == "mtllib") {
                std::string mtlName;
                ss >> mtlName;
                if (!mtlName.empty()) loadMTL(diretorioDe(filename) + mtlName);
            } else if (type == "usemtl") {
                std::string mtlName;
                ss >> mtlName;
                materialAtual = indiceDoMaterial(mtlName);
            } else if (type == "f") {
                Face face;
                face.matIndex = materialAtual;
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

    // Desenha o modelo aplicando os materiais do .mtl, trocando o material do
    // OpenGL a cada grupo "usemtl". É o que permite um único objeto ter partes
    // com propriedades ópticas diferentes (ex.: pernas de metal, com brilho
    // especular concentrado, e assento de plástico, mais fosco).
    //
    // O draw() comum continua existindo pra quem quer controlar o material por
    // fora (é o caso do jogador, que tem material próprio definido na cena).
    void drawWithMaterials() const {
        int ultimoMaterial = -2; // -2 = nenhum aplicado ainda (-1 é "sem material")

        for (const auto& face : faces) {
            if (face.matIndex != ultimoMaterial) {
                if (face.matIndex >= 0 && face.matIndex < (int)materials.size()) {
                    aplicarMaterial(materials[face.matIndex]);
                }
                ultimoMaterial = face.matIndex;
            }
            desenharFace(face);
        }
    }

    // Desenha o modelo na tela aplicando as texturas e normais
    void draw() const {
        for (const auto& face : faces) {
            desenharFace(face);
        }
    }

private:
    // Emite uma face para o OpenGL, com normais e coordenadas de textura.
    // Usada tanto pelo draw() quanto pelo drawWithMaterials().
    void desenharFace(const Face& face) const {
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
};

#endif // OBJ_LOADER_H