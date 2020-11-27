#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <heightmap.hpp>
#include <model.hpp>
#include <track.hpp>
#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 5.0f;
const float SENSITIVTY = 0.1f;
const float ZOOM = 45.0f;
const float G = 9.8f;


// An abstract camera class that processes input and calculates the corresponding Eular Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;

	// Camera Attributes running on the background,
	// it keeps folloing the spline
	glm::vec3 bg_Position;
	glm::vec3 bg_Front;
	glm::vec3 bg_Up;
	glm::vec3 bg_Right;

	glm::vec3 WorldUp;
	// Eular Angles
	float Yaw;
	float Pitch;
	// Camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;
	// Our Parameters
	float s;  // Position you are on the track
	bool onTrack = false; // Whether or not you are following the track

	// Constructor with vectors
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}
	// Constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVTY), Zoom(ZOOM)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// Returns the view matrix calculated using Eular Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAt(Position, Position + Front, Up);
	}

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		float velocity = MovementSpeed * deltaTime;
		if (direction == FORWARD)
			Position += Front * velocity;
		if (direction == BACKWARD)
			Position -= Front * velocity;
		if (direction == LEFT)
			Position -= Right * velocity;
		if (direction == RIGHT)
			Position += Right * velocity;
	}

	//  Find the next camera position based on the amount of passed time, the track, and the track position s (defined in this class).  You can just use your code from the track function. 
	void ProcessTrackMovement(float deltaTime, Track &track)
	{
		// velocity will never be zero because hmax is higher than the actual highest point
		float velocity = sqrt(2 * G * (track.hmax - track.get_point(s).y));
		float frame_movement = velocity * deltaTime;

		// Use modula operation to limit value s;
		if (s > track.max_s)
			s = fmod(s, track.max_s);

		// small step movement
		float step_size = 0.0001f;

		// Calculate distance between current s and next_s
		glm::vec3 temp_pos = track.get_point(s);
		glm::vec3 next_pos = track.get_point(s + step_size);
		float distance = glm::distance(next_pos, temp_pos);
		while (distance < frame_movement) {
			s += step_size;
			frame_movement -= distance;

			glm::vec3 temp_pos = next_pos;
			glm::vec3 next_pos = track.get_point(s + step_size);
			float distance = glm::distance(next_pos, temp_pos);
		}

		// When exit the while loop, already move enough distances
		// Then update the orientation
		bg_Front = glm::normalize(next_pos - temp_pos);
		bg_Up = glm::normalize(glm::cross(bg_Right, bg_Front));
		// At the end of the spline, add offset to let the up vector at the
		// beginning and at the end match.
		if (s >= track.max_s - 2.0f && s <= track.max_s) {
			float local_step = (s - (track.max_s - 2.0f)) / 2.0f;
			bg_Up += local_step * (glm::vec3(0.0f, 1.0f, 0.0f) - bg_Up);
		}
		bg_Right = glm::normalize(glm::cross(bg_Front, bg_Up));
		
		bg_Position = temp_pos + bg_Up; //camera need to be above the rail

		if (onTrack)
		{
			Front = bg_Front;
			Up = bg_Up;
			Right = bg_Right;
			Position = bg_Position;
		}
	}

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		// Update Front, Right and Up Vectors using the updated Eular angles
		updateCameraVectors();
	}

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	//    Not really necessary, you can use this for something else if you like
	void ProcessMouseScroll(float yoffset)
	{
		if (Zoom >= 1.0f && Zoom <= 45.0f)
			Zoom -= yoffset;
		if (Zoom <= 1.0f)
			Zoom = 1.0f;
		if (Zoom >= 45.0f)
			Zoom = 45.0f;
	}

	// Given position and orientation, returns the transformation that move the cart to the given location
	glm::mat4 getCartTrans(glm::vec3 origin, glm::vec3 front, glm::vec3 up, glm::vec3 right)
	{
		glm::mat4 rotation;
		rotation[0] = glm::vec4(right, 0.0f);
		rotation[1] = glm::vec4(up, 0.0f);
		rotation[2] = glm::vec4(front, 0.0f);
		rotation[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		glm::mat4 translation;
		translation = glm::translate(translation, origin - 1.0f * up + 1.5f * front);

		return translation * rotation;
	}

private:
	// Calculates the front vector from the Camera's (updated) Eular Angles
	void updateCameraVectors()
	{
		// Calculate the new Front vector
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		// Also re-calculate the Right and Up vector
		Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Right, Front));
	}
};
#endif
