// Trabalho final de Computacao Grafica - GB
// Aluno: Leonardo Assumpção Eugenio


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// ---------------------------------------------------------
// Estruturas
// ---------------------------------------------------------

struct Material
{
    glm::vec3 ka = glm::vec3(0.2f);
    glm::vec3 kd = glm::vec3(0.7f);
    glm::vec3 ks = glm::vec3(0.3f);
    float shininess = 32.0f;
    string mapKd = "";
};

struct Mesh
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    int nVertices = 0;

    GLuint textureID = 0;
    bool hasTexture = false;

    Material material;
    string materialName = "default";
};

struct BezierAnimation
{
    bool enabled = false;

    glm::vec3 p0 = glm::vec3(0.0f);
    glm::vec3 p1 = glm::vec3(0.0f);
    glm::vec3 p2 = glm::vec3(0.0f);
    glm::vec3 p3 = glm::vec3(0.0f);

    float t = 0.0f;
    float speed = 0.20f;
    float direction = 1.0f;
};

struct SceneObject
{
    string name;
    vector<Mesh> meshes;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    BezierAnimation animation;
};

struct Light
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    bool enabled = true;
};

struct GrupoTemporario
{
    string materialName = "default";
    vector<float> buffer;
};

// ---------------------------------------------------------
// Variaveis globais
// ---------------------------------------------------------

GLFWwindow *Window = nullptr;

int WIDTH = 1000;
int HEIGHT = 700;

GLuint ShaderProgram = 0;

vector<SceneObject> Objetos;
Camera Cam;

Light KeyLight;
Light FillLight;
Light BackLight;

float GlobalIntensity = 1.0f;

int ObjetoSelecionado = 0;
bool AnimacaoLigada = false;
bool ExibirTextura = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool firstMouse = true;
double lastX = WIDTH / 2.0;
double lastY = HEIGHT / 2.0;

// ---------------------------------------------------------
// Shaders
// ---------------------------------------------------------

const char *vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoord = aTex;
    Normal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char *fragmentShaderSource = R"(
#version 330 core

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos[3];
uniform vec3 lightColor[3];
uniform float lightIntensity[3];
uniform int lightEnabled[3];
uniform float globalIntensity;

uniform vec3 viewPos;

uniform vec3 materialKa;
uniform vec3 materialKd;
uniform vec3 materialKs;
uniform float shininess;

uniform sampler2D texBuff;
uniform bool hasTexture;

void main()
{
    vec3 baseColor = vec3(1.0);

    if (hasTexture)
        baseColor = texture(texBuff, TexCoord).rgb;

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    // Componente ambiente.
    vec3 result =
        materialKa *
        baseColor *
        0.25 *
        globalIntensity;

    // Iluminacao de Phong com tres fontes.
    for (int i = 0; i < 3; i++)
    {
        if (lightEnabled[i] == 0)
            continue;

        vec3 L = normalize(lightPos[i] - FragPos);
        vec3 R = reflect(-L, N);

        float diff = max(dot(N, L), 0.0);

        vec3 diffuse =
            materialKd *
            diff *
            baseColor *
            lightColor[i] *
            lightIntensity[i];

        float spec = 0.0;

        if (diff > 0.0)
            spec = pow(max(dot(R, V), 0.0), shininess);

        vec3 specular =
            materialKs *
            spec *
            lightColor[i] *
            lightIntensity[i];

        result +=
            (diffuse + specular) *
            globalIntensity;
    }

    FragColor = vec4(result, 1.0);
}
)";

// ---------------------------------------------------------
// Funcoes auxiliares
// ---------------------------------------------------------

bool arquivoExiste(const string &path)
{
    ifstream teste(path.c_str(), ios::binary);
    return teste.good();
}

string arrumaCaminho(const string &path)
{
    if (
        path.empty() ||
        path == "none" ||
        path == "NONE" ||
        path == "-"
    )
    {
        return "";
    }

    vector<string> tentativas = {
        path,
        "../" + path,
        "../../" + path
    };

    for (const string &tentativa : tentativas)
    {
        if (arquivoExiste(tentativa))
            return tentativa;
    }

    return path;
}

string pegaDiretorio(const string &path)
{
    size_t pos = path.find_last_of("/\\");

    if (pos == string::npos)
        return "";

    return path.substr(0, pos + 1);
}

int indiceOBJ(int idx, int tamanho)
{
    if (idx > 0)
        return idx - 1;

    if (idx < 0)
        return tamanho + idx;

    return -1;
}

void selecionaObjeto(int indice)
{
    if (Objetos.empty())
        return;

    if (indice < 0)
        indice = (int)Objetos.size() - 1;

    if (indice >= (int)Objetos.size())
        indice = 0;

    ObjetoSelecionado = indice;

    cout
        << "Objeto selecionado ["
        << (ObjetoSelecionado + 1)
        << "/"
        << Objetos.size()
        << "]: "
        << Objetos[ObjetoSelecionado].name
        << endl;
}

// ---------------------------------------------------------
// Textura
// ---------------------------------------------------------

GLuint loadTexture(string filePath)
{
    filePath = arrumaCaminho(filePath);

    if (filePath.empty())
        return 0;

    int width = 0;
    int height = 0;
    int nrChannels = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(
        filePath.c_str(),
        &width,
        &height,
        &nrChannels,
        0
    );

    if (!data)
    {
        cout
            << "Nao consegui carregar textura: "
            << filePath
            << endl;

        return 0;
    }

    GLenum format = GL_RGB;

    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;

    GLuint texID = 0;

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    stbi_image_free(data);

    cout
        << "Textura carregada: "
        << filePath
        << endl;

    return texID;
}

// ---------------------------------------------------------
// Leitor MTL: Ka, Kd, Ks, Ns e map_Kd
// ---------------------------------------------------------

map<string, Material> loadMTL(string mtlPath)
{
    map<string, Material> materiais;

    mtlPath = arrumaCaminho(mtlPath);

    ifstream arq(mtlPath.c_str());

    if (!arq.is_open())
    {
        cout
            << "Nao achei MTL: "
            << mtlPath
            << endl;

        return materiais;
    }

    string dir = pegaDiretorio(mtlPath);
    string line;
    string word;
    string nomeAtual;

    while (getline(arq, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        istringstream ss(line);
        ss >> word;

        if (word == "newmtl")
        {
            ss >> nomeAtual;
            materiais[nomeAtual] = Material();
        }
        else if (word == "Ka" && !nomeAtual.empty())
        {
            ss
                >> materiais[nomeAtual].ka.r
                >> materiais[nomeAtual].ka.g
                >> materiais[nomeAtual].ka.b;
        }
        else if (word == "Kd" && !nomeAtual.empty())
        {
            ss
                >> materiais[nomeAtual].kd.r
                >> materiais[nomeAtual].kd.g
                >> materiais[nomeAtual].kd.b;
        }
        else if (word == "Ks" && !nomeAtual.empty())
        {
            ss
                >> materiais[nomeAtual].ks.r
                >> materiais[nomeAtual].ks.g
                >> materiais[nomeAtual].ks.b;
        }
        else if (word == "Ns" && !nomeAtual.empty())
        {
            ss >> materiais[nomeAtual].shininess;

            if (materiais[nomeAtual].shininess < 1.0f)
                materiais[nomeAtual].shininess = 32.0f;
        }
        else if (word == "map_Kd" && !nomeAtual.empty())
        {
            string tex;
            ss >> tex;

            materiais[nomeAtual].mapKd =
                dir + tex;
        }
    }

    cout
        << "MTL carregado: "
        << mtlPath
        << " | materiais: "
        << materiais.size()
        << endl;

    return materiais;
}

// ---------------------------------------------------------
// Leitor OBJ
// ---------------------------------------------------------

void adicionaVerticeFace(
    const string &token,
    const vector<glm::vec3> &posicoes,
    const vector<glm::vec2> &uvs,
    const vector<glm::vec3> &normais,
    vector<float> &buffer
)
{
    int vi = 0;
    int ti = 0;
    int ni = 0;

    string parte;
    vector<string> partes;
    stringstream ss(token);

    while (getline(ss, parte, '/'))
        partes.push_back(parte);

    if (!partes.empty() && !partes[0].empty())
        vi = stoi(partes[0]);

    if (partes.size() > 1 && !partes[1].empty())
        ti = stoi(partes[1]);

    if (partes.size() > 2 && !partes[2].empty())
        ni = stoi(partes[2]);

    int pIndex =
        indiceOBJ(vi, (int)posicoes.size());

    int tIndex =
        indiceOBJ(ti, (int)uvs.size());

    int nIndex =
        indiceOBJ(ni, (int)normais.size());

    glm::vec3 p(0.0f);
    glm::vec2 t(0.0f);
    glm::vec3 n(0.0f, 1.0f, 0.0f);

    if (
        pIndex >= 0 &&
        pIndex < (int)posicoes.size()
    )
    {
        p = posicoes[pIndex];
    }

    if (
        tIndex >= 0 &&
        tIndex < (int)uvs.size()
    )
    {
        t = uvs[tIndex];
    }

    if (
        nIndex >= 0 &&
        nIndex < (int)normais.size()
    )
    {
        n = normais[nIndex];
    }

    buffer.push_back(p.x);
    buffer.push_back(p.y);
    buffer.push_back(p.z);

    buffer.push_back(t.x);
    buffer.push_back(t.y);

    buffer.push_back(n.x);
    buffer.push_back(n.y);
    buffer.push_back(n.z);
}

vector<Mesh> loadOBJ(
    string objPath,
    string mtlForcado,
    const Material &materialPadrao,
    string texturaForcada
)
{
    vector<Mesh> meshes;

    objPath = arrumaCaminho(objPath);
    mtlForcado = arrumaCaminho(mtlForcado);
    texturaForcada = arrumaCaminho(texturaForcada);

    ifstream arq(objPath.c_str());

    if (!arq.is_open())
    {
        cout
            << "Erro ao abrir OBJ: "
            << objPath
            << endl;

        return meshes;
    }

    string dir = pegaDiretorio(objPath);

    vector<glm::vec3> posicoes;
    vector<glm::vec2> uvs;
    vector<glm::vec3> normais;

    vector<GrupoTemporario> grupos;
    grupos.push_back(GrupoTemporario());

    map<string, Material> materiais;

    if (!mtlForcado.empty())
        materiais = loadMTL(mtlForcado);

    string line;
    string word;
    int grupoAtual = 0;

    while (getline(arq, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        istringstream ss(line);
        ss >> word;

        if (word == "mtllib")
        {
            string mtlDoOBJ;
            ss >> mtlDoOBJ;

            if (mtlForcado.empty())
                materiais = loadMTL(dir + mtlDoOBJ);
        }
        else if (word == "v")
        {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            posicoes.push_back(v);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ss >> vt.x >> vt.y;
            uvs.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            normais.push_back(vn);
        }
        else if (word == "usemtl")
        {
            string mat;
            ss >> mat;

            if (grupos[grupoAtual].buffer.empty())
            {
                grupos[grupoAtual].materialName = mat;
            }
            else
            {
                GrupoTemporario novo;
                novo.materialName = mat;

                grupos.push_back(novo);
                grupoAtual =
                    (int)grupos.size() - 1;
            }
        }
        else if (word == "f")
        {
            vector<string> tokens;
            string token;

            while (ss >> token)
                tokens.push_back(token);

            // Aceita triangulos e triangula poligonos em leque.
            if (tokens.size() >= 3)
            {
                for (
                    int i = 1;
                    i < (int)tokens.size() - 1;
                    i++
                )
                {
                    adicionaVerticeFace(
                        tokens[0],
                        posicoes,
                        uvs,
                        normais,
                        grupos[grupoAtual].buffer
                    );

                    adicionaVerticeFace(
                        tokens[i],
                        posicoes,
                        uvs,
                        normais,
                        grupos[grupoAtual].buffer
                    );

                    adicionaVerticeFace(
                        tokens[i + 1],
                        posicoes,
                        uvs,
                        normais,
                        grupos[grupoAtual].buffer
                    );
                }
            }
        }
    }

    arq.close();

    for (GrupoTemporario &grupo : grupos)
    {
        if (grupo.buffer.empty())
            continue;

        Mesh mesh;

        mesh.nVertices =
            (int)grupo.buffer.size() / 8;

        mesh.materialName =
            grupo.materialName;

        mesh.material =
            materialPadrao;

        auto encontrado =
            materiais.find(grupo.materialName);

        if (encontrado != materiais.end())
            mesh.material = encontrado->second;

        string texPath = texturaForcada;

        if (
            texPath.empty() &&
            !mesh.material.mapKd.empty()
        )
        {
            texPath = mesh.material.mapKd;
        }

        if (!texPath.empty())
        {
            mesh.textureID =
                loadTexture(texPath);

            mesh.hasTexture =
                mesh.textureID != 0;
        }

        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(1, &mesh.VBO);

        glBindVertexArray(mesh.VAO);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            mesh.VBO
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            grupo.buffer.size() * sizeof(float),
            grupo.buffer.data(),
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            8 * sizeof(float),
            (void *)0
        );

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            8 * sizeof(float),
            (void *)(3 * sizeof(float))
        );

        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            2,
            3,
            GL_FLOAT,
            GL_FALSE,
            8 * sizeof(float),
            (void *)(5 * sizeof(float))
        );

        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        meshes.push_back(mesh);
    }

    cout
        << "OBJ carregado: "
        << objPath
        << " | submeshes: "
        << meshes.size()
        << endl;

    return meshes;
}

// ---------------------------------------------------------
// Shader
// ---------------------------------------------------------

GLuint compilaShader(
    GLenum tipo,
    const char *codigo
)
{
    GLuint shader =
        glCreateShader(tipo);

    glShaderSource(
        shader,
        1,
        &codigo,
        nullptr
    );

    glCompileShader(shader);

    int ok = 0;
    char infoLog[1024];

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &ok
    );

    if (!ok)
    {
        glGetShaderInfoLog(
            shader,
            sizeof(infoLog),
            nullptr,
            infoLog
        );

        cout
            << "Erro compilando shader:\n"
            << infoLog
            << endl;
    }

    return shader;
}

GLuint criaShaderProgram()
{
    GLuint vertex =
        compilaShader(
            GL_VERTEX_SHADER,
            vertexShaderSource
        );

    GLuint fragment =
        compilaShader(
            GL_FRAGMENT_SHADER,
            fragmentShaderSource
        );

    GLuint program =
        glCreateProgram();

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    int ok = 0;
    char infoLog[1024];

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &ok
    );

    if (!ok)
    {
        glGetProgramInfoLog(
            program,
            sizeof(infoLog),
            nullptr,
            infoLog
        );

        cout
            << "Erro linkando shader:\n"
            << infoLog
            << endl;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}

// ---------------------------------------------------------
// Cena
// ---------------------------------------------------------

void configuraLuzesPadrao()
{
    KeyLight.position =
        glm::vec3(2.2f, 2.2f, 2.8f);

    KeyLight.color =
        glm::vec3(1.0f, 0.95f, 0.86f);

    KeyLight.intensity = 1.20f;
    KeyLight.enabled = true;

    FillLight.position =
        glm::vec3(-2.5f, 1.5f, 2.4f);

    FillLight.color =
        glm::vec3(0.70f, 0.82f, 1.0f);

    FillLight.intensity = 0.55f;
    FillLight.enabled = true;

    BackLight.position =
        glm::vec3(0.0f, 2.1f, -2.2f);

    BackLight.color =
        glm::vec3(1.0f);

    BackLight.intensity = 0.75f;
    BackLight.enabled = true;
}

void leLuz(
    istringstream &ss,
    Light &luz
)
{
    ss
        >> luz.position.x
        >> luz.position.y
        >> luz.position.z;

    ss
        >> luz.color.r
        >> luz.color.g
        >> luz.color.b;

    ss >> luz.intensity;

    luz.enabled = true;
}

bool leObjeto(istringstream &ss)
{
    SceneObject obj;

    string objPath;
    string mtlPath;
    string texPath;

    Material materialPadrao;

    // Formato:
    // object nome objPath mtlPath texturePath
    // px py pz rx ry rz sx sy sz
    // Ka(3) Kd(3) Ks(3) shininess

    ss
        >> obj.name
        >> objPath
        >> mtlPath
        >> texPath;

    ss
        >> obj.position.x
        >> obj.position.y
        >> obj.position.z;

    ss
        >> obj.rotation.x
        >> obj.rotation.y
        >> obj.rotation.z;

    ss
        >> obj.scale.x
        >> obj.scale.y
        >> obj.scale.z;

    ss
        >> materialPadrao.ka.r
        >> materialPadrao.ka.g
        >> materialPadrao.ka.b;

    ss
        >> materialPadrao.kd.r
        >> materialPadrao.kd.g
        >> materialPadrao.kd.b;

    ss
        >> materialPadrao.ks.r
        >> materialPadrao.ks.g
        >> materialPadrao.ks.b;

    ss >> materialPadrao.shininess;

    if (ss.fail())
    {
        cout
            << "Linha object invalida no configurador.txt."
            << endl;

        return false;
    }

    obj.meshes = loadOBJ(
        objPath,
        mtlPath,
        materialPadrao,
        texPath
    );

    if (obj.meshes.empty())
    {
        cout
            << "Objeto ignorado porque falhou: "
            << obj.name
            << endl;

        return false;
    }

    // IMPORTANTE: nao limpa o vetor.
    // Assim o programa mantem todos os OBJ carregados.
    Objetos.push_back(obj);

    return true;
}

bool leBezier(
    istringstream &ss,
    string &nomeObjeto,
    BezierAnimation &animacao
)
{
    ss >> nomeObjeto;

    ss
        >> animacao.p0.x
        >> animacao.p0.y
        >> animacao.p0.z;

    ss
        >> animacao.p1.x
        >> animacao.p1.y
        >> animacao.p1.z;

    ss
        >> animacao.p2.x
        >> animacao.p2.y
        >> animacao.p2.z;

    ss
        >> animacao.p3.x
        >> animacao.p3.y
        >> animacao.p3.z;

    ss >> animacao.speed;

    if (ss.fail())
    {
        cout
            << "Linha bezier invalida no configurador.txt."
            << endl;

        return false;
    }

    animacao.enabled = true;
    animacao.t = 0.0f;
    animacao.direction = 1.0f;
    animacao.speed =
        max(animacao.speed, 0.001f);

    return true;
}

void criaCenaPadrao()
{
    cout
        << "Nao consegui usar assets/configurador.txt."
        << " Carregando apenas o Sofa como emergencia."
        << endl;

    Material materialPadrao;

    SceneObject sofa;
    sofa.name = "Sofa";
    sofa.position =
        glm::vec3(0.0f, -0.70f, 0.0f);
    sofa.rotation =
        glm::vec3(0.0f, 180.0f, 0.0f);
    sofa.scale =
        glm::vec3(2.5f);

    sofa.meshes = loadOBJ(
        "assets/Modelos3D/Sofa.obj",
        "assets/Modelos3D/Sofa.mtl",
        materialPadrao,
        "assets/Textures/Sofa_01_diff_1k.jpg"
    );

    if (!sofa.meshes.empty())
        Objetos.push_back(sofa);
}

void carregaCena(string configPath)
{
    Objetos.clear();
    configuraLuzesPadrao();

    map<string, BezierAnimation>
        animacoesPorNome;

    configPath =
        arrumaCaminho(configPath);

    ifstream arq(configPath.c_str());

    if (!arq.is_open())
    {
        criaCenaPadrao();
        selecionaObjeto(0);
        return;
    }

    cout
        << "Lendo configuracao: "
        << configPath
        << endl;

    string line;
    string tipo;

    while (getline(arq, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        istringstream ss(line);
        ss >> tipo;

        if (tipo == "camera")
        {
            ss
                >> Cam.position.x
                >> Cam.position.y
                >> Cam.position.z;

            ss
                >> Cam.yaw
                >> Cam.pitch;

            ss
                >> Cam.fov
                >> Cam.nearPlane
                >> Cam.farPlane;
        }
        else if (tipo == "keyLight")
        {
            leLuz(ss, KeyLight);
        }
        else if (tipo == "fillLight")
        {
            leLuz(ss, FillLight);
        }
        else if (tipo == "backLight")
        {
            leLuz(ss, BackLight);
        }
        else if (tipo == "globalIntensity")
        {
            ss >> GlobalIntensity;

            GlobalIntensity =
                max(GlobalIntensity, 0.0f);
        }
        else if (tipo == "object")
        {
            leObjeto(ss);
        }
        else if (tipo == "bezier")
        {
            string nomeObjeto;
            BezierAnimation animacao;

            if (
                leBezier(
                    ss,
                    nomeObjeto,
                    animacao
                )
            )
            {
                animacoesPorNome[nomeObjeto] =
                    animacao;
            }
        }
        else
        {
            cout
                << "Comando ignorado no configurador.txt: "
                << tipo
                << endl;
        }
    }

    arq.close();

    // Liga cada definicao Bezier ao objeto de mesmo nome.
    for (SceneObject &obj : Objetos)
    {
        auto encontrado =
            animacoesPorNome.find(obj.name);

        if (
            encontrado !=
            animacoesPorNome.end()
        )
        {
            obj.animation =
                encontrado->second;

            // A posicao inicial passa a ser P0.
            obj.position =
                obj.animation.p0;
        }
    }

    if (Objetos.empty())
        criaCenaPadrao();

    selecionaObjeto(0);

    cout
        << "Total de OBJ carregados: "
        << Objetos.size()
        << endl;

    if (Objetos.size() < 2)
    {
        cout
            << "ATENCAO: o enunciado pede mais de um OBJ."
            << " Adicione outra linha 'object' ao configurador.txt."
            << endl;
    }
}

// ---------------------------------------------------------
// Controle da câmera, teclado e mouse
// ---------------------------------------------------------

void framebuffer_size_callback(
    GLFWwindow *,
    int width,
    int height
)
{
    WIDTH = max(width, 1);
    HEIGHT = max(height, 1);

    glViewport(
        0,
        0,
        WIDTH,
        HEIGHT
    );
}

void mouse_callback(
    GLFWwindow *,
    double xpos,
    double ypos
)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset =
        (float)(xpos - lastX);

    float yoffset =
        (float)(lastY - ypos);

    lastX = xpos;
    lastY = ypos;

    const float sensibilidade =
        0.10f;

    Cam.yaw +=
        xoffset * sensibilidade;

    Cam.pitch +=
        yoffset * sensibilidade;

    Cam.pitch =
        max(
            -89.0f,
            min(Cam.pitch, 89.0f)
        );
}

void mostraEstadoLuz(
    const string &nome,
    const Light &luz
)
{
    cout
        << nome
        << ": "
        << (
            luz.enabled
                ? "ligada"
                : "desligada"
        )
        << endl;
}

void key_callback(
    GLFWwindow *window,
    int key,
    int,
    int action,
    int
)
{
    if (action != GLFW_PRESS)
        return;

    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(
            window,
            true
        );
    }
    else if (key == GLFW_KEY_TAB)
    {
        selecionaObjeto(
            ObjetoSelecionado + 1
        );
    }
    else if (key == GLFW_KEY_1)
    {
        KeyLight.enabled =
            !KeyLight.enabled;

        mostraEstadoLuz(
            "Key light",
            KeyLight
        );
    }
    else if (key == GLFW_KEY_2)
    {
        FillLight.enabled =
            !FillLight.enabled;

        mostraEstadoLuz(
            "Fill light",
            FillLight
        );
    }
    else if (key == GLFW_KEY_3)
    {
        BackLight.enabled =
            !BackLight.enabled;

        mostraEstadoLuz(
            "Back light",
            BackLight
        );
    }
    else if (key == GLFW_KEY_T)
    {
        ExibirTextura =
            !ExibirTextura;

        cout
            << "Textura: "
            << (
                ExibirTextura
                    ? "ligada"
                    : "desligada"
            )
            << endl;
    }
    else if (key == GLFW_KEY_P)
    {
        AnimacaoLigada =
            !AnimacaoLigada;

        cout
            << "Animacoes Bezier: "
            << (
                AnimacaoLigada
                    ? "em execucao"
                    : "pausadas"
            )
            << endl;
    }
    else if (key == GLFW_KEY_R)
    {
        for (SceneObject &obj : Objetos)
        {
            if (!obj.animation.enabled)
                continue;

            obj.animation.t = 0.0f;
            obj.animation.direction = 1.0f;
        }

        cout
            << "Trajetorias Bezier reiniciadas."
            << endl;
    }
}

void transladaObjeto(
    SceneObject &obj,
    const glm::vec3 &delta
)
{
    obj.position += delta;

    // Se o objeto tiver Bezier, move a curva inteira.
    if (obj.animation.enabled)
    {
        obj.animation.p0 += delta;
        obj.animation.p1 += delta;
        obj.animation.p2 += delta;
        obj.animation.p3 += delta;
    }
}

void processInput(GLFWwindow *window)
{
    float camSpeed =
        3.0f * deltaTime;

    glm::vec3 front =
        Cam.getFront();

    glm::vec3 right =
        glm::normalize(
            glm::cross(
                front,
                glm::vec3(0.0f, 1.0f, 0.0f)
            )
        );

    if (
        glfwGetKey(
            window,
            GLFW_KEY_W
        ) == GLFW_PRESS
    )
    {
        Cam.position +=
            front * camSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_S
        ) == GLFW_PRESS
    )
    {
        Cam.position -=
            front * camSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_A
        ) == GLFW_PRESS
    )
    {
        Cam.position -=
            right * camSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_D
        ) == GLFW_PRESS
    )
    {
        Cam.position +=
            right * camSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_SPACE
        ) == GLFW_PRESS
    )
    {
        Cam.position.y +=
            camSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_LEFT_SHIFT
        ) == GLFW_PRESS
    )
    {
        Cam.position.y -=
            camSpeed;
    }

    if (Objetos.empty())
        return;

    SceneObject &obj =
        Objetos[ObjetoSelecionado];

    float moveSpeed =
        1.5f * deltaTime;

    float rotSpeed =
        70.0f * deltaTime;

    float scaleSpeed =
        1.0f * deltaTime;

    glm::vec3 delta(0.0f);

    if (
        glfwGetKey(
            window,
            GLFW_KEY_I
        ) == GLFW_PRESS
    )
    {
        delta.z -= moveSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_K
        ) == GLFW_PRESS
    )
    {
        delta.z += moveSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_J
        ) == GLFW_PRESS
    )
    {
        delta.x -= moveSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_L
        ) == GLFW_PRESS
    )
    {
        delta.x += moveSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_UP
        ) == GLFW_PRESS
    )
    {
        delta.y += moveSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_DOWN
        ) == GLFW_PRESS
    )
    {
        delta.y -= moveSpeed;
    }

    if (glm::length(delta) > 0.0f)
        transladaObjeto(obj, delta);

    if (
        glfwGetKey(
            window,
            GLFW_KEY_Q
        ) == GLFW_PRESS
    )
    {
        obj.rotation.y -=
            rotSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_E
        ) == GLFW_PRESS
    )
    {
        obj.rotation.y +=
            rotSpeed;
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_Z
        ) == GLFW_PRESS
    )
    {
        obj.scale -=
            glm::vec3(scaleSpeed);
    }

    if (
        glfwGetKey(
            window,
            GLFW_KEY_X
        ) == GLFW_PRESS
    )
    {
        obj.scale +=
            glm::vec3(scaleSpeed);
    }

    obj.scale.x =
        max(obj.scale.x, 0.05f);

    obj.scale.y =
        max(obj.scale.y, 0.05f);

    obj.scale.z =
        max(obj.scale.z, 0.05f);
}

// ---------------------------------------------------------
// Bezier e desenho
// ---------------------------------------------------------

glm::vec3 calculaBezierCubica(
    float t,
    const glm::vec3 &p0,
    const glm::vec3 &p1,
    const glm::vec3 &p2,
    const glm::vec3 &p3
)
{
    t =
        max(
            0.0f,
            min(t, 1.0f)
        );

    float u = 1.0f - t;
    float uu = u * u;
    float uuu = uu * u;
    float tt = t * t;
    float ttt = tt * t;

    return
        uuu * p0 +
        3.0f * uu * t * p1 +
        3.0f * u * tt * p2 +
        ttt * p3;
}

void atualizaAnimacoesBezier()
{
    if (!AnimacaoLigada)
        return;

    for (SceneObject &obj : Objetos)
    {
        if (!obj.animation.enabled)
            continue;

        obj.animation.t +=
            obj.animation.direction *
            obj.animation.speed *
            deltaTime;

        // Movimento de ida e volta sem salto.
        if (obj.animation.t >= 1.0f)
        {
            obj.animation.t = 1.0f;
            obj.animation.direction = -1.0f;
        }
        else if (obj.animation.t <= 0.0f)
        {
            obj.animation.t = 0.0f;
            obj.animation.direction = 1.0f;
        }
    }
}

glm::mat4 montaModelMatrix(
    SceneObject &obj
)
{
    glm::vec3 pos =
        obj.position;

    if (obj.animation.enabled)
    {
        pos = calculaBezierCubica(
            obj.animation.t,
            obj.animation.p0,
            obj.animation.p1,
            obj.animation.p2,
            obj.animation.p3
        );
    }

    glm::mat4 model(1.0f);

    model =
        glm::translate(
            model,
            pos
        );

    model =
        glm::rotate(
            model,
            glm::radians(obj.rotation.x),
            glm::vec3(1, 0, 0)
        );

    model =
        glm::rotate(
            model,
            glm::radians(obj.rotation.y),
            glm::vec3(0, 1, 0)
        );

    model =
        glm::rotate(
            model,
            glm::radians(obj.rotation.z),
            glm::vec3(0, 0, 1)
        );

    model =
        glm::scale(
            model,
            obj.scale
        );

    return model;
}

void enviaLuzParaShader(
    int indice,
    const Light &luz
)
{
    string base =
        "[" + to_string(indice) + "]";

    glUniform3fv(
        glGetUniformLocation(
            ShaderProgram,
            ("lightPos" + base).c_str()
        ),
        1,
        glm::value_ptr(luz.position)
    );

    glUniform3fv(
        glGetUniformLocation(
            ShaderProgram,
            ("lightColor" + base).c_str()
        ),
        1,
        glm::value_ptr(luz.color)
    );

    glUniform1f(
        glGetUniformLocation(
            ShaderProgram,
            ("lightIntensity" + base).c_str()
        ),
        luz.intensity
    );

    glUniform1i(
        glGetUniformLocation(
            ShaderProgram,
            ("lightEnabled" + base).c_str()
        ),
        luz.enabled ? 1 : 0
    );
}

void desenhaCena()
{
    glUseProgram(ShaderProgram);

    glm::mat4 view =
        glm::lookAt(
            Cam.position,
            Cam.position + Cam.getFront(),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

    glm::mat4 projection =
        glm::perspective(
            glm::radians(Cam.fov),
            (float)WIDTH / (float)HEIGHT,
            Cam.nearPlane,
            Cam.farPlane
        );

    glUniformMatrix4fv(
        glGetUniformLocation(
            ShaderProgram,
            "view"
        ),
        1,
        GL_FALSE,
        glm::value_ptr(view)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(
            ShaderProgram,
            "projection"
        ),
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );

    glUniform3fv(
        glGetUniformLocation(
            ShaderProgram,
            "viewPos"
        ),
        1,
        glm::value_ptr(Cam.position)
    );

    glUniform1f(
        glGetUniformLocation(
            ShaderProgram,
            "globalIntensity"
        ),
        GlobalIntensity
    );

    glUniform1i(
        glGetUniformLocation(
            ShaderProgram,
            "texBuff"
        ),
        0
    );

    enviaLuzParaShader(
        0,
        KeyLight
    );

    enviaLuzParaShader(
        1,
        FillLight
    );

    enviaLuzParaShader(
        2,
        BackLight
    );

    for (SceneObject &obj : Objetos)
    {
        glm::mat4 model =
            montaModelMatrix(obj);

        glUniformMatrix4fv(
            glGetUniformLocation(
                ShaderProgram,
                "model"
            ),
            1,
            GL_FALSE,
            glm::value_ptr(model)
        );

        for (Mesh &mesh : obj.meshes)
        {
            Material &material =
                mesh.material;

            glUniform3fv(
                glGetUniformLocation(
                    ShaderProgram,
                    "materialKa"
                ),
                1,
                glm::value_ptr(material.ka)
            );

            glUniform3fv(
                glGetUniformLocation(
                    ShaderProgram,
                    "materialKd"
                ),
                1,
                glm::value_ptr(material.kd)
            );

            glUniform3fv(
                glGetUniformLocation(
                    ShaderProgram,
                    "materialKs"
                ),
                1,
                glm::value_ptr(material.ks)
            );

            glUniform1f(
                glGetUniformLocation(
                    ShaderProgram,
                    "shininess"
                ),
                material.shininess
            );

            bool usarTextura =
                ExibirTextura &&
                mesh.hasTexture;

            glUniform1i(
                glGetUniformLocation(
                    ShaderProgram,
                    "hasTexture"
                ),
                usarTextura ? 1 : 0
            );

            if (usarTextura)
            {
                glActiveTexture(
                    GL_TEXTURE0
                );

                glBindTexture(
                    GL_TEXTURE_2D,
                    mesh.textureID
                );
            }

            glBindVertexArray(
                mesh.VAO
            );

            glDrawArrays(
                GL_TRIANGLES,
                0,
                mesh.nVertices
            );

            glBindVertexArray(0);
        }
    }
}

// ---------------------------------------------------------
// Inicializacao e encerramento
// ---------------------------------------------------------

bool inicializaOpenGL()
{
    if (!glfwInit())
    {
        cout
            << "Erro ao iniciar GLFW"
            << endl;

        return false;
    }

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    Window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "Visualizador 3D - Multiplos OBJ",
            nullptr,
            nullptr
        );

    if (!Window)
    {
        cout
            << "Erro ao criar janela"
            << endl;

        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(Window);

    glfwSetFramebufferSizeCallback(
        Window,
        framebuffer_size_callback
    );

    glfwSetCursorPosCallback(
        Window,
        mouse_callback
    );

    glfwSetKeyCallback(
        Window,
        key_callback
    );

    glfwSetInputMode(
        Window,
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED
    );

    if (
        !gladLoadGLLoader(
            (GLADloadproc)
                glfwGetProcAddress
        )
    )
    {
        cout
            << "Erro ao iniciar GLAD"
            << endl;

        glfwDestroyWindow(Window);
        glfwTerminate();

        return false;
    }

    glEnable(GL_DEPTH_TEST);

    cout
        << "Placa de video: "
        << glGetString(GL_RENDERER)
        << endl;

    cout
        << "OpenGL: "
        << glGetString(GL_VERSION)
        << endl;

    return true;
}

void liberaRecursos()
{
    for (SceneObject &obj : Objetos)
    {
        for (Mesh &mesh : obj.meshes)
        {
            if (mesh.textureID != 0)
            {
                glDeleteTextures(
                    1,
                    &mesh.textureID
                );
            }

            if (mesh.VBO != 0)
            {
                glDeleteBuffers(
                    1,
                    &mesh.VBO
                );
            }

            if (mesh.VAO != 0)
            {
                glDeleteVertexArrays(
                    1,
                    &mesh.VAO
                );
            }
        }
    }

    if (ShaderProgram != 0)
        glDeleteProgram(ShaderProgram);
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------

int main()
{
    if (!inicializaOpenGL())
        return -1;

    ShaderProgram =
        criaShaderProgram();

    carregaCena(
        "assets/configurador.txt"
    );

    if (Objetos.empty())
    {
        cout
            << "Nenhum objeto foi carregado."
            << endl;

        liberaRecursos();
        glfwDestroyWindow(Window);
        glfwTerminate();

        return -1;
    }

    cout << "\nControles:" << endl;
    cout << "W A S D = mover camera" << endl;
    cout << "SPACE / SHIFT = subir/descer camera" << endl;
    cout << "Mouse = olhar ao redor" << endl;
    cout << "TAB = selecionar proximo objeto" << endl;
    cout << "I J K L / setas = transladar selecionado" << endl;
    cout << "Q / E = rotacionar selecionado" << endl;
    cout << "Z / X = escala uniforme" << endl;
    cout << "T = ligar/desligar texturas" << endl;
    cout << "1 / 2 / 3 = ligar/desligar luzes" << endl;
    cout << "P = iniciar/pausar Bezier" << endl;
    cout << "R = reiniciar trajetorias Bezier" << endl;
    cout << "ESC = sair\n" << endl;

    while (
        !glfwWindowShouldClose(Window)
    )
    {
        float currentFrame =
            (float)glfwGetTime();

        deltaTime =
            currentFrame - lastFrame;

        lastFrame =
            currentFrame;

        processInput(Window);
        atualizaAnimacoesBezier();

        glClearColor(
            0.08f,
            0.08f,
            0.10f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        desenhaCena();

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    liberaRecursos();

    glfwDestroyWindow(Window);
    glfwTerminate();

    return 0;
}
