#include "context.h"
#include "image.h"
#include <imgui.h>
ContextUPtr Context::Create(){
    auto context = ContextUPtr(new Context());
    if (!context->Init())
        return nullptr;
    return std::move(context);
}

void Context::ProcessInput(GLFWwindow* window) {
    if (!m_cameraControl)
        return;
    const float cameraSpeed = 0.01f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * m_cameraFront;

    auto cameraRight = glm::normalize(glm::cross(m_cameraUp, -m_cameraFront));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * cameraRight;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * cameraRight;

    auto cameraUp = glm::normalize(glm::cross(-m_cameraFront, cameraRight));
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * cameraUp;
}

void Context::Reshape(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height);
    
    m_framebuffer = Framebuffer::Create(Texture::Create(width, height, GL_RGBA)); 
}

void Context::MouseMove(double x, double y) {   
    if (!m_cameraControl)
        return;
    auto pos = glm::vec2((float)x, (float)y);
    auto deltaPos = pos - m_prevMousePos;

    const float cameraRotSpeed = 0.3f;
    m_cameraYaw -= deltaPos.x * cameraRotSpeed;
    m_cameraPitch -= deltaPos.y * cameraRotSpeed;

    if (m_cameraYaw < 0.0f)      m_cameraYaw += 360.0f;     
    if (m_cameraYaw > 360.0f)      m_cameraYaw -= 360.0f;

    if (m_cameraPitch > 89.0f)    m_cameraPitch = 89.0f;  
    if (m_cameraPitch < -89.0f)    m_cameraPitch = -89.0f;
    

    m_prevMousePos = pos;
}

void Context::MouseButton(int button, int action, double x, double y) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS){
            m_prevMousePos = glm::vec2((float)x, (float)y);
            m_cameraControl = true;
        }
    else if (action == GLFW_RELEASE) {
        m_cameraControl = false;
    }
  }
}

bool Context::Init(){
    glEnable(GL_MULTISAMPLE);
    m_box=Mesh::CreateBox();
    m_model = Model::Load("./model/helmet.obj");
    if (!m_model)
        return false;

    m_simpleProgram = Program::Create("./shader/simple.vs", "./shader/simple.fs");
    if (!m_simpleProgram)
        return false;

    m_program = Program::Create("./shader/lighting.vs", "./shader/lighting.fs");
    if (!m_program)
        return false;

    m_textureProgram = Program::Create("./shader/texture.vs", "./shader/texture.fs");
    if (!m_textureProgram)
        return false;

    m_postProgram = Program::Create("./shader/texture.vs", "./shader/gamma.fs");                                  
    if (!m_postProgram)
        return false;

    glClearColor(0.5f,1.0f,0.8f,0.5f);

    TexturePtr darkGrayTexture = Texture::CreateFromImage(Image::CreateSingleColorImage(4, 4,
                                                                                        glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)) .get());
    TexturePtr grayTexture = Texture::CreateFromImage(Image::CreateSingleColorImage(4, 4,
                                                                                        glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)).get());                                

    auto cubeRight = Image::Load("./image/skybox/right.jpg", false);
    auto cubeLeft = Image::Load("./image/skybox/left.jpg", false);
    auto cubeTop = Image::Load("./image/skybox/top.jpg", false);
    auto cubeBottom = Image::Load("./image/skybox/bottom.jpg", false);
    auto cubeFront = Image::Load("./image/skybox/front.jpg", false);
    auto cubeBack = Image::Load("./image/skybox/back.jpg", false);
    m_cubeTexture = CubeTexture::CreateFromImages({
        cubeRight.get(),
        cubeLeft.get(),
        cubeTop.get(),
        cubeBottom.get(),
        cubeFront.get(),
        cubeBack.get(),
    });
    m_skyboxProgram = Program::Create("./shader/skybox.vs", "./shader/skybox.fs");
    m_envMapProgram = Program::Create("./shader/env_map.vs", "./shader/env_map.fs");
    m_envModelProgram = Program::Create("./shader/lighting.vs", "./shader/lighting_env_map.fs");

    return true;
}

void Context::Render(){
    if (ImGui::Begin("UI_WINDOW")){
        if(ImGui::ColorEdit4("clear color",glm::value_ptr(m_clearColor)))
            glClearColor(m_clearColor.x,m_clearColor.y,m_clearColor.z,m_clearColor.w);  
        ImGui::DragFloat("gamma", &m_gamma, 0.01f, 0.0f, 2.0f);
        ImGui::Separator();
        ImGui::DragFloat3("camera pos",glm::value_ptr(m_cameraPos),0.01f);
        ImGui::DragFloat("camera yaw",&m_cameraYaw,0.5f);
        ImGui::DragFloat("camera pitch",&m_cameraPitch, 0.5f, -89.0f, 89.0f);
        ImGui::Separator();
        if(ImGui::Button("reset camera")){
                m_cameraYaw = 0.0f;
                m_cameraPitch = 0.0f;
                m_cameraPos = glm::vec3(0.0f,0.0f,3.0f);
        }

        if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("l.position", glm::value_ptr(m_light.position), 0.01f);
            ImGui::DragFloat3("l.direction", glm::value_ptr(m_light.direction), 0.01f);
            ImGui::DragFloat("1.distance",&m_light.distance, 0.5f, 0.0f, 3000.0f);
            ImGui::DragFloat2("1.cutoff", glm::value_ptr(m_light.cutoff), 0.5f, 0.0f, 180.0f);
            ImGui::ColorEdit3("l.ambient", glm::value_ptr(m_light.ambient));
            ImGui::ColorEdit3("l.diffuse", glm::value_ptr(m_light.diffuse));
            ImGui::ColorEdit3("l.specular", glm::value_ptr(m_light.specular));
            ImGui::Checkbox("flash light",&m_flashLightMode);
        }


        const char *items[] = {"lighting", "env", "env + lighting"};
        static const char *current_item = items[0];
        if (ImGui::BeginCombo("select combo", current_item)){
            for (int n = 0; n < IM_ARRAYSIZE(items); n++){
                bool is_selected = (current_item == items[n]);
                if (ImGui::Selectable(items[n], is_selected)){
                    current_item = items[n];      
                    m_textureType= n;     
                }
            }
            ImGui::EndCombo();
        }
        if (current_item == items[2])
            ImGui::DragFloat("env_scale", &m_env_scale, 0.1f, 1.0f, 50.0f);
    }
    ImGui::End();

    //framebuffer->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_cameraFront =
        glm::rotate(glm::mat4(1.0f),glm::radians(m_cameraYaw),glm::vec3(0.0f, 1.0f, 0.0f)) *              
        glm::rotate(glm::mat4(1.0f), glm::radians(m_cameraPitch),glm::vec3(1.0f, 0.0f, 0.0f)) *              
        glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

    // m_light.position=m_cameraPos;
    // m_light.direction=m_cameraFront;

    auto projection = glm::perspective(glm::radians(45.0f), (float)m_width / (float)m_height, 0.1f, 100.0f);                                  
    auto view = glm::lookAt( m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);

    auto skyboxModelTransform =
        glm::translate(glm::mat4(1.0), m_cameraPos) *
        glm::scale(glm::mat4(1.0), glm::vec3(50.0f));
    m_skyboxProgram->Use();
    m_cubeTexture->Bind();
    m_skyboxProgram->SetUniform("skybox", 0);
    m_skyboxProgram->SetUniform("transform", projection * view * skyboxModelTransform);
    m_box->Draw(m_skyboxProgram.get());

    glm::vec3 lightPos = m_light.position;
    glm::vec3 lightDir = m_light.direction; 
    if(m_flashLightMode){
        lightPos = m_cameraPos;
        lightDir = m_cameraFront;
    }
    else{
        auto lightModelTransform = glm::translate(glm::mat4(1.0), m_light.position) *glm::scale(glm::mat4(1.0), glm::vec3(0.1f));
        m_simpleProgram->Use();
        m_simpleProgram->SetUniform("color", glm::vec4(m_light.ambient + m_light.diffuse, 1.0f));
        m_simpleProgram->SetUniform("transform", projection * view * lightModelTransform);
        m_box->Draw(m_simpleProgram.get());
    }

    auto modelTransform = glm::mat4(1.0f);
    auto transform = projection * view * modelTransform;
    switch(m_textureType){
    default:
        m_program->Use();
        m_program->SetUniform("viewPos", m_cameraPos);
        m_program->SetUniform("light.position", lightPos);
        m_program->SetUniform("light.direction", lightDir);
        m_program->SetUniform("light.cutoff", glm::vec2(cosf(glm::radians(m_light.cutoff[0])),
                                                        cosf(glm::radians(m_light.cutoff[0] + m_light.cutoff[1]))));
        m_program->SetUniform("light.attenuation", GetAttenuationCoeff(m_light.distance));
        m_program->SetUniform("light.ambient", m_light.ambient);
        m_program->SetUniform("light.diffuse", m_light.diffuse);
        m_program->SetUniform("light.specular", m_light.specular);
          
        m_program->SetUniform("transform", transform);
        m_program->SetUniform("modelTransform", modelTransform);
        m_model->Draw(m_program.get());
        break;
    case 1:
        m_envMapProgram->Use();
        m_envMapProgram->SetUniform("model", modelTransform);
        m_envMapProgram->SetUniform("view", view);
        m_envMapProgram->SetUniform("projection", projection);
        m_envMapProgram->SetUniform("cameraPos", m_cameraPos);
       //m_cubeTexture->Bind();
        m_envMapProgram->SetUniform("skybox", 0);
        m_model->Draw(m_envMapProgram.get());
        break;
    case 2:
        m_envModelProgram->Use();
        m_envModelProgram->SetUniform("viewPos", m_cameraPos);
        m_envModelProgram->SetUniform("light.position", lightPos);
        m_envModelProgram->SetUniform("light.direction", lightDir);
        m_envModelProgram->SetUniform("light.cutoff", glm::vec2(cosf(glm::radians(m_light.cutoff[0])),
                                                                cosf(glm::radians(m_light.cutoff[0] + m_light.cutoff[1]))));
        m_envModelProgram->SetUniform("light.attenuation", GetAttenuationCoeff(m_light.distance));
        m_envModelProgram->SetUniform("light.ambient", m_light.ambient);
        m_envModelProgram->SetUniform("light.diffuse", m_light.diffuse);
        m_envModelProgram->SetUniform("light.specular", m_light.specular);

        m_envModelProgram->SetUniform("transform", transform);
        m_envModelProgram->SetUniform("modelTransform", modelTransform);
        m_envModelProgram->SetUniform("env_scale", m_env_scale);
        m_envModelProgram->SetUniform("skybox", 0);
        m_model->Draw(m_envModelProgram.get());
        break;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}