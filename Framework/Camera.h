


#ifndef CAMERA_H
#define CAMERA_H
#include <slmath/slmath.h>

class Camera
{

protected:
	slmath::vec3 m_Position;
	slmath::vec3 pointToFollow;

    float m_DistanceFromTarget;
    float m_AngleHeight;
    float m_AngleHorizon;


    slmath::mat4 viewMatrix;

	static const float speedRotation;
	static const float speedRotationUpDown;

public:
	Camera();
	virtual ~Camera();
    
    // Accesors and setter
	virtual void setPointToFollow(const slmath::vec3& pointToFollow);
    virtual slmath::vec3 getPointToFollow() const;
    virtual void setDistanceFromTarget(float distanceFromTarget);
    virtual float getDistanceFromTarget()const;
    virtual void setAngleHeight(float angleHeight);
    virtual float getAngleHeight() const;
    virtual void setAngleHorizon(float angleHorizon);
    virtual float getAngleHorizon() const;
    
    // Output from the camera
	virtual slmath::mat4 getViewMatrix()const;
	virtual slmath::vec3 getPosition()const;
    
    // Camera update
	virtual void move();

    // Utility functions
	virtual void zoomIn(float distance);
	virtual void zoomOut(float distance);
	virtual void moveUp(float distance);
	virtual void moveDown(float distance);
	virtual void moveRight(float distance);
	virtual void moveLeft(float distance);
};

#endif CAMERA_H
