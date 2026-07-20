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
float speedMultiplier = 1.0f;
int spawnCooldown = 0;

// Variáveis de Objetos
OBJModel estudanteModel;
OBJModel obstaculoModel;
OBJModel provaModel;

// Variáveis de Texturas
GLuint texProfessor;
GLuint texPisoUFCA;
GLuint texParedeUFCA;

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
float professorZ = -15.0f;        // Localização do Professor
float profSpeed = 0.001f;         // Velocidade dele
const float Z_PERIGO = playerZ;   // Distância para erro = gameover
const float Z_GAMEOVER = -3.0f;   // Distância para gameover

// Variáveis de Cenário
float offsetCenario = 0.0f; // Faz o cenario se mover

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

void init()
{
    // Define a cor do céu
    glClearColor(0.85f, 0.55f, 0.4f, 1.0f);

    // Ativa o Z-Buffer
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_TEXTURE_2D); // Permite usar texturas
    glEnable(GL_LIGHTING);   // Permite usar materiais

    // Luzes
    glEnable(GL_LIGHT0); // Ativa a Luz 0
    GLfloat luz_posicao[] = {0.0f, 5.0f, 15.0f, 1.0f};
    GLfloat luz_ambiente[] = {0.3f, 0.3f, 0.3f, 1.0f};  // Ilumina as sombras
    GLfloat luz_difusa[] = {0.8f, 0.8f, 0.8f, 1.0f};    // Cor principal da luz
    GLfloat luz_especular[] = {1.0f, 1.0f, 1.0f, 1.0f}; // Brilho para reflexos fortes
    glLightfv(GL_LIGHT0, GL_POSITION, luz_posicao);
    glLightfv(GL_LIGHT0, GL_AMBIENT, luz_ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_difusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz_especular);

    // Luz 1: Spotlight do Corredor (Frente)
    glEnable(GL_LIGHT1);
    GLfloat spot1_pos[] = {0.0f, 6.0f, 5.0f, 1.0f};   // Posicionada no teto
    GLfloat spot1_dir[] = {0.0f, -1.0f, -0.5f};       // Apontando pra baixo e pro fundo
    GLfloat spot_difusa[] = {1.0f, 1.0f, 0.8f, 1.0f}; // Cor amarelada de lâmpada
    glLightfv(GL_LIGHT1, GL_POSITION, spot1_pos);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spot1_dir);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 45.0f);  // Abertura do cone de luz
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 5.0f); // Foco no centro
    glLightfv(GL_LIGHT1, GL_DIFFUSE, spot_difusa);

    // Luz 2: Spotlight do Corredor (ilumina o professor)
    glEnable(GL_LIGHT2);
    GLfloat spot2_pos[] = {0.0f, 6.0f, -15.0f, 1.0f};
    GLfloat spot2_dir[] = {0.0f, -1.0f, 0.5f};
    glLightfv(GL_LIGHT2, GL_POSITION, spot2_pos);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spot2_dir);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 50.0f);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 5.0f);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, spot_difusa);

    texProfessor = loadTexture("assets/textures/fig1.png");
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

void desenharCorredores()
{
    glEnable(GL_TEXTURE_2D);

    // Material neutro para as paredes e chão não brilharem demais
    GLfloat mat_cenario[] = {0.9f, 0.9f, 0.9f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_cenario);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f); // Cenário é fosco

    // CHÃO (Piso da UFCA)
    glBindTexture(GL_TEXTURE_2D, texPisoUFCA); // DESCOMENTADO!
    glBegin(GL_QUADS);
    // Desliza a textura no eixo V (profundidade do chão)
    glTexCoord2f(0.0f, 0.0f - offsetCenario);
    glVertex3f(-4.0f, 0.0f, 15.0f);
    glTexCoord2f(1.0f, 0.0f - offsetCenario);
    glVertex3f(4.0f, 0.0f, 15.0f);
    glTexCoord2f(1.0f, 10.0f - offsetCenario);
    glVertex3f(4.0f, 0.0f, -30.0f);
    glTexCoord2f(0.0f, 10.0f - offsetCenario);
    glVertex3f(-4.0f, 0.0f, -30.0f);
    glEnd();

    // PAREDE ESQUERDA
    glBindTexture(GL_TEXTURE_2D, texParedeUFCA); // DESCOMENTADO!
    glBegin(GL_QUADS);
    // CORREÇÃO: O offset agora é no eixo U (horizontal) e nos 4 cantos para não rasgar!
    glTexCoord2f(0.0f - offsetCenario, 0.0f);
    glVertex3f(-4.0f, 0.0f, 15.0f);
    glTexCoord2f(10.0f - offsetCenario, 0.0f);
    glVertex3f(-4.0f, 0.0f, -30.0f);
    glTexCoord2f(10.0f - offsetCenario, 1.0f);
    glVertex3f(-4.0f, 10.0f, -30.0f);
    glTexCoord2f(0.0f - offsetCenario, 1.0f);
    glVertex3f(-4.0f, 10.0f, 15.0f);
    glEnd();

    // PAREDE DIREITA
    glBindTexture(GL_TEXTURE_2D, texParedeUFCA); // DESCOMENTADO!
    glBegin(GL_QUADS);
    // CORREÇÃO: Mesma coisa, desliza suavemente no eixo U
    glTexCoord2f(0.0f - offsetCenario, 0.0f);
    glVertex3f(4.0f, 0.0f, 15.0f);
    glTexCoord2f(10.0f - offsetCenario, 0.0f);
    glVertex3f(4.0f, 0.0f, -30.0f);
    glTexCoord2f(10.0f - offsetCenario, 1.0f);
    glVertex3f(4.0f, 10.0f, -30.0f);
    glTexCoord2f(0.0f - offsetCenario, 1.0f);
    glVertex3f(4.0f, 10.0f, 15.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void desenharCena()
{

    glEnable(GL_TEXTURE_2D); // Reativa as texturas

    // Desenha professor
    glBindTexture(GL_TEXTURE_2D, texProfessor);
    GLfloat mat_prof[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_prof);

    glPushMatrix();
    glTranslatef(0.0f, 3.0f, professorZ);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-5.0f, -5.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(5.0f, -5.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(5.0f, 5.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-5.0f, 5.0f, 0.0f);
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

    // Desliga luzes e profundidade para desenhar o texto
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    // Desenha o Score no canto da tela
    glColor3f(1.0f, 1.0f, 1.0f);
    std::string scoreTxt = "PONTOS: " + std::to_string(score);
    glRasterPos2f(20, 30); // Posição (X, Y) na tela
    for (char &c : scoreTxt)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Desenha mensagem de Game Over
    if (gameOver && professorZ > -2.0f)
    {
        glColor3f(1.0f, 0.2f, 0.2f); // Vermelho
        std::string goTxt = "GAME OVER! \n Pressione 'R' para reiniciar.";
        // Tenta centralizar mais ou menos
        glRasterPos2f(windowWidth / 2.0f - 150.0f, windowHeight / 2.0f);
        for (char &c : goTxt)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    // Restaura o estado para 3D
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

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
        exit(0); // Fechar o jogo no ESC

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

    if (key == 'r' || key == 'R')
    {
        if (gameOver)
        {
            gameOver = false;
            professorZ = -15.0f;
            playerX = 0.0f;
            playerY = 0.0f;
            targetPlayerX = 0.0f;
            isJumping = false;
            score = 0;              // Zera a pontuação
            speedMultiplier = 1.0f; // Zera a dificuldade
            for (int i = 0; i < MAX_OBSTACULOS; i++)
                obstaculos[i].ativo = false;
            for (int i = 0; i < MAX_PROVAS; i++)
                provas[i].ativo = false;
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

    if (gameOver)
    {
        tremorTempo = 0;
        if (professorZ < 6.0f)
        {
            professorZ += 0.7f;
        }
        glutPostRedisplay();
        glutTimerFunc(16, timer, 0);
        return;
    }

    // Incrementa a velocidade do cenário
    score += 1;                 // Ganha pontos
    speedMultiplier += 0.0002f; // O jogo vai ficando mais rápido

    // Multiplica a velocidade de todos os movimentos
    offsetCenario += (0.05f * speedMultiplier);
    if (offsetCenario > 10.0f)
        offsetCenario -= 10.0f;

    // Movimento melhorado do jogador
    playerX += (targetPlayerX - playerX) * 0.15f;
    if (std::abs(targetPlayerX - playerX) <= 0.015f)
    {
        lastPlayerX = targetPlayerX;
    }

    // Física do pulo
    if (isJumping)
    {
        playerY += jumpSpeed; // Sobe o jogador
        jumpSpeed -= 0.015f;  // Aplica gravidade reduzindo a velocidade

        // Verifica se tocou no chão
        if (playerY <= 0.0f)
        {
            playerY = 0.0f;
            isJumping = false; // Termina o pulo
        }
    }

    if (professorZ < Z_GAMEOVER)
    {
        professorZ += profSpeed;
    }
    else
    {
        gameOver = true;
    }

    if (tremorTempo > 0)
        tremorTempo--;

    // Spawn de objetos
    if (spawnCooldown > 0)
    {
        spawnCooldown--;
    }
    else
    {
        // Se a recarga acabou gera algo
        int chance = rand() % 100;

        if (chance < 80)
        { // 80% de chance de ser Obstáculo
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

                    // Define o tempo mínimo até a próxima geração
                    spawnCooldown = 10 + rand() % 30;
                    break;
                }
            }
        }
        else
        { // 20% de chance de ser Prova
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

            // Um cubo de tamanho 1.0 engloba -0.5 a +0.5 a partir do seu centro.
            // A janela é centrada em PLAYER_COLLISION_Z (posição real do mesh, não playerZ puro).
            if (obstaculos[i].z <= PLAYER_COLLISION_Z + 0.5f && obstaculos[i].z >= PLAYER_COLLISION_Z - 0.5f)
            {
                // Checa se o corpo do jogador está cruzando a largura do cubo
                bool bateuX = (std::abs(playerX - obstaculos[i].x) < 0.8f);
                bool bateuY = (playerY < 0.8f);

                if (bateuX && bateuY)
                {
                    obstaculos[i].ativo = false;
                    PlaySound(TEXT("assets/sounds/colisao.wav"), NULL, SND_ASYNC); // Toca o som de batida
                    if (professorZ >= Z_PERIGO)
                    {
                        gameOver = true;
                    }
                    else
                    {
                        targetPlayerX = lastPlayerX; // Colisão lateral: volta pra pista anterior
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

            if (provas[i].z <= PLAYER_COLLISION_Z + 1.5f && provas[i].z >= PLAYER_COLLISION_Z - 1.5f)
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
