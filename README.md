# Visualizador de Cena 3D com OpenGL

Projeto desenvolvido para a disciplina de Computação Gráfica. O programa implementa um visualizador de cenas tridimensionais com carregamento de arquivos OBJ e MTL, texturas, iluminação de Phong, câmera sintética, transformações geométricas e animação por curva de Bézier.

## Funcionalidades

* Carregamento de múltiplos arquivos OBJ
* Leitura de materiais MTL
* Leitura dos coeficientes `Ka`, `Kd`, `Ks` e `Ns`
* Aplicação de texturas JPG
* Iluminação de Phong com três fontes de luz
* Controle de câmera por teclado e mouse
* Seleção de objetos
* Translação, rotação e escala uniforme
* Animação por curva de Bézier
* Configuração da cena por arquivo de texto

## Setup

### Dependências

Para compilar e executar o projeto, são necessárias as seguintes dependências:

* C++
* CMake
* OpenGL
* GLFW
* GLAD
* GLM
* STB Image

### Compilação

Abra o terminal na pasta principal do projeto e execute:

```powershell
cmake -S . -B build
cmake --build build
```

### Execução

No Windows, execute:

```powershell
.\build\main.exe
```

Caso o terminal já esteja dentro da pasta `build`, execute:

```powershell
.\main.exe
```

Os arquivos da cena devem permanecer dentro da pasta `assets`, mantendo a seguinte estrutura:

```text
assets/
├── configuracao_sala.txt
├── Modelos3D/
│   ├── Sofa.obj
│   ├── Sofa.mtl
│   ├── Television.obj
│   └── Television.mtl
└── Textures/
    ├── Sofa_01_diff_1k.jpg
    └── Television_01_diff_1k.jpg
```

## Controles

* `W`, `A`, `S`, `D`: movimentar a câmera
* Mouse: controlar a direção da câmera
* `SPACE` e `SHIFT`: subir e descer a câmera
* `TAB`: selecionar o próximo objeto
* `I`, `J`, `K`, `L` e setas: transladar o objeto selecionado
* `Q` e `E`: rotacionar o objeto
* `Z` e `X`: alterar a escala uniformemente
* `1`, `2` e `3`: ligar ou desligar as fontes de luz
* `T`: ligar ou desligar as texturas
* `P`: iniciar ou pausar a animação Bézier
* `R`: reiniciar a trajetória
* `ESC`: fechar o programa

## Assets

### Sofá

* Modelo: Sofa
* Formato original: OBJ, MTL e textura JPG
* Fonte: [COLOCAR LINK DO MODELO DO SOFÁ]
* Licença: [COLOCAR LICENÇA INFORMADA PELO SITE]

### Televisão

* Modelo: Television
* Formato original: OBJ, MTL e textura JPG
* Fonte: [COLOCAR LINK DO MODELO DA TELEVISÃO]
* Licença: [COLOCAR LICENÇA INFORMADA PELO SITE]

Os modelos foram processados no Blender 5.1.2 para verificação da malha, exportação para OBJ/MTL e preparação das faces, normais e coordenadas de textura.

Também foram realizados ajustes nos arquivos MTL para corrigir os caminhos das texturas e garantir a leitura dos coeficientes `Ka`, `Kd`, `Ks` e `Ns`.

## Arquivo de configuração

O arquivo `configuracao.txt` armazena:

* Posição e orientação inicial da câmera
* Definição do frustum
* Caminhos dos arquivos OBJ, MTL e texturas
* Transformações iniciais dos objetos
* Posição, cor e intensidade das luzes
* Pontos de controle das curvas de Bézier
* Velocidade das animações

## Referências

* GLM Documentation.
  https://github.com/g-truc/glm

* GLAD OpenGL Loader.
  https://glad.dav1d.de/

* STB Image.
  https://github.com/nothings/stb

* Blender Documentation.
  https://docs.blender.org/

* Material disponibilizado durante as aulas da disciplina de Computação Gráfica.
