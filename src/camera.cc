#include <glm/gtx/rotate_vector.hpp>
#include "camera.h"
#include <iostream>

using namespace std;

namespace {
	float pan_speed = 0.1f;
	float roll_speed = 0.1f;
	float rotation_speed = 0.05f;
	float zoom_speed = 0.1f;
};


void Camera::yaw(float direct) {	// rotate | up_ direction
	glm::vec3 focus = eye_ + camera_distance_ * look_;	// forcus not change
	glm::mat4 rotateMat = glm::rotate(direct * rotation_speed, up_);
	eye_ = glm::vec3(rotateMat * glm::vec4(eye_, 1));
	look_ = glm::normalize(focus - eye_);
	right_ = glm::normalize(glm::cross(look_, up_));
}

void Camera::pitch(float direct) {	// rotate | right_ direction. right = cross(look_, up_)
	glm::vec3 focus = eye_ + camera_distance_ * look_;
	glm::mat4 rotateMat = glm::rotate(direct * rotation_speed, right_);
	eye_ = glm::vec3(rotateMat * glm::vec4(eye_, 1));
	look_ = glm::normalize(focus - eye_);
	up_ = glm::normalize(glm::cross(right_, look_));
}

void Camera::roll(float direct) {	// rotate | look_ direction
	glm::mat4 rotateMat = glm::rotate(direct * roll_speed, look_);
	up_ = glm::normalize(glm::vec3(rotateMat * glm::vec4(up_, 0.0f)));
	right_ = glm::normalize(glm::vec3(rotateMat * glm::vec4(right_, 0.0f)));
}

void Camera::zoom(float direct) {
	camera_distance_ += direct * zoom_speed;
	eye_ = glm::vec3(0.0f, 0.0f, camera_distance_);
}

void Camera::translate(glm::vec2 direct) {
	glm::mat4 translateMat = glm::translate((direct.x * right_ + direct.y * up_) * pan_speed);
	eye_ = glm::vec3(translateMat * glm::vec4(eye_, 1));
}


// FIXME: Calculate the view matrix
glm::mat4 Camera::get_view_matrix() const
{
	glm::vec3 Z = - glm::normalize(look_);
	glm::vec3 X = glm::normalize(glm::cross(up_, Z));
	glm::vec3 Y = glm::normalize(glm::cross(Z, X));

	glm::mat4 res(1.0f);
	res[0] = glm::vec4(X.x, Y.x, Z.x, 0);
	res[1] = glm::vec4(X.y, Y.y, Z.y, 0);
	res[2] = glm::vec4(X.z, Y.z, Z.z, 0);
	res[3] = glm::vec4(glm::dot(-X, eye_), glm::dot(-Y, eye_), glm::dot(-Z, eye_), 1);
	for(int i = 0; i < 4; i++) {
		cout << res[i].x << ", " << res[i].y << ", " << res[i].z << ", " << 0 << endl;
	}
	cout << "\n" << endl;
	return res;
}
