


#ifndef CAMERA_H
#define CAMERA_H
#include <slmath/slmath.h>

class Camera
{

protected:
	slmath::vec3 position;
	slmath::vec3 pointToFollow;
	slmath::vec3 speed;

    float m_DistanceFromTarget;
    float m_AngleHeight;
    float m_AngleY;


    slmath::mat4 viewMatrix;
	float springConstant;
	float damping;

	static const float speedRotation;
	static const float speedRotationUpDown;
	static const float speedZoom;

public:
	Camera();
	virtual ~Camera();
	virtual void setPointToFollow(const slmath::vec3& pointToFollow);
	virtual void setPosition(const slmath::vec3& position);
	virtual slmath::mat4 getViewMatrix()const;
	virtual slmath::vec3 getPosition()const;
	virtual void move(float frameDuration);
	void zoomUp(float frameDuration);
	void zoomDown(float frameDuration);
	virtual void moveUp(float frameDuration);
	virtual void moveDown(float frameDuration);
	virtual void moveRight(float frameDuration);
	virtual void moveLeft(float frameDuration);

private:
	float checkPositionMax(const float position, const float pointToFollow, const float distanceMax);
	float checkPositionMin(const float position, const float pointToFollow, const float distanceMin);
};

#endif CAMERA_H
