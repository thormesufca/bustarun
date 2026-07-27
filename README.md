# UFCA Runner - Projeto de Computação Gráfica

Um jogo estilo *Endless Runner* desenvolvido em C++ com OpenGL para a disciplina de Computação Gráfica da UFCA (Universidade Federal do Cariri). 

Neste jogo, o jogador é um estudante correndo pelos corredores da universidade, desviando de obstáculos e coletando provas para evitar ser pego pelo temido Professor que o persegue.

---

## Como compilar e rodar

O projeto compila no Linux e no Windows a partir do mesmo código. O áudio é a única parte que muda entre os dois (OpenAL no Linux, MMSystem no Windows), e isso fica isolado em `include/audio.h`.

```bash
make       # compila para bin/ufca-runner
make run   # compila e executa
make clean # apaga os arquivos gerados
```

O jogo precisa ser executado a partir da raiz do projeto, porque os assets são carregados por caminho relativo (`assets/...`) — é o que o `make run` já faz.

### Dependências

| Sistema | Instalação |
|---|---|
| Arch / Manjaro | `sudo pacman -S freeglut openal mesa` |
| Ubuntu / Debian | `sudo apt install freeglut3-dev libopenal-dev libglu1-mesa-dev` |
| Fedora | `sudo dnf install freeglut-devel openal-soft-devel mesa-libGLU-devel` |
| Windows (MinGW) | freeglut; o `winmm` já vem com o compilador |

Opções extras do Makefile:

- `make NO_AUDIO=1` — compila sem som, para máquinas sem OpenAL instalado.
- `make DEBUG=1` — compila com símbolos de depuração e sem otimização.

### Controles

`A`/`D` ou setas para trocar de faixa, `W`/`Espaço`/seta para cima para pular, `F` alterna tela cheia, `R` reinicia após o game over e `ESC` volta ao menu.

---

## Lista de Objetivos do projeto:
- [x] **Câmera & Projeção** 
- [x] **Carregador de .obj e PNGs**
- [x] **Ilusão de movimento com UV Scrolling nas paredes e chão**
- [x] **Iluminação Básica**
- [x] **Movimentação entre 3 faixas e salto**
- [x] **Colisão com objetos**
- [x] **Gerador aleatório de Obstáculos e Provas**
- [x] **Jumpscare do professor e Screen Shake**
- [x] **Modelo do Obstáculo** Substituir o cubo vermelho por um modelo 3D de itens da faculdade. Três tipos sorteados: carteira universitária, lixeira e pilha de livros.
- [x] **Modelo da Prova** Substituir o cubo dourado por um modelo 3D de uma pilha de provas.
- [ ] **Skins do Jogador** Editar o arquivo de textura do personagem para colar o rosto dos membros da equipe.
- [ ] **Texturas Finais da UFCA** Trocar as texturas de teste provisórias por fotos reais que lembrem os blocos da UFCA.
- [x] **Sombras Projetadas** Criar a projeção da sombra no chão achatando os vértices do jogador e dos objetos com a cor cinza escura, sem iluminação
- [x] **HUD** Adicionar texto 2D na tela para exibir o tempo sobrevivido e uma mensagem de "GAME OVER - Pressione R para reiniciar".
- [x] **Sound Effects** Adicionar música e sons de coleta ao jogo
- [x] **Progressão de Dificuldade** Aumentar lentamente as variáveis e a velocidade que os objetos caem no eixo Z conforme o tempo de jogo passa, para o jogo ficar frenético.
- [ ] **Menu Inicial** Uma preta simples aguardando o usuário apertar ENTER para começar o jogo, mostrar créditos e selecionar skins.