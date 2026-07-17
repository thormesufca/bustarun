#include <GL/glut.h> 
#include <iostream>
#include "../include/obj_loader.h"
#include "../include/texture_loader.h"

// Tamanho da janela
int windowWidth = 800;
int windowHeight = 600;
bool isFullscreen = false;

GLuint texProfessor; // Variável para armazenar a textura
OBJModel estudanteModel;

void init() {
    // Define a cor do céu
    glClearColor(0.85f, 0.55f, 0.4f, 1.0f);

    // Ativa o Z-Buffer 
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_TEXTURE_2D); // Permite usar texturas
    glEnable(GL_LIGHTING);   // Permite usar materiais
    glEnable(GL_LIGHT0);     // Ativa a Luz 0
    
    // Luz branca para ser o Sol
    GLfloat luz_pos[] = { 0.0f, 10.0f, 10.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, luz_pos);

    texProfessor = loadTexture("assets/textures/fig1.png");

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

void desenharCena() {
    glDisable(GL_TEXTURE_2D); // Desativa para desenhar cor sólida [Mudar depois para uma textura de piso da ufca]
    // Material fosco para o asfalto
    GLfloat mat_chao[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_chao);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.0f); // Sem brilho

    glBegin(GL_QUADS);
        glVertex3f(-4.0f, 0.0f,  15.0f);
        glVertex3f( 4.0f, 0.0f,  15.0f);
        glVertex3f( 4.0f, 0.0f, -30.0f);
        glVertex3f(-4.0f, 0.0f, -30.0f);
    glEnd();
    glEnable(GL_TEXTURE_2D); // Reativa as texturas
    
    // Desenha professor
    glBindTexture(GL_TEXTURE_2D, texProfessor);
    GLfloat mat_prof[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_prof);

    glPushMatrix();
        glTranslatef(0.0f, 5.0f, -12.0f); 
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
        float balancoCorrida = sin(tempo * 15.0f) * 0.1f;
        glTranslatef(0.0f, 1.0f + balancoCorrida, 0.0f);
        
        
        float inclinacao = cos(tempo * 7.0f) * 5.0f;
        glRotatef(inclinacao, 0.0f, 0.0f, 1.0f); // Gira no eixo Z
        
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f); // Deixa o boneco de frente para a câmera

        glTranslatef(0.0f, 1.0f, 0.0f);
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

    // Desenha um obstaculo de teste
    /*glPushMatrix();
        glTranslatef(-1.5f, 0.5f, 4.0f); 
        glColor3f(0.9f, 0.9f, 0.1f);     
        glutSolidCube(1.0f);             
    glPopMatrix();*/
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

    // --- AQUI ENTRARÃO OS DESENHOS DOS OBJETOS ---
    desenharCena(); 

    // Double Buffer
    glutSwapBuffers();
}

// Função para capturar teclas de movimentação
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { // Esc fecha o jogo
        exit(0);     
    }

    if (key == 'f' || key == 'F') { // Colocar e tirar da tela cheia
        if (isFullscreen) {
            glutReshapeWindow(800, 600);
            glutPositionWindow(100, 100);
            isFullscreen = false;
        } else {
            glutFullScreen();
            isFullscreen = true;
        }
    }

    // Lógica de movimentação entrará aqui
}

// Função para capturar teclas especiais 
void specialKeys(int key, int x, int y) {
    // Lógica das setas do teclado entrará aqui
}

// Loop Principal
void timer(int value) {
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