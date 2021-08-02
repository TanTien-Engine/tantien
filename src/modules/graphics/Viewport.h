#pragma once

#include <SM_Matrix.h>

namespace tt
{

class Viewport
{
public:
	Viewport();

	float Width() const { return m_width; }
	float Height() const { return m_height; }

	void SetSize(float width, float height);

	sm::vec2 TransPos3ProjectToScreen(const sm::vec3& proj, float fovy, float aspect) const;
	//sm::vec3 TransPos3ScreenToDir(const sm::vec2& screen, const PerspCam& cam) const;

	sm::vec2 TransPosProj3ToProj2(const sm::vec3& proj, const sm::mat4& cam_mat) const;

	sm::vec3 MapToSphere(const sm::vec2& touchpoint) const;

private:
	float m_width, m_height;

	sm::mat4 m_2d_proj_mat_inv;

}; // Viewport

}