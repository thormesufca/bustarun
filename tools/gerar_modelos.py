#!/usr/bin/env python3
"""Gera os modelos .obj/.mtl do obstaculo (carteira universitaria) e da prova
(pilha de folhas) usados pelo UFCA Runner.

Os modelos sao montados a partir de caixas posicionadas no espaco, e o script
escreve o .obj com vertices, normais e grupos usemtl -- ou seja, o jogo carrega
arquivos .obj de verdade pelo OBJModel, e nao geometria embutida no codigo.

Uso (a partir da raiz do projeto):
    python3 tools/gerar_modelos.py

Os arquivos gerados ficam em assets/models/ e sao versionados junto do projeto,
entao so e preciso rodar isso de novo se algum modelo for alterado.
"""

import math
import os

# As faces sao emitidas em sentido anti-horario vistas de fora, porque o jogo
# roda com GL_CULL_FACE e glFrontFace(GL_CCW): na ordem errada, a caixa fica
# invisivel (so as faces internas seriam desenhadas).
NORMAIS = {
    "+X": (1.0, 0.0, 0.0), "-X": (-1.0, 0.0, 0.0),
    "+Y": (0.0, 1.0, 0.0), "-Y": (0.0, -1.0, 0.0),
    "+Z": (0.0, 0.0, 1.0), "-Z": (0.0, 0.0, -1.0),
}


def rotacionar_y(p, angulo_graus):
    """Gira um ponto (ou vetor) em torno do eixo Y."""
    a = math.radians(angulo_graus)
    x, y, z = p
    return (x * math.cos(a) + z * math.sin(a), y, -x * math.sin(a) + z * math.cos(a))


def rotacionar_z(p, angulo_graus):
    """Gira um ponto (ou vetor) em torno do eixo Z."""
    a = math.radians(angulo_graus)
    x, y, z = p
    return (x * math.cos(a) - y * math.sin(a), x * math.sin(a) + y * math.cos(a), z)


class Malha:
    def __init__(self):
        self.vertices = []
        self.normais = []
        self.grupos = []  # (material, [(idx_vertices, idx_normal), ...])

    def _add_vertice(self, v):
        self.vertices.append(v)
        return len(self.vertices)  # .obj indexa a partir de 1

    def _add_normal(self, n):
        self.normais.append(n)
        return len(self.normais)

    def caixa(self, material, centro, tamanho, rot_y=0.0, rot_z=0.0):
        """Adiciona uma caixa com o material indicado.

        centro/tamanho sao (x, y, z); rot_y e rot_z giram a caixa em torno dos
        eixos Y e Z, aplicando o mesmo giro nas normais para a iluminacao
        continuar correta.
        """
        cx, cy, cz = centro
        sx, sy, sz = tamanho
        hx, hy, hz = sx / 2.0, sy / 2.0, sz / 2.0

        # Cantos no espaco local da caixa, antes de girar e transladar
        cantos_locais = [
            (-hx, -hy, -hz), (hx, -hy, -hz), (hx, -hy, hz), (-hx, -hy, hz),
            (-hx, hy, -hz), (hx, hy, -hz), (hx, hy, hz), (-hx, hy, hz),
        ]
        indices = []
        for c in cantos_locais:
            gx, gy, gz = rotacionar_y(rotacionar_z(c, rot_z), rot_y)
            indices.append(self._add_vertice((cx + gx, cy + gy, cz + gz)))

        # Cada face: (4 cantos em sentido anti-horario visto de fora, normal)
        faces = [
            ((3, 2, 6, 7), "+Z"), ((1, 0, 4, 5), "-Z"),
            ((2, 1, 5, 6), "+X"), ((0, 3, 7, 4), "-X"),
            ((7, 6, 5, 4), "+Y"), ((0, 1, 2, 3), "-Y"),
        ]

        faces_grupo = []
        for cantos, nome_normal in faces:
            n = rotacionar_y(rotacionar_z(NORMAIS[nome_normal], rot_z), rot_y)
            ni = self._add_normal(n)
            faces_grupo.append(([indices[i] for i in cantos], ni))

        self.grupos.append((material, faces_grupo))

    def salvar(self, caminho_obj, nome_mtl, comentario):
        with open(caminho_obj, "w") as f:
            f.write(f"# {comentario}\n")
            f.write("# Gerado por tools/gerar_modelos.py\n")
            f.write(f"mtllib {nome_mtl}\n")
            f.write(f"o {os.path.basename(caminho_obj).replace('.obj', '')}\n")
            for v in self.vertices:
                f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
            for n in self.normais:
                f.write(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}\n")
            for material, faces in self.grupos:
                f.write(f"usemtl {material}\n")
                for cantos, ni in faces:
                    verts = " ".join(f"{c}//{ni}" for c in cantos)
                    f.write(f"f {verts}\n")


def escrever_mtl(caminho, materiais, comentario):
    """materiais: lista de (nome, Kd, Ks, Ns)."""
    with open(caminho, "w") as f:
        f.write(f"# {comentario}\n")
        f.write("# Gerado por tools/gerar_modelos.py\n")
        for nome, kd, ks, ns in materiais:
            ka = tuple(c * 0.35 for c in kd)  # Ambiente = tom mais escuro da cor difusa
            f.write(f"\nnewmtl {nome}\n")
            f.write(f"Ka {ka[0]:.6f} {ka[1]:.6f} {ka[2]:.6f}\n")
            f.write(f"Kd {kd[0]:.6f} {kd[1]:.6f} {kd[2]:.6f}\n")
            f.write(f"Ks {ks[0]:.6f} {ks[1]:.6f} {ks[2]:.6f}\n")
            f.write(f"Ns {ns:.6f}\n")
            f.write("d 1.000000\n")
            f.write("illum 2\n")


def gerar_carteira(destino):
    """Carteira universitaria: o obstaculo que o jogador precisa desviar.

    Origem na base (y=0), para o objeto ser posicionado direto sobre o chao.
    Altura total ~1.0 e largura ~0.9, cabendo na faixa de colisao de 0.8.
    """
    m = Malha()

    # Pernas de metal tubular
    for dx in (-0.20, 0.20):
        for dz in (-0.20, 0.20):
            m.caixa("metal_estrutura", (dx, 0.235, dz), (0.05, 0.47, 0.05))

    # Travessa que liga as pernas (reforco horizontal)
    m.caixa("metal_estrutura", (0.0, 0.12, -0.20), (0.45, 0.04, 0.04))
    m.caixa("metal_estrutura", (0.0, 0.12, 0.20), (0.45, 0.04, 0.04))

    # Assento e encosto de plastico azul
    m.caixa("plastico_assento", (0.0, 0.49, 0.0), (0.52, 0.06, 0.48))
    m.caixa("plastico_assento", (0.0, 0.76, -0.23), (0.52, 0.48, 0.05), rot_y=0.0)

    # Braco de apoio e prancheta de escrita, do lado direito
    m.caixa("metal_estrutura", (0.28, 0.60, 0.02), (0.04, 0.24, 0.04))
    m.caixa("madeira_prancheta", (0.42, 0.73, 0.04), (0.40, 0.04, 0.36))

    m.salvar(
        os.path.join(destino, "carteira.obj"),
        "carteira.mtl",
        "Carteira universitaria - obstaculo do UFCA Runner",
    )
    escrever_mtl(
        os.path.join(destino, "carteira.mtl"),
        [
            # Metal: reflexo branco e bem concentrado (Ns alto)
            ("metal_estrutura", (0.24, 0.24, 0.28), (0.90, 0.90, 0.95), 260.0),
            # Plastico: brilho medio e mais espalhado
            ("plastico_assento", (0.13, 0.30, 0.72), (0.40, 0.40, 0.45), 70.0),
            # Madeira: praticamente fosca
            ("madeira_prancheta", (0.62, 0.44, 0.24), (0.12, 0.10, 0.07), 15.0),
        ],
        "Materiais da carteira universitaria",
    )


def gerar_lixeira(destino):
    """Lixeira de corredor: obstaculo mais baixo e largo que a carteira.

    Origem na base (y=0), como a carteira.
    """
    m = Malha()

    # Corpo, levemente mais estreito embaixo (duas caixas empilhadas dao a
    # impressao de afunilamento sem precisar de geometria inclinada)
    m.caixa("plastico_lixeira", (0.0, 0.14, 0.0), (0.36, 0.28, 0.36))
    m.caixa("plastico_lixeira", (0.0, 0.44, 0.0), (0.42, 0.32, 0.42))

    # Aro metalico da borda
    m.caixa("metal_aro", (0.0, 0.62, 0.0), (0.46, 0.05, 0.46))

    # Saco de lixo transbordando
    m.caixa("saco_lixo", (0.0, 0.69, 0.0), (0.36, 0.12, 0.36))
    m.caixa("saco_lixo", (0.06, 0.76, -0.04), (0.18, 0.10, 0.16), rot_y=25.0)

    m.salvar(
        os.path.join(destino, "lixeira.obj"),
        "lixeira.mtl",
        "Lixeira de corredor - obstaculo do UFCA Runner",
    )
    escrever_mtl(
        os.path.join(destino, "lixeira.mtl"),
        [
            ("plastico_lixeira", (0.13, 0.42, 0.20), (0.35, 0.40, 0.35), 55.0),
            ("metal_aro", (0.30, 0.30, 0.33), (0.90, 0.90, 0.95), 240.0),
            # Saco preto: fosco, quase sem reflexo
            ("saco_lixo", (0.10, 0.10, 0.12), (0.18, 0.18, 0.18), 20.0),
        ],
        "Materiais da lixeira",
    )


def gerar_livros(destino):
    """Pilha de livros deixada no chao: obstaculo baixo, da pra pular por cima.

    Origem na base (y=0).
    """
    m = Malha()

    capas = ["capa_vermelha", "capa_azul", "capa_verde", "capa_amarela", "capa_vermelha"]
    espessura = 0.09
    for i, capa in enumerate(capas):
        y = espessura / 2.0 + i * espessura
        angulo = -14.0 + i * 7.0  # Livros tortos, como uma pilha real
        largura = 0.42 - i * 0.015  # Os de cima um pouco menores
        profundidade = 0.32 - i * 0.010
        # Capa
        m.caixa(capa, (0.0, y, 0.0), (largura, espessura, profundidade), rot_y=angulo)
        # Miolo de paginas, um pouco menor, aparecendo nas bordas da capa
        m.caixa("paginas", (0.0, y, 0.0),
                (largura * 0.94, espessura * 0.7, profundidade * 0.92), rot_y=angulo)

    m.salvar(
        os.path.join(destino, "livros.obj"),
        "livros.mtl",
        "Pilha de livros - obstaculo do UFCA Runner",
    )
    escrever_mtl(
        os.path.join(destino, "livros.mtl"),
        [
            # Capas: cores distintas, com o brilho leve de capa plastificada
            ("capa_vermelha", (0.62, 0.12, 0.14), (0.45, 0.40, 0.40), 90.0),
            ("capa_azul", (0.12, 0.26, 0.60), (0.45, 0.45, 0.50), 90.0),
            ("capa_verde", (0.14, 0.46, 0.24), (0.40, 0.45, 0.40), 90.0),
            ("capa_amarela", (0.78, 0.62, 0.12), (0.50, 0.48, 0.35), 90.0),
            # Paginas: papel fosco
            ("paginas", (0.90, 0.88, 0.80), (0.10, 0.10, 0.10), 10.0),
        ],
        "Materiais da pilha de livros",
    )


def gerar_prova(destino):
    """Pilha de provas: o item coletavel.

    Origem no centro do objeto, porque ele flutua e gira em torno do proprio
    eixo enquanto vem pelo corredor.
    """
    m = Malha()

    # As folhas ficam em pe (plano XY, como uma folha de papel apoiada de
    # frente pra camera) e empilhadas em profundidade, no eixo Z. Em pe elas
    # aparecem inteiras pro jogador; deitadas, so a espessura da pilha era
    # visivel de cima.
    folhas = 6
    espessura = 0.010  # Espessura de cada folha
    passo = 0.014      # Distancia entre uma folha e a seguinte
    largura, altura = 0.46, 0.64  # Proporcao aproximada de uma folha A4

    for i in range(folhas):
        z = -(folhas - 1) * passo / 2.0 + i * passo
        # Folhas levemente tortas, pra pilha nao parecer um bloco solido
        inclinacao = -7.0 + i * 2.8
        desloc_x = math.sin(i * 1.7) * 0.012
        desloc_y = math.cos(i * 2.1) * 0.012
        # A folha da frente e a prova corrigida: material dourado, bem
        # especular, que capta os spots do corredor e sinaliza "colecionavel".
        material = "papel_prova" if i < folhas - 1 else "papel_nota"
        m.caixa(material, (desloc_x, desloc_y, z), (largura, altura, espessura),
                rot_z=inclinacao)

    m.salvar(
        os.path.join(destino, "prova.obj"),
        "prova.mtl",
        "Pilha de provas - item coletavel do UFCA Runner",
    )
    escrever_mtl(
        os.path.join(destino, "prova.mtl"),
        [
            # Papel comum: quase sem reflexo
            ("papel_prova", (0.93, 0.92, 0.86), (0.10, 0.10, 0.10), 10.0),
            # Prova "nota 10": dourada e muito especular
            ("papel_nota", (0.94, 0.76, 0.16), (1.00, 0.92, 0.55), 200.0),
        ],
        "Materiais da pilha de provas",
    )


if __name__ == "__main__":
    destino = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "assets", "models")
    destino = os.path.normpath(destino)
    gerar_carteira(destino)
    gerar_lixeira(destino)
    gerar_livros(destino)
    gerar_prova(destino)
    print(f"Modelos gerados em {destino}/:")
    for nome in ("carteira", "lixeira", "livros", "prova"):
        print(f"  {nome}.obj + {nome}.mtl")
