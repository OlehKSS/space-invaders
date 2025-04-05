#include <cstdint>
#include <memory>
#include <print>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct Buffer
{
    size_t width;
    size_t height;
    std::vector<uint32_t> data;
};

struct Sprite
{
    size_t width;
    size_t height;
    std::vector<uint8_t> data;
};

enum AlienType: uint8_t
{
    ALIEN_DEAD = 0,
    ALIEN_TYPE_A = 1,
    ALIEN_TYPE_B,
    ALIEN_TYPE_C,
    N
};

// A position (x, y) given in pixels from the bottom left corner of the window
struct Alien
{
    size_t x;
    size_t y;
    AlienType type;
};

struct Player
{
    size_t x;
    size_t y;
    size_t life;
};

struct Bullet
{
    size_t x;
    size_t y;
    int dir;
};

constexpr size_t GAME_MAX_BULLETS = 128;

struct Game
{
    size_t width;
    size_t height;
    size_t num_aliens;
    size_t num_bullets;
    std::vector<Alien> aliens;
    Player player;
    Bullet bullets[GAME_MAX_BULLETS];
};

struct SpriteAnimation
{
    bool loop;
    size_t num_frames;
    size_t frame_duration;
    size_t time;
    std::vector<Sprite> frames;
};

bool sprite_overlap_check(const Sprite& sp_a, size_t x_a, size_t y_a,
    const Sprite& sp_b, size_t x_b, size_t y_b)
{
    return (x_a < (x_b + sp_b.width) && (x_a + sp_a.width) > x_b &&
        y_a < (y_b + sp_b.height) && (y_a + sp_a.height) > y_b);
}

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

bool game_running = false;
int move_dir = 0;
bool fire_pressed = 0;

void key_callback(GLFWwindow* window, int key, int scancode, int action,
    int modes /* Shift, Ctrl, etc. */)
{
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS) game_running = false;
        break;
    case GLFW_KEY_RIGHT:
        if (action == GLFW_PRESS) move_dir += 1;
        else if (action == GLFW_RELEASE) move_dir -= 1;
        break;
    case GLFW_KEY_LEFT:
        if (action == GLFW_PRESS) move_dir -= 1;
        else if (action == GLFW_RELEASE) move_dir += 1;
        break;
    case GLFW_KEY_SPACE:
        if (action == GLFW_RELEASE) fire_pressed = true;
        break;
    default:
        break;
    }
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
    glfwSetKeyCallback(window, key_callback);

    auto err = glewInit();

    if (err != GLEW_OK)
    {
        std::println("Error initializing GLEW.");
        glfwTerminate();
        return -1;
    }

    // Turn on V-sync, an option wherein video card updates are synchronized with the monitor refresh rate, e.g., 60 Hz
    glfwSwapInterval(1);

    glClearColor(1.0, 0.0, 0.0, 1.0);

    Buffer buffer;
    buffer.width = buffer_width;
    buffer.height = buffer_height;
    buffer.data = std::vector<uint32_t>(buffer.width * buffer.height);
    buffer_clear(&buffer, 0);

    GLuint buffer_texture;
    glGenTextures(1, &buffer_texture);

    glBindTexture(GL_TEXTURE_2D, buffer_texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8,
        buffer.width, buffer.height, 0,
        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data.data()
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
        return -1;
    }

    glUseProgram(shader_id);

    // Attach the texture to the uniform sampler2D variable in the fragment shader
    GLint location = glGetUniformLocation(shader_id, "buffer");
    glUniform1i(location, 0);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(fullscreen_triangle_vao);

    Sprite alien_sprites[6];

    alien_sprites[0].width = 8;
    alien_sprites[0].height = 8;
    alien_sprites[0].data = std::vector<uint8_t> 
    {
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,0,0,0,0,0,1, // @......@
        0,1,0,0,0,0,1,0  // .@....@.
    };

    alien_sprites[1].width = 8;
    alien_sprites[1].height = 8;
    alien_sprites[1].data = std::vector<uint8_t>
    {
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,0,1,0,0,1,0,0, // ..@..@..
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,1,0,0,1,0,1  // @.@..@.@
    };

    alien_sprites[2].width = 11;
    alien_sprites[2].height = 8;
    alien_sprites[2].data = std::vector<uint8_t>
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

    alien_sprites[3].width = 11;
    alien_sprites[3].height = 8;
    alien_sprites[3].data = std::vector<uint8_t>
    {
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
    };

    alien_sprites[4].width = 12;
    alien_sprites[4].height = 8;
    alien_sprites[4].data = std::vector<uint8_t>
    {
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
        0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
        1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
    };


    alien_sprites[5].width = 12;
    alien_sprites[5].height = 8;
    alien_sprites[5].data = std::vector<uint8_t>
    {
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
        0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
        0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
    };

    Sprite alien_death_sprite;
    alien_death_sprite.width = 13;
    alien_death_sprite.height = 7;
    alien_death_sprite.data = std::vector<uint8_t>
    {
        0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
    };

    std::vector<SpriteAnimation> alien_animation(3);

    for (size_t i = 0; i < 3; ++i)
    {
        alien_animation[i].loop = true;
        alien_animation[i].num_frames = 2;
        alien_animation[i].frame_duration = 10;
        alien_animation[i].time = 0;

        alien_animation[i].frames = std::vector<Sprite>(
            { alien_sprites[2 * i], alien_sprites[2 * i + 1] });
    }

    Sprite player_sprite;
    player_sprite.width = 11;
    player_sprite.height = 7;
    player_sprite.data = std::vector<uint8_t>
    {
        0,0,0,0,0,1,0,0,0,0,0, // .....@.....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
    };

    Sprite bullet_sprite;
    bullet_sprite.width = 1;
    bullet_sprite.height = 3;
    bullet_sprite.data =std::vector<uint8_t>
    {
        1, // @
        1, // @
        1  // @
    };

    Game game;
    game.width = buffer_width;
    game.height = buffer_height;
    game.num_aliens = 55;
    game.num_bullets = 0;
    game.aliens = std::vector<Alien>(game.num_aliens);
    game.player.x = 112 - 5;
    game.player.y = 32;
    game.player.life = 3;

    for (size_t yi = 0; yi < 5; ++yi)
    {
        for (size_t xi = 0; xi < 11; ++xi)
        {
            auto& alien = game.aliens[yi * 11 + xi];
            alien.type = static_cast<AlienType>((5 - yi) / 2 + 1);

            auto& sprite = alien_sprites[2 * (alien.type - 1)];
            alien.x = 16 * xi + 20 + (alien_death_sprite.width - sprite.width)/2;
            alien.y = 17 * yi + 128;
        }
    }

    auto death_counters = std::vector<uint8_t>(game.num_aliens, 10);

    // Game loop
    auto clear_color = rgb_to_uint32(0, 0, 0);
    game_running = true;

    while(!glfwWindowShouldClose(window) && game_running)
    {
        buffer_clear(&buffer, clear_color);

        for (size_t ai = 0; ai < game.num_aliens; ++ai)
        {
            /* If an alien is dead, we decrement the death counter,
            and "remove" the alien from the game when the counter reaches 0. 
            When drawing the aliens, we now need to check if the death counter 
            is bigger than 0, otherwise we don't have to draw the alien. 
            This way, the death sprite is shown for 10 frames.*/
            if (!death_counters[ai]) continue;
    
            const auto& alien = game.aliens[ai];

            if (alien.type == ALIEN_DEAD)
            {
                buffer_sprite_draw(&buffer, alien_death_sprite, alien.x, alien.y,
                    rgb_to_uint32(128, 0, 0));
            }
            else
            {
                const auto& animation = alien_animation[alien.type - 1];
                size_t current_frame = animation.time / animation.frame_duration;
                const auto& sprite = animation.frames[current_frame];
                buffer_sprite_draw(&buffer, sprite, alien.x, alien.y,
                    rgb_to_uint32(128, 0, 0));
            }
        }

        for (size_t bi = 0; bi < game.num_bullets; ++bi)
        {
            const auto& bullet = game.bullets[bi];
            const auto& sprite = bullet_sprite;
            buffer_sprite_draw(&buffer, sprite, bullet.x, bullet.y, rgb_to_uint32(128, 0, 0));
        }

        buffer_sprite_draw(&buffer, player_sprite, game.player.x, game.player.y,
            rgb_to_uint32(128, 0, 0));

        
        // Update animations
        for (auto& animation : alien_animation)
        {
            animation.time += 1;
            if (animation.time == animation.num_frames * animation.frame_duration)
            {
                animation.time = 0;
            }
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            buffer.width, buffer.height,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
            buffer.data.data()
        );

        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glfwSwapBuffers(window);
        // Simulate aliens
        for (size_t ai = 0; ai < game.num_aliens; ++ai)
        {
            const auto& alien = game.aliens[ai];
            if (alien.type == ALIEN_DEAD && death_counters[ai])
            {
                --death_counters[ai];
            }
        }
        // Simulate bullets
        for (size_t bi = 0; bi < game.num_bullets;)
        {
            game.bullets[bi].y += game.bullets[bi].dir;

            if (game.bullets[bi].y >= game.height ||
                game.bullets[bi].y < bullet_sprite.height)
            {
                // Could be pop?
                game.bullets[bi] = game.bullets[game.num_bullets - 1];
                --game.num_bullets;
                continue;
            }

            // Check hit
            for (auto& alien : game.aliens)
            {
                if (alien.type == ALIEN_DEAD) continue;

                auto& animation = alien_animation[alien.type - 1];
                size_t current_frame = animation.time / animation.frame_duration;
                const auto& alien_sprite = animation.frames[current_frame];
                bool overlap = sprite_overlap_check(bullet_sprite, game.bullets[bi].x, game.bullets[bi].y,
                    alien_sprite, alien.x, alien.y);

                if (overlap)
                {
                    alien.type = ALIEN_DEAD;
                    // NOTE: Hack to recenter death sprite
                    alien.x -=  (alien_death_sprite.width - alien_sprite.width) / 2;
                    game.bullets[bi] = game.bullets[game.num_bullets - 1];
                    --game.num_bullets;
                    continue;
                }
            }

            ++bi;
        }
        // Simulate player
        if (int player_move_dir = 2 * move_dir;
            player_move_dir != 0)
        {
            if (game.player.x + player_sprite.width + player_move_dir >= game.width)
            {
                game.player.x = game.width - player_sprite.width;
            }
            else if ((int) game.player.x + player_move_dir <= 0)
            {
                game.player.x = 0;
            }
            else
            {
                game.player.x += player_move_dir;
            }
        }

        // Player's fire
        if (fire_pressed && game.num_bullets < GAME_MAX_BULLETS)
        {
            game.bullets[game.num_bullets].x = game.player.x + player_sprite.width / 2;
            game.bullets[game.num_bullets].y = game.player.y + player_sprite.height;
            game.bullets[game.num_bullets].dir = 2;
            ++game.num_bullets;
        }

        fire_pressed = false;

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    glDeleteVertexArrays(1, &fullscreen_triangle_vao);

    // delete[] alien_sprite.data;
    // delete[] buffer.data;

    return 0;
}