#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

// OpenGL library includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <debuggl.h>
#include "menger.h"
#include "camera.h"
#include <chrono>
#include <ctime>


int window_width = 800, window_height = 600;

// VBO and VAO descriptors.
enum { kVertexBuffer, kIndexBuffer, kNumVbos };

// These are our VAOs.
enum { kGeometryVao, kFloorVao, kNumVaos };

GLuint g_array_objects[kNumVaos];  // This will store the VAO descriptors.
GLuint g_buffer_objects[kNumVaos][kNumVbos];  // These will store VBO descriptors.

// C++ 11 String Literal
// See http://en.cppreference.com/w/cpp/language/string_literal
const char* vertex_shader =
R"zzz(#version 410 core
in vec4 vertex_position;
uniform mat4 view;
uniform vec4 light_position;
out vec4 vs_light_direction_0;
out vec4 vertex_position_world_0;
out vec4 vs_light_direction;
out vec4 vertex_position_world;
void main()
{
	gl_Position = view * vertex_position;
	vs_light_direction_0 = -gl_Position + view * light_position;
	vertex_position_world_0 = vertex_position;
	vs_light_direction = -gl_Position + view * light_position;
	vertex_position_world = vertex_position;
}
)zzz";




// const char* triangleTessControlShader =
// R"zzz(#version 410 core

// in vec4 vs_light_direction_0[];
// in vec4 vertex_position_world_0[];
// uniform int innerLevel;
// uniform int outerLevel;
// out vec4 vs_light_direction_1[];
// out vec4 vertex_position_world_1[];



// layout (vertices = 3) out;
// void main(void) {
// 	if(gl_InvocationID == 0) {
// 		gl_TessLevelInner[0] = 1.0 + innerLevel;
// 		gl_TessLevelOuter[0] = 1.0 + outerLevel;
// 		gl_TessLevelOuter[1] = 1.0 + outerLevel;
// 		gl_TessLevelOuter[2] = 1.0 + outerLevel;
// 	}
// 	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
// 	vs_light_direction_1[gl_InvocationID] = vs_light_direction_0[gl_InvocationID];
// 	vertex_position_world_1[gl_InvocationID] = vertex_position_world_0[gl_InvocationID];
// }

// )zzz";


const char* quadTessControlShader =
R"zzz(#version 410 core

in vec4 vs_light_direction_0[];
in vec4 vertex_position_world_0[];
uniform int innerLevel;
uniform int outerLevel;
uniform float tide_time;
uniform int isOceanMode;

out vec4 vs_light_direction_1[];
out vec4 vertex_position_world_1[];
out vec4 control_tide_center[];

layout (vertices = 4) out;
void main(void) {
	if(gl_InvocationID == 0) {
		gl_TessLevelInner[0] = 1.0 + innerLevel;
		gl_TessLevelInner[1] = 1.0 + innerLevel;
		gl_TessLevelOuter[0] = 1.0 + outerLevel;
		gl_TessLevelOuter[1] = 1.0 + outerLevel;
		gl_TessLevelOuter[2] = 1.0 + outerLevel;
		gl_TessLevelOuter[3] = 1.0 + outerLevel;
	}
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	vs_light_direction_1[gl_InvocationID] = vs_light_direction_0[gl_InvocationID];
	vertex_position_world_1[gl_InvocationID] = vertex_position_world_0[gl_InvocationID];

	// calculate tide center for adaptive tessellation
	float tide_speed = 5.0;
	vec4 tide_direct = vec4(1.0, 0.0, 0.0, 0.0);
	vec4 tide_start = vec4(0.0, 0.0, 0.0, 0.0);
	control_tide_center[gl_InvocationID] = tide_start + tide_direct * tide_time * tide_speed;
	vec4 curr_pos = vertex_position_world_0[gl_InvocationID];
	float adaptive_range = 5.0f;
	
	if(isOceanMode == 1 && distance(curr_pos, control_tide_center[gl_InvocationID]) < adaptive_range) {
		gl_TessLevelInner[0] *= 3;
		gl_TessLevelInner[1] *= 3;
		gl_TessLevelOuter[0] *= 3;
		gl_TessLevelOuter[1] *= 3;
		gl_TessLevelOuter[2] *= 3;
		gl_TessLevelOuter[3] *= 3;
	}
}

)zzz";


// const char* triangleTessEvaluationShader =
// R"zzz(#version 410 core
// in vec4 vs_light_direction_1[];
// in vec4 vertex_position_world_1[];

// out vec4 vs_light_direction;
// out vec4 vertex_position_world;


// layout(triangles, equal_spacing, cw) in;
// void main(void) {
// 	gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position
// 					+ gl_TessCoord.y * gl_in[1].gl_Position
// 					+ gl_TessCoord.z * gl_in[2].gl_Position);

// 	vs_light_direction = (gl_TessCoord.x * vs_light_direction_1[0]
// 					+ gl_TessCoord.y * vs_light_direction_1[1]
// 					+ gl_TessCoord.z * vs_light_direction_1[2]);

// 	vertex_position_world = (gl_TessCoord.x * vertex_position_world_1[0]
// 					+ gl_TessCoord.y * vertex_position_world_1[1]
// 					+ gl_TessCoord.z * vertex_position_world_1[2]);
// }

// )zzz";


const char* quadTessEvaluationShader =
R"zzz(#version 410 core
in vec4 vs_light_direction_1[];
in vec4 vertex_position_world_1[];
in vec4 control_tide_center[];

out vec4 vs_light_direction;
out vec4 vertex_position_world;
out vec4 eval_tide_center;



layout(quads, equal_spacing, cw) in;
void main(void) {
	vec4 p1 = mix(gl_in[1].gl_Position, gl_in[0].gl_Position, gl_TessCoord.x);
	vec4 p2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.x);
	gl_Position = mix(p1, p2, gl_TessCoord.y);

	vec4 light_dir1 = mix(vs_light_direction_1[1], vs_light_direction_1[0], gl_TessCoord.x);
	vec4 light_dir2 = mix(vs_light_direction_1[2], vs_light_direction_1[3], gl_TessCoord.x);
	vs_light_direction = mix(light_dir1, light_dir2, gl_TessCoord.y);


	vec4 vertex_pos1 = mix(vertex_position_world_1[1], vertex_position_world_1[0], gl_TessCoord.x);
	vec4 vertex_pos2 = mix(vertex_position_world_1[2], vertex_position_world_1[3], gl_TessCoord.x);
	vertex_position_world = mix(vertex_pos1, vertex_pos2, gl_TessCoord.y);

	vec4 tide_center_pos1 = mix(control_tide_center[1], control_tide_center[0], gl_TessCoord.x);
	vec4 tide_center_pos2 = mix(control_tide_center[2], control_tide_center[3], gl_TessCoord.x);
	eval_tide_center = mix(tide_center_pos1, tide_center_pos2, gl_TessCoord.y);


}

)zzz";


const char* geometry_shader =
R"zzz(#version 410 core


layout (triangles) in;
layout (triangle_strip, max_vertices = 4) out;
uniform mat4 view;
uniform mat4 projection;
uniform float elapsedTime;
uniform float tide_time;
uniform int isOceanMode;

in vec4 vs_light_direction[];
in vec4 vertex_position_world[];
in vec4 eval_tide_center[];



flat out vec4 normal;
out vec4 light_direction;
out vec4 vertex_position_world_;
out vec3 v_bycentric;
void main()
{
	mat4 inv = inverse(view);
	vec4 a = inv * vec4(gl_in[0].gl_Position.xyz, 1.0f);
	vec4 b = inv * vec4(gl_in[1].gl_Position.xyz, 1.0f);
	vec4 c = inv * vec4(gl_in[2].gl_Position.xyz, 1.0f);
	
	vec3 temp_a = vec3(a.x, a.y, a.z);
	vec3 temp_b = vec3(b.x, b.y, b.z);
	vec3 temp_c = vec3(c.x, c.y, c.z);
	// float area = length(cross(temp_b - temp_a, temp_c - temp_a));
	float area = length(cross(gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz,
								gl_in[0].gl_Position.xyz - gl_in[2].gl_Position.xyz));

	if(isOceanMode == 0) {
		int n = 0;
		
		normal = vec4(normalize(cross(temp_b - temp_a, temp_c - temp_a)), 1.0f);
		for (n = 0; n < gl_in.length(); n++) {
			light_direction = vs_light_direction[n];
			vertex_position_world_ = vertex_position_world[n];
			gl_Position = projection * gl_in[n].gl_Position;
			if(n == 0) {
				v_bycentric = vec3(1, 0, 0) * sqrt(area);
			}
			else if(n == 1) {
				v_bycentric = vec3(0, 1, 0) * sqrt(area);
			}
			else {
				v_bycentric = vec3(0, 0, 1) * sqrt(area);
			}
			EmitVertex();
		}
		EndPrimitive();
	}
	else {
		int n = 0;

		for (n = 0; n < gl_in.length(); n++) {
			light_direction = vs_light_direction[n];
			vertex_position_world_ = vertex_position_world[n];


			/*---------------------------------------------------------------------------------------------*/
			vec4 base_position = gl_in[n].gl_Position;


			// rewrite gl_Position to create waves

			float amp = 1.0;	// amplitude
			float waveLen = 2.0;	// crest-to-crest distance
			float w = 2.0 / waveLen;
			float speed = 2.0;
			float phi = speed * w;
			vec4 wave_dir = normalize(vec4(1.0, 0.0, 1.0, 0.0));	// x and z direction
			float Q = 2.0;	//Qi is a parameter that controls the steepness of the waves

			vec4 wave_pos = base_position;
			wave_pos.y += amp * sin(w * dot(wave_pos, wave_dir) + phi * elapsedTime);	// height

			
			float heightDiffX = w * wave_dir.x * amp * cos(dot(wave_dir, wave_pos) * w + phi * elapsedTime);	
			float heightDiffZ = w * wave_dir.z * amp * cos(dot(wave_dir, wave_pos) * w + phi * elapsedTime);	
			vec3 wave_normal = vec3(-heightDiffX, 1.0, -heightDiffZ);
			normal = vec4(wave_normal, 1);
			

			// // Gassian tide
			float PI = 3.14;
			float tide_speed = 5.0;
			float tide_amp = 20.0;
			float sigma = 2.0;

			vec4 curr_pos = vertex_position_world_;
			vec4 tide_center = eval_tide_center[n];

			float distance_square = dot(curr_pos - tide_center, curr_pos - tide_center);
			float tide_height = tide_amp * exp(- distance_square / (2.0 * sigma * sigma));

			wave_pos.y += tide_height;
			
			gl_Position = projection * wave_pos;

			/*---------------------------------------------------------------------------------------------*/



			
			if(n == 0) {
				v_bycentric = vec3(1, 0, 0) * sqrt(area);
			}
			else if(n == 1) {
				v_bycentric = vec3(0, 1, 0) * sqrt(area);
			}
			else {
				v_bycentric = vec3(0, 0, 1) * sqrt(area);
			}
			EmitVertex();
		}
		EndPrimitive();
	}
	
}
)zzz";





const char* fragment_shader =
R"zzz(#version 410 core

flat in vec4 normal;

in vec4 light_direction;
out vec4 fragment_color;
void main()
{
	vec4 color = abs(normal) + vec4(0.0, 0.0, 0.0, 1.0);
	float dot_nl = dot(normalize(light_direction), normal);
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
	


}
)zzz";

// FIXME: Implement shader effects with an alternative shader.
const char* floor_fragment_shader =
R"zzz(#version 410 core
flat in vec4 normal;

uniform float wireframeThresh;
uniform int isOceanMode;
uniform vec3 eye_position;
uniform vec4 light_position;

in vec3 v_bycentric;
in vec4 light_direction;
in vec4 vertex_position_world_;
out vec4 fragment_color;

void main()
{

	vec4 color;
	if(isOceanMode != 0) {
		color = vec4(normalize(vec3(0.0, 41.0, 58.0)), 1.0);
	}
	else {
		if (mod(floor(vertex_position_world_[0]) + floor(vertex_position_world_[2]), 2.0) == 0) {
			color = vec4(0.0, 0.0, 0.0, 1.0);
		} else {
			color = vec4(1.0, 1.0, 1.0, 1.0);
		}
	}

	// add specular effects
	vec3 water_ks = vec3(0.45, 0.45, 0.45);
	float alpha = 1.0;
	vec3 look_dir = normalize(eye_position - vertex_position_world_.xyz);
	vec3 light_pixel_dir = normalize(light_position.xyz - vertex_position_world_.xyz);
	vec3 R = 2 * dot(normal.xyz, light_pixel_dir) * normal.xyz - light_pixel_dir;
	color += vec4(water_ks * pow(max(0.0, dot(look_dir, R)), alpha), 0.0);


	float dot_nl = dot(normalize(light_direction), normalize(normal) );
	dot_nl = clamp(dot_nl, 0.0, 1.0);
	fragment_color = clamp(dot_nl * color, 0.0, 1.0);
	float minBc = min(min(v_bycentric.x, v_bycentric.y), v_bycentric.z);
	if(minBc < wireframeThresh) {
		fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
	}
}
)zzz";

std::vector<glm::vec4> obj_vertices;
std::vector<glm::uvec3> obj_faces;

float wireframeThresh = 0.0f;
auto polygonMode = GL_FILL;
int innerLevel = 0, outerLevel = 0;
int isOceanMode = 0;


auto start_time = std::chrono::system_clock::now();
float getElapsedTime() {
	auto end_time = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end_time - start_time;
	return elapsed_seconds.count();
}
float tideStartTime = getElapsedTime();

void
CreateTriangle(std::vector<glm::vec4>& vertices,
        std::vector<glm::uvec3>& indices)
{

	vertices.push_back(glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.5f, -0.5f, -0.5f, 1.0f));
	vertices.push_back(glm::vec4(0.0f, 0.5f, -0.5f, 1.0f));
	indices.push_back(glm::uvec3(0, 1, 2));
}

void 
toggleWireframe() {
	if(wireframeThresh == 0.0f) {
		wireframeThresh = 0.01f;
	}
	else {
		wireframeThresh = 0.0f;
	}
}


void
togglePolygonMode() {
	if(polygonMode == GL_FILL) {
		polygonMode = GL_LINE;
	}
	else {
		polygonMode = GL_FILL;
	}
	glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
}

void
toggleOceanMode() {
	if(isOceanMode == 0) {
		isOceanMode = 1;
	}
	else {
		isOceanMode = 0;
	}
}

// FIXME: Save geometry to OBJ file
void
SaveObj(const std::string& file,
        const std::vector<glm::vec4>& vertices,
        const std::vector<glm::uvec3>& indices)
{
	std::cout << "writing obj file" << std::endl;
	std::ofstream outfile;
	outfile.open(file);
	for(auto& v : vertices) {
		outfile << "v " << v.x << " " << v.y << " " << v.z << "\n";
	}
	// std::cout << "indices size: " << indices.size() << std::endl;
	for(auto& idx : indices) {
		// std::cout << "f " << idx.x + 1 << " " << idx.y + 1<< " " << idx.z + 1 << std::endl;
		outfile << "f " << idx.x + 1 << " " << idx.y + 1 << " " << idx.z + 1 << "\n";
	}
	outfile.close();
	std::cout << "write obj file done " << std::endl;
}

void
ErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << description << "\n";
}

std::shared_ptr<Menger> g_menger;
Camera g_camera;

void
KeyCallback(GLFWwindow* window,
            int key,
            int scancode,
            int action,
            int mods)
{
	// Note:
	// This is only a list of functions to implement.
	// you may want to re-organize this piece of code.
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else if (key == GLFW_KEY_S && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		// FIXME: save geometry to OBJ
		SaveObj("geometry.obj", obj_vertices, obj_faces);
	} else if (key == GLFW_KEY_W && action != GLFW_RELEASE) {
		// FIXME: WASD
		g_camera.keyZoom(1);
	} else if (key == GLFW_KEY_S && action != GLFW_RELEASE) {
		g_camera.keyZoom(-1);
	} else if (key == GLFW_KEY_A && action != GLFW_RELEASE) {
		g_camera.moveHorizontal(-1);
	} else if (key == GLFW_KEY_D && action != GLFW_RELEASE) {
		g_camera.moveHorizontal(1);
	} else if (key == GLFW_KEY_LEFT && action != GLFW_RELEASE) {
		// FIXME: Left Right Up and Down
		g_camera.roll(-1);
	} else if (key == GLFW_KEY_RIGHT && action != GLFW_RELEASE) {
		g_camera.roll(1);
	} else if (key == GLFW_KEY_DOWN && action != GLFW_RELEASE) {
		g_camera.moveVertical(-1);
	} else if (key == GLFW_KEY_UP && action != GLFW_RELEASE) {
		g_camera.moveVertical(1);
	} else if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
		// FIXME: FPS mode on/off
		g_camera.toggleFPS();
	} else if(key ==  GLFW_KEY_F && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		togglePolygonMode();
	} else if(key == GLFW_KEY_F && mods != GLFW_MOD_CONTROL && action != GLFW_RELEASE) {
		toggleWireframe();
	} else if(key == GLFW_KEY_O && mods == GLFW_MOD_CONTROL && action == GLFW_RELEASE) {
		toggleOceanMode();
	} else if(key == GLFW_KEY_MINUS && action != GLFW_RELEASE) {
		outerLevel = std::max(0, outerLevel - 1);
	} else if(key == GLFW_KEY_EQUAL && action != GLFW_RELEASE) {
		outerLevel = outerLevel + 1;
	} else if(key == GLFW_KEY_COMMA && action != GLFW_RELEASE) {
		innerLevel = std::max(0, innerLevel - 1);
	} else if(key == GLFW_KEY_PERIOD && action != GLFW_RELEASE) {
		innerLevel = innerLevel + 1;
	} else if(key == GLFW_KEY_T && action != GLFW_RELEASE) {
		tideStartTime = getElapsedTime();
		std::cout << "tide start time updated. value: " << tideStartTime << std::endl;
	}






	if (!g_menger)
		return ; // 0-4 only available in Menger mode.
	if (key == GLFW_KEY_0 && action != GLFW_RELEASE) {
		// FIXME: Change nesting level of g_menger
		// Note: GLFW_KEY_0 - 4 may not be continuous.
		g_menger->set_nesting_level(0);
	} else if (key == GLFW_KEY_1 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(1);
	} else if (key == GLFW_KEY_2 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(2);
	} else if (key == GLFW_KEY_3 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(3);
	} else if (key == GLFW_KEY_4 && action != GLFW_RELEASE) {
		g_menger->set_nesting_level(4);
	}
}

int g_current_button;
bool g_mouse_pressed;
bool mouse_clicked = false;



void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
	if(!g_mouse_pressed)
		return;
	if(mouse_clicked) {
		g_camera.setMouseCoord(mouse_x, mouse_y);
		mouse_clicked = false;
	}
	if(g_current_button == GLFW_MOUSE_BUTTON_LEFT) {
		g_camera.rotate(mouse_x, mouse_y);
	} 
	else if(g_current_button == GLFW_MOUSE_BUTTON_RIGHT) {
		g_camera.mouseZoom(mouse_y);
	}
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	mouse_clicked = true;
	g_mouse_pressed = (action == GLFW_PRESS);
	g_current_button = button;
}

// void make_floor(std::vector<glm::vec4>& floor_vertices,
// 					 std::vector<glm::uvec3>& floor_faces) 
// {
// 	float max = 1000.0f;
// 	float min = -1000.0f;

// 	floor_vertices.push_back(glm::vec4(min, -2.0f, min, 1.0f));
// 	floor_vertices.push_back(glm::vec4(min, -2.0f, max, 1.0f));
// 	floor_vertices.push_back(glm::vec4(max, -2.0f, max, 1.0f));
// 	floor_vertices.push_back(glm::vec4(max, -2.0f, min, 1.0f));

// 	floor_faces.push_back(glm::uvec3(0, 1, 2));
// 	floor_faces.push_back(glm::uvec3(3, 4, 5));
// }

unsigned int getVertexIdx(int x, int y, int width) {
	return (unsigned int) x * width + y;
}

void make_floor(std::vector<glm::vec4>& floor_vertices,
					 std::vector<glm::uvec4>& floor_faces) {
	float min = -20.0f, max = 20.0f;
	int fragmentNums = 16;
	float step = (max - min) / fragmentNums;
	// push vertices
	for(int x = 0; x <= fragmentNums; x++) {
		for(int y = 0; y <= fragmentNums; y++) {
			floor_vertices.push_back(glm::vec4(min + step * x, -2.0f, min + step * y, 1.0f));
		}
	}
	// push faces
	for(int x = 0; x < fragmentNums; x++) {
		for(int y = 0; y < fragmentNums; y++) {
			floor_faces.push_back(glm::uvec4(
				getVertexIdx(x, y, fragmentNums + 1),
				getVertexIdx(x, y + 1, fragmentNums + 1),
				getVertexIdx(x + 1, y + 1, fragmentNums + 1),
				getVertexIdx(x + 1, y, fragmentNums + 1)
			));
		}
	}

	// float max = 20.0f;
	// float min = -20.0f;
	// float planey = -2.0f;

	// float step = float((max - min) / 16.0);

	// for (int i = min; i < max; i += step) {
	// 	for (int j = min; j < max; j += step) {
	// 		floor_vertices.push_back(glm::vec4(i, planey, j, 1.0f));
	// 		floor_vertices.push_back(glm::vec4(i + step, planey, j, 1.0f));
	// 		floor_vertices.push_back(glm::vec4(i + step, planey, j + step, 1.0f));
	// 		floor_vertices.push_back(glm::vec4(i, planey, j + step, 1.0f));

	// 		int v = floor_vertices.size();
	// 		floor_faces.push_back(glm::vec4(v, v+1, v+2, v+3));
	// 	}
	// }
}



int main(int argc, char* argv[])
{
	float elapsedTime = getElapsedTime();	// in miliseconds
	std::cout << "elapsedTime: " << elapsedTime << std::endl;
 

	std::string window_title = "Menger";
	if (!glfwInit()) exit(EXIT_FAILURE);
	g_menger = std::make_shared<Menger>(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, 0.5));
	glfwSetErrorCallback(ErrorCallback);

	// Ask an OpenGL 4.1 core profile context
	// It is required on OSX and non-NVIDIA Linux
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(window_width, window_height,
			&window_title[0], nullptr, nullptr);
	CHECK_SUCCESS(window != nullptr);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;

	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MousePosCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	

        //FIXME: Create the geometry from a Menger object.
        //CreateTriangle(obj_vertices, obj_faces);

	g_menger->set_nesting_level(0);
	g_menger->generate_geometry(obj_vertices, obj_faces);

	glm::vec4 min_bounds = glm::vec4(std::numeric_limits<float>::max());
	glm::vec4 max_bounds = glm::vec4(-std::numeric_limits<float>::max());
	for (const auto& vert : obj_vertices) {
		min_bounds = glm::min(vert, min_bounds);
		max_bounds = glm::max(vert, max_bounds);
	}
	std::cout << "min_bounds = " << glm::to_string(min_bounds) << "\n";
	std::cout << "max_bounds = " << glm::to_string(max_bounds) << "\n";

	// Setup our VAO array.
	CHECK_GL_ERROR(glGenVertexArrays(kNumVaos, &g_array_objects[0]));

	// Switch to the VAO for Geometry.
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

	// Generate buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kGeometryVao][0]));

	// Setup vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * obj_vertices.size() * 4, obj_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * obj_faces.size() * 3,
				obj_faces.data(), GL_STATIC_DRAW));

	/*
 	 * By far, the geometry is loaded into g_buffer_objects[kGeometryVao][*].
	 * These buffers are binded to g_array_objects[kGeometryVao]
	 */

	// FIXME: load the floor into g_buffer_objects[kFloorVao][*],
	//        and bind these VBO to g_array_objects[kFloorVao]
	std::vector<glm::vec4> floor_vertices;
	std::vector<glm::uvec4> floor_faces;

	make_floor(floor_vertices, floor_faces);

	// Switch to the VAO for floor
	CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));

	// Generate floor buffer objects
	CHECK_GL_ERROR(glGenBuffers(kNumVbos, &g_buffer_objects[kFloorVao][0]));

	// Setup floor vertex data in a VBO.
	CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kVertexBuffer]));
	// NOTE: We do not send anything right now, we just describe it to OpenGL.
	CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
				sizeof(float) * floor_vertices.size() * 4, floor_vertices.data(),
				GL_STATIC_DRAW));
	CHECK_GL_ERROR(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
	CHECK_GL_ERROR(glEnableVertexAttribArray(0));

	// Setup floor element array buffer.
	CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kFloorVao][kIndexBuffer]));
	CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(uint32_t) * floor_faces.size() * 4,
				floor_faces.data(), GL_STATIC_DRAW));

	// Setup vertex shader.
	GLuint vertex_shader_id = 0;
	const char* vertex_source_pointer = vertex_shader;
	CHECK_GL_ERROR(vertex_shader_id = glCreateShader(GL_VERTEX_SHADER));
	CHECK_GL_ERROR(glShaderSource(vertex_shader_id, 1, &vertex_source_pointer, nullptr));
	glCompileShader(vertex_shader_id);
	CHECK_GL_SHADER_ERROR(vertex_shader_id);


	/*---------------------our codes ---------------------------*/
	// Setup tessllation control shader
	GLuint tess_control_shader_id = 0;
	// const char* tess_control_source_pointer = triangleTessControlShader;
	// CHECK_GL_ERROR(tess_control_shader_id = glCreateShader(GL_TESS_CONTROL_SHADER));
	// CHECK_GL_ERROR(glShaderSource(tess_control_shader_id, 1, &tess_control_source_pointer, nullptr));
	// glCompileShader(tess_control_shader_id);
	// CHECK_GL_SHADER_ERROR(tess_control_shader_id);
	const char* tess_control_source_pointer = quadTessControlShader;
	CHECK_GL_ERROR(tess_control_shader_id = glCreateShader(GL_TESS_CONTROL_SHADER));
	CHECK_GL_ERROR(glShaderSource(tess_control_shader_id, 1, &tess_control_source_pointer, nullptr));
	glCompileShader(tess_control_shader_id);
	CHECK_GL_SHADER_ERROR(tess_control_shader_id);


	// Setup tessllation evaluation shader
	GLuint tess_eval_shader_id = 0;
	// const char* tess_eval_source_pointer = triangleTessEvaluationShader;
	// CHECK_GL_ERROR(tess_eval_shader_id = glCreateShader(GL_TESS_EVALUATION_SHADER));
	// CHECK_GL_ERROR(glShaderSource(tess_eval_shader_id, 1, &tess_eval_source_pointer, nullptr));
	// glCompileShader(tess_eval_shader_id);
	// CHECK_GL_SHADER_ERROR(tess_eval_shader_id);
	const char* tess_eval_source_pointer = quadTessEvaluationShader;
	CHECK_GL_ERROR(tess_eval_shader_id = glCreateShader(GL_TESS_EVALUATION_SHADER));
	CHECK_GL_ERROR(glShaderSource(tess_eval_shader_id, 1, &tess_eval_source_pointer, nullptr));
	glCompileShader(tess_eval_shader_id);
	CHECK_GL_SHADER_ERROR(tess_eval_shader_id);

	// Setup geometry shader.
	GLuint geometry_shader_id = 0;
	const char* geometry_source_pointer = geometry_shader;
	CHECK_GL_ERROR(geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER));
	CHECK_GL_ERROR(glShaderSource(geometry_shader_id, 1, &geometry_source_pointer, nullptr));
	glCompileShader(geometry_shader_id);
	CHECK_GL_SHADER_ERROR(geometry_shader_id);

	// Setup ocean geometry shader.
	// GLuint ocean_geometry_shader_id = 0;
	// const char* ocean_geometry_source_pointer = ocean_geometry_shader;
	// CHECK_GL_ERROR(ocean_geometry_shader_id = glCreateShader(GL_GEOMETRY_SHADER));
	// CHECK_GL_ERROR(glShaderSource(ocean_geometry_shader_id, 1, &ocean_geometry_source_pointer, nullptr));
	// glCompileShader(ocean_geometry_shader_id);
	// CHECK_GL_SHADER_ERROR(ocean_geometry_shader_id);


	// Setup fragment shader.
	GLuint fragment_shader_id = 0;
	const char* fragment_source_pointer = fragment_shader;
	CHECK_GL_ERROR(fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(fragment_shader_id, 1, &fragment_source_pointer, nullptr));
	glCompileShader(fragment_shader_id);
	CHECK_GL_SHADER_ERROR(fragment_shader_id);

	// Let's create our program.
	GLuint program_id = 0;
	CHECK_GL_ERROR(program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(program_id, vertex_shader_id));
	CHECK_GL_ERROR(glAttachShader(program_id, fragment_shader_id));
	CHECK_GL_ERROR(glAttachShader(program_id, geometry_shader_id));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(program_id, 0, "fragment_color"));
	glLinkProgram(program_id);
	CHECK_GL_PROGRAM_ERROR(program_id);

	// Get the uniform locations.
	GLint projection_matrix_location = 0;
	CHECK_GL_ERROR(projection_matrix_location = 
			glGetUniformLocation(program_id, "projection"));
	GLint view_matrix_location = 0;
	CHECK_GL_ERROR(view_matrix_location =
			glGetUniformLocation(program_id, "view"));
	GLint light_position_location = 0;
	CHECK_GL_ERROR(light_position_location =
			glGetUniformLocation(program_id, "light_position"));

	// Setup fragment shader for the floor
	GLuint floor_fragment_shader_id = 0;
	const char* floor_fragment_source_pointer = floor_fragment_shader;
	CHECK_GL_ERROR(floor_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER));
	CHECK_GL_ERROR(glShaderSource(floor_fragment_shader_id, 1,
				&floor_fragment_source_pointer, nullptr));
	glCompileShader(floor_fragment_shader_id);
	CHECK_GL_SHADER_ERROR(floor_fragment_shader_id);

	// FIXME: Setup another program for the floor, and get its locations.
	// Note: you can reuse the vertex and geometry shader objects
	// GLuint floor_program_id = 0;
	// GLint floor_projection_matrix_location = 0;
	// GLint floor_view_matrix_location = 0;
	// GLint floor_light_position_location = 0;

	GLuint floor_program_id = 0;
	CHECK_GL_ERROR(floor_program_id = glCreateProgram());
	CHECK_GL_ERROR(glAttachShader(floor_program_id, vertex_shader_id));

	/* our shaders attached here */
	// CHECK_GL_ERROR(glAttachShader(floor_program_id, tess_control_shader_id));
	// CHECK_GL_ERROR(glAttachShader(floor_program_id, tess_eval_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, tess_control_shader_id));
	CHECK_GL_ERROR(glAttachShader(floor_program_id, tess_eval_shader_id));
	
	CHECK_GL_ERROR(glAttachShader(floor_program_id, geometry_shader_id));
	// CHECK_GL_ERROR(glAttachShader(floor_program_id, ocean_geometry_shader_id));

	CHECK_GL_ERROR(glAttachShader(floor_program_id, floor_fragment_shader_id));

	// Bind attributes.
	CHECK_GL_ERROR(glBindAttribLocation(floor_program_id, 0, "vertex_position"));
	CHECK_GL_ERROR(glBindFragDataLocation(floor_program_id, 0, "fragment_color"));
	glLinkProgram(floor_program_id);
	CHECK_GL_PROGRAM_ERROR(floor_program_id);

	// Get the uniform locations.
	GLint floor_projection_matrix_location = 0;
	CHECK_GL_ERROR(floor_projection_matrix_location =
			glGetUniformLocation(floor_program_id, "projection"));
	GLint floor_view_matrix_location = 0;
	CHECK_GL_ERROR(floor_view_matrix_location =
			glGetUniformLocation(floor_program_id, "view"));
	GLint eye_position_location = 0;
	CHECK_GL_ERROR(eye_position_location =
			glGetUniformLocation(floor_program_id, "eye_position"));

	GLint floor_light_position_location = 0;
	CHECK_GL_ERROR(floor_light_position_location =
			glGetUniformLocation(floor_program_id, "light_position"));
	GLint floor_wireframe_thresh_location = 0;
	CHECK_GL_ERROR(floor_wireframe_thresh_location =
			glGetUniformLocation(floor_program_id, "wireframeThresh"));
	GLint tess_inner_level_location = 0;
	CHECK_GL_ERROR(tess_inner_level_location =
			glGetUniformLocation(floor_program_id, "innerLevel"));
	GLint tess_outer_level_location = 0;
	CHECK_GL_ERROR(tess_outer_level_location =
			glGetUniformLocation(floor_program_id, "outerLevel"));
	GLint elapsed_time_location = 0;
	CHECK_GL_ERROR(elapsed_time_location =
			glGetUniformLocation(floor_program_id, "elapsedTime"));
	GLint tide_time_location = 0;
	CHECK_GL_ERROR(tide_time_location =
			glGetUniformLocation(floor_program_id, "tide_time"));
	GLint is_ocean_mode_location = 0;
	CHECK_GL_ERROR(is_ocean_mode_location =
			glGetUniformLocation(floor_program_id, "isOceanMode"));





	// glm::vec4 light_position = glm::vec4(10.0f, 10.0f, 10.0f, 1.0f);
	glm::vec4 light_position = glm::vec4(-10.0f, 10.0f, 0.0f, 1.0f);
	float aspect = 0.0f;
	float theta = 0.0f;
	while (!glfwWindowShouldClose(window)) {
		elapsedTime = getElapsedTime();
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);

		// Switch to the Geometry VAO.
		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kGeometryVao]));

		if (g_menger && g_menger->is_dirty()) {
			g_menger->generate_geometry(obj_vertices, obj_faces);
			g_menger->set_clean();

			// FIXME: Upload your vertex data here.
			CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kVertexBuffer]));
			CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER,
		                            sizeof(float) * obj_vertices.size() * 4,
		                            obj_vertices.data(), GL_STATIC_DRAW));
			CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_buffer_objects[kGeometryVao][kIndexBuffer]));
			CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
									sizeof(uint32_t) * obj_faces.size() * 3,
									obj_faces.data(), GL_STATIC_DRAW));
		}

		// Compute the projection matrix.
		aspect = static_cast<float>(window_width) / window_height;
		glm::mat4 projection_matrix =
			glm::perspective(glm::radians(45.0f), aspect, 0.0001f, 1000.0f);

		// Compute the view matrix
		// FIXME: change eye and center through mouse/keyboard events.
		glm::mat4 view_matrix = g_camera.get_view_matrix();
		glm::vec3 eye_position = g_camera.get_eye_position();

		// Use our program.
		CHECK_GL_ERROR(glUseProgram(program_id));

		// Pass uniforms in.
		CHECK_GL_ERROR(glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(light_position_location, 1, &light_position[0]));

		// Draw our triangles.
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, obj_faces.size() * 3, GL_UNSIGNED_INT, 0));

		// FIXME: Render the floor
		// Note: What you need to do is
		// 	1. Switch VAO
		// 	2. Switch Program
		// 	3. Pass Uniforms
		// 	4. Call glDrawElements, since input geometry is
		// 	indicated by VAO.

		CHECK_GL_ERROR(glBindVertexArray(g_array_objects[kFloorVao]));
		
		CHECK_GL_ERROR(glUseProgram(floor_program_id));

		CHECK_GL_ERROR(glUniformMatrix4fv(floor_projection_matrix_location, 1, GL_FALSE,
					&projection_matrix[0][0]));
		CHECK_GL_ERROR(glUniformMatrix4fv(floor_view_matrix_location, 1, GL_FALSE,
					&view_matrix[0][0]));
		CHECK_GL_ERROR(glUniform4fv(floor_light_position_location, 1, &light_position[0]));

		CHECK_GL_ERROR(glUniform3fv(eye_position_location, 1, &eye_position[0]));

		CHECK_GL_ERROR(glUniform1f(floor_wireframe_thresh_location, wireframeThresh));

		CHECK_GL_ERROR(glUniform1i(tess_inner_level_location, innerLevel));

		CHECK_GL_ERROR(glUniform1i(tess_outer_level_location, outerLevel));

		CHECK_GL_ERROR(glUniform1f(elapsed_time_location, elapsedTime));	// elapsed time for waves
		
		CHECK_GL_ERROR(glUniform1f(tide_time_location, elapsedTime - tideStartTime));	// elapsed time for waves

		CHECK_GL_ERROR(glUniform1i(is_ocean_mode_location, isOceanMode));





		// std::cout << "tide time: " << elapsedTime - tideStartTime << std::endl;

		glPatchParameteri(GL_PATCH_VERTICES, 4);
		CHECK_GL_ERROR(glDrawElements(GL_PATCHES, floor_faces.size() * 4, GL_UNSIGNED_INT, 0));


		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
