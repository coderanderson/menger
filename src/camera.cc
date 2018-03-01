#include <fstream>
#include <iostream>
#include "camera.h"

namespace {
	float pan_speed = 0.1f;
	float roll_speed = 0.1f;
	float rotation_speed = 0.02f;
	float zoom_speed = 0.1f;
};

glm::mat4 Camera::get_view_matrix() const{
	glm::vec3 Z = glm::normalize(eye_ - center_);
	glm::vec3 Y = up_;
	glm::vec3 X = glm::cross(Y, Z);
	Y = glm::normalize(glm::cross(Z, X));
	X = glm::normalize(X);
	glm::mat4 res(1.0f);
	res[0] = glm::vec4(X.x, Y.x, Z.x, 0);
	res[1] = glm::vec4(X.y, Y.y, Z.y, 0);
	res[2] = glm::vec4(X.z, Y.z, Z.z, 0);
	res[3] = glm::vec4(glm::dot(-X, eye_), glm::dot(-Y, eye_), glm::dot(-Z, eye_), 1);
	return res;
}

glm::vec3
Camera::get_eye_position() const {
	return eye_;
}

void
Camera::toggleFPS() {
	fps_on = !fps_on;
	std::cout << "fps toggled. now on? " << fps_on << std::endl;
}


void
Camera::setMouseCoord(float x, float y) {
	prev_mouse_x = x;
	prev_mouse_y = y;
}

void Camera::rotate(float mouse_x, float mouse_y) {
	float delta_x = mouse_x - prev_mouse_x, delta_y = mouse_y - prev_mouse_y;
	setMouseCoord(mouse_x, mouse_y);
	if(fps_on) {
		look_ = glm::rotate(look_, delta_x * rotation_speed, up_);
		right_ = glm::normalize(glm::cross(look_, up_));
		look_ = glm::rotate(look_, delta_y * rotation_speed, right_);
		up_ = glm::normalize(glm::cross(look_, -right_));
		center_ = look_ * camera_distance_ + eye_;
	}
	else {
		eye_ = glm::rotate(eye_, delta_x * rotation_speed, up_);
		look_ = center_ - eye_;
		right_ = glm::normalize(glm::cross(look_, up_));
		eye_ = glm::rotate(eye_, delta_y * rotation_speed, right_);
		look_ = center_ - eye_;
		up_ = glm::normalize(glm::cross(look_, -right_));
	}
	look_ = glm::normalize(look_);
}

void Camera::roll(float direct) {	// direct positive: right. negative: left.
	up_ = glm::normalize(glm::rotate(up_, roll_speed * direct, look_));
  	right_ = glm::normalize(glm::cross(look_, up_));
}

void
Camera::moveHorizontal(float direct) {
	center_ += right_ * pan_speed * direct;
	eye_ += right_ * pan_speed * direct;
}

void
Camera::moveVertical(float direct) {
    center_ += up_ * pan_speed * direct;
    eye_ += up_ * pan_speed * direct;
}

void Camera::mouseZoom(float mouse_y) {
   	eye_ += look_ * (zoom_speed * (prev_mouse_y - mouse_y));
	camera_distance_ = glm::length(center_ - eye_);
	setMouseCoord(0.0f, mouse_y);
}

void Camera::keyZoom(float direct) {
	glm::vec3 move = look_ * zoom_speed;
	if(fps_on) {
		eye_ += direct * move;
		center_ += direct * move;
	}
	else {
		float distance = glm::length(center_ - eye_);
		camera_distance_ = distance - zoom_speed * direct;
		eye_ += direct * move;
	}
}

