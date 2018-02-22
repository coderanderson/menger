#include "camera.h"
#include <iostream>
#include <sstream>
#include <glm/gtx/rotate_vector.hpp>

namespace {
	float pan_speed = 0.1f;
	float roll_speed = 0.1f;
	float rotation_speed = 0.05f;
	float zoom_speed = 0.1f;
};

void Camera::yaw(float dir) {
	rotate *= glm::rotate(rotation_speed * dir, up_);
	glm::vec3 eye(rotate * glm::vec4(eye_, 1));
	look_ = glm::normalize(middle_ - eye);
	right_ = glm::normalize(glm::cross(look_, up_));
	up_ = glm::normalize(glm::cross(right_, look_));
}

void Camera::pitch(float dir) {
	rotate = glm::rotate(rotation_speed*dir, right_) * rotate;
	glm::vec3 eye(rotate * glm::vec4(eye_, 1));
	look_ = glm::normalize(middle_ - eye);
	right_ = glm::normalize(glm::cross(look_, up_));
	up_ = glm::normalize(glm::cross(right_, look_));
}

void Camera::roll(float dir) {
	glm::mat4 roll_rotate = glm::rotate(dir * roll_speed, look_);
	right_ = glm::normalize(glm::vec3(roll_rotate * glm::vec4(right_, 0.0f)));
	up_ = glm::normalize(glm::vec3(roll_rotate * glm::vec4(up_, 0.0f)));
}

void Camera::trans(glm::vec2 dir) {
	translate *= glm::translate((dir.y * up_ + dir.x * right_) * pan_speed);
}

void Camera::zoom(float dir) {
	camera_distance_ += zoom_speed * dir;
	eye_ = glm::vec3(0.0f, 0.0f, camera_distance_);
}

glm::mat4 Camera::get_view_matrix() const {
	glm::vec3 newEye(translate * rotate * glm::vec4(eye_, 1));
	glm::vec3 newmiddle(translate * rotate * glm::vec4(middle_, 1));

	glm::vec3 Z = glm::normalize(newEye - newmiddle);
	glm::vec3 X = glm::cross(up_, Z);
	glm::vec3 Y = glm::normalize(glm::cross(Z, X));
	X = glm::normalize(X);

	glm::mat4 eye(1.0f);
	eye[0] = glm::vec4(X.x, Y.x, Z.x, 0);
	eye[1] = glm::vec4(X.y, Y.y, Z.y, 0);
	eye[2] = glm::vec4(X.z, Y.z, Z.z, 0);
	eye[3] = glm::vec4(glm::dot(-X, newEye), glm::dot(-Y, newEye), glm::dot(-Z, newEye), 1);

	return eye;
}
