#include <GL/glut.h> 
#include <iostream>

// Tamanho da janela
int windowWidth = 800;
int windowHeight = 600;

void init() {
    // Define a cor do céu
    glClearColor(0.5f, 0.8f, 0.9f, 1.0f);

    // Ativa o Z-Buffer 
    glEnable(GL_DEPTH_TEST);
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

    // Aplica a Projeção Perspetiva (FOV, Aspect Ratio, Near, Far) (Usamos um FOV de 60 graus para parecer o subway surfers)
    gluPerspective(60.0f, aspect, 0.1f, 500.0f);
}

// Função para desenhar um chão temporário
void desenharChao() {
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
        glVertex3f(-10.0f, 0.0f,  10.0f);
        glVertex3f( 10.0f, 0.0f,  10.0f);
        glVertex3f( 10.0f, 0.0f, -200.0f);
        glVertex3f(-10.0f, 0.0f, -200.0f);
    glEnd();
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
        0.0f, 3.0f, 5.0f,    // Posição da Câmara (x, y, z)
        0.0f, 1.0f, -10.0f,  // Ponto para onde olha (x, y, z)
        0.0f, 1.0f, 0.0f     // Vetor Up (x, y, z)
    );

    // --- AQUI ENTRARÃO OS DESENHOS DOS OBJETOS ---
    desenharChao(); 

    // Double Buffer
    glutSwapBuffers();
}

// Função para capturar teclas de movimentação
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { 
        exit(0);     
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
    glutInitWindowPosition(100, 100); // Posição inicial da janela
    glutCreateWindow("UFCA Runner");
    init();

    glutDisplayFunc(display);       // Renderização
    glutReshapeFunc(reshape);       // Redimensionamento
    glutKeyboardFunc(keyboard);     // Input (letras)
    glutSpecialFunc(specialKeys);   // Input (setas)
    glutTimerFunc(0, timer, 0);     // Inicia o loop

    glutMainLoop();

    return 0;
}