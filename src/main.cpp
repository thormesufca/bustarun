#include <GL/glut.h> 
#include <iostream>
#include "../include/obj_loader.h"
#include "../include/texture_loader.h"
#include <cstdlib> 

// Tamanho da janela
int windowWidth = 800;
int windowHeight = 600;
bool isFullscreen = false;

// Variáveis de Objetos
OBJModel estudanteModel;

// Variáveis do jogo
bool gameOver = false;

// Variáveis de Texturas
GLuint texProfessor; 
GLuint texPisoUFCA;
GLuint texParedeUFCA;

// Variáveis do Jogador
float playerX = 0.0f;   // Posição X
float playerY = 0.0f;   // Posição Y
bool isJumping = false; // Se está pulando
float jumpSpeed = 0.0f; // Força do pulo

// Variáveis do Professor
float professorZ = -15.0f;      // Localização do Professor
float profSpeed = 0.01f;        // Velocidade dele
const float Z_PERIGO = -5.0f;   // Distância para erro = gameover
const float Z_GAMEOVER = -3.0f; // Distância para gameover

// Variáveis de Cenário
float offsetCenario = 0.0f; // Faz o cenario se mover

// Struct dos objetos presentes no jogo
enum TipoObjeto {OBSTACULO, PROVA};
struct Objeto {  
    float x, y, z;
    bool ativo;
    TipoObjeto tipo;
};
const int MAX_OBSTACULOS = 5;           // Qtd max de obstaculos na tela
Objeto obstaculos[MAX_OBSTACULOS];   // Lista de obstaculos

const int MAX_PROVAS = 2;
Objeto provas[MAX_PROVAS];

// Variáveis para o tremor
int tremorTempo = 0;            // Quantos frames a tela vai tremer
float intensidadeTremor = 0.3f; // O quão forte a tela balança




void init() {
    // Define a cor do céu
    glClearColor(0.85f, 0.55f, 0.4f, 1.0f);

    // Ativa o Z-Buffer 
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_TEXTURE_2D); // Permite usar texturas
    glEnable(GL_LIGHTING);   // Permite usar materiais
    glEnable(GL_LIGHT0);     // Ativa a Luz 0
    
    // Iluminação
    GLfloat luz_posicao[]   = { 0.0f, 5.0f, 15.0f, 1.0f }; 
    GLfloat luz_ambiente[]  = { 0.3f, 0.3f, 0.3f, 1.0f }; // Ilumina as sombras
    GLfloat luz_difusa[]    = { 0.8f, 0.8f, 0.8f, 1.0f }; // Cor principal da luz
    GLfloat luz_especular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Brilho para reflexos fortes

    glLightfv(GL_LIGHT0, GL_POSITION, luz_posicao);
    glLightfv(GL_LIGHT0, GL_AMBIENT, luz_ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_difusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz_especular);

    texProfessor = loadTexture("assets/textures/fig1.png");
    texPisoUFCA = loadTexture("assets/textures/fig2.png");
    texParedeUFCA = loadTexture("assets/textures/fig3.png");

    if (!estudanteModel.load("assets/models/estudante.obj")) {
         std::cerr << "Falha ao carregar o modelo do estudante!" << std::endl;
    }
}

// Redesenho da janela e configuração da projeção
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;

    // Previne divisão por zero
    if (h == 0) h = 1;
    float aspect = (float)w / (float)h;

    // Define o Viewport para ocupar toda a janela
    glViewport(0, 0, w, h);

    // Configura a Matriz de Projeção
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Aplica a Projeção Perspetiva (FOV, Aspect Ratio, Near, Far)
    gluPerspective(70.0f, aspect, 0.1f, 100.0f);
}

void desenharCorredores() {
    glEnable(GL_TEXTURE_2D);

    // Material neutro para as paredes e chão não brilharem demais
    GLfloat mat_cenario[] = { 0.9f, 0.9f, 0.9f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_cenario);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f); // Cenário é fosco

    // 1. CHÃO (Piso da UFCA)
    glBindTexture(GL_TEXTURE_2D, texPisoUFCA); // DESCOMENTADO!
    glBegin(GL_QUADS);
        // Desliza a textura no eixo V (profundidade do chão)
        glTexCoord2f(0.0f, 0.0f - offsetCenario); glVertex3f(-4.0f, 0.0f,  15.0f);
        glTexCoord2f(1.0f, 0.0f - offsetCenario); glVertex3f( 4.0f, 0.0f,  15.0f);
        glTexCoord2f(1.0f, 10.0f - offsetCenario); glVertex3f( 4.0f, 0.0f, -30.0f);
        glTexCoord2f(0.0f, 10.0f - offsetCenario); glVertex3f(-4.0f, 0.0f, -30.0f);
    glEnd();

    // 2. PAREDE ESQUERDA (Tijolos ou pintura)
    glBindTexture(GL_TEXTURE_2D, texParedeUFCA); // DESCOMENTADO!
    glBegin(GL_QUADS);
        // CORREÇÃO: O offset agora é no eixo U (horizontal) e nos 4 cantos para não rasgar!
        glTexCoord2f(0.0f - offsetCenario, 0.0f);  glVertex3f(-4.0f, 0.0f,  15.0f);
        glTexCoord2f(10.0f - offsetCenario, 0.0f); glVertex3f(-4.0f, 0.0f, -30.0f);
        glTexCoord2f(10.0f - offsetCenario, 1.0f); glVertex3f(-4.0f, 10.0f, -30.0f);
        glTexCoord2f(0.0f - offsetCenario, 1.0f);  glVertex3f(-4.0f, 10.0f,  15.0f);
    glEnd();

    // 3. PAREDE DIREITA
    glBindTexture(GL_TEXTURE_2D, texParedeUFCA); // DESCOMENTADO!
    glBegin(GL_QUADS);
        // CORREÇÃO: Mesma coisa, desliza suavemente no eixo U
        glTexCoord2f(0.0f - offsetCenario, 0.0f);  glVertex3f( 4.0f, 0.0f,  15.0f);
        glTexCoord2f(10.0f - offsetCenario, 0.0f); glVertex3f( 4.0f, 0.0f, -30.0f);
        glTexCoord2f(10.0f - offsetCenario, 1.0f); glVertex3f( 4.0f, 10.0f, -30.0f);
        glTexCoord2f(0.0f - offsetCenario, 1.0f);  glVertex3f( 4.0f, 10.0f,  15.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}


void desenharCena() {

    glEnable(GL_TEXTURE_2D); // Reativa as texturas
    
    // Desenha professor
    glBindTexture(GL_TEXTURE_2D, texProfessor);
    GLfloat mat_prof[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_prof);

    glPushMatrix();
        glTranslatef(0.0f, 3.0f, professorZ); 
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-5.0f, -5.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 5.0f, -5.0f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 5.0f,  5.0f, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-5.0f,  5.0f, 0.0f);
        glEnd();
    glPopMatrix();

    // Desenha o jogador
    glDisable(GL_TEXTURE_2D); 
    glPushMatrix();
        // Define um periodo com base no loop do jogo
        float tempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; 
        float balancoCorrida = gameOver ? 0.0f : (sin(tempo * 15.0f) * 0.1f);
        glTranslatef(playerX, 1.0f + playerY + balancoCorrida, 0.0f);
        
        float inclinacao = gameOver ? 0.0f : (cos(tempo * 7.0f) * 5.0f);
        glRotatef(inclinacao, 0.0f, 0.0f, 1.0f); 
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

        GLfloat mat_amb[]  = { 0.2f, 0.2f, 0.2f, 1.0f }; // Luz natural do corpo
        GLfloat mat_dif[]  = { 0.1f, 0.3f, 0.8f, 1.0f }; // Roupa azul refletindo
        GLfloat mat_spec[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Brilho de suor/plástico
        GLfloat mat_shin[] = { 50.0f };                  // reflexão especular

        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_amb);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_dif);
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_spec);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_shin);

        // Para quando tiver com o obj:
        //drawOBJ("assets/models/estudante.obj"); 
        estudanteModel.draw(); 
    glPopMatrix();

    // Desenha os obstáculos
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
    for (int i = 0; i < MAX_OBSTACULOS; i++) {
        if (obstaculos[i].ativo) {
            glPushMatrix();
                glTranslatef(obstaculos[i].x, obstaculos[i].y, obstaculos[i].z);
                GLfloat mat_obs[] = { 0.8f, 0.1f, 0.1f, 1.0f }; // Vermelho
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_obs);
                glutSolidCube(1.0f); 
            glPopMatrix();
        }
    }

    // Desenha as provas
    for (int i = 0; i < MAX_PROVAS; i++) {
        if (provas[i].ativo) {
            glPushMatrix();
                glTranslatef(provas[i].x, provas[i].y, provas[i].z);
                GLfloat mat_prova[] = { 0.9f, 0.8f, 0.1f, 1.0f }; // Amarelo Dourado
                glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_prova);
                
                // Faz a prova girar
                float tempoAnim = glutGet(GLUT_ELAPSED_TIME) / 10.0f;
                glRotatef(tempoAnim, 0.0f, 1.0f, 0.0f);
                glutSolidCube(0.8f); 
            glPopMatrix();
        }
    }
}

// Função principal de renderização
void display() {
    // Limpa os buffers de cor e de profundidade
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configura a Matriz de Modelview
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Eye: Atrás e acima do ponto de origem
    // Center: A olhar para o horizonte
    // Up: Eixo Y
    gluLookAt(
        0.0f, 4.0f, 9.0f,    // Posição da Câmara (x, y, z)
        0.0f, 1.0f, -15.0f,  // Ponto para onde olha (x, y, z)
        0.0f, 1.0f, 0.0f     // Vetor Up (x, y, z)
    );

    glPushMatrix(); // Salva a matriz da camera limpa
    if (tremorTempo > 0) {
        // Gera valores aleatórios entre -intensidadeTremor e +intensidadeTremor
        float dx = ((rand() % 100) / 100.0f - 0.5f) * intensidadeTremor;
        float dy = ((rand() % 100) / 100.0f - 0.5f) * intensidadeTremor;
        glTranslatef(dx, dy, 0.0f);
    }

    // --- AQUI ENTRARÃO OS DESENHOS DOS OBJETOS ---
    desenharCorredores();
    desenharCena(); 

    glPopMatrix();
    // Double Buffer
    glutSwapBuffers();
}

// Função para capturar teclas de movimentação
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0); // Fechar o jogo no ESC

    // Colocar e tirar da tela cheia
    if (key == 'f' || key == 'F') { 
        if (isFullscreen) {
            glutReshapeWindow(800, 600);
            glutPositionWindow(100, 100);
            isFullscreen = false;
        } else {
            glutFullScreen();
            isFullscreen = true;
        }
    }

    // Movimentação do Jogador
    if ((key == 'a' || key == 'A') && playerX > -2.0f) {
        playerX -= 2.0f;
    }
    if ((key == 'd' || key == 'D') && playerX < 2.0f) {
        playerX += 2.0f;
    }
    // Pulo
    if ((key == 'w' || key == 'W' || key == ' ') && !isJumping) {
        isJumping = true;
        jumpSpeed = 0.25f; // "Força" inicial do pulo
    }
}

// Função para capturar teclas especiais 
void specialKeys(int key, int x, int y) {
    // Movimentação com as setas do teclado 
    if (key == GLUT_KEY_LEFT && playerX > -2.0f) playerX -= 2.0f;
    if (key == GLUT_KEY_RIGHT && playerX < 2.0f) playerX += 2.0f;
    if (key == GLUT_KEY_UP && !isJumping) {
        isJumping = true;
        jumpSpeed = 0.25f;
    }
}

// Loop Principal
void timer(int value) {
    
    if (gameOver) {
        professorZ += 0.3f; 
        if (professorZ > 12.0f) {
            exit(0); 
        }
        glutPostRedisplay();
        glutTimerFunc(16, timer, 0);
        return; 
    }
    
    
    // Incrementa a velocidade do cenário (Ajuste o 0.05f para mais rápido ou mais devagar)
    offsetCenario += 0.05f; 
    // Reseta o offset para não estourar o limite de memória do float
    if (offsetCenario > 10.0f) offsetCenario -= 10.0f;

    // Física do pulo
    if (isJumping) {
        playerY += jumpSpeed;  // Sobe o jogador
        jumpSpeed -= 0.015f;   // Aplica gravidade reduzindo a velocidade
        
        // Verifica se tocou no chão
        if (playerY <= 0.0f) {
            playerY = 0.0f;
            isJumping = false; // Termina o pulo
        }
    }

    if (professorZ < Z_GAMEOVER) {
        professorZ += profSpeed; 
    } else {
        gameOver = true; 
    }

    if (tremorTempo > 0) tremorTempo--;

    // --- LÓGICA DOS OBSTÁCULOS ---
    for (int i = 0; i < MAX_OBSTACULOS; i++) {
        if (obstaculos[i].ativo) {
            obstaculos[i].z -= 0.3f; 
            if (obstaculos[i].z < -15.0f) obstaculos[i].ativo = false;

            // Colisão Frontal (Obstáculo)
            if (obstaculos[i].z <= 0.5f && obstaculos[i].z >= -0.5f) {
                bool bateuX = (playerX > obstaculos[i].x - 1.0f && playerX < obstaculos[i].x + 1.0f);
                bool bateuY = (playerY < 0.8f); 
                
                if (bateuX && bateuY) {
                    obstaculos[i].ativo = false; 
                    if (professorZ >= Z_PERIGO) {
                        gameOver = true; 
                    } else {
                        tremorTempo = 15; 
                        professorZ += 3.0f; 
                    }
                }
            }
        } else {
            // Chance de gerar Obstáculo (3% por frame)
            if (rand() % 100 < 3) {
                obstaculos[i].ativo = true;
                obstaculos[i].tipo = OBSTACULO;
                obstaculos[i].z = 25.0f; 
                obstaculos[i].y = 0.5f; // Fica no chão
                
                int faixa = rand() % 3;
                obstaculos[i].x = (faixa == 0) ? -2.0f : ((faixa == 1) ? 0.0f : 2.0f);
            }
        }
    }

    // --- LÓGICA DAS PROVAS (Coletáveis) ---
    for (int i = 0; i < MAX_PROVAS; i++) {
        if (provas[i].ativo) {
            provas[i].z -= 0.3f; 
            if (provas[i].z < -15.0f) provas[i].ativo = false;

            // Coleta da Prova
            if (provas[i].z <= 0.5f && provas[i].z >= -0.5f) {
                bool bateuX = (playerX > provas[i].x - 1.0f && playerX < provas[i].x + 1.0f);
                // Não precisa verificar o Y com rigor, basta encostar no eixo X
                if (bateuX) {
                    provas[i].ativo = false; 
                    professorZ -= 2.5f; // Afasta o professor (Recompensa)
                    if (professorZ < -20.0f) professorZ = -20.0f; 
                }
            }
        } else {
            // Chance de gerar Prova (Apenas 1% por frame, são mais raras)
            if (rand() % 100 < 1) {
                provas[i].ativo = true;
                provas[i].tipo = PROVA;
                provas[i].z = 25.0f; 
                provas[i].y = 1.0f; // Flutua um pouco mais alto
                
                int faixa = rand() % 3;
                provas[i].x = (faixa == 0) ? -2.0f : ((faixa == 1) ? 0.0f : 2.0f);
            }
        }
    }
    // Faz o OpenGL renderizar novamente
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    // Display com double buffer, rgb padrão e z-buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowSize(windowWidth, windowHeight);
    //glutInitWindowPosition(100, 100); // Posição inicial da janela
    glutCreateWindow("UFCA Runner");
    
    if (isFullscreen) {
        glutFullScreen();
    }
    
    init();

    glutDisplayFunc(display);       // Renderização
    glutReshapeFunc(reshape);       // Redimensionamento
    glutKeyboardFunc(keyboard);     // Input (letras)
    glutSpecialFunc(specialKeys);   // Input (setas)
    glutTimerFunc(0, timer, 0);     // Inicia o loop

    glutMainLoop();

    return 0;
}