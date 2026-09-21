#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace std;
using namespace glm;

int width = 1000;
int height = 1000;

unsigned int bottleIndexCount = 0;
unsigned int liquidIndexCount = 0;

unsigned int shaderProgram, colourShaderProgram, pointShaderProgram, VAO_bottle, VBO_bottle, EBO_bottle, VAO_liquid, VBO_liquid, EBO_liquid, VAO_bubbles, VBO_bubbles;

struct Bubble{
  float x;
  float y;
  float z;
  float speed;
  float size;
  float phase;
};

vector<Bubble> bubbles;

float random_number(float min, float max){
return min + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(max-min)));
}

// Add this function to create a point shader for bubbles
unsigned int createPointShader(){
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 pos;
        layout (location = 1) in float size;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main(){
            gl_Position = projection * view * model * vec4(pos, 1.0);
            gl_PointSize = 200 * size / gl_Position.w; // Scale factor for visibility
        } 
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            // Create a circular point
            vec2 center = gl_PointCoord - vec2(0.5);
            float dist = length(center);
            if(dist > 0.5){
             discard;
            }

            // Soft edge
            float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
            float red = 1;
            float green = 0.5;
            float blue = 0.0; 
            FragColor = vec4(red, green, blue, alpha * 1);
        }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

unsigned int createColourShader(){
  const char* vertexShaderSource = R"(
      #version 330 core
      layout (location = 0) in vec3 pos;
      layout (location = 1) in vec3 normal;

      out vec3 Normal;
      out vec3 FragPos;

      uniform mat4 model;
      uniform mat4 view;
      uniform mat4 projection;

      void main(){
       vec4 worldPos = model * vec4(pos, 1.0);
       FragPos = vec3(worldPos);
       Normal = mat3(transpose(inverse(model))) * normal;
       gl_Position = projection * view * worldPos;
      }
)";

  const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 Normal;
        in vec3 FragPos;

        out vec4 FragColor;

        uniform vec3 lightDir;   // direction the light points, e.g. (-0.5, -1.0, -0.3)
        uniform vec4 liquidColor;

        void main() {
         vec3 N = normalize(Normal);
         vec3 L = normalize(-lightDir);
         float diff = max(dot(N, L), 0.0);
         vec3 baseColor = liquidColor.rgb;
         vec3 lit = baseColor * (0.3 + 0.7 * diff);
         FragColor = vec4(lit, liquidColor.a);
}
    )";

  // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    // Link program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

unsigned int shaders(){
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main(){
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D ourTexture;

     void main(){
        vec4 texColor = texture(ourTexture, TexCoord);
        
        // Use the texture's alpha channel (transparent background = invisible)
        float alpha = texColor.a;  // 0 = transparent, 1 = opaque
        
        // If the texture has no alpha, use 1.0
        if(alpha < 0.01){
            discard;  // Don't draw transparent pixels
        }
        
        FragColor = vec4(texColor.rgb, alpha);
    }
)";

     // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    // Link program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}

unsigned int loadTexture(const char* path){
    unsigned int textureID = 0;
    int width, height, channels;
    
    // Load the image
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    
    if(!data){
        cout << "Failed to load texture: " << path;
        return 0;
    }
    
    // Determine the format
    GLenum format;
    if(channels == 1){
      format = GL_RED;
    }
    
    else if(channels == 3){
      format = GL_RGB;
    }
    
    else if(channels == 4){
      	format = GL_RGBA;
      }
      
    // Generate and bind the texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Upload to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Free CPU memory
    stbi_image_free(data);
    
    return textureID;
}

void initBottle(){
// --- VERTEX DATA ---
  float x = -1.81f;
  float y = -0.8f;
  float w = 3.6f;
  float h = 1.6f;
  float z = 0.0f;
  float u = 0.0f;
  float du = 1.0f;
  float v = 0.0f;
  float dv = 1.0f;
  
  float bottle_vertices[] = {// x,y,z,u,v
    // positions          // texture coordinates
    x, y, z, u, v,   // bottom-left
    x + w, y, z,  u + du, v,   // bottom-right
    x + w,  y + h, z, u + du, v + dv,   // top-right
    x,  y + h, z, u, v + dv    // top-left
};

unsigned int bottle_indices[] = {
    0, 1, 2,
    0, 2, 3
};

  glGenVertexArrays(1, &VAO_bottle);
    glGenBuffers(1, &VBO_bottle);
    glGenBuffers(1, &EBO_bottle);

    glBindVertexArray(VAO_bottle);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_bottle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bottle_vertices), bottle_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_bottle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bottle_indices), bottle_indices, GL_STATIC_DRAW);

    // location 0: position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // location 1: texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    bottleIndexCount = 6;
}

void getLiquidGeometry(vector<float>& vertices, vector<unsigned int>& indices, float time){
  vertices.clear();
  indices.clear();

  int sectors = 32;
  int stacks = 15;
  float bottomY = -0.5f;
  float topY = 0.195f;
  float radius = 0.2f;
  float topTaperStart = 0.89f;
  float waveAmplitude = 0.01f;
  float waveSpeed = 4.0f;

  // bottom-center vertex: position + normal
  vertices.push_back(0.0f);
  vertices.push_back(bottomY);
  vertices.push_back(0.0f);
  vertices.push_back(0.0f);
  vertices.push_back(-1.0f);
  vertices.push_back(0.0f);			
 
  int surfaceCenterIdx = (stacks + 1) * (sectors + 1) + 1;

  for(int i = 0; i <= stacks; i++){
    float factor = (float)i/stacks;
    float y = bottomY + factor * (topY - bottomY);

    float r = radius;

    if(factor > topTaperStart){
      r = radius * (1.0f - (factor - topTaperStart) * 1.5f);
    }

    if(r < 0.05){
      r = 0.05;
    }
    
    for(int j = 0; j <= sectors; j++){
      float angle = (float)j/sectors * 2.0f * M_PI;
      float currY = y;

      if(i == stacks){
	currY += waveAmplitude * sin(time * waveSpeed + 2.0f * angle);
      }

      float x = r * cos(angle);
      float z = r * sin(angle);
      vertices.push_back(x);
      vertices.push_back(currY);
      vertices.push_back(z);

      float nx = cos(angle);
      float ny = 0.0f;
      float nz = sin(angle);
      vertices.push_back(nx);
      vertices.push_back(ny);
      vertices.push_back(nz);
      }
    }

  for(int i = 0; i < stacks; i++){
    for(int j = 0; j < sectors; j++){
      unsigned int first = 1 + (i * (sectors + 1)) + j;
      unsigned int second = first + sectors + 1;
      indices.push_back(first);
      indices.push_back(second);
      indices.push_back(first + 1);
      indices.push_back(second);
      indices.push_back(second + 1);
      indices.push_back(first + 1);
  }
  }

  // top-center vertex: position + normal
  vertices.push_back(0.0f);
  vertices.push_back(topY + waveAmplitude * sin(time * waveSpeed));
  vertices.push_back(0.0f);
  vertices.push_back(0.0f);
  vertices.push_back(1.0f);
  vertices.push_back(0.0f);
 
  unsigned int lastStackOffset = 1 + stacks * (sectors + 1);
  for(int j = 0; j < sectors; j++){
    indices.push_back(surfaceCenterIdx);
    indices.push_back(lastStackOffset + j + 1);
    indices.push_back(lastStackOffset + j);
  }
  
  liquidIndexCount = indices.size();
}

void initLiquid(){
  vector<float> vertices;
  vector<unsigned int> indices;
  getLiquidGeometry(vertices, indices, 0.0f);

  glGenVertexArrays(1, &VAO_liquid);
  glGenBuffers(1, &VBO_liquid);
  glGenBuffers(1, &EBO_liquid);
  
  glBindVertexArray(VAO_liquid);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO_liquid);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) * 2, NULL, GL_DYNAMIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
 
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_liquid);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
  
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void updateLiquid(){
  vector<float> vertices;
  vector<unsigned int> indices;
  getLiquidGeometry(vertices,indices,glfwGetTime());

  glBindBuffer(GL_ARRAY_BUFFER,VBO_liquid);
  glBufferSubData(GL_ARRAY_BUFFER,0,vertices.size() * sizeof(float),vertices.data());
  glBindBuffer(GL_ARRAY_BUFFER,0);
}

void initBubbles(int num_bubbles){
  bubbles.clear();
  vector<float> bubbleData;
  float liquidRadius = 0.2f;
  
  for(int i = 0; i < num_bubbles; i++){
    Bubble b;
    float angle = random_number(0.0f,2*M_PI);
    float r = random_number(0.25f, liquidRadius);

    b.x = r * cos(angle);
    b.y = random_number(-0.5f,0.195f);
    b.z = r * sin(angle);
    b.speed = random_number(0.15f, 0.45f);
    b.size = random_number(0.04f, 0.12f);
    b.phase = random_number(0.0f, 6.28f);
    bubbles.push_back(b);

    bubbleData.push_back(b.x);
    bubbleData.push_back(b.y);
    bubbleData.push_back(b.z);
    bubbleData.push_back(b.size);
    }
    
    // Create VBO with positions and sizes
    glGenBuffers(1, &VBO_bubbles);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_bubbles);
    glBufferData(GL_ARRAY_BUFFER, bubbleData.size() * sizeof(float), bubbleData.data(), GL_DYNAMIC_DRAW);
    
    // Setup VAO for bubbles
    glGenVertexArrays(1, &VAO_bubbles);
    glBindVertexArray(VAO_bubbles);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void updateBubbles(float deltaTime){
  vector<float> bubbleData;
  float liquidRadius = 0.2f;
  
  for(auto &b : bubbles){
    b.x += 0.02*sin(b.phase + 5.0f*b.y) * deltaTime;
    b.y += b.speed * deltaTime;
    b.z += 0.02*cos(b.phase + 5.0f*b.y) * deltaTime;

    float r2 = b.x*b.x + b.z*b.z;
    float maxR = liquidRadius * 0.9f;   // stay a bit inside the wall
    if(r2 > maxR*maxR){
      float r = sqrtf(r2);
      b.x = b.x / r * maxR;
      b.z = b.z / r * maxR;
    }
 
    if(b.y >= 0.18f){
      b.y = random_number(-0.5f,0.195f);
      float angle = random_number(0.0f,2*M_PI);
      float r = random_number(0.0f, 0.9f*liquidRadius);
      b.x = r*cos(angle);
      b.z = r*sin(angle);
    }
    
   // Store position and size
        bubbleData.push_back(b.x);
        bubbleData.push_back(b.y);
	bubbleData.push_back(b.z);
        bubbleData.push_back(b.size);
    }
    
    // Update the VBO with new data
    glBindBuffer(GL_ARRAY_BUFFER, VBO_bubbles);
    glBufferSubData(GL_ARRAY_BUFFER, 0, bubbleData.size() * sizeof(float), bubbleData.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

int main(){

  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  
  if(!glfwInit()){
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
  glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window;
  window = glfwCreateWindow(width,height,"3D Beer Bottle Liquid Simulation!",NULL,NULL);
  if(window == NULL){
    fprintf(stderr,"Failed to open GLFW window\n");
    glfwTerminate();
    return -1;
  }
  
  glfwMakeContextCurrent(window);
  glewExperimental = true;

  if(glewInit() != GLEW_OK){
    fprintf(stderr,"Failed to initalize GLEW\n");
    return -1;
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  
  shaderProgram = shaders();
  colourShaderProgram = createColourShader();
  pointShaderProgram = createPointShader();

  initBottle();
  initLiquid();
  initBubbles(150);

  stbi_set_flip_vertically_on_load(true);  
  unsigned int bottleTexture = loadTexture("beer_bottle.png");
  
  float lastFrame = 0.0f;
  
  while(!glfwWindowShouldClose(window)){
    // ... input handling ...

    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    updateBubbles(deltaTime);
    updateLiquid();

    // Create ONE unified model matrix for all components
    mat4 model = mat4(1.0f);
    // Spin everything around the Y-axis together
    model = rotate(model, (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f)); 

    mat4 view = translate(mat4(1.0f), vec3(0.0f, 0.0f, -2.0f)); // Pushed back slightly (-2.0f) so the larger bottle fits perfectly in view
    mat4 projection = perspective(radians(45.0f), (float)width/(float)height, 0.1f, 100.0f);

    // Static bottle model matrix
    mat4 bottleModel = mat4(1.0f);
    
    // 1. Draw bottle
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, value_ptr(bottleModel));  
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, bottleTexture);
    glBindVertexArray(VAO_bottle);
    glDrawElements(GL_TRIANGLES, bottleIndexCount, GL_UNSIGNED_INT, 0);
 
    // 2. Draw bubbles (Shares the exact same 'model' matrix)
    glUseProgram(pointShaderProgram);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUniformMatrix4fv(glGetUniformLocation(pointShaderProgram, "model"), 1, GL_FALSE, value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(pointShaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(pointShaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
    glBindVertexArray(VAO_bubbles);
    glDrawArrays(GL_POINTS, 0, bubbles.size());
    glDisable(GL_PROGRAM_POINT_SIZE);

        // 3. Draw liquid (Shares the exact same 'model' matrix)
    glUseProgram(colourShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(colourShaderProgram, "model"), 1, GL_FALSE, value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(colourShaderProgram, "view"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(colourShaderProgram, "projection"), 1, GL_FALSE, value_ptr(projection));
    glUniform4f(glGetUniformLocation(colourShaderProgram, "liquidColor"), 0.9f, 0.5f, 0.0f, 0.8f);
    glUniform3f(glGetUniformLocation(colourShaderProgram, "lightDir"), -0.5f, -1.0f, -0.3f);
    glDepthMask(GL_FALSE);
    glBindVertexArray(VAO_liquid);
    glDrawElements(GL_TRIANGLES, liquidIndexCount, GL_UNSIGNED_INT, 0);
    glDepthMask(GL_TRUE);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
