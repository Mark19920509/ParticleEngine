#include "Camera.h"
#include <math.h>

const float Camera::speedRotation = 6.0f;


Camera::Camera()
 : m_DistanceFromTarget(20.0f)
 , m_AngleHeight(0.0f)
 , m_AngleHorizon(0.0f)
 , m_Position(0,800,0)
{
}

Camera::~Camera()
{
}

void Camera::setPointToFollow(const slmath::vec3& pointToFollow)
{
	this->pointToFollow = pointToFollow;
}

slmath::vec3 Camera::getPointToFollow() const
{
    return pointToFollow;
}

void Camera::setDistanceFromTarget(float distanceFromTarget)
{
    m_DistanceFromTarget = distanceFromTarget;
}

float Camera::getDistanceFromTarget() const
{
    return m_DistanceFromTarget;
}

void Camera::setAngleHeight(float angleHeight)
{
    m_AngleHeight = angleHeight;
}

float Camera::getAngleHeight() const
{
    return m_AngleHeight;
}

void Camera::setAngleHorizon(float angleHorizon)
{
    m_AngleHorizon = angleHorizon;
}

float Camera::getAngleHorizon() const
{
    return m_AngleHorizon;
}

slmath::mat4 Camera::getViewMatrix()const 
{
	return viewMatrix;
}

slmath::vec3 Camera::getPosition()const 
{
	return m_Position;
}

void Camera::move() 
{
    assert(m_Position != pointToFollow);
    	
    //Rotate from x axis
    slmath::mat4 rotation = slmath::rotationX(m_AngleHeight);
    slmath::vec3 position = slmath::mul(rotation, slmath::vec3(0.0f, 0.0f, 1.0f)).xyz();

     //Rotate from y axis
    rotation = slmath::rotationY(m_AngleHorizon);
    position = slmath::mul(rotation, slmath::vec4(position, 1.0f)).xyz();

    // Position at the right distance
	position = pointToFollow  + position * m_DistanceFromTarget;

    slmath::vec3 separation = position - pointToFollow;
    if (slmath::dot(separation, separation) > 1.0f)
    {
        m_Position = position;
    }

	// Set up the view matrix
	viewMatrix = slmath::lookAtLH(m_Position, pointToFollow, slmath::vec3(0.0f, 1.0f, 0.0f));

}

void Camera::zoomIn(float distance) 
{
	m_DistanceFromTarget -= distance;
}
void Camera::zoomOut(float distance)
{
	m_DistanceFromTarget += distance;
}

void Camera::moveUp(float distance) 
{
    const float epsilon = 1e-3f;
    m_AngleHeight += distance;
    if (m_AngleHeight > slmath::PI / 2.0f - epsilon)
    {
        m_AngleHeight = slmath::PI / 2.0f - epsilon;
    }
    
}
void Camera::moveDown(float distance)
{
    const float epsilon = 1e-3f;
    m_AngleHeight -= distance;
    if (m_AngleHeight < -slmath::PI / 2.0f + epsilon)
    {
        m_AngleHeight = -slmath::PI / 2.0f + epsilon;
    }
}
void Camera::moveRight(float distance)
{
	 m_AngleHorizon -= distance;
}
void Camera::moveLeft(float distance)
{
	 m_AngleHorizon += distance;
}

