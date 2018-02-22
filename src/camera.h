#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
	glm::mat4 get_view_matrix() const;
	// FIXME: add functions to manipulate camera objects.
	void yaw(float direct);
	void pitch(float direct);
	void roll(float direct);
	void zoom(float direct);
	void translate(glm::vec2 direct);



private:
	float camera_distance_ = 3.0;
	glm::vec3 look_ = glm::vec3(0.0f, 0.0f, -1.0f);	// direction
	glm::vec3 up_ = glm::vec3(0.0f, 1.0, 0.0f);	// direction
	glm::vec3 eye_ = glm::vec3(0.0f, 0.0f, camera_distance_);	// location
	// Note: you may need additional member variables
	glm::vec3 right_ = glm::vec3(1.0f, 0.0f, 0.0f);	// direction
};

#endif
