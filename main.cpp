#include <cstdint>
#include <print>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct Buffer
{
    size_t width;
    size_t height;
    uint32_t* data;
};

struct Sprite
{
    size_t width;
    size_t height;
    uint8_t* data;
};

constexpr uint32_t rgb_to_uint32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r << 24) | (g << 16) | (b << 8) | 255);
}

void buffer_clear(Buffer* buffer, uint32_t color)
{
    for (size_t i = 0; i < buffer->width * buffer->height; ++i)
    {
        buffer->data[i] = color;
    }
}

void buffer_sprite_draw(Buffer* buffer, const Sprite& sprite,
    size_t x, size_t y, uint32_t color)
{
    for (size_t xi = 0; xi < sprite.width; ++xi)
    {
        for (size_t yi = 0; yi < sprite.height; ++yi)
        {
            size_t sy = sprite.height - 1 + y - yi;
            size_t sx = x + xi;

            if (sprite.data[yi * sprite.width + xi] &&
                sy < buffer->height && 
                sx < buffer->width)
            {
                buffer->data[sy * buffer->width + sx] = color;
            }
        }
    }
}

void validate_shader(GLuint shader, const char* file = 0)
{
    constexpr auto BUFFER_SIZE = 512;
    char buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetShaderInfoLog(shader, BUFFER_SIZE, &length, buffer);

    if (length > 0)
    {
        std::println("Shader {:d}({:s}) compile error: {:s}",
            shader, (file ? file : ""), buffer);
    }
}

bool validate_program(GLuint program)
{
    constexpr auto BUFFER_SIZE = 512;
    GLchar buffer[BUFFER_SIZE];
    GLsizei length = 0;

    glGetProgramInfoLog(program, BUFFER_SIZE, &length, buffer);

    if (length > 0)
    {
        std::println("Program {:d} link error: {:s}", program, buffer);
        return false;
    }

    return true;
}

void error_callback(int error, const char* description)
{
    std::println("Error: {}", description);
}

// https://nicktasios.nl/posts/space-invaders-from-scratch-part-1.html

int main(int argc, char** argv)
{
    constexpr auto buffer_width = 224;
    constexpr auto buffer_height = 256;

    glfwSetErrorCallback(error_callback);

    if(!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    auto window = glfwCreateWindow(640, 480, "Space Invaders", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    auto err = glewInit();

    if (err != GLEW_OK)
    {
        std::println("Error initializing GLEW.");
        glfwTerminate();
        return -1;
    }

    glClearColor(1.0, 0.0, 0.0, 1.0);

    Buffer buffer;
    buffer.width = buffer_width;
    buffer.height = buffer_height;
    buffer.data = new uint32_t[buffer.width * buffer.height];
    buffer_clear(&buffer, 0);

    GLuint buffer_texture;
    glGenTextures(1, &buffer_texture);

    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8,
        buffer.width, buffer.height, 0,
        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint fullscreen_triangle_vao;
    glGenVertexArrays(1, &fullscreen_triangle_vao);

    // Generate a quad that covers the screen
    auto vertex_shader = 
R"sh(
# version 330

noperspective out vec2 TexCoord;

void main(void) {
    TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;
    TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;
    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);
}
)sh";

    auto fragment_shader = 
R"sh(
# version 330

uniform sampler2D buffer;
noperspective in vec2 TexCoord;

out vec3 outColor;

void main(void) {
    outColor = texture(buffer, TexCoord).rgb;
}
)sh";

    GLuint shader_id = glCreateProgram();
    // Create vertex shader
    {
        GLuint shader_vp = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(shader_vp, 1, &vertex_shader, 0);
        glCompileShader(shader_vp);
        validate_shader(shader_vp, vertex_shader);
        glAttachShader(shader_id, shader_vp);

        glDeleteShader(shader_vp);
    }

    // Create fragment shader
    {
        GLuint shader_fp = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(shader_fp, 1, &fragment_shader, 0);
        glCompileShader(shader_fp);
        validate_shader(shader_fp, fragment_shader);
        glAttachShader(shader_id, shader_fp);

        glDeleteShader(shader_fp);
    }

    glLinkProgram(shader_id);

    if (!validate_program(shader_id))
    {
        std::println("Error while validating shader.");
        glfwTerminate();
        glDeleteVertexArrays(1, &fullscreen_triangle_vao);
        delete[] buffer.data;
        return -1;
    }

    glUseProgram(shader_id);

    // Attach the texture to the uniform sampler2D variable in the fragment shader
    GLint location = glGetUniformLocation(shader_id, "buffer");
    glUniform1i(location, 0);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(fullscreen_triangle_vao);

    Sprite alien_sprite;
    alien_sprite.width = 11;
    alien_sprite.height = 8;
    alien_sprite.data = new uint8_t[11 * 8]
    {
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
        0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
    };

    // Game loop
    auto clear_color = rgb_to_uint32(0, 128, 0);

    while(!glfwWindowShouldClose(window))
    {
        buffer_clear(&buffer, clear_color);

        buffer_sprite_draw(&buffer, alien_sprite, 112, 127,
            rgb_to_uint32(128, 0, 0));

        buffer_sprite_draw(&buffer, alien_sprite, 112, 128, 
            rgb_to_uint32(128, 0, 0));

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            buffer.width, buffer.height,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
            buffer.data
        );

        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    glDeleteVertexArrays(1, &fullscreen_triangle_vao);

    delete[] alien_sprite.data;
    delete[] buffer.data;

    return 0;
}