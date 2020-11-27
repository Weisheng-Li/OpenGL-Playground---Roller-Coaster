#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>

#include <shader.hpp>
#include <rc_spline.h>

struct Orientation {
	// Front
	glm::vec3 Front;
	// Up
	glm::vec3 Up;
	// Right
	glm::vec3 Right;
	// origin
	glm::vec3 origin;
};


class Track
{
public:

	// VAO
	unsigned int VAO;

	// Control Points Loading Class for loading from File
	rc_Spline g_Track;

	// Vector of control points
	std::vector<glm::vec3> controlPoints;

	// Track data
	std::vector<Vertex> vertices;
	// indices for EBO
	std::vector<unsigned int> indices;

	// hmax for camera
	float hmax = 0.0f;

	// maximun s value, calculated by create_track()
	int max_s;

	// constructor, just use same VBO as before, 
	Track(const char* trackPath)
	{		
		// load Track data
		load_track(trackPath);

		create_track();

		setup_track();
	}

	// render the mesh
	void Draw(Shader shader, unsigned int textureID)
	{
		// Set the shader properties
		shader.use();
		glm::mat4 track_model;
		shader.setMat4("model", track_model);


		// Set material properties
		shader.setVec3("material.specular", 0.3f, 0.3f, 0.3f);
		shader.setFloat("material.shininess", 64.0f);


		// active proper texture unit before binding
		glActiveTexture(GL_TEXTURE0);
		// and finally bind the textures
		glBindTexture(GL_TEXTURE_2D, textureID);

		// draw mesh
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}

	// give a positive float s, find the point by interpolation
	// determine pA, pB, pC, pD based on the integer of s
	// determine u based on the decimal of s
	// E.g. s=1.5 is the at the halfway point between the 1st and 2nd control point,
	//		the 4 control points are:[0,1,2,3], with u=0.5
	glm::vec3 get_point(float s)
	{
		// use modulo operation to ensure all points are valid (max_s hard-coded)
		int pA = ((int)floor(s) + max_s - 1) % max_s;
		int pB = ((int)floor(s) + max_s) % max_s;
		int pC = ((int)floor(s) + max_s + 1) % max_s;
		int pD = ((int)floor(s) + max_s + 2) % max_s;
		float u = s - floor(s);

		return interpolate(controlPoints[pA], controlPoints[pB], controlPoints[pC], controlPoints[pD], 0.5f, u);
	}


	void delete_buffers()
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
	}

	
	private:
	
	/*  Render data  */
	unsigned int VBO, EBO;

	void load_track(const char* trackPath)
	{
		// Set folder path for our projects (easier than repeatedly defining it)
		g_Track.folder = "../Project_2/Media/";

		// Load the control points
		g_Track.loadSplineFrom(trackPath);

	}

	// Implement the Catmull-Rom Spline here
	//	Given 4 points, a tau and the u value 
	//	u in range of [0,1]  
	//	Since you can just use linear algebra from glm, just make the vectors and matrices and multiply them.  
	//	This should not be a very complicated function
	glm::vec3 interpolate(glm::vec3 pointA, glm::vec3 pointB, glm::vec3 pointC, glm::vec3 pointD, float tau, float u)
	{
		// Construct a matrix with points 		
		glm::mat4x3 mat_points;
		mat_points[0] = pointA;
		mat_points[1] = pointB;
		mat_points[2] = pointC;
		mat_points[3] = pointD;

		// Construct a matrix with tau
		glm::mat4 mat_tau;

		// each group is a column
		mat_tau[0].x = 0;
		mat_tau[0].y = -tau;
		mat_tau[0].z = 2*tau;
		mat_tau[0].w = -tau;

		mat_tau[1].x = 1;
		mat_tau[1].y = 0;
		mat_tau[1].z = tau-3;
		mat_tau[1].w = 2-tau;

		mat_tau[2].x = 0;
		mat_tau[2].y = tau;
		mat_tau[2].z = 3-2*tau;
		mat_tau[2].w = tau-2;

		mat_tau[3].x = 0;
		mat_tau[3].y = 0;
		mat_tau[3].z = -tau;
		mat_tau[3].w = tau;

		mat_tau = glm::transpose(mat_tau);

		// Construct a vector with u
		glm::vec4 vec_u;

		vec_u[0] = 1;
		vec_u[1] = u;
		vec_u[2] = u * u;
		vec_u[3] = u * u * u;

		// Return the interpolated point
		return mat_points * mat_tau * vec_u; 
	}

	// Here is the class where you will make the vertices or positions of the necessary objects of the track (calling subfunctions)
	//  For example, to make a basic roller coster:
	//    First, make the vertices for each rail here (and indices for the EBO if you do it that way).  
	//        You need the XYZ world coordinates, the Normal Coordinates, and the texture coordinates.
	//        The normal coordinates are necessary for the lighting to work.  
	//    Second, make vector of transformations for the planks across the rails
	void create_track()
	{
		// Create the vertices and indices (optional) for the rails
		//    One trick in creating these is to move along the spline and 
		//    shift left and right (from the forward direction of the spline) 
		//     to find the 3D coordinates of the rails.

		glm::vec3 currentpos = glm::vec3(-2.0f, 0.0f, -2.0f);
		max_s = 0;

		for (pointVectorIter ptsiter = g_Track.points().begin(); ptsiter != g_Track.points().end(); ptsiter++)
		{
			/* get the next point from the iterator */
			glm::vec3 pt(*ptsiter);

			/* now just the uninteresting code that is no use at all for this project */
			currentpos += pt;

			// Set the control points apart
			controlPoints.push_back(currentpos * 2.0f);

			// record the hmax
			if (currentpos.y * 2.0f > hmax) hmax = currentpos.y * 2.0f;

			max_s += 1;
		}

		hmax *= 1.05f;

		// Then traverse the spline with small steps
		float step_size = 0.03125f;

		Orientation Ori_Pn_1, Ori_Pn;

		// Initialize Ori_Pn_1 (Initially on s=0)
		Ori_Pn.origin = get_point(0.0f);
		Ori_Pn.Front = glm::normalize(get_point(0.0f + step_size) - get_point(0.0f));
		Ori_Pn.Up = glm::vec3(0.0f, 1.0f, 0.0f);
		Ori_Pn.Right = glm::normalize(glm::cross(Ori_Pn.Front, Ori_Pn.Up));

		// On first iteration, Pn_1 = Position(s=0), Pn = Position(s=1 * step_size)
		for (float s = 1 * step_size; s <= max_s; s += step_size)
		{
			Ori_Pn_1.origin = Ori_Pn.origin;
			Ori_Pn_1.Up = Ori_Pn.Up;
			Ori_Pn_1.Front = Ori_Pn.Front;
			Ori_Pn_1.Right = Ori_Pn.Right;

			Ori_Pn.origin = get_point(s);
			Ori_Pn.Front = glm::normalize(get_point(s + step_size) - Ori_Pn.origin);
			Ori_Pn.Up = glm::normalize(glm::cross(Ori_Pn_1.Right, Ori_Pn.Front));
			if (s >= max_s - 64 * step_size) {
				float local_step = (s - (max_s - 64 * step_size)) / (64 * step_size);
				Ori_Pn.Up += local_step * (glm::vec3(0.0f, 1.0f, 0.0f) - Ori_Pn.Up);
			}
			Ori_Pn.Right = glm::normalize(glm::cross(Ori_Pn.Front, Ori_Pn.Up));

			makeRailPart(Ori_Pn_1, Ori_Pn, glm::vec2(0.5f, 0.1f));

			// create plank when s is a multiple of 0.125f.
			if (fmod(s, 0.125f) <= 0.02f) makePlank(Ori_Pn, glm::vec2(0.5f, 0.1f));
		}
	}


	// Given 3 Points, create a triangle and push it into vertices (and EBO if you are using one)
		// Optional boolean to flip the normal if you need to

	//			A---------------------B
	//			|					  |
	//			|					  |
	//			C---------------------D
	
	// By default, All four points has same normal, calculated by AC X AB

	void make_face(glm::vec3 pointA, glm::vec3 pointB, glm::vec3 pointC, glm::vec3 pointD, bool flipNormal)
	{
		glm::vec3 normal = glm::normalize(glm::cross((pointC - pointA), (pointB - pointA)));

		Vertex A, B, C, D;
		A.Position = pointA; B.Position = pointB; C.Position = pointC; D.Position = pointD;
		A.Normal = normal; B.Normal = normal; C.Normal = normal; D.Normal = normal;
		A.TexCoords = glm::vec2(0.0f, 1.0f);
		B.TexCoords = glm::vec2(1.0f, 1.0f);
		C.TexCoords = glm::vec2(0.0f, 0.0f);
		D.TexCoords = glm::vec2(1.0f, 0.0f);

		// Push: up triangle
		vertices.push_back(A);
		vertices.push_back(B);
		vertices.push_back(C);

		// Push: down triangle
		vertices.push_back(C);
		vertices.push_back(B);
		vertices.push_back(D);
	}

	// Given two orintations, create the rail between them.  Offset can be useful if you want to call this for more than for multiple rails
	//			A-----B						 E-----F
	//			|	  |      Center of       |	   |
	//			|	  |		 Ori.origin      |	   |
	//			C-----D						 G-----H

	// In the figure above, front vector is point into the screen, dimension is shown below:
	// lengh(AC) = 2*up_offset; lengh(AF)=2*right_offset; lengh(AB) = 2*up_offset

	void makeRailPart(Orientation ori_prev, Orientation ori_cur, glm::vec2 offset)
	{
		// offset[0] = left & right offset, offset[1] = up & down offset
		glm::vec3 right_offset_1, right_offset_2, up_offset;

		// Get all the points, 4 points for each Ori struct, see above picture for naming rule
		// 4 points around ori_prev
		right_offset_1 = ori_prev.Right * offset[0];			   // long horizontal offset, like A to center
		right_offset_2 = ori_prev.Right * (offset[0] - offset[1]); // short horizontal offset, like B to center
		up_offset = ori_prev.Up * offset[1];

		glm::vec3 prev_A = ori_prev.origin - right_offset_1 + up_offset;
		glm::vec3 prev_F = ori_prev.origin + right_offset_1 + up_offset;
		glm::vec3 prev_C = ori_prev.origin - right_offset_1 - up_offset;
		glm::vec3 prev_H = ori_prev.origin + right_offset_1 - up_offset;

		glm::vec3 prev_B = ori_prev.origin - right_offset_2 + up_offset;
		glm::vec3 prev_E = ori_prev.origin + right_offset_2 + up_offset;
		glm::vec3 prev_D = ori_prev.origin - right_offset_2 - up_offset;
		glm::vec3 prev_G = ori_prev.origin + right_offset_2 - up_offset;

		// 4 points around ori_cur
		right_offset_1 = ori_cur.Right * offset[0];
		right_offset_2 = ori_cur.Right * (offset[0] - offset[1]);
		up_offset = ori_cur.Up * offset[1];

		glm::vec3 cur_A = ori_cur.origin - right_offset_1 + up_offset;
		glm::vec3 cur_F = ori_cur.origin + right_offset_1 + up_offset;
		glm::vec3 cur_C = ori_cur.origin - right_offset_1 - up_offset;
		glm::vec3 cur_H = ori_cur.origin + right_offset_1 - up_offset;

		glm::vec3 cur_B = ori_cur.origin - right_offset_2 + up_offset;
		glm::vec3 cur_E = ori_cur.origin + right_offset_2 + up_offset;
		glm::vec3 cur_D = ori_cur.origin - right_offset_2 - up_offset;
		glm::vec3 cur_G = ori_cur.origin + right_offset_2 - up_offset;

		// Make faces
		make_face(prev_B, cur_B, prev_D, cur_D, false); // right face
		make_face(prev_A, cur_A, prev_B, cur_B, false); // up face
		make_face(prev_C, cur_C, prev_A, cur_A, false); // left face
		make_face(prev_D, cur_D, prev_C, cur_C, false); // bottom face

		make_face(prev_F, cur_F, prev_H, cur_H, false); // right face
		make_face(prev_E, cur_E, prev_F, cur_F, false); // up face
		make_face(prev_G, cur_G, prev_E, cur_E, false); // left face
		make_face(prev_H, cur_H, prev_G, cur_G, false); // bottom face
	}

	// ori_cur is the center of the plank, offset is the size of the rail,
	// which should be the same as the makerailpart offset

	//			A----------------------------E
	//		   /|                           /|
	//		  B-|--------------------------F |
	//        | D                          | H
	//        |/                           |/
	//        C----------------------------G
	//

	void makePlank(Orientation ori_cur, glm::vec2 offset)
	{
		glm::vec3 front_offset, up_offset, right_offset;

		// Calculate the offsets based on the given rail size
		// offset[0] = left & right offset, offset[1] = up & down offset
		up_offset = ori_cur.Up * offset[1] * 0.7f;
		right_offset = ori_cur.Right * offset[0] * 0.9f;
		front_offset = ori_cur.Front * offset[1] * 0.7f;

		glm::vec3 pA = ori_cur.origin - right_offset + up_offset + front_offset;
		glm::vec3 pB = ori_cur.origin - right_offset + up_offset - front_offset;
		glm::vec3 pC = ori_cur.origin - right_offset - up_offset - front_offset;
		glm::vec3 pD = ori_cur.origin - right_offset - up_offset + front_offset;

		glm::vec3 pE = ori_cur.origin + right_offset + up_offset + front_offset;
		glm::vec3 pF = ori_cur.origin + right_offset + up_offset - front_offset;
		glm::vec3 pG = ori_cur.origin + right_offset - up_offset - front_offset;
		glm::vec3 pH = ori_cur.origin + right_offset - up_offset + front_offset;

		// Make faces
		make_face(pA, pE, pB, pF, false); // up face
		make_face(pD, pH, pA, pE, false); // back face
		make_face(pC, pG, pD, pH, false); // bottom face
		make_face(pB, pF, pC, pG, false); // front face
	}

	// Find the normal for each triangle uisng the cross product and then add it to all three vertices of the triangle.  
	//   The normalization of all the triangles happens in the shader which averages all norms of adjacent triangles.   
	//   Order of the triangles matters here since you want to normal facing out of the object.  
	void set_normals(Vertex &p1, Vertex &p2, Vertex &p3)
	{
		glm::vec3 normal = glm::cross(p2.Position - p1.Position, p3.Position - p1.Position);
		p1.Normal += normal;
		p2.Normal += normal;
		p3.Normal += normal;
	}

	void setup_track()
	{
		// create buffers/arrays
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);
		// load data into vertex buffers
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// A great thing about structs is that their memory layout is sequential for all its items.
		// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/3/2 array which
		// again translates to 3/3/2 floats which translates to a byte array.
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normal coords
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}

};
