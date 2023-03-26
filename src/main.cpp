#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "rg/Error.h"
#include "rg/Texture2D.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <iostream>
#include <vector>

#include <iostream>

#define NUM_OF_FLOWERS 10

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(char const * path, bool gammaCorrection);

void drawChurch(Shader ourShader, Model churchModel);
void drawPlane(Shader ourShader, unsigned int planeVAO, unsigned int floorTexture);
void drawSun(Shader ourShader, Model sunModel);
void drawAngel(Shader ourShader, Model angelModel);
void drawLamp(Shader ourShader, Model lampModel);

unsigned int loadCubemap(vector<std::string> &faces);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool spotlightOn = true;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct directionalLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 churchPosition = glm::vec3(0.0f, -0.1f, 0.0f);
    glm::vec3 planePosition = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 sunPosition = glm::vec3 (0.0f, 13.0f, 13.0f);
    glm::vec3 lampPosition = glm::vec3(2.0f, 0.05f, 0.8f);
    glm::vec3 angelPosition = glm::vec3(-20.0f, 0.05f, 0.8f);
    float churchScale = 0.1f;
    float planeScale = 10.0;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(3.0f, 0.0f, 0.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    float planeVertices[] = {
            // positions                     // normals                       // texcoords
            1.0f, -0.5f,  1.0f,  0.0f, 1.0f, 0.0f,      1.0f,  0.0f,
            -1.0f, -0.5f,  1.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -1.0f, -0.5f, -1.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

            1.0f, -0.5f,  1.0f,  0.0f, 1.0f, 0.0f,  1.0f,  0.0f,
            -1.0f, -0.5f, -1.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
            1.0f, -0.5f, -1.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f
    };

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };


    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader sunShader("resources/shaders/sun.vs", "resources/shaders/sun.fs");
    Shader skyboxShader("resources/shaders/skyBox.vs","resources/shaders/skyBox.fs");
    Shader flowerShader("resources/shaders/flowers.vs", "resources/shaders/flowers.fs");

    // load models
    // -----------
    Model churchModel("resources/objects/Obj/Parish Church Model+.obj");
    churchModel.SetShaderTextureNamePrefix("material.");
    Model sunModel("resources/objects/sun/sphere.OBJ");
    Model angelModel("resources/objects/engel/source/Engel_C/Engel_C.obj");
    angelModel.SetShaderTextureNamePrefix("material.");
    Model lampModel("resources/objects/street_lamp/STLamp.obj");
    lampModel.SetShaderTextureNamePrefix("material.");

    unsigned int planeTexture = loadTexture(FileSystem::getPath("resources/textures/grass.jpg").c_str(), true);

    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    unsigned int flowerTex1 = loadTexture(FileSystem::getPath("resources/textures/vegetation_textures/flowers1.png").c_str(), true);
    unsigned int flowerTex2 = loadTexture(FileSystem::getPath("resources/textures/vegetation_textures/flowers2.png").c_str(), true);
    unsigned int flowerTex3 = loadTexture(FileSystem::getPath("resources/textures/vegetation_textures/flowers3.png").c_str(), true);
    unsigned int flowerTex4 = loadTexture(FileSystem::getPath("resources/textures/vegetation_textures/flowers4.png").c_str(), true);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox_textures/right.png"),
                    FileSystem::getPath("resources/textures/skybox_textures/left.png"),
                    FileSystem::getPath("resources/textures/skybox_textures/top.png"),
                    FileSystem::getPath("resources/textures/skybox_textures/bottom.png"),
                    FileSystem::getPath("resources/textures/skybox_textures/front.png"),
                    FileSystem::getPath("resources/textures/skybox_textures/back.png")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    PointLight& directionalLight = programState->pointLight;
    directionalLight.position = programState->sunPosition;
    directionalLight.ambient = glm::vec3(0.3f, 0.3f, 0.3f);
    directionalLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    directionalLight.specular = glm::vec3(0.4, 0.4, 0.4);

    vector<glm::vec3> flowers
            {
                    glm::vec3(-2.5f, 0.5f, -0.48f),
                    glm::vec3( 1.5f, 0.5f, 1.51f),
                    glm::vec3( 1.0f, 0.5f, 0.7f),
                    glm::vec3(-0.34f, 0.5f, -1.3f),
                    glm::vec3 (1.55f, 0.5f, -0.6f),
                    glm::vec3(-1.5f, 0.5f, -1.48f),
                    glm::vec3( -1.5f, 0.5f, -1.51f),
                    glm::vec3( -2.3f, 0.5f, -2.7f),
                    glm::vec3(-0.34f, 0.5f, -2.3f),
                    glm::vec3 (1.55f, 0.5f, -3.6f)
            };

    glm::vec3 pointLightPositions[] = {
            glm::vec3(2.0f, 2.0f, 0.8f),
            glm::vec3(-15.0f, 10.0f, 2.0f),
    };

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        // draw objects on scene
        ourShader.use();

        //directional light (from moon)
        directionalLight.position = programState->sunPosition;
        ourShader.setVec3("directionalLight.direction", directionalLight.position);
        ourShader.setVec3("directionalLight.ambient", directionalLight.ambient);
        ourShader.setVec3("directionalLight.diffuse", directionalLight.diffuse);
        ourShader.setVec3("directionalLight.specular", directionalLight.specular);

        //point lights
        ourShader.setVec3("pointLights[0].position", pointLightPositions[0]);
        ourShader.setVec3("pointLights[0].ambient", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("pointLights[0].diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("pointLights[0].constant", 1.0f);
        ourShader.setFloat("pointLights[0].linear", 0.02f);
        ourShader.setFloat("pointLights[0].quadratic", 0.1f);

        ourShader.setVec3("pointLights[1].position", pointLightPositions[1]);
        ourShader.setVec3("pointLights[1].ambient", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("pointLights[1].diffuse", 0.7f, 0.7f, 1.1f);
        ourShader.setVec3("pointLights[1].specular", 0.3f, 0.3f, 0.3f);
        ourShader.setFloat("pointLights[1].constant", 1.0f);
        ourShader.setFloat("pointLights[1].linear", 0.07f);
        ourShader.setFloat("pointLights[1].quadratic", 0.17f);

        //spotlight
        ourShader.setVec3("spotlight.position", programState->camera.Position);
        ourShader.setVec3("spotlight.direction", programState->camera.Front);
        ourShader.setVec3("spotlight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotlight.diffuse", 0.5f, 0.5f, 0.8f);
        ourShader.setVec3("spotlight.specular", 0.3f, 0.5f, 0.9f);
        ourShader.setFloat("spotlight.constant", 1.0f);
        ourShader.setFloat("spotlight.linear", 0.014f);
        ourShader.setFloat("spotlight.quadratic", 0.0007f);
        ourShader.setFloat("spotlight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotlight.outerCutOff", glm::cos(glm::radians(17.5f)));
        ourShader.setBool("spotlightOn", spotlightOn);
        ourShader.setFloat("material.shininess", 128.0f);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded models

        //church
        drawChurch(ourShader, churchModel);
        //angel
        drawAngel(ourShader, angelModel);
        //lamp
        drawLamp(ourShader, lampModel);

        //plane
        drawPlane(ourShader, planeVAO, planeTexture);

        // cubes
        flowerShader.use();
        flowerShader.setInt("texture1", 0);
        flowerShader.setMat4("view", view);
        flowerShader.setMat4("projection", projection);

        // flowers
        int x = 0;
        int z = 0;
        glBindVertexArray(transparentVAO);
        for (unsigned int i = 0; i < flowers.size(); i++){
            {
                if (i % 5 == 0){
                    glBindTexture(GL_TEXTURE_2D, flowerTex1);
                } else if (i % 4 == 1){
                    glBindTexture(GL_TEXTURE_2D, flowerTex2);
                } else if (i % 4 == 3){
                    glBindTexture(GL_TEXTURE_2D, flowerTex3);
                } else{
                    glBindTexture(GL_TEXTURE_2D, flowerTex4);
                }
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));
                model = glm::translate(model, flowers[i]);
                ourShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
        sunShader.use();
        sunShader.setMat4("projection", projection);
        sunShader.setMat4("view", view);

        //sun
        drawSun(sunShader, sunModel);

        //skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime, true);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime, true);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime, true);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->churchPosition);
        ImGui::DragFloat("Backpack scale", &programState->churchScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        spotlightOn = !spotlightOn;
    }
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void drawChurch(Shader ourShader, Model churchModel)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model,
                           programState->churchPosition); // translate it down so it's at the center of the scene
    model = glm::scale(model,
                       glm::vec3(programState->churchScale));    // it's a bit too big for our scene, so scale it down
    model = glm::rotate(model, 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ourShader.setMat4("model", model);
    churchModel.Draw(ourShader);
}

void drawPlane(Shader ourShader, unsigned int planeVAO, unsigned int planeTexture)
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, programState->planePosition);
    model = glm::scale(model, glm::vec3(programState->planeScale));
    ourShader.setMat4("model", model);
    glBindVertexArray(planeVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, planeTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void drawSun(Shader ourShader, Model sunModel){
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, programState->sunPosition);
    ourShader.setMat4("model", model);
    sunModel.Draw(ourShader);
}

void drawAngel(Shader ourShader, Model angelModel){
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
//    model = glm::rotate(model, -100.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, programState->angelPosition);
    ourShader.setMat4("model", model);
    angelModel.Draw(ourShader);
}

void drawLamp(Shader ourShader, Model lampModel){
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.3f, 0.3f, 0.3f));
    model = glm::translate(model, programState->lampPosition);
    ourShader.setMat4("model", model);
    lampModel.Draw(ourShader);
}

unsigned int loadCubemap(vector<std::string> &faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

