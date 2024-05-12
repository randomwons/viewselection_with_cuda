#include "graphic/window.h"
#include <algorithm>

#include <chrono>
#include <string>

#include "loader/depth.h"
#include "loader/intrinsic.h"
#include "loader/pose.h"

void updateOctree(octomap::OcTree* octree, const Depth& depth, const Intrinsic& intrinsic, const Pose& pose) {

    octomap::point3d origin(
        -(pose.data[0] * pose.data[3] + pose.data[4] * pose.data[7] + pose.data[8] * pose.data[11]),
        -(pose.data[1] * pose.data[3] + pose.data[5] * pose.data[7] + pose.data[9] * pose.data[11]),
        -(pose.data[2] * pose.data[3] + pose.data[6] * pose.data[7] + pose.data[10] * pose.data[11])
    );

    octomap::Pointcloud pc;
    for(int y = 0; y < depth.height(); y++){
        for(int x = 0; x < depth.width(); x++) {
            double d = depth.data[y * depth.width() + x];
            if(d == 0) continue;
            
            double dx = (x - intrinsic.cx()) / intrinsic.fx();
            double dy = (y - intrinsic.cy()) / intrinsic.fy();
            double dz = 1;

            octomap::point3d dir = octomap::point3d(
                pose.data[0] * dx + pose.data[4] * dy + pose.data[8] * dz,
                pose.data[1] * dx + pose.data[5] * dy + pose.data[9] * dz,
                pose.data[2] * dx + pose.data[6] * dy + pose.data[10] * dz
            );
            dir.normalized();
            
            pc.push_back(origin + dir * d);
        }
    }
    octree->insertPointCloud(pc, origin);
}

void getOcTreeVertices(const octomap::OcTree* octree, std::vector<GLfloat>& vertices) {

    for (octomap::OcTree::leaf_iterator it = octree->begin_leafs(), end = octree->end_leafs(); it != end; ++it) {
        float size = (float)it.getSize();
        float x = (float)it.getX();
        float y = (float)it.getY();
        float z = (float)it.getZ();

        // 각 voxel의 vertices 계산
        GLfloat cubeVertices[] = {
            x - size / 2, y - size / 2, z + size / 2,  // Top-left
            x + size / 2, y - size / 2, z + size / 2,  // Top-right
            x + size / 2, y - size / 2, z - size / 2,  // Bottom-right
            x - size / 2, y - size / 2, z - size / 2,  // Bottom-left
            x - size / 2, y + size / 2, z + size / 2,  // Top-left
            x + size / 2, y + size / 2, z + size / 2,  // Top-right
            x + size / 2, y + size / 2, z - size / 2,  // Bottom-right
            x - size / 2, y + size / 2, z - size / 2   // Bottom-left
        };

        vertices.insert(vertices.end(), std::begin(cubeVertices), std::end(cubeVertices));
    }

}

void Window::frameBufferSizeCallback(GLFWwindow* window, int width, int height){
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if(win) win->reshape(width, height);
}

void Window::keyCallbackWindow(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if(win) win->keyCallback(key, scancode, action, mods);
}


void Window::reshape(int width, int height){
    m_width = width;
    m_height = height;
    float aspectRatio = width / float(height);

    m_projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    glViewport(0, 0, width, height);
}

std::unique_ptr<Window> Window::create(const int width, const int height, const char* title) {

    auto window = std::unique_ptr<Window>(new Window());
    if(!window->initialize(width, height, title)){
        return nullptr;
    }
    return std::move(window);

}

void Window::keyCallback(int key, int scancode, int action, int mods) {

    if(key == GLFW_KEY_U && action == GLFW_PRESS) {

        std::string datapath = "/home/vava/Dataset/armadillo";
        Depth depth(datapath + "/depth/" + std::to_string(0) + ".npy");
        Intrinsic intrinsic(datapath + "/intrinsic/" + std::to_string(0) + ".txt");
        Pose pose(datapath + "/pose/" + std::to_string(0) + ".txt");

        updateOctree(m_tree.get(), depth, intrinsic, pose);
        getOcTreeVertices(m_tree.get(), vertices);
        
        m_program->use();
        m_program->setAttribute("projection", m_projection);
        glm::mat4 model(1.0f);
        m_program->setAttribute("model", model);
        m_program->setAttribute("view", m_view);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

}


void Window::processInput() {


}

bool Window::initialize(const int width, const int height, const char* title) {

    if(!glfwInit()){
        printf("Failed to initialize glfw\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if(!m_window) {
        printf("Failed to create window\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_window);

    if(!gladLoadGLES2((GLADloadfunc)glfwGetProcAddress)){
        printf("Failed to initialize glad\n");
        glfwTerminate();
        return false;
    }
    auto glVersion = glGetString(GL_VERSION);
    printf("OpenGL context version : '%s'\n", reinterpret_cast<const char*>(glVersion));

    glfwSetWindowUserPointer(m_window, this);
    frameBufferSizeCallback(m_window, width, height);
    glfwSetFramebufferSizeCallback(m_window, frameBufferSizeCallback);
    glfwSetKeyCallback(m_window, keyCallbackWindow);

    if(!loadShaderProgram()){
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_ALPHA_TEST);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // glDepthFunc(GL_LEQUAL);

    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glfwSwapInterval(0);


    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLint posAttrib = glGetAttribLocation(m_program->get(), "aPos");
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(posAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, 0);



    m_tree = std::make_unique<octomap::OcTree>(0.01);


    glm::vec3 cameraPos = glm::vec3(3.0f, 5.0f, 15.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    m_view = glm::lookAt(cameraPos, cameraTarget, upVector);

    




    return true;

}

void Window::render() {
    glfwPollEvents();
    glClearColor(0.1, 0.2, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glfwSwapBuffers(m_window);
}

bool Window::loadShaderProgram() {

    std::shared_ptr<Shader> vertShader = Shader::createFromFile("shader/simple.vs", GL_VERTEX_SHADER);
    std::shared_ptr<Shader> fragShader = Shader::createFromFile("shader/simple.fs", GL_FRAGMENT_SHADER);
    m_program = Program::create({vertShader, fragShader});
    if(!m_program) return false;
    return true;
}

