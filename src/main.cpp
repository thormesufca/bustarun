#include <GL/glut.h>
#include <iostream>
#include <string>
#include "../include/obj_loader.h"
#include "../include/texture_loader.h"
#include <cstdlib>
#include <cmath>
#include <windows.h>
#include <mmsystem.h>

// Tamanho da janela
int windowWidth = 800;
int windowHeight = 600;
bool isFullscreen = true;

// Variáveis do jogo
bool gameOver = false;
int score = 0;
int topScore = 0;
float speedMultiplier = 1.0f;
int spawnCooldown = 0;

enum GameState
{
    MENU_INICIAL,
    JOGANDO,
    GAMEOVER_TELA,
    CREDITOS,
    SKINS
};
GameState currentState = MENU_INICIAL;

int menuSelecionado = 0; // 0: Jogar, 1: Skins, 2: Creditos, 3: Sair
const int MAX_MENU_OPCOES = 4;

int skinSelecionada = 0;
const int MAX_SKINS = 3;
const char *nomesSkins[MAX_SKINS] = {"Luis", "Tony", "Bustamante"};

// Nomes para os créditos
const char *creditosNomes[] = {
    "ARTHUR LOBO FEITOSA DE OLIVEIRA",
    "GUILHERME VIANA BATISTA",
    "LUÍS FAGNER DE CARVALHO SILVA ",
    "RUAN PABLO FURTADO OLIVEIRA",
    "THORMES FILGUEIRA LEITE PEREIRA ",
    "TONY ESAU DE OLIVEIRA",
    "Agradecimento a professora Luana!"};
const int numCreditos = 7;

// Variáveis de Objetos
OBJModel estudanteModel;
OBJModel obstaculoModel;
OBJModel provaModel;

// Variáveis de Texturas
GLuint texProfessor;
GLuint texPisoUFCA;
GLuint texParedeUFCA;
GLuint texMenu;
GLuint texSkins[MAX_SKINS]; // Array para guardar as skins carregadas

// Variáveis do Jogador
float playerX = 0.0f;        // Posição X
float playerY = 0.0f;        // Posição Y
const float playerZ = -7.5f; // Posição Z
float targetPlayerX = 0.0f;  // Vai ser usada para garantir a colisão
float lastPlayerX = 0.0f;    // Ultima posição do player antes de colidir
bool isJumping = false;      // Se está pulando
float jumpSpeed = 0.0f;      // Força do pulo

// Modelos .obj podem não ter a origem centralizada, então o mesh renderizado
// pode ficar deslocado do ponto de translação (playerZ). PLAYER_COLLISION_Z é
// calculado em init(), a partir do centro real da bounding box do modelo
// carregado, para que a colisão continue correta mesmo se o .obj for trocado.
float PLAYER_COLLISION_Z = playerZ;

// Variáveis do Professor
float professorZ = -15.0f;      // Localização do Professor
float profSpeed = 0.001f;       // Velocidade dele
const float Z_PERIGO = playerZ; // Distância para erro = gameover
const float Z_GAMEOVER = -3.0f; // Distância para gameover

// Variáveis de Cenário
float offsetCenario = 0.0f; // Faz o cenario se mover

float spotViajanteZ = 5.0f;             // Z atual da LIGHT1
const float SPOT_TETO_Z_INICIO = 15.0f; // Onde a luz "nasce" (mesmo limite do chão)
const float SPOT_TETO_Z_FIM = -30.0f;   // Onde a luz some e reinicia (mesmo limite do chão)
const float SPOT_TETO_PERCURSO = SPOT_TETO_Z_INICIO - SPOT_TETO_Z_FIM;
const float SPOT_PROFESSOR_ALTURA = 3.0f;
const float SPOT_PROFESSOR_OFFSET_Z = 4.0f;

// Struct dos objetos presentes no jogo
enum TipoObjeto
{
    OBSTACULO,
    PROVA
};
struct Objeto
{
    float x, y, z;
    bool ativo;
    TipoObjeto tipo;
};
const int MAX_OBSTACULOS = 5;      // Qtd max de obstaculos na tela
Objeto obstaculos[MAX_OBSTACULOS]; // Lista de obstaculos

const int MAX_PROVAS = 2;
Objeto provas[MAX_PROVAS];

// Variáveis para o tremor
int tremorTempo = 0;            // Quantos frames a tela vai tremer
float intensidadeTremor = 0.3f; // O quão forte a tela balança

// Função para gerar textos
void texto2D(float x, float y, void *fonte, const char *texto, float r, float g, float b)
{
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    while (*texto)
    {
        glutBitmapCharacter(fonte, *texto);
        texto++;
    }
}

// Função de reset
void resetarJogo()
{
    gameOver = false;
    professorZ = -15.0f;
    playerX = 0.0f;
    targetPlayerX = 0.0f;
    playerY = 0.0f;
    isJumping = false;
    score = 0;
    speedMultiplier = 1.0f;
    spawnCooldown = 0;
    for (int i = 0; i < MAX_OBSTACULOS; i++)
        obstaculos[i].ativo = false;
    for (int i = 0; i < MAX_PROVAS; i++)
        provas[i].ativo = false;
}

void init()
{
    // Define a cor do céu
    glClearColor(0.85f, 0.55f, 0.4f, 1.0f);

    // Ativa o Z-Buffer
    glEnable(GL_DEPTH_TEST);

    // Remoção de superfícies ocultas: descarta as faces de trás (o lado que
    // nunca é visto de dentro do corredor/pelos objetos da cena).
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // Convenção padrão: vértices em sentido anti-horário = face da frente

    glEnable(GL_TEXTURE_2D); // Permite usar texturas
    glEnable(GL_LIGHTING);   // Permite usar materiais

    // Luzes
    // Ambiente global baixo: deixa espaço de contraste pros spots do teto
    // aparecerem (se a base já satura a cor em branco, o spot some).
    GLfloat luz_ambiente_global[] = {0.02f, 0.02f, 0.02f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luz_ambiente_global);

    glEnable(GL_LIGHT0); // Ativa a Luz 0
    GLfloat luz_posicao[] = {0.0f, 5.0f, 15.0f, 1.0f};
    GLfloat luz_ambiente[] = {0.0f, 0.0f, 0.0f, 1.0f};  // Ilumina as sombras
    GLfloat luz_difusa[] = {0.45f, 0.45f, 0.45f, 1.0f}; // Cor principal da luz (reduzida p/ não saturar)
    // Especular reduzida: LIGHT0 é fixa e nunca muda, então um brilho especular
    // forte dela ficaria constante no jogador e disfarçaria a passagem dos spots.
    GLfloat luz_especular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, luz_posicao);
    glLightfv(GL_LIGHT0, GL_AMBIENT, luz_ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_difusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz_especular);

    // Luz 1 e Luz 2: Spots do teto do corredor. Posição, direção, difusa e
    // especular são redefinidas a cada frame em atualizarSpotsTeto() (chamada
    // logo após o gluLookAt), pra usar sempre a mesma transformação da câmera.
    glEnable(GL_LIGHT1);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 35.0f);   // Abertura do cone de luz
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 10.0f); // Foco no centro

    glEnable(GL_LIGHT2);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 15.0f);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 60.0f);

    // Carregamento de Texturas
    texMenu = loadTexture("assets/textures/background.png");
    texProfessor = loadTexture("assets/textures/Luis.png");
    texPisoUFCA = loadTexture("assets/textures/fig2.png");
    texParedeUFCA = loadTexture("assets/textures/fig3.png");

    if (!estudanteModel.load("assets/models/estudante.obj"))
    {
        std::cerr << "Falha ao carregar o modelo do estudante!" << std::endl;
    }
    else
    {
        PLAYER_COLLISION_Z = playerZ + estudanteModel.getCenter().z;
    }
    // Skins
    texSkins[0] = loadTexture("assets/textures/Luis.png");
    texSkins[1] = loadTexture("assets/textures/Tony.png");
    texSkins[2] = loadTexture("assets/textures/Bustamante.png");
    texProfessor = texSkins[0]; // Skin Default

    // Descomente quando tiver os modelos
    /*
    if (!obstaculoModel.load("assets/models/cadeira.obj")) {
         std::cerr << "Falha ao carregar o obstaculo!" << std::endl;
    }
    if (!provaModel.load("assets/models/prova.obj")) {
         std::cerr << "Falha ao carregar a prova!" << std::endl;
    }
    */
}

// Redesenho da janela e configuração da projeção
void reshape(int w, int h)
{
    windowWidth = w;
    windowHeight = h;

    // Previne divisão por zero
    if (h == 0)
        h = 1;
    float aspect = (float)w / (float)h;

    // Define o Viewport para ocupar toda a janela
    glViewport(0, 0, w, h);

    // Configura a Matriz de Projeção
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Aplica a Projeção Perspetiva (FOV, Aspect Ratio, Near, Far)
    gluPerspective(70.0f, aspect, 0.1f, 100.0f);
}

// Limites do corredor no eixo Z (mesmos usados pelo scroll dos obstáculos/spots)
const float CORREDOR_Z_INICIO = 15.0f;
const float CORREDOR_Z_FIM = -30.0f;
const int CORREDOR_SEGMENTOS_Z = 45; // 1 unidade por segmento

void desenharCorredores()
{
    glEnable(GL_TEXTURE_2D);

    // Material neutro para as paredes e chão não brilharem demais
    GLfloat mat_cenario[] = {0.8f, 0.8f, 0.8f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_cenario);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f); // Cenário é fosco

    // Chão e paredes precisam ser desenhados em vários segmentos pequenos, e não
    // como um único quad gigante: a iluminação do OpenGL é calculada por vértice
    // e depois interpolada, então um quad de 45 unidades só teria vértices nas
    // duas pontas — um spot passando pelo meio nunca apareceria.
    const float passoZ = (CORREDOR_Z_INICIO - CORREDOR_Z_FIM) / CORREDOR_SEGMENTOS_Z;

    // CHÃO (Piso da UFCA)
    glBindTexture(GL_TEXTURE_2D, texPisoUFCA);
    const int segmentosX = 8;
    const float passoX = 8.0f / segmentosX;
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f); // Chão aponta pra cima, recebe luz dos spots do teto
    for (int iz = 0; iz < CORREDOR_SEGMENTOS_Z; iz++)
    {
        float z0 = CORREDOR_Z_INICIO - iz * passoZ;
        float z1 = z0 - passoZ;
        // Desliza a textura no eixo V (profundidade do chão)
        float v0 = (float)iz / CORREDOR_SEGMENTOS_Z * 10.0f - offsetCenario;
        float v1 = (float)(iz + 1) / CORREDOR_SEGMENTOS_Z * 10.0f - offsetCenario;
        for (int ix = 0; ix < segmentosX; ix++)
        {
            float x0 = -4.0f + ix * passoX;
            float x1 = x0 + passoX;
            float u0 = (float)ix / segmentosX;
            float u1 = (float)(ix + 1) / segmentosX;

            glTexCoord2f(u0, v0);
            glVertex3f(x0, 0.0f, z0);
            glTexCoord2f(u1, v0);
            glVertex3f(x1, 0.0f, z0);
            glTexCoord2f(u1, v1);
            glVertex3f(x1, 0.0f, z1);
            glTexCoord2f(u0, v1);
            glVertex3f(x0, 0.0f, z1);
        }
    }
    glEnd();

    // PAREDES (esquerda e direita)
    glBindTexture(GL_TEXTURE_2D, texParedeUFCA);
    const int segmentosY = 4;
    const float passoY = 10.0f / segmentosY;
    const float paredesX[2] = {-4.0f, 4.0f};
    const float paredesNormalX[2] = {1.0f, -1.0f}; // Sempre apontando pra dentro do corredor
    // A parede direita precisa da ordem dos vértices invertida: com o cull face
    // ativo, a face "de frente" (visível de dentro do corredor) é definida pelo
    // sentido anti-horário dos vértices, e as duas paredes são espelhadas.
    const bool paredesInverter[2] = {false, true};

    for (int p = 0; p < 2; p++)
    {
        float x = paredesX[p];
        bool inverter = paredesInverter[p];
        glBegin(GL_QUADS);
        glNormal3f(paredesNormalX[p], 0.0f, 0.0f);
        for (int iz = 0; iz < CORREDOR_SEGMENTOS_Z; iz++)
        {
            float z0 = CORREDOR_Z_INICIO - iz * passoZ;
            float z1 = z0 - passoZ;
            // Desliza a textura no eixo U (horizontal, acompanhando o corredor)
            float u0 = (float)iz / CORREDOR_SEGMENTOS_Z * 10.0f - offsetCenario;
            float u1 = (float)(iz + 1) / CORREDOR_SEGMENTOS_Z * 10.0f - offsetCenario;
            for (int iy = 0; iy < segmentosY; iy++)
            {
                float y0 = iy * passoY;
                float y1 = y0 + passoY;
                float v0 = (float)iy / segmentosY;
                float v1 = (float)(iy + 1) / segmentosY;

                if (!inverter)
                {
                    glTexCoord2f(u0, v0);
                    glVertex3f(x, y0, z0);
                    glTexCoord2f(u1, v0);
                    glVertex3f(x, y0, z1);
                    glTexCoord2f(u1, v1);
                    glVertex3f(x, y1, z1);
                    glTexCoord2f(u0, v1);
                    glVertex3f(x, y1, z0);
                }
                else
                {
                    glTexCoord2f(u0, v0);
                    glVertex3f(x, y0, z0);
                    glTexCoord2f(u0, v1);
                    glVertex3f(x, y1, z0);
                    glTexCoord2f(u1, v1);
                    glVertex3f(x, y1, z1);
                    glTexCoord2f(u1, v0);
                    glVertex3f(x, y0, z1);
                }
            }
        }
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
}

void desenharCena()
{

    glEnable(GL_TEXTURE_2D); // Reativa as texturas

    // Desenha professor
    glBindTexture(GL_TEXTURE_2D, texProfessor);
    GLfloat mat_prof[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_prof);

    // "dist" normalizada: 1 quando o professor está longe (Z inicial, -15),
    // 0 quando ele chega no limite de game over (Z_GAMEOVER). Zerando G e B
    // do especular conforme ele se aproxima, o reflexo vai de quase branco
    // para vermelho puro — um aviso visual de perigo.
    float dist = (professorZ - Z_GAMEOVER) / (-15.0f - Z_GAMEOVER);
    if (dist < 0.0f)
        dist = 0.0f;
    if (dist > 1.0f)
        dist = 1.0f;

    GLfloat mat_prof_espec[] = {1.0f, 1.0f * dist, 1.0f * dist, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_prof_espec);
    glMaterialf(GL_FRONT, GL_SHININESS, 90.0f);

    glPushMatrix();
    glTranslatef(0.0f, 3.0f, professorZ);
    // Subdividido numa grade (não um quad único): a iluminação do OpenGL é
    // calculada por vértice, então um único quad de 10x10 só teria os 4 cantos
    // como vértices, e a luz da LIGHT2 nunca formaria um foco concentrado no
    // meio da imagem (mesmo motivo pelo qual o chão/paredes foram subdivididos
    // em desenharCorredores()).
    const int segmentosProf = 10;
    const float ladoProf = 10.0f; // De -5 a 5
    const float passoProf = ladoProf / segmentosProf;
    glBegin(GL_QUADS);
    // Sem isso, o quad herda a normal deixada pela última superfície desenhada
    // em desenharCorredores() (uma parede lateral) e a LIGHT2 não ilumina nada.
    glNormal3f(0.0f, 0.0f, 1.0f); // Billboard voltado pra câmera (+Z)
    for (int iy = 0; iy < segmentosProf; iy++)
    {
        float y0 = -5.0f + iy * passoProf;
        float y1 = y0 + passoProf;
        float v0 = (float)iy / segmentosProf;
        float v1 = (float)(iy + 1) / segmentosProf;
        for (int ix = 0; ix < segmentosProf; ix++)
        {
            float x0 = -5.0f + ix * passoProf;
            float x1 = x0 + passoProf;
            float u0 = (float)ix / segmentosProf;
            float u1 = (float)(ix + 1) / segmentosProf;

            glTexCoord2f(u0, v0);
            glVertex3f(x0, y0, 0.0f);
            glTexCoord2f(u1, v0);
            glVertex3f(x1, y0, 0.0f);
            glTexCoord2f(u1, v1);
            glVertex3f(x1, y1, 0.0f);
            glTexCoord2f(u0, v1);
            glVertex3f(x0, y1, 0.0f);
        }
    }
    glEnd();
    glPopMatrix();

    // Desenha o jogador
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();
    // Faz a animação do jogador
    float tempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float balancoCorrida = gameOver ? 0.0f : (sin(tempo * 15.0f) * 0.1f);
    float inclinacao = gameOver ? 0.0f : (cos(tempo * 7.0f) * 5.0f);

    // Desenhar Sombra
    glPushMatrix();
    // Acompanha a posição X do jogador
    glTranslatef(playerX, 0.01f, playerZ);
    glRotatef(inclinacao, 0.0f, 0.0f, 1.0f); // Animacao

    // Quanto mais alto o jogador pula a sombra muda
    float shadowScale = 1.0f - (playerY * 0.2f);
    if (shadowScale < 0.8f)
        shadowScale = 0.8f; // Limite mínimo de tamanho

    // Achata a sombra no Y
    glScalef(shadowScale, 0.01f, shadowScale);

    glDisable(GL_LIGHTING);      // Sombra não recebe luz
    glColor3f(0.0f, 0.0f, 0.0f); // Cor cinza bem escuro
    estudanteModel.draw();
    glEnable(GL_LIGHTING);
    glPopMatrix();

    // Desenhar Jogador
    glTranslatef(playerX, 0.0f + playerY + balancoCorrida, playerZ);
    glRotatef(inclinacao, 0.0f, 0.0f, 1.0f);

    GLfloat mat_amb[] = {0.2f, 0.2f, 0.2f, 1.0f};  // Luz natural do corpo
    GLfloat mat_dif[] = {0.1f, 0.3f, 0.8f, 1.0f};  // Roupa azul refletindo
    GLfloat mat_spec[] = {1.0f, 1.0f, 1.0f, 1.0f}; // Brilho de suor/plástico
    GLfloat mat_shin[] = {50.0f};                  // reflexão especular

    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_dif);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shin);

    estudanteModel.draw();
    glPopMatrix();

    // Desenha os obstáculos
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
    GLfloat sem_brilho[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, sem_brilho);
    for (int i = 0; i < MAX_OBSTACULOS; i++)
    {
        if (obstaculos[i].ativo)
        {
            glPushMatrix();
            glTranslatef(obstaculos[i].x, obstaculos[i].y, obstaculos[i].z);
            GLfloat mat_obs[] = {0.8f, 0.1f, 0.1f, 1.0f}; // Vermelho
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_obs);
            glutSolidCube(1.0f); // Comente essa linha quando tiver o modelo
            // obstaculoModel.draw(); // Descomente esta linha
            glPopMatrix();
        }
    }

    // Desenha as provas
    for (int i = 0; i < MAX_PROVAS; i++)
    {
        if (provas[i].ativo)
        {
            glPushMatrix();
            glTranslatef(provas[i].x, provas[i].y, provas[i].z);
            GLfloat mat_prova[] = {0.9f, 0.8f, 0.1f, 1.0f}; // Amarelo Dourado
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_prova);

            // Faz a prova girar
            float tempoAnim = glutGet(GLUT_ELAPSED_TIME) / 10.0f;
            glRotatef(tempoAnim, 0.0f, 1.0f, 0.0f);
            glutSolidCube(0.8f); // Comente essa linha quando tiver o modelo
            // provaModel.draw(); // Descomente esta linha
            glPopMatrix();
        }
    }
}

// Move o spot viajante (LIGHT1) ao longo do corredor (mesma lógica de scroll
// usada pelos obstáculos/chão), acompanha o professor com a LIGHT2 (sempre
// a uma altura fixa acima dele, não presa a um ponto do corredor) e faz a
// intensidade piscar com uma senoide. Precisa ser chamada a cada frame,
// depois do gluLookAt.
void atualizarSpotsTeto()
{
    float tempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    if (currentState == JOGANDO && !gameOver)
    {
        spotViajanteZ -= (0.2f * speedMultiplier);
        if (spotViajanteZ < SPOT_TETO_Z_FIM)
            spotViajanteZ += SPOT_TETO_PERCURSO;
    }

    GLfloat spot1_pos[] = {0.0f, 6.0f, spotViajanteZ, 1.0f};
    // O centro do billboard do professor é desenhado em (0, 3, professorZ)
    // (mesmo translate usado em desenharCena()). A luz fica um pouco à frente
    // dele (Z maior, do lado da câmera/jogador) em vez de direto acima, pra
    // não ficar perpendicular à imagem (que não refletiria nada).
    float profCentroX = 0.0f, profCentroY = 3.0f, profCentroZ = professorZ;
    GLfloat spot2_pos[] = {0.0f, SPOT_PROFESSOR_ALTURA, professorZ + SPOT_PROFESSOR_OFFSET_Z, 1.0f};
    // Levemente inclinada para +/-Z (não reto pra baixo): o chão tem normal
    // pra cima e pega luz bem de qualquer jeito, mas o personagem é visto de
    // frente/costas (normal ~Z) e reto-pra-baixo quase não o atinge (N.L~0).
    // Uma leve inclinação dá um N.L bem maior quando o spot passa perto dele.
    GLfloat dirFrente[] = {0.0f, -1.0f, -0.4f};
    // Recalculada a cada frame pra sempre apontar de volta pro centro do
    // professor, já que a luz agora está deslocada dele (não mais direto acima).
    GLfloat dirProf[] = {
        profCentroX - spot2_pos[0],
        profCentroY - spot2_pos[1],
        profCentroZ - spot2_pos[2]};
    glLightfv(GL_LIGHT1, GL_POSITION, spot1_pos);
    glLightfv(GL_LIGHT2, GL_POSITION, spot2_pos);
    // Redefinida aqui (não só em init()) pra usar a mesma transformação de
    // câmera (gluLookAt) que a posição, já aplicada nesse ponto do frame.
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, dirFrente);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, dirProf);

    // Piscar: intensidade oscila entre quase apagado e o brilho total.
    // Fases diferentes (0 e PI) pra as duas luzes não piscarem em sincronia.
    float piscar1 = powf(0.5f + 0.5f * sinf(tempo * 4.0f), 3.0f);
    float piscar2 = powf(0.5f + 0.5f * sinf(tempo * 4.0f + 3.14159f), 3.0f);
    piscar1 = 0.1f + 0.9f * piscar1; // Nunca apaga 100%
    piscar2 = 0.1f + 0.9f * piscar2;
    piscar1 = 1.0f; // Comente para ativar o efeito de piscar da luz 1
    piscar2 = 1.0f; // Comente para ativar o efeito de piscar da luz 2

    GLfloat spot1_difusa[] = {1.0f * piscar1, 1.0f * piscar1, 0.8f * piscar1, 1.0f};
    GLfloat spot2_difusa[] = {1.0f * piscar2, 1.0f * piscar2, 0.8f * piscar2, 1.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, spot1_difusa);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, spot2_difusa);

    // Especular acompanhando o piscar: reforça a sensação de "luz passando"
    // no personagem (que é bem mais visível como brilho do que como difusa,
    // já que suas superfícies voltadas pra câmera pegam pouca luz de cima).
    GLfloat spot1_espec[] = {1.0f * piscar1, 1.0f * piscar1, 1.0f * piscar1, 1.0f};
    GLfloat spot2_espec[] = {1.0f * piscar2, 1.0f * piscar2, 1.0f * piscar2, 1.0f};
    glLightfv(GL_LIGHT1, GL_SPECULAR, spot1_espec);
    glLightfv(GL_LIGHT2, GL_SPECULAR, spot2_espec);
}

void desenharHUD()
{

    // Salva matrizes atuais e muda para projeção ortogonal
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, windowHeight, 0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Desliga luzes, profundidade e cull face para desenhar o HUD.
    // O cull face precisa sair: essa projeção ortográfica (gluOrtho2D) tem o eixo
    // Y invertido (origem no topo), o que inverte o sentido dos quads 2D e faria
    // o cull remover os quads da imagem de fundo/skins por engano.
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    char buffer[100];

    if (currentState != JOGANDO)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texMenu);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(windowWidth, 0);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(windowWidth, windowHeight);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0, windowHeight);
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    if (currentState == MENU_INICIAL)
    {

        texto2D(windowWidth / 2 - 100, 150, GLUT_BITMAP_TIMES_ROMAN_24, "UFCA RUNNER", 1.0f, 1.0f, 1.0f);

        texto2D(windowWidth / 2 - 50, 300, GLUT_BITMAP_HELVETICA_18, menuSelecionado == 0 ? "> Jogar" : "  Jogar", 1.0f, 1.0f, 0.0f);
        texto2D(windowWidth / 2 - 50, 340, GLUT_BITMAP_HELVETICA_18, menuSelecionado == 1 ? "> Skins do Professor" : "  Skins do Professor", 1.0f, 1.0f, 0.0f);
        texto2D(windowWidth / 2 - 50, 380, GLUT_BITMAP_HELVETICA_18, menuSelecionado == 2 ? "> Creditos" : "  Creditos", 1.0f, 1.0f, 0.0f);
        texto2D(windowWidth / 2 - 50, 420, GLUT_BITMAP_HELVETICA_18, menuSelecionado == 3 ? "> Sair" : "  Sair", 1.0f, 1.0f, 0.0f);
    }
    else if (currentState == JOGANDO)
    {
        // Score maior e no topo
        sprintf(buffer, "SCORE: %d", score);
        texto2D(20, 40, GLUT_BITMAP_TIMES_ROMAN_24, buffer, 1.0f, 1.0f, 1.0f);
        sprintf(buffer, "TOP SCORE: %d", topScore);
        texto2D(windowWidth - 240, 40, GLUT_BITMAP_TIMES_ROMAN_24, buffer, 1.0f, 1.0f, 1.0f);
    }
    else if (currentState == GAMEOVER_TELA)
    {
        texto2D(windowWidth / 2 - 100, windowHeight / 2 - 50, GLUT_BITMAP_TIMES_ROMAN_24, "GAME OVER!", 1.0f, 0.0f, 0.0f);
        texto2D(windowWidth / 2 - 180, windowHeight / 2 + 20, GLUT_BITMAP_HELVETICA_18, "Pressione [R] para Reiniciar", 1.0f, 1.0f, 1.0f);
        texto2D(windowWidth / 2 - 200, windowHeight / 2 + 60, GLUT_BITMAP_HELVETICA_18, "Pressione [ESC] para Menu Principal", 1.0f, 1.0f, 1.0f);
    }
    else if (currentState == SKINS)
    {
        texto2D(windowWidth / 2 - 100, 150, GLUT_BITMAP_TIMES_ROMAN_24, "SELECIONE A SKIN", 1.0f, 1.0f, 1.0f);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texSkins[skinSelecionada]);

        float imgSize = 250.0f;
        float x = windowWidth / 2.0f - imgSize / 2.0f;
        float y = windowHeight / 2.0f - imgSize / 2.0f;

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(x, y);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(x + imgSize, y);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(x + imgSize, y + imgSize);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(x, y + imgSize);
        glEnd();
        glDisable(GL_TEXTURE_2D);

        sprintf(buffer, "< %s >", nomesSkins[skinSelecionada]);
        texto2D(windowWidth / 2 - 250, windowHeight / 2, GLUT_BITMAP_HELVETICA_18, buffer, 0.0f, 1.0f, 0.0f);
        texto2D(windowWidth / 2 - 250, windowHeight - 50, GLUT_BITMAP_HELVETICA_18, "Pressione [ENTER] para confirmar", 1.0f, 1.0f, 1.0f);
    }
    else if (currentState == CREDITOS)
    {
        // Aqui o resto da equipe pode renderizar a textura de fundo da UFCA depois
        texto2D(windowWidth / 2 - 60, 100, GLUT_BITMAP_TIMES_ROMAN_24, "CREDITOS", 1.0f, 1.0f, 0.0f);
        for (int i = 0; i < numCreditos; i++)
        {
            texto2D(windowWidth / 2 - 100, 200 + (i * 40), GLUT_BITMAP_HELVETICA_18, creditosNomes[i], 1.0f, 1.0f, 1.0f);
        }
        texto2D(windowWidth / 2 - 100, windowHeight - 50, GLUT_BITMAP_HELVETICA_18, "Pressione [ESC] para voltar", 0.5f, 0.5f, 0.5f);
    }

    // Restaura o estado para 3D
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Função principal de renderização
void display()
{
    // Limpa os buffers de cor e de profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configura a Matriz de Modelview
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Eye: Atrás e acima do ponto de origem
    // Center: A olhar para o horizonte
    // Up: Eixo Y
    gluLookAt(
        0.0f, 4.0f, 9.0f,   // Posição da Câmara (x, y, z)
        0.0f, 1.0f, -15.0f, // Ponto para onde olha (x, y, z)
        0.0f, 1.0f, 0.0f    // Vetor Up (x, y, z)
    );

    glPushMatrix(); // Salva a matriz da camera limpa
    if (tremorTempo > 0)
    {
        // Gera valores aleatórios entre -intensidadeTremor e +intensidadeTremor
        float dx = ((rand() % 100) / 100.0f - 0.5f) * intensidadeTremor;
        float dy = ((rand() % 100) / 100.0f - 0.5f) * intensidadeTremor;
        glTranslatef(dx, dy, 0.0f);
    }

    // --- AQUI ENTRARÃO OS DESENHOS DOS OBJETOS ---
    atualizarSpotsTeto();
    desenharCorredores();
    desenharCena();

    glPopMatrix();
    desenharHUD();
    // Double Buffer
    glutSwapBuffers();
}

// Função para capturar teclas de movimentação
void keyboard(unsigned char key, int x, int y)
{
    if (key == 27)
    {
        if (currentState == MENU_INICIAL)
            exit(0); // Fecha o jogo se estiver no MENU
        else if (currentState == JOGANDO || currentState == GAMEOVER_TELA || currentState == CREDITOS || currentState == SKINS)
        {
            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);
            currentState = MENU_INICIAL; // Volta ao menu
            resetarJogo();
        }
    }
    // Colocar e tirar da tela cheia
    if (key == 'f' || key == 'F')
    {
        if (isFullscreen)
        {
            glutReshapeWindow(800, 600);
            glutPositionWindow(100, 100);
            isFullscreen = false;
        }
        else
        {
            glutFullScreen();
            isFullscreen = true;
        }
    }

    if (currentState == MENU_INICIAL)
    {
        if (key == 13)
        { // Tecla ENTER
            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);
            if (menuSelecionado == 0)
            {
                currentState = JOGANDO;
                resetarJogo();
            }
            else if (menuSelecionado == 1)
                currentState = SKINS;
            else if (menuSelecionado == 2)
                currentState = CREDITOS;
            else if (menuSelecionado == 3)
                exit(0);
        }
    }
    else if (currentState == SKINS)
    {
        if (key == 13)
        { // Tecla ENTER
            std::string caminhoSkin = "assets/textures/" + std::string(nomesSkins[skinSelecionada]) + ".png";

            // Recarrega a textura do professor com a nova imagem
            texProfessor = texSkins[skinSelecionada];

            // Volta para o menu inicial
            currentState = MENU_INICIAL;
        }
    }
    else if (currentState == GAMEOVER_TELA)
    {
        if (key == 'r' || key == 'R')
        {
            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);

            // Reinicia o jogo
            currentState = JOGANDO;
            resetarJogo();
        }
    }

    if (key == 'r' || key == 'R')
    {
        if (gameOver)
        {
            resetarJogo();
        }
    }

    // Movimentação do Jogador (só aceita input fora de game over e do tremor de colisão)
    if (!gameOver && tremorTempo <= 5)
    {
        if ((key == 'a' || key == 'A') && targetPlayerX > -2.0f)
        {
            lastPlayerX = targetPlayerX;
            targetPlayerX -= 2.0f;
        }

        if ((key == 'd' || key == 'D') && targetPlayerX < 2.0f)
        {
            lastPlayerX = targetPlayerX;
            targetPlayerX += 2.0f;
        }
        // Pulo
        if ((key == 'w' || key == 'W' || key == ' ') && !isJumping)
        {
            isJumping = true;
            jumpSpeed = 0.25f; // "Força" inicial do pulo
        }
    }
}

// Função para capturar teclas especiais
void specialKeys(int key, int x, int y)
{
    if (currentState == MENU_INICIAL)
    {
        if (key == GLUT_KEY_UP)
        {
            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);
            menuSelecionado--;
            if (menuSelecionado < 0)
                menuSelecionado = MAX_MENU_OPCOES - 1;
        }
        if (key == GLUT_KEY_DOWN)
        {
            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);
            menuSelecionado++;
            if (menuSelecionado >= MAX_MENU_OPCOES)
                menuSelecionado = 0;
        }
    }
    else if (currentState == SKINS)
    {
        if (key == GLUT_KEY_LEFT)
        {
            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);
            skinSelecionada--;
            if (skinSelecionada < 0)
                skinSelecionada = MAX_SKINS - 1;
        }
        if (key == GLUT_KEY_RIGHT)
        {
            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);
            skinSelecionada++;
            if (skinSelecionada >= MAX_SKINS)
                skinSelecionada = 0;
        }
    }
    // Movimentação com as setas do teclado
    if (!gameOver && tremorTempo <= 5)
    {
        if (key == GLUT_KEY_LEFT && targetPlayerX > -2.0f)
            targetPlayerX -= 2.0f;
        if (key == GLUT_KEY_RIGHT && targetPlayerX < 2.0f)
            targetPlayerX += 2.0f;
        if (key == GLUT_KEY_UP && !isJumping)
        {
            isJumping = true;
            jumpSpeed = 0.25f;
        }
    }
}

// Loop Principal
void timer(int value)
{
    if (currentState == JOGANDO || currentState == GAMEOVER_TELA)
    {
        if (gameOver)
        {
            tremorTempo = 0;
            if (professorZ < 6.0f)
            {
                professorZ += 0.7f;
            }
        }
        else
        {
            // Incrementa a velocidade do cenário
            score += (int)(10 * speedMultiplier); // Ganha pontos
            speedMultiplier += 0.001f;

            offsetCenario += (0.05f * speedMultiplier);
            if (offsetCenario > 10.0f)
                offsetCenario -= 10.0f;

            playerX += (targetPlayerX - playerX) * 0.15f;
            if (std::abs(targetPlayerX - playerX) <= 0.015f)
            {
                lastPlayerX = targetPlayerX;
            }

            if (isJumping)
            {
                playerY += jumpSpeed;
                jumpSpeed -= 0.015f;

                if (playerY <= 0.0f)
                {
                    playerY = 0.0f;
                    isJumping = false;
                }
            }

            if (professorZ < Z_GAMEOVER)
            {
                professorZ += profSpeed;
            }
            else
            {
                gameOver = true;
                currentState = GAMEOVER_TELA; // NOVO: Força o estado pra garantir
                if (score > topScore)
                {
                    topScore = score;
                }
            }

            if (tremorTempo > 0)
                tremorTempo--;

            if (spawnCooldown > 0)
            {
                spawnCooldown--;
            }
            else
            {
                int chance = rand() % 100;
                if (chance < 80)
                {
                    for (int i = 0; i < MAX_OBSTACULOS; i++)
                    {
                        if (!obstaculos[i].ativo)
                        {
                            obstaculos[i].ativo = true;
                            obstaculos[i].tipo = OBSTACULO;
                            obstaculos[i].z = 25.0f;
                            obstaculos[i].y = 0.5f;
                            int faixa = rand() % 3;
                            obstaculos[i].x = (faixa == 0) ? -2.0f : ((faixa == 1) ? 0.0f : 2.0f);
                            spawnCooldown = 10 + rand() % 30;
                            break;
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < MAX_PROVAS; i++)
                    {
                        if (!provas[i].ativo)
                        {
                            provas[i].ativo = true;
                            provas[i].tipo = PROVA;
                            provas[i].z = 25.0f;
                            provas[i].y = 1.0f;
                            int faixa = rand() % 3;
                            provas[i].x = (faixa == 0) ? -2.0f : ((faixa == 1) ? 0.0f : 2.0f);
                            spawnCooldown = 20 + rand() % 30;
                            break;
                        }
                    }
                }
            }

            // Colisões
            for (int i = 0; i < MAX_OBSTACULOS; i++)
            {
                if (obstaculos[i].ativo)
                {
                    obstaculos[i].z -= (0.3f * speedMultiplier);
                    if (obstaculos[i].z < -15.0f)
                        obstaculos[i].ativo = false;

                    if (obstaculos[i].z <= (PLAYER_COLLISION_Z + 0.5f) && obstaculos[i].z >= (PLAYER_COLLISION_Z - 0.5f))
                    {
                        bool bateuX = (std::abs(playerX - obstaculos[i].x) < 0.8f);
                        bool bateuY = (playerY < 0.8f);

                        if (bateuX && bateuY)
                        {
                            obstaculos[i].ativo = false;
                            PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC);
                            if (professorZ >= Z_PERIGO)
                            {
                                gameOver = true;
                                currentState = GAMEOVER_TELA;
                                if (score > topScore)
                                {
                                    topScore = score;
                                }
                            }
                            else
                            {
                                targetPlayerX = lastPlayerX;
                                tremorTempo = 15;
                                professorZ += 3.0f;
                            }
                        }
                    }
                }
            }

            for (int i = 0; i < MAX_PROVAS; i++)
            {
                if (provas[i].ativo)
                {
                    provas[i].z -= (0.3f * speedMultiplier);
                    if (provas[i].z < -15.0f)
                        provas[i].ativo = false;

                    if (provas[i].z <= (PLAYER_COLLISION_Z + 1.5f) && provas[i].z >= (PLAYER_COLLISION_Z - 1.5f))
                    {
                        bool pegouX = (std::abs(playerX - provas[i].x) < 0.8f);
                        bool pegouY = (playerY < 1.8f);

                        if (pegouX && pegouY)
                        {
                            provas[i].ativo = false;
                            professorZ -= 2.5f;
                            if (professorZ < -20.0f)
                                professorZ = -20.0f;
                            PlaySound(TEXT("assets/sounds/coleta.wav"), NULL, SND_ASYNC);
                        }
                    }
                }
            }
        }
    }

    // Faz o OpenGL renderizar novamente
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    // Display com double buffer, rgb padrão e z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(windowWidth, windowHeight);
    // glutInitWindowPosition(100, 100); // Posição inicial da janela
    glutCreateWindow("UFCA Runner");

    if (isFullscreen)
    {
        glutFullScreen();
    }

    init();

    glutDisplayFunc(display);     // Renderização
    glutReshapeFunc(reshape);     // Redimensionamento
    glutKeyboardFunc(keyboard);   // Input (letras)
    glutSpecialFunc(specialKeys); // Input (setas)
    glutTimerFunc(0, timer, 0);   // Inicia o loop

    glutMainLoop();

    return 0;
}
