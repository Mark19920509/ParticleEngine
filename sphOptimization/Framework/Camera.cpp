#include "Camera.h"
#include <math.h>

const float Camera::speedRotation = 6.0f;
const float Camera::speedRotationUpDown = 400.0f;
const float Camera::speedZoom = 2000.0f;

Camera::Camera():m_DistanceFromTarget(20.0f), m_AngleHeight(0.0f),m_AngleY(0.0f),
    position(0,800,0), speed(0.0f, 0.0f, 0.0f),
    springConstant(0.6f), damping(0.3f)
{
}

Camera::~Camera()
{
}

void Camera::setPointToFollow(const slmath::vec3& pointToFollow)
{
	this->pointToFollow = pointToFollow;
}

void Camera::setPosition(const slmath::vec3& position) 
{
	this->position = position;
}

slmath::mat4 Camera::getViewMatrix()const 
{
	return viewMatrix;
}

slmath::vec3 Camera::getPosition()const 
{
	return position;
}

//void Camera::move(float /*frameDuration*/) 
//{
//    assert(position != pointToFollow);
//
//    // Position at the right distance
//	// position = pointToFollow  + slmath::vec3(0.0f, 0.0f, 1.0f) * m_DistanceFromTarget;
//
//	
//    slmath::vec3 direction = position - pointToFollow;
//    direction = normalize(direction);
//
//    pointToFollow += m_DistanceFromTarget * direction;
//    m_DistanceFromTarget = 0.0f;
//
//    const float fixLength = 3.0f;
//
//    position = pointToFollow + direction * fixLength;
//
//    //Rotate from x axis
//    slmath::vec3 rotationPoint = position - pointToFollow;
//    slmath::mat4 rotation = slmath::rotationX(m_AngleHeight);
//     rotationPoint = slmath::mul(rotation, slmath::vec4(rotationPoint, 1.0f)).xyz();
//     position = position + rotationPoint;
//     m_AngleHeight = 0.0f;
//     //Rotate from y axis
//    rotationPoint = position - pointToFollow;
//    rotation = slmath::rotationY(m_AngleY);
//    rotationPoint = slmath::mul(rotation, slmath::vec4(rotationPoint, 1.0f)).xyz();
//    position = position + rotationPoint;
//    m_AngleY = 0.0f;
//
//	// Set up the view matrix
//	viewMatrix = slmath::lookAtLH(position, pointToFollow, slmath::vec3(0.0f, 1.0f, 0.0f));
//
//}

void Camera::move(float /*frameDuration*/) 
{
    assert(position != pointToFollow);

    // Position at the right distance
	position = pointToFollow  + slmath::vec3(0.0f, 0.0f, 1.0f) * m_DistanceFromTarget;
	
    //Rotate from x axis
    slmath::mat4 rotation = slmath::rotationX(m_AngleHeight);
     position = slmath::mul(rotation, slmath::vec4(position, 1.0f)).xyz();

     //Rotate from y axis
    rotation = slmath::rotationY(m_AngleY);
    position = slmath::mul(rotation, slmath::vec4(position, 1.0f)).xyz();

	// Set up the view matrix
	viewMatrix = slmath::lookAtLH(position, pointToFollow, slmath::vec3(0.0f, 1.0f, 0.0f));

}

void Camera::zoomUp(float frameDuration) 
{
	m_DistanceFromTarget -= frameDuration*speedZoom;
}
void Camera::zoomDown(float frameDuration)
{
	m_DistanceFromTarget += frameDuration*speedZoom;
}

void Camera::moveUp(float frameDuration) 
{
    m_AngleHeight += frameDuration*speedRotation;
}
void Camera::moveDown(float frameDuration)
{
    m_AngleHeight -= frameDuration*speedRotation;
}
void Camera::moveRight(float frameDuration)
{
	 m_AngleY -= frameDuration*speedRotation;
}
void Camera::moveLeft(float frameDuration)
{
	 m_AngleY += frameDuration*speedRotation;
}

float Camera::checkPositionMax(const float position, const float pointToFollow, const float distanceMax) 
{
	if (abs(position - pointToFollow) > distanceMax) {
		if (position - pointToFollow < 0) {
			return pointToFollow - distanceMax;
		} else {
			return pointToFollow + distanceMax;
		}
	}
	return position;
}

float Camera::checkPositionMin(const float position, const float pointToFollow, const float distanceMin) 
{
	if (abs(position - pointToFollow) < distanceMin) 
    {
        return pointToFollow + distanceMin;
	}
	return position;
}
