❤️ Nicolas ❤️ (Só faltou a onça pintuda)

# ODEIOS
- ODEIO DÁRIO
- ODEIO HELOYSA
- ODEIO AUZIER
- ODEIO KELVIN

# DoomLike OpenGL Project

Este projeto utiliza **OpenGL (pipeline fixo + GLSL 1.20)** para renderização,  
**GLUT** para gerenciamento de janela/entrada e **GLEW** para carregar funções modernas  
como shaders, VBOs e extensões necessárias.

## 🎥 Demonstração
https://github.com/user-attachments/assets/e15d84f7-5f58-44b0-a429-0d45a9c4fcaf

---

## 📦 Dependências

Certifique-se de ter instalados os seguintes pacotes no seu sistema Linux:

### 🛠️ Compilação
- `g++`
- `make`

### 🖥️ Bibliotecas OpenGL
- `freeglut`
- `glew` (NOVA BIBLIOTECA QUE PRECISA INSTALAR)
- `mesa`
- `glu`

### 🖼️ Carregamento de Texturas  
- `stb_image.h` (arquivo de cabeçalho incluso no projeto)

---

## 🚀 Compilar e Executar

Use o comando abaixo para compilar o projeto e executá-lo imediatamente:

### 🐧 Linux
```bash
g++ main.cpp draw.cpp input.cpp scene.cpp texture.cpp shader.cpp \
    -o DoomLike \
    -lGLEW -lGL -lGLU -lglut && ./DoomLike
```

### 🪟 Windows
```bash
g++ main.cpp draw.cpp input.cpp scene.cpp texture.cpp shader.cpp ^
    -o DoomLike.exe ^
    -lglew32 -lfreeglut -lopengl32 -lglu32 && DoomLike.exe
```
## 🎮 Como Jogar

A cena pode ser explorada em primeira pessoa, com movimentação típica de FPS clássico.

---

## ⌨️ Controles

### 🧭 Movimento
| Tecla | Ação |
|-------|------|
| **W** | Avançar |
| **A** | Mover para a esquerda (strafe) |
| **S** | Recuar |
| **D** | Mover para a direita (strafe) |

---

### 🖱️ Visão
| Ação | Resultado |
|------|-----------|
| **Mover o mouse** | Olhar em qualquer direção |

---

### 🪟 Janelas e Sistema
| Tecla | Ação |
|-------|------|
| **Alt + Enter** | Alterna entre tela cheia e modo janela |
| **ESC** | Encerra o programa |

---

## 🗺️ Criando o Mapa (Matriz em `.txt`)

O mapa do jogo é definido por um arquivo **texto (ASCII)**, onde **cada caractere representa um tile** do mundo.  
Cada **linha do arquivo** corresponde a uma linha do mapa, e **todas as linhas devem ter o mesmo comprimento** (mesma quantidade de colunas).

---

### ✅ Regras importantes
- O arquivo deve ser salvo como `.txt`
- Cada linha representa uma “fileira” do mapa
- Todas as linhas precisam ter o mesmo tamanho
- Use **apenas os caracteres da legenda abaixo**
- Deve existir **exatamente um `9`** (posição inicial do jogador)

---

### 🧩 Legenda do mapa (originais)
| Caractere | Significado |
|----------|-------------|
| `1` | Parede |
| `0` | Chão normal (piso) |
| `L` | Lava (tile com shader de calor) |
| `B` | Sangue (tile com shader de distorção) |
| `9` | Spawn do jogador *(o loader converte para `0` após ler)* |

---

### 📌 Exemplo simples de mapa
```txt
1111111111
1000000001
10L0000B01
1000090001
1000000001
1111111111
```

---

### 👾 Inimigos no mapa

Os inimigos são definidos diretamente no arquivo do mapa por letras.  
Esses caracteres **não representam blocos sólidos**, servem apenas como ponto de spawn.

| Caractere | Tipo de inimigo |
|----------|----------------|
| `J` | Inimigo tipo J |
| `T` | Inimigo tipo T |
| `M` | Inimigo tipo M |
| `K` | Inimigo tipo K |
| `G` | Inimigo tipo G |

Após o carregamento do mapa, o caractere é convertido para piso e a entidade é criada separadamente.

---

### 🧰 Itens no mapa

Os itens também são definidos diretamente no mapa e funcionam como pontos de spawn.

| Caractere | Item | Descrição |
|----------|-----|-----------|
| `H` | Health | Recupera vida do jogador |
| `A` | Ammo | Recupera munição |

- não geram geometria,
- não bloqueiam passagem,
- não participam da colisão do cenário,
- são renderizados como sprites (billboards).

---

### 🔍 Parâmetros globais do raycast (culling)

```cpp
static float gCullHFovDeg      = 170.0f;
static float gCullNearTiles    = 2.0f;
static float gCullMaxDistTiles = 20.0f;
```

gCullHFovDeg
Define o campo de visão horizontal (em graus) usado no culling. Objetos fora desse ângulo, no plano XZ, não são renderizados.

gCullNearTiles
Define uma zona próxima ao jogador (em tiles) onde o culling angular é desativado, garantindo que objetos muito próximos sempre sejam desenhados.

gCullMaxDistTiles
Define a distância máxima de renderização (em tiles). Objetos além desse raio não são renderizados. Se for 0, não há limite de distância.

