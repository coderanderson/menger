#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

class Camera {
public:
	glm::mat4 get_view_matrix() const;
	void toggleFPS();
	void setMouseCoord(float mouse_x, float mouse_y);
	void rotate(float mouse_x, float mouse_y);
	void roll(float direct);
	void moveHorizontal(float direct);
	void moveVertical(float direct);
	void mouseZoom(float mouse_y);
	void keyZoom(float direct);
	
private:
	// float camera_distance_ = 3.0;
	
	// glm::vec3 eye_ = glm::vec3(0.0f, 0.0f, camera_distance_);
	glm::vec3 center_ = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 eye_ = glm::vec3(0.0f, 10.0f, 10.0f);
	glm::vec3 look_ = glm::normalize(center_ - eye_);
	glm::vec3 right_ = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 up_ = glm::normalize(glm::cross(right_, look_));
	float camera_distance_ = glm::length(center_ - eye_);

	bool fps_on = false;
	float prev_mouse_x;
	float prev_mouse_y;
	
};

#endif
