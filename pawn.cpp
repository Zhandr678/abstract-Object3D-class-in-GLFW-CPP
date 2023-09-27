#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>
#include <cmath>

#define NULL_FLOAT_VECTOR std::vector <float>({ -2598445.9842f })
const float PI = acos(-1);

struct ShaderProgramInfo
{
    std::string vertexShaderProgramInfo;
    std::string fragmentShaderProgramInfo;
};
std::string shader_path(char** argv, std::string path)
{
    std::string exe_path = std::filesystem::path(argv[0]).parent_path().string();
    exe_path = exe_path.substr(0, exe_path.find_last_of("/\\"));
    exe_path += "/winAPI_glew";
    std::string shader_path = exe_path + path;
    return shader_path;
}
ShaderProgramInfo parseShader(const std::string& filepath)
{
    std::ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::stringstream ss[2];
    std::string line;
    ShaderType type = ShaderType::NONE;

    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
            {
                type = ShaderType::VERTEX;
            }
            if (line.find("fragment") != std::string::npos)
            {
                type = ShaderType::FRAGMENT;
            }
        }
        else
        {
            if (type != ShaderType::NONE)
            {
                ss[static_cast <int>(type)] << line << '\n';
            }
        }
    }
    return { ss[0].str(), ss[1].str() };
}
unsigned int compileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    /* Source of our shader */
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    /* If shader is not compiled correctly */
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::unique_ptr <char[]> message = std::make_unique <char[]>(length);
        if (type == GL_VERTEX_SHADER)
        {
            std::cout << "Failed to compile GL_VERTEX_SHADER!" << std::endl;
        }
        else
        {
            std::cout << "Failed to compile GL_FRAGMENT_SHADER!" << std::endl;
        }
        glGetShaderInfoLog(id, length, &length, message.get());
        std::cout << *message.get() << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}
unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    /* Link to the program */
    glLinkProgram(program);
    glValidateProgram(program);

    /* Since shaders are attached to the program, we can delete shaders */
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

class Rotatable
{
private:
    glm::mat4 transform;
    glm::vec3 pivot;
    unsigned int matloc = 0;
public:
    Rotatable() : pivot(glm::vec3(0.3f, 0.3f, 0.3f)), transform(glm::mat4(1)) {}
    Rotatable(glm::vec3 pivot) : pivot(pivot), transform(glm::mat4(1)) {}

    glm::mat4 get_transform() { return transform; }
    glm::vec3 get_pivot() { return pivot; }
    void set_pivot(glm::vec3 pivot) { this->pivot = pivot; }

    void init_rotation(unsigned int& shader)
    {
        this->matloc = glGetUniformLocation(shader, "transform");
        glUniformMatrix4fv(matloc, 1, GL_FALSE, glm::value_ptr<float>(transform));
        glUseProgram(shader);
    }
    virtual void apply_rotation(unsigned int& shader)
    {
        transform = glm::rotate(transform, 0.001f, pivot);
        glUniformMatrix4fv(matloc, 1, GL_FALSE, glm::value_ptr<float>(transform));
        glUseProgram(shader);
    }

    virtual void random_pure_virtual_function() = 0;
    virtual ~Rotatable() {};
};

class Object3D
{
private:
    std::vector <float> positions;
    std::vector <float> colors;
    std::vector <float> normals;
    unsigned int vbo = 0, cbo = 0, nbo = 0;

    const unsigned int DIMENSIONS = 3;
public:
    Object3D() = default;
    Object3D(const Object3D& other)
    {
        if (other.is_vbo_init()) { this->init_vbo(other.positions); }
        if (other.is_cbo_init()) { this->init_cbo(other.colors); }
        if (other.is_nbo_init()) { this->init_nbo(other.normals); }
    }
    Object3D& operator =(const Object3D& other) 
    {
        if (other.is_vbo_init()) { this->init_vbo(other.positions); }
        if (other.is_cbo_init()) { this->init_cbo(other.colors); }
        if (other.is_nbo_init()) { this->init_nbo(other.normals); }

        return *this;
    };

    std::vector <float> get_positions() const { return positions; }
    std::vector <float> get_colors() const { return colors; }
    std::vector <float> get_normals() const { return normals; }

    unsigned int get_vbo() const { return vbo; };
    unsigned int get_cbo() const { return cbo; };
    unsigned int get_nbo() const { return nbo; };

    void push_to_positions(float position) { positions.push_back(position); }
    void push_to_colors(float color) { colors.push_back(color); }
    void push_to_normals(float normal) { normals.push_back(normal); }

    virtual void draw_shape(unsigned int& shader_source) const = 0;

    bool is_vbo_init() const { return vbo != 0; }
    bool is_cbo_init() const { return cbo != 0; }
    bool is_nbo_init() const { return nbo != 0; }
    bool is_entirely_init() const { return is_vbo_init() and is_cbo_init() and is_nbo_init() ? true : false; }

    void init_vbo()
    {
        glDeleteBuffers(1, &vbo);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, this->positions.size() * sizeof(float), this->positions.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void init_cbo()
    {
        glDeleteBuffers(1, &cbo);
        glGenBuffers(1, &cbo);
        glBindBuffer(GL_ARRAY_BUFFER, cbo);
        glBufferData(GL_ARRAY_BUFFER, this->colors.size() * sizeof(float), this->colors.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void init_nbo()
    {
        glDeleteBuffers(1, &nbo);
        glGenBuffers(1, &nbo);
        glBindBuffer(GL_ARRAY_BUFFER, nbo);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(normals[0]), normals.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void init_vbo(std::vector <float> positions)
    {
        glDeleteBuffers(1, &vbo);
        this->positions = positions;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, this->positions.size() * sizeof(float), this->positions.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void init_cbo(std::vector <float> colors)
    {
        glDeleteBuffers(1, &cbo);
        this->colors = colors;
        glGenBuffers(1, &cbo);
        glBindBuffer(GL_ARRAY_BUFFER, cbo);
        glBufferData(GL_ARRAY_BUFFER, this->colors.size() * sizeof(float), this->colors.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void init_nbo(std::vector <float> normals)
    {
        glDeleteBuffers(1, &nbo);
        this->normals = normals;
        glGenBuffers(1, &nbo);
        glBindBuffer(GL_ARRAY_BUFFER, nbo);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(normals[0]), normals.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    virtual ~Object3D() 
    {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &cbo);
        glDeleteBuffers(1, &nbo);
    };
};

class Sphere : public Object3D, public Rotatable
{
private:
    float x, y, z, r;
public:
    Sphere(float x, float y, float z, float r, unsigned int layer_quality, unsigned int density_quality, 
        std::vector <float> normalized_rgb, std::vector <float> normals) : Object3D(), Rotatable(), x(x), y(y), z(z), r(r)
    { 
        this->generate_sphere(this->x, this->y, this->z, this->r, layer_quality, density_quality);
        if (normalized_rgb != NULL_FLOAT_VECTOR) { this->apply_colors(normalized_rgb, layer_quality, density_quality); }
        if (normals != NULL_FLOAT_VECTOR) { this->init_nbo(normals); }
    }
    void generate_sphere(float x, float y, float z, float r, unsigned int layer_quality, unsigned int density_quality)
    {
        for (int i = 0; i < layer_quality; ++i)
        {
            for (int j = 0; j < density_quality; ++j)
            {

                float x1 = x + r * cosf(2 * PI * i / static_cast <float>(layer_quality)) * sinf(j / static_cast <float>(density_quality) * PI);
                float y1 = y + r * sinf(2 * PI * i / static_cast <float>(layer_quality)) * sinf(j / static_cast <float>(density_quality) * PI);
                float z1 = z + r * cosf(j / static_cast <float>(density_quality) * PI);
                this->push_to_positions(x1);
                this->push_to_positions(y1);
                this->push_to_positions(z1);

                float x2 = x + r * cosf(2 * PI * (i + 1) / static_cast <float>(layer_quality)) * sinf(j / static_cast <float>(density_quality) * PI);
                float y2 = y + r * sinf(2 * PI * (i + 1) / static_cast <float>(layer_quality)) * sinf(j / static_cast <float>(density_quality) * PI);
                float z2 = z + r * cosf(j / static_cast <float>(density_quality) * PI);
                this->push_to_positions(x2);
                this->push_to_positions(y2);
                this->push_to_positions(z2);

                float x3 = x + r * cosf(2 * PI * i / static_cast <float>(layer_quality)) * sinf((j + 1) / static_cast <float>(density_quality) * PI);
                float y3 = y + r * sinf(2 * PI * i / static_cast <float>(layer_quality)) * sinf((j + 1) / static_cast <float>(density_quality) * PI);
                float z3 = z + r * cosf((j + 1) / static_cast <float>(density_quality) * PI);
                this->push_to_positions(x3);
                this->push_to_positions(y3);
                this->push_to_positions(z3);

                this->push_to_positions(x1);
                this->push_to_positions(y1);
                this->push_to_positions(z1);
                this->push_to_positions(x2);
                this->push_to_positions(y2);
                this->push_to_positions(z2);

                float x4 = x + r * cosf(2 * PI * (i + 1) / static_cast <float>(layer_quality)) * sinf((j + 1) / static_cast <float>(density_quality) * PI);
                float y4 = y + r * sinf(2 * PI * (i + 1) / static_cast <float>(layer_quality)) * sinf((j + 1) / static_cast <float>(density_quality) * PI);
                float z4 = z + r * cosf((j + 1) / static_cast <float>(density_quality) * PI);
                this->push_to_positions(x4);
                this->push_to_positions(y4);
                this->push_to_positions(z4);
            }
        }
        this->init_vbo();
    }
    void apply_colors(std::vector <float> normalized_rgb, unsigned int layer_quality, unsigned int density_quality)
    {
        int num_vertices = static_cast<int>(this->get_positions().size() / 3);

        for (int i = 0; i < num_vertices; ++i)
        {
            float gradientFactor = glm::length(glm::vec3(
                this->get_positions()[i * 3],
                this->get_positions()[i * 3 + 1],
                this->get_positions()[i * 3 + 2]
            )) / glm::length(glm::vec3(1.0));

            float r = normalized_rgb[0] * (1.0 - gradientFactor);
            float g = normalized_rgb[1] * (1.0 - gradientFactor);
            float b = normalized_rgb[2] * (1.0 - gradientFactor);

            push_to_colors(r);
            push_to_colors(g);
            push_to_colors(b);
        }

        this->init_cbo();
    }
    void draw_shape(unsigned int& shader_source) const override
    {
        unsigned int posloc = glGetAttribLocation(shader_source, "inPosition");
        glBindBuffer(GL_ARRAY_BUFFER, this->get_vbo());
        glVertexAttribPointer(posloc, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(posloc);

        unsigned int colorloc = glGetAttribLocation(shader_source, "inColor");
        glBindBuffer(GL_ARRAY_BUFFER, this->get_cbo());
        glVertexAttribPointer(colorloc, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(colorloc);

        unsigned int normloc = glGetAttribLocation(shader_source, "inNormal");
        glBindBuffer(GL_ARRAY_BUFFER, this->get_nbo());
        glVertexAttribPointer(normloc, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normloc);

        glDrawArrays(GL_TRIANGLES, 0, this->get_positions().size() / 3);

        glDisableVertexAttribArray(posloc);
        glDisableVertexAttribArray(colorloc);
        glDisableVertexAttribArray(normloc);
    }

    void random_pure_virtual_function() override { return; }
};

class StandingCylinder : public Object3D, public Rotatable
{
private:
    float bottom_x, bottom_y, bottom_z;
    float r, h;
public:
    StandingCylinder(float x, float y, float z, float r, float h, unsigned int circle_quality, unsigned int side_quality,
        std::vector <float> normalized_rgb, std::vector <float> normals) : Object3D(), Rotatable(), bottom_x(x), bottom_y(y), bottom_z(z), r(r), h(h)
    {
        generate_cylinder(x, y, z, r, h, circle_quality, side_quality);
        if (normalized_rgb != NULL_FLOAT_VECTOR) { apply_color(normalized_rgb, circle_quality, side_quality); }
        if (normals != NULL_FLOAT_VECTOR) { this->init_nbo(normals); }
    }

    void apply_color(std::vector <float> normalized_rgb, unsigned int circle_quality, unsigned int side_quality)
    {
        int num_vertices = static_cast<int>(this->get_positions().size() / 3);

        for (int i = 0; i < num_vertices; ++i)
        {
            // Calculate the normalized gradient factor based on the vertex's position
            float gradientFactor = glm::length(glm::vec3(
                this->get_positions()[i * 3],
                this->get_positions()[i * 3 + 1],
                this->get_positions()[i * 3 + 2]
            )) / glm::length(glm::vec3(1.0));

            // Darken the input color based on the gradient factor
            float r = normalized_rgb[0] * (1.0 - gradientFactor);
            float g = normalized_rgb[1] * (1.0 - gradientFactor);
            float b = normalized_rgb[2] * (1.0 - gradientFactor);

            // Add the darkened color to the colors vector
            push_to_colors(r);
            push_to_colors(g);
            push_to_colors(b);
        }

        this->init_cbo();
    }
    void generate_cylinder(float bx, float by, float bz, float r, float h, unsigned int circle_quality, unsigned int side_quality)
    {
        for (int i = 0; i < circle_quality; i++)
        {
            float x = bx + r * cosf(2.0f * PI * static_cast <float>(i) / static_cast <float>(circle_quality));
            float z = bz + r * sinf(2.0f * PI * static_cast <float>(i) / static_cast <float>(circle_quality));

            float xnext = bx + r * cosf(2.0f * PI * static_cast <float>(i + 1) / static_cast <float>(circle_quality));
            float znext = bz + r * sinf(2.0f * PI * static_cast <float>(i + 1) / static_cast <float>(circle_quality));

            this->push_to_positions(x); this->push_to_positions(by); this->push_to_positions(z);
            this->push_to_positions(xnext); this->push_to_positions(by); this->push_to_positions(znext);
            this->push_to_positions(bx); this->push_to_positions(by); this->push_to_positions(bz);
        }
        for (int i = 0; i < circle_quality; i++)
        {
            float x = bx + r * cosf(2.0f * PI * static_cast <float>(i) / static_cast <float>(circle_quality));
            float z = bz + r * sinf(2.0f * PI * static_cast <float>(i) / static_cast <float>(circle_quality));

            float xnext = bx + r * cosf(2.0f * PI * static_cast <float>(i + 1) / static_cast <float>(circle_quality));
            float znext = bz + r * sinf(2.0f * PI * static_cast <float>(i + 1) / static_cast <float>(circle_quality));

            this->push_to_positions(x); this->push_to_positions(by + h); this->push_to_positions(z);
            this->push_to_positions(xnext); this->push_to_positions(by + h); this->push_to_positions(znext);
            this->push_to_positions(bx); this->push_to_positions(by + h); this->push_to_positions(bz);
        }

        for (int i = 0; i < side_quality; i++)
        {
            float x = bx + r * cosf(2 * PI * static_cast <float>(i) / static_cast <float>(side_quality));
            float z = bz + r * sinf(2 * PI * static_cast <float>(i) / static_cast <float>(side_quality));
            float xnext = bx + r * cosf(2 * PI * static_cast <float>(i + 1) / static_cast <float>(side_quality));
            float znext = bz + r * sinf(2 * PI * static_cast <float>(i + 1) / static_cast <float>(side_quality));

            this->push_to_positions(x); this->push_to_positions(by); this->push_to_positions(z);
            this->push_to_positions(xnext); this->push_to_positions(by); this->push_to_positions(znext);
            this->push_to_positions(x); this->push_to_positions(by + h); this->push_to_positions(z);

            this->push_to_positions(xnext); this->push_to_positions(by); this->push_to_positions(znext);
            this->push_to_positions(xnext); this->push_to_positions(by + h); this->push_to_positions(znext);
            this->push_to_positions(x); this->push_to_positions(by + h); this->push_to_positions(z);
        }

        this->init_vbo();
    }
    void draw_shape(unsigned int& shader_source) const override
    {
        unsigned int posloc = glGetAttribLocation(shader_source, "inPosition");
        glBindBuffer(GL_ARRAY_BUFFER, this->get_vbo());
        glVertexAttribPointer(posloc, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(posloc);

        unsigned int colorloc = glGetAttribLocation(shader_source, "inColor");
        glBindBuffer(GL_ARRAY_BUFFER, this->get_cbo());
        glVertexAttribPointer(colorloc, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(colorloc);

        unsigned int normloc = glGetAttribLocation(shader_source, "inNormal");
        glBindBuffer(GL_ARRAY_BUFFER, this->get_nbo());
        glVertexAttribPointer(normloc, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normloc);

        glDrawArrays(GL_TRIANGLES, 0, this->get_positions().size() / 3);

        glDisableVertexAttribArray(posloc);
        glDisableVertexAttribArray(colorloc);
        glDisableVertexAttribArray(normloc);
    }

    void random_pure_virtual_function() override { return; }
};

class Composition
{
private:
    std::vector <Object3D*> figures;
public:
    void add(Object3D* obj)
    {
        figures.push_back(obj);
    }

    void init_rotation(unsigned int& shader)
    {
        for (const auto& i : figures)
        {
            if (Rotatable* r = dynamic_cast <Rotatable*>(i))
            {
                r->init_rotation(shader);
            }
        }
    }

    void apply_rotation(unsigned int& shader)
    {
        for (const auto& i : figures)
        {
            if (Rotatable* r = dynamic_cast <Rotatable*>(i))
            {
                r->apply_rotation(shader);
            }
        }
    }

    void draw_composition(unsigned int& shader)
    {
        for (const auto& i : figures)
        {
            i->draw_shape(shader);
        }
    }
    ~Composition()
    {
        for (auto& i : figures)
        {
            delete i;
        }
    }
};

int main(int argc, char** argv)
{
    GLFWwindow* window;

    if (!glfwInit())
    {
        std::cout << "[GLFW]: Initialization Error!\n";
        return -1;
    }
    window = glfwCreateWindow(640, 640, "GLFW", NULL, NULL);
    if (!window)
    {
        std::cout << "[GLFW]: Window Creation Error!\n";
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK)
    {
        std::cout << "[GLEW]: Initialization Error!\n";
        return -1;
    }
    std::cout << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);

    Composition comp;
    comp.add(new Sphere(0.0f, 0.5f, 0.0f, 0.2f, 20, 20, { 0.5f, 0.5f, 0.5f }, NULL_FLOAT_VECTOR));
    comp.add(new StandingCylinder(0.0f, 0.05f, 0.0f, 0.1f, 0.4f, 20, 20, { 0.5f, 0.5f, 0.5f }, NULL_FLOAT_VECTOR));
    comp.add(new StandingCylinder(0.0f, 0.25f, 0.0f, 0.2f, 0.05f, 20, 20, { 0.5f, 0.5f, 0.5f }, NULL_FLOAT_VECTOR));
    comp.add(new StandingCylinder(0.0f, -0.35f, 0.0f, 0.15f, 0.4f, 20, 20, { 0.5f, 0.5f, 0.5f }, NULL_FLOAT_VECTOR));
    comp.add(new StandingCylinder(0.0f, -0.5f, 0.0f, 0.25f, 0.2f, 20, 20, { 0.5f, 0.5f, 0.5f }, NULL_FLOAT_VECTOR));

    std::string path_shader = shader_path(argv, "/pawn.shader");
    ShaderProgramInfo source = parseShader(path_shader);
    unsigned int shader = createShader(source.vertexShaderProgramInfo, source.fragmentShaderProgramInfo);


    comp.init_rotation(shader);

    glUseProgram(shader);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glClearColor(1, 1, 1, 1);

        comp.draw_composition(shader);
        comp.apply_rotation(shader);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glDeleteProgram(shader);
    glfwTerminate();

	return 0;
}