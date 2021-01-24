#pragma once

#include <vector>

namespace tt
{

template<typename VT, typename IT>
class RenderBuffer
{
public:
    void Reserve(size_t idx_count, size_t vtx_count)
    {
	    size_t sz = vertices.size();
	    vertices.resize(sz + vtx_count);
	    vert_ptr = vertices.data() + sz;

	    sz = indices.size();
	    indices.resize(sz + idx_count);
	    index_ptr = indices.data() + sz;
    }

    void Clear()
    {
	    vertices.resize(0);
	    indices.resize(0);

	    curr_index = 0;
	    vert_ptr   = nullptr;
	    index_ptr  = nullptr;
    }

public:
    // for indices use unsigned short
    static const int MAX_VERTEX_NUM = 0xffff;

public:
    std::vector<VT> vertices;
    std::vector<IT> indices;

    IT  curr_index = 0;
    VT* vert_ptr   = nullptr;
    IT* index_ptr  = nullptr;

}; // RenderBuffer

}