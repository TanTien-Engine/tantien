#pragma once

#include "modules/script/Proxy.h"

#include <SM_Vector.h>
#include <SM_Matrix.h>
#include <polymesh3/Polytope.h>
#include <vessel.h>

#include <vector>
#include <memory>
#include <string>

namespace tt
{

std::vector<sm::vec2> list_to_vec2_array(int index);
std::vector<sm::vec3> list_to_vec3_array(int index);
template <typename T>
std::vector<T> list_to_array(int index);
template <typename T>
std::vector<std::vector<T>> list_to_array2(int index);

std::vector<std::string> list_to_string_array(int index);

uint32_t list_to_abgr(int index);
uint32_t list_to_rgba(int index);

sm::vec2 list_to_vec2(int index);
sm::vec3 list_to_vec3(int index);
sm::vec4 list_to_vec4(int index);

sm::vec2 map_to_vec2(int index);
sm::vec3 map_to_vec3(int index);
sm::vec4 map_to_vec4(int index);

template<typename T>
void list_to_foreigns(int index, std::vector<std::shared_ptr<T>>& foreigns);
template<typename T>
void list_to_foreigns(int index, std::vector<T>& foreigns);

void return_list(const std::vector<std::vector<sm::vec2>>& polys);
void return_list(const std::vector<sm::vec2>& pts);
void return_list(const std::vector<sm::ivec2>& pts);
void return_list(const std::vector<sm::vec3>& pts);
void return_list(const std::vector<std::string>& strs);

template<typename T>
void return_list(const std::vector<T>& vals);
template<typename T>
void return_list2(const std::vector<std::vector<T>>& vals);

void return_vec(const sm::vec2& vec2);
void return_vec(const sm::vec3& vec3);

void return_poly(const std::shared_ptr<pm3::Polytope>& poly);
void return_poly_list(const std::vector<std::shared_ptr<pm3::Polytope>>& polys);

}

#include "TransHelper.inl"